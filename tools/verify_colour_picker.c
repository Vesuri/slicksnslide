#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/ui/colour_picker.h"
#include "../src/ui/chunky_ui.h"
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned a,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(u,a,b,2)); }
static unsigned readword(uc_engine *u,unsigned a)
{ unsigned char b[2]; check(uc_mem_read(u,a,b,2)); return b[0]|b[1]<<8; }
struct DrawCall { unsigned type; short values[5]; };
struct DrawTrace { struct DrawCall calls[9]; unsigned count; };
static unsigned char nearest(void *context,unsigned char r,unsigned char g,unsigned char b)
{
    struct DrawTrace *t=context; if(t->count>=9) abort();
    struct DrawCall *c=&t->calls[t->count++]; c->type=0;
    c->values[0]=r; c->values[1]=g; c->values[2]=b;
    return (unsigned char)(r+3*g+7*b);
}
static void rectangle(void *context,short x,short y,short right,short bottom,unsigned char colour)
{
    struct DrawTrace *t=context; if(t->count>=9) abort();
    struct DrawCall *c=&t->calls[t->count++]; c->type=1;
    c->values[0]=x; c->values[1]=y; c->values[2]=right; c->values[3]=bottom; c->values[4]=colour;
}
static void draw_boundary(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size; uint16_t ss,sp,cs,ip,ax;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    check(uc_reg_read(u,UC_X86_REG_AX,&ax)); unsigned stack=ss*16U+sp;
    if(address==0x36fae) ax=nearest(context,(unsigned char)readword(u,stack+4),
        (unsigned char)readword(u,stack+6),(unsigned char)readword(u,stack+8));
    else rectangle(context,(short)readword(u,stack+4),(short)readword(u,stack+6),
        (short)readword(u,stack+8),(short)readword(u,stack+10),(unsigned char)readword(u,stack+12));
    ip=readword(u,stack); cs=readword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
}
static void frame(uc_engine *u,unsigned key)
{
    uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000,ax=key;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    check(uc_reg_write(u,UC_X86_REG_BP,&bp)); check(uc_reg_write(u,UC_X86_REG_AX,&ax));
}
static void verify_draw(uc_engine *u)
{
    struct DrawTrace dos; uc_hook hooks[2];
    check(uc_hook_add(u,&hooks[0],UC_HOOK_CODE,draw_boundary,&dos,0x36fae,0x36fae));
    check(uc_hook_add(u,&hooks[1],UC_HOOK_CODE,draw_boundary,&dos,0x39ed8,0x39ed8));
    unsigned cases=0; const short xs[]={160,-10,32760,-32768},ys[]={100,-28,32760,-32768};
    for(unsigned phase=0;phase<256;++phase) for(unsigned channel=0;channel<3;++channel)
    for(unsigned changed=0;changed<2;++changed) for(unsigned pattern=0;pattern<4;++pattern) {
        struct SlicksColourPicker s={{(unsigned char)phase,(unsigned char)(phase+79),(unsigned char)(phase+173)},(unsigned char)channel,0};
        struct SlicksColourPickerPulse pulse={0xfffffffeUL,(unsigned char)phase};
        unsigned long tick=changed?0xffffffffUL:0xfffffffeUL;
        const unsigned char bars[]={17,129,255}; unsigned char unselected=47;
        check(uc_mem_write(u,0x8eff6,s.rgb,3)); check(uc_mem_write(u,0x8eff9,&s.channel,1));
        check(uc_mem_write(u,0x8eff2,bars,3)); check(uc_mem_write(u,0x8eff1,&unselected,1));
        check(uc_mem_write(u,0x8efe7,&pulse.phase,1));
        word(u,0x8efec,0x200); word(u,0x8efee,0x6000);
        word(u,0x8efe8,pulse.tick); word(u,0x8efea,pulse.tick>>16);
        word(u,0x60200,tick); word(u,0x60202,tick>>16);
        word(u,0x8f006,(unsigned short)xs[pattern]); word(u,0x8f008,(unsigned short)ys[pattern]);
        memset(&dos,0,sizeof dos); frame(u,0);
        check(uc_emu_start(u,0x2f43f,0x2f5b6,0,10000));
        struct DrawTrace native={0}; struct SlicksColourPickerDrawOps ops={nearest,rectangle,&native};
        slicks_colour_picker_draw(&s,&pulse,tick,xs[pattern],ys[pattern],bars,unselected,&ops);
        unsigned char actual_phase; check(uc_mem_read(u,0x8efe7,&actual_phase,1));
        unsigned long actual_tick=readword(u,0x8efe8)|((unsigned long)readword(u,0x8efea)<<16);
        uint16_t ip,sp; check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        if(ip!=0x2f5b6-0x2e0f0 || sp!=0xeb00 || actual_phase!=pulse.phase || actual_tick!=pulse.tick || memcmp(&dos,&native,sizeof dos)) {
            fprintf(stderr,"Picker draw mismatch phase=%u channel=%u changed=%u pattern=%u\n",phase,channel,changed,pattern); exit(1);
        }
        ++cases;
    }
    for(unsigned i=0;i<2;++i) check(uc_hook_del(u,hooks[i]));
    printf("DOS colour-picker drawing: %u rectangle/palette-call/pulse-state comparisons pass\n",cases);
}

struct Vga { unsigned char planes[262144]; unsigned mask,index; };
static void vga_out(uc_engine *u,uint32_t port,int size,uint32_t value,void *context)
{
    (void)u; struct Vga *v=context;
    if(size!=1) abort();
    if(port==0x3c4) { v->index=value; if(value!=2) abort(); }
    else if(port==0x3c5 && v->index==2) v->mask=value&15;
    else abort();
}
static void vga_write(uc_engine *u,uc_mem_type type,uint64_t address,int size,int64_t value,void *context)
{
    (void)u; (void)type; struct Vga *v=context;
    if(size!=1 || !v->mask || address<0xa0000 || address>=0xb0000) abort();
    for(unsigned p=0;p<4;++p) if(v->mask&(1U<<p)) v->planes[p*65536+address-0xa0000]=(unsigned char)value;
}
static void verify_pixels(const unsigned char *runtime,size_t bytes)
{
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,bytes));
    static struct Vga v; uc_hook h;
    check(uc_hook_add(u,&h,UC_HOOK_INSN,vga_out,&v,1,0,UC_X86_INS_OUT));
    check(uc_hook_add(u,&h,UC_HOOK_MEM_WRITE,vga_write,&v,0xa0000,0xaffff));
    const short xs[]={160,-30,290,160},ys[]={100,100,180,-15};
    unsigned cases=0;
    for(unsigned pattern=0;pattern<64;++pattern) for(unsigned place=0;place<4;++place) {
        unsigned char pixels[64000],palette[768];
        for(unsigned i=0;i<768;++i) palette[i]=(unsigned char)(i*61+pattern*43+(i>>3)*17);
        if(!pattern) memset(palette,0,sizeof palette);
        for(unsigned i=0;i<64000;++i) pixels[i]=(unsigned char)(i*37+pattern);
        memset(v.planes,0xa5,sizeof v.planes); v.mask=v.index=0;
        for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;++x)
            v.planes[(x&3)*65536+y*100+x/4]=pixels[y*320+x];
        struct SlicksColourPicker s={{(unsigned char)pattern,(unsigned char)((pattern+17)%64),(unsigned char)((pattern+43)%64)},(unsigned char)(pattern%3),0};
        struct SlicksColourPickerPulse pulse={123,(unsigned char)(pattern%61)};
        struct SlicksChunkyUi ui={pixels,palette,0,0};
        check(uc_mem_write(u,0x60400,palette,sizeof palette));
        word(u,0x3cbf0+0x71b8,0x400); word(u,0x3cbf0+0x71ba,0x6000);
        word(u,0x3cbf0+0x1d87,0); word(u,0x3cbf0+0x1d7b,100);
        word(u,0x3cbf0+0x1d8d,0); word(u,0x3cbf0+0x1d8f,200);
        word(u,0x3cbf0+0x1d91,0); word(u,0x3cbf0+0x1d93,79);
        frame(u,0); check(uc_emu_start(u,0x2f3a0,0x2f420,0,100000));
        check(uc_mem_write(u,0x8eff6,s.rgb,3)); check(uc_mem_write(u,0x8eff9,&s.channel,1));
        check(uc_mem_write(u,0x8efe7,&pulse.phase,1));
        word(u,0x8efec,0x200); word(u,0x8efee,0x6000);
        word(u,0x8efe8,123); word(u,0x8efea,0); word(u,0x60200,124); word(u,0x60202,0);
        word(u,0x8f006,(unsigned short)xs[place]); word(u,0x8f008,(unsigned short)ys[place]);
        frame(u,0); check(uc_emu_start(u,0x2f43f,0x2f5b6,0,1000000));
        uint16_t ip,sp; check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        if(ip!=0x2f5b6-0x2e0f0 || sp!=0xeb00) abort();
        slicks_ui_colour_picker(&ui,&s,&pulse,124,xs[place],ys[place]);
        for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;++x)
            if(v.planes[(x&3)*65536+y*100+x/4]!=pixels[y*320+x]) {
                fprintf(stderr,"Picker pixel mismatch pattern=%u place=%u x=%u y=%u DOS=%u native=%u\n",pattern,place,x,y,v.planes[(x&3)*65536+y*100+x/4],pixels[y*320+x]); exit(1);
            }
        ++cases;
    }
    printf("DOS colour-picker pixels: %u full 320x200 comparisons pass with real original palette matching and VGA fills\n",cases);
    cases=0;
    const short widths[]={1,2,5,65,167,320},heights[]={1,2,11,30};
    for(unsigned pattern=0;pattern<8;++pattern) for(unsigned place=0;place<4;++place)
    for(unsigned wi=0;wi<6;++wi) for(unsigned hi=0;hi<4;++hi) {
        unsigned char pixels[64000],palette[768];
        for(unsigned i=0;i<768;++i) palette[i]=(unsigned char)(i*61+pattern*43+(i>>3)*17);
        for(unsigned i=0;i<64000;++i) pixels[i]=(unsigned char)(i*37+pattern);
        memset(v.planes,0xa5,sizeof v.planes); v.mask=v.index=0;
        for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;++x)
            v.planes[(x&3)*65536+y*100+x/4]=pixels[y*320+x];
        check(uc_mem_write(u,0x60400,palette,sizeof palette));
        unsigned char red=(unsigned char)(pattern*37),green=(unsigned char)(pattern*51+10),blue=(unsigned char)(pattern*71+20);
        const unsigned args[]={(unsigned short)xs[place],(unsigned short)ys[place],(unsigned short)widths[wi],(unsigned short)heights[hi],red,green,blue,0};
        frame(u,0); uint16_t sp=0xf000,ip,ax; check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        word(u,0x8f000,0); word(u,0x8f002,0x9000);
        for(unsigned i=0;i<8;++i) word(u,0x8f004+2*i,args[i]);
        check(uc_emu_start(u,0x309cf,0x90000,0,1000000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp)); check(uc_reg_read(u,UC_X86_REG_AX,&ax));
        struct SlicksChunkyUi ui={pixels,palette,0,0};
        unsigned char colour=slicks_ui_bevel(&ui,xs[place],ys[place],widths[wi],heights[hi],red,green,blue);
        if(ip || sp!=0xf004 || (ax&255)!=colour) abort();
        for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;++x)
            if(v.planes[(x&3)*65536+y*100+x/4]!=pixels[y*320+x]) {
                fprintf(stderr,"Bevel pixel mismatch pattern=%u place=%u size=%dx%d pixel=%u,%u DOS=%u native=%u\n",pattern,place,widths[wi],heights[hi],x,y,v.planes[(x&3)*65536+y*100+x/4],pixels[y*320+x]); exit(1);
            }
        ++cases;
    }
    printf("DOS rounded bevel: %u full 320x200 pixel/return comparisons pass\n",cases);
    check(uc_close(u));
}

int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f); int error=ferror(f); fclose(f);
    if(error || bytes<200000 || bytes==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,bytes));
    unsigned cases=0;
    for(unsigned value=0;value<256;++value) for(unsigned channel=0;channel<3;++channel)
    for(unsigned scan=0;scan<256;++scan) {
        struct SlicksColourPicker s={{(unsigned char)value,(unsigned char)(value+79),(unsigned char)(value+173)},(unsigned char)channel,0};
        check(uc_mem_write(u,0x8eff6,s.rgb,3)); check(uc_mem_write(u,0x8eff9,&s.channel,1));
        check(uc_mem_write(u,0x8efe6,&s.result,1)); frame(u,scan);
        check(uc_emu_start(u,0x2f5be,0x2f676,0,1000));
        uint16_t ip,sp; check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        slicks_colour_picker_key(&s,(unsigned char)scan);
        unsigned char actual[5]; check(uc_mem_read(u,0x8eff6,actual,4)); check(uc_mem_read(u,0x8efe6,actual+4,1));
        if(ip!=0x2f676-0x2e0f0 || sp!=0xeb00 || memcmp(actual,&s,5)) {
            fprintf(stderr,"Colour key mismatch value=%u channel=%u scan=%u\n",value,channel,scan); return 1;
        }
        ++cases;
    }
    for(unsigned value=0;value<256;++value) {
        struct SlicksColourPicker s={{(unsigned char)value,(unsigned char)(value+79),(unsigned char)(value+173)},0,(signed char)value};
        unsigned char output[3]={0xa5,0x5a,0xcc},actual[3];
        check(uc_mem_write(u,0x60000,output,3));
        check(uc_mem_write(u,0x8eff6,s.rgb,3)); check(uc_mem_write(u,0x8efe6,&s.result,1));
        for(unsigned i=0;i<3;++i) { word(u,0x8f00e +4*i,i); word(u,0x8f010+4*i,0x6000); }
        frame(u,0); check(uc_emu_start(u,0x2f6ad,0x2f6d6,0,1000));
        uint16_t ip,ax; check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_AX,&ax));
        unsigned result=slicks_colour_picker_finish(&s,output);
        check(uc_mem_read(u,0x60000,actual,3));
        if(ip!=0x2f6d6-0x2e0f0 || (ax&255)!=result || memcmp(output,actual,3)) abort();
    }
    verify_draw(u);
    check(uc_close(u));
    verify_pixels(runtime,bytes);
    printf("DOS colour picker: %u original key/state comparisons and 256 commit/cancel comparisons pass\n",cases);
    return 0;
}
