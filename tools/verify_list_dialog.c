#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/ui/list_dialog.h"
#include "../src/ui/list_dialog_draw.h"

static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned a,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(u,a,b,2)); }
static unsigned rd(uc_engine *u,unsigned a,unsigned n)
{ unsigned char b[2]={0,0}; check(uc_mem_read(u,a,b,n)); return b[0]|b[1]<<8; }
static void stop(uc_engine *u,uint64_t a,uint32_t n,void *p)
{ (void)a; (void)n; (void)p; check(uc_emu_stop(u)); }
static void put(uc_engine *u,const struct SlicksListDialog *s)
{
    word(u,0x8f01a,s->selected); word(u,0x8effc,s->initial);
    word(u,0x8f018,s->count); word(u,0x8efea,s->visible);
    word(u,0x8efec,s->top); word(u,0x8eff4,s->actions); word(u,0x8efd6,s->pulse);
    check(uc_mem_write(u,0x8efff,&s->action,1));
    check(uc_mem_write(u,0x8efd1,&s->focus_actions,1));
    check(uc_mem_write(u,0x8efd3,&s->redraw_actions,1));
    check(uc_mem_write(u,0x8efd4,&s->redraw_list,1));
    check(uc_mem_write(u,0x8efd2,&s->done,1));
    word(u,0x8f020,s->flags);
}
static int equal(uc_engine *u,const struct SlicksListDialog *s)
{
    return (short)rd(u,0x8f01a,2)==s->selected && (short)rd(u,0x8effc,2)==s->initial &&
        (short)rd(u,0x8f018,2)==s->count && (short)rd(u,0x8efea,2)==s->visible &&
        (short)rd(u,0x8efec,2)==s->top && (short)rd(u,0x8eff4,2)==s->actions &&
        (short)rd(u,0x8efd6,2)==s->pulse && rd(u,0x8efff,1)==s->action &&
        rd(u,0x8efd1,1)==s->focus_actions && rd(u,0x8efd3,1)==s->redraw_actions &&
        rd(u,0x8efd4,1)==s->redraw_list && rd(u,0x8efd2,1)==s->done && rd(u,0x8f020,1)==s->flags;
}
static void verify_init(uc_engine *u)
{
    const short counts[]={0,1,3,10,100,101,127,128,255,256,512,1000,1560,2849}, heights[]={50,100,180};
    const unsigned char fonts[]={5,6,9};
    const short selected[]={-3,-2,-1,0,1,9,50,99};
    const short captions[]={-1,0,1,2,127,128,255};
    unsigned cases=0;
    for(unsigned c=0;c<sizeof counts/sizeof counts[0];++c) for(unsigned h=0;h<3;++h)
    for(unsigned f=0;f<3;++f) for(unsigned n=0;n<8;++n)
    for(short actions=0;actions<4;++actions) for(unsigned cap=0;cap<7;++cap) {
        short caption=captions[cap],selection=n==6?counts[c]/2:n==7?counts[c]-1:selected[n];
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xe000,bp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        word(u,0x8f01a,selection); word(u,0x8f018,counts[c]); word(u,0x8eff4,actions);
        word(u,0x8f008,7); word(u,0x8f00c,7+heights[h]);
        word(u,0x8f01c,0); word(u,0x8f01e,0x6000); word(u,0x8f020,n&1);
        check(uc_mem_write(u,0x60002,&fonts[f],1));
        check(uc_emu_start(u,0x30cca,0x30ce7,0,100));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); if(ip!=0x30ce7-0x2e0f0) abort();
        if(caption>=0) { unsigned char a=(unsigned char)caption; check(uc_mem_write(u,0x8efff,&a,1)); }
        check(uc_emu_start(u,0x30f39,0x30fa9,0,1000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); if(ip!=0x30fa9-0x2e0f0) abort();
        check(uc_emu_start(u,0x3113c,0x31155,0,100));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); if(ip!=0x31155-0x2e0f0) abort();
        struct SlicksListDialog s; short thumb;
        if(slicks_list_dialog_init(&s,selection,counts[c],heights[h],fonts[f],actions,caption,n&1,&thumb) ||
           !equal(u,&s) || (s.visible<s.count && thumb!=(short)rd(u,0x8efe8,2))) {
            fprintf(stderr,"List init mismatch count=%d height=%d font=%u selected=%d actions=%d caption=%d\n",
                counts[c],heights[h],fonts[f],selection,actions,caption); exit(1);
        }
        ++cases;
    }
    struct SlicksListDialog s, before; memset(&s,0xa5,sizeof s); before=s;
    short thumb=123;
    if(slicks_list_dialog_init(&s,0,3,10,6,1,-1,1,&thumb)!=-1 ||
       memcmp(&s,&before,sizeof s) || thumb!=123) abort();
    printf("DOS list initialization: %u selection/action/scroll/geometry comparisons pass; invalid geometry is atomic\n",cases);
}
struct DrawCall { short kind,args[6]; };
struct DrawTrace { struct DrawCall calls[100]; unsigned count; unsigned char colour; };
static struct DrawCall *call(struct DrawTrace *t,short kind)
{ if(t->count==100) abort(); struct DrawCall *c=&t->calls[t->count++]; c->kind=kind; return c; }
static void restore(void *p,short x,short y,short sx,short sy,short w,short h)
{ short a[]={x,y,sx,sy,w,h}; memcpy(call(p,0)->args,a,sizeof a); }
static void rectangle(void *p,short x,short y,short r,short b,unsigned char colour)
{ short a[]={x,y,r,b,colour}; memcpy(call(p,1)->args,a,sizeof a); }
static void text(void *p,short index,short x,short y)
{ short a[]={index,x,y}; memcpy(call(p,2)->args,a,sizeof a); }
static unsigned char font_colour(void *p,unsigned char value)
{ struct DrawTrace *t=p; call(t,3)->args[0]=value; unsigned char old=t->colour; t->colour=value; return old; }
static void draw_boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)size; uint16_t ss,sp,cs,ip;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=16U*ss+sp; short a[6];
    for(unsigned i=0;i<6;++i) a[i]=(short)rd(u,stack+4+2*i,2);
    if(address==0x3b9de) restore(p,a[0],a[1],a[2],a[3],a[4],(unsigned char)a[5]);
    else if(address==0x39ed8) rectangle(p,a[0],a[1],a[2],a[3],(unsigned char)a[4]);
    else if(address==0x2fe63) {
        uint16_t ax=font_colour(p,(unsigned char)a[0]); check(uc_reg_write(u,UC_X86_REG_AX,&ax));
    } else {
        if((unsigned short)a[3]!=0x6100 || (unsigned short)a[2]%21) abort();
        text(p,(short)((unsigned short)a[2]/21),a[0],a[1]);
    }
    ip=(uint16_t)rd(u,stack,2); cs=(uint16_t)rd(u,stack+2,2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void verify_draw(uc_engine *u)
{
    uc_hook hooks[4]; struct DrawTrace dos;
    const unsigned addresses[]={0x3b9de,0x39ed8,0x301ab,0x2fe63};
    for(unsigned i=0;i<4;++i) check(uc_hook_add(u,&hooks[i],UC_HOOK_CODE,draw_boundary,&dos,addresses[i],addresses[i]));
    const short counts[]={0,1,3,10,100,101,127,128,255,256,512,1000,1560,2849}; const unsigned char fonts[]={5,6,9};
    unsigned cases=0;
    for(unsigned c=0;c<sizeof counts/sizeof counts[0];++c) for(unsigned f=0;f<3;++f)
    for(unsigned v=0;v<12;++v) for(unsigned dirty=0;dirty<5;++dirty) {
        struct SlicksListDialog s={0}; short thumb;
        short selected=v%3==0?0:v%3==1?counts[c]/2:counts[c]-1;
        if(slicks_list_dialog_init(&s,selected,counts[c],100,fonts[f],1,-1,1,&thumb)) abort();
        slicks_list_dialog_normalize(&s);
        s.redraw_list=dirty==0?255:dirty==1?1:(unsigned char)(s.selected?s.selected:1);
        if(dirty>=3) { s.redraw_list=0; s.focus_actions=(unsigned char)(dirty==4 || s.count==0); }
        put(u,&s);
        short left=(short)(120+v),top=(short)(15+v),right=270;
        word(u,0x8f006,left); word(u,0x8f008,top); word(u,0x8f00a,right);
        word(u,0x8f00c,top+100); word(u,0x8efe8,thumb);
        word(u,0x8f01c,0); word(u,0x8f01e,0x6000);
        word(u,0x8f012,0); word(u,0x8f014,0x6100); word(u,0x8f016,21);
        check(uc_mem_write(u,0x60002,&fonts[f],1));
        unsigned char colours[]={11,23,37};
        check(uc_mem_write(u,0x8efe6,&colours[0],1)); check(uc_mem_write(u,0x8efe5,&colours[1],1));
        check(uc_mem_write(u,0x8efe4,&colours[2],1));
        unsigned char highlight=191; check(uc_mem_write(u,0x8efbd,&highlight,1));
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xe000,bp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        memset(&dos,0,sizeof dos); dos.colour=73;
        check(uc_emu_start(u,0x31252,0x315a7,0,10000));
        struct DrawTrace native={0}; const struct SlicksListDialogDrawOps ops={restore,rectangle,text,&native};
        native.colour=73;
        if(s.redraw_list) {
            slicks_list_dialog_draw_rows(&s,left,top,right,fonts[f],colours,&ops);
            slicks_list_dialog_draw_scrollbar(&s,left,top,right,top+100,thumb,colours,&ops);
        } else slicks_list_dialog_draw_active(&s,left,top,fonts[f],highlight,font_colour,&ops);
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        if(ip!=0x315a7-0x2e0f0 || sp!=0xe000 || memcmp(&dos,&native,sizeof dos) ||
           s.redraw_list!=rd(u,0x8efd4,1)) {
            fprintf(stderr,"List row draw mismatch count=%d font=%u variant=%u dirty=%u DOS=%u native=%u\n",
                s.count,fonts[f],v,dirty,dos.count,native.count); exit(1);
        }
        ++cases;
    }
    for(unsigned i=0;i<4;++i) check(uc_hook_del(u,hooks[i]));
    printf("DOS list rows/scrollbar/active text: %u complete drawing and font-colour comparisons pass\n",cases);
}
struct PulseQuery { unsigned char rgb[3]; unsigned count; };
static unsigned char pulse_nearest(void *p,unsigned char r,unsigned char g,unsigned char b)
{
    struct PulseQuery *q=p; q->rgb[0]=r; q->rgb[1]=g; q->rgb[2]=b; ++q->count;
    return 173;
}
static void pulse_boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)address; (void)size; uint16_t ss,sp,cs,ip,ax;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=16U*ss+sp;
    ax=pulse_nearest(p,rd(u,stack+4,1),rd(u,stack+6,1),rd(u,stack+8,1));
    ip=rd(u,stack,2); cs=rd(u,stack+2,2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_CS,&cs));
    check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void verify_pulse(uc_engine *u)
{
    struct PulseQuery dos; uc_hook hook;
    check(uc_hook_add(u,&hook,UC_HOOK_CODE,pulse_boundary,&dos,0x36fae,0x36fae));
    const unsigned long ticks[]={0,1,0xffff,0x10000,0xffffffffUL}; unsigned cases=0;
    for(short value=-7;value<=57;++value) for(short step=-7;step<=7;step+=14)
    for(unsigned t=0;t<5;++t) for(unsigned changed=0;changed<2;++changed) {
        struct SlicksListDialog s={0}; s.pulse=value;
        struct SlicksListPulse pulse={ticks[t],(signed char)step};
        unsigned long tick=changed?(ticks[t]+17)&0xffffffffUL:ticks[t];
        word(u,0x8efd6,value); unsigned char byte=(unsigned char)step;
        check(uc_mem_write(u,0x8efd5,&byte,1));
        word(u,0x8efdc,pulse.tick); word(u,0x8efde,pulse.tick>>16);
        word(u,0x8efd8,0x6c); word(u,0x8efda,0x40);
        word(u,0x46c,tick); word(u,0x46e,tick>>16);
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xe000,bp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp)); memset(&dos,0,sizeof dos);
        check(uc_emu_start(u,0x311b5,0x31223,0,1000));
        struct PulseQuery native={0};
        unsigned char colour=slicks_list_dialog_pulse(&s,&pulse,tick,pulse_nearest,&native);
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        unsigned long actual=rd(u,0x8efdc,2)|((unsigned long)rd(u,0x8efde,2)<<16);
        if(ip!=0x31223-0x2e0f0 || sp!=0xe000 || memcmp(&dos,&native,sizeof dos) ||
           colour!=rd(u,0x8efbd,1) || s.pulse!=(short)rd(u,0x8efd6,2) ||
           pulse.step!=(signed char)rd(u,0x8efd5,1) || pulse.tick!=actual) {
            fprintf(stderr,"List pulse mismatch value=%d step=%d tick=%lx changed=%u\n",value,step,ticks[t],changed); exit(1);
        }
        ++cases;
    }
    check(uc_hook_del(u,hook));
    printf("DOS list pulse: %u palette-query/tick/overshoot comparisons pass\n",cases);
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t size=fread(runtime,1,sizeof runtime,f); int error=ferror(f); fclose(f);
    if(error || size<200000 || size==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,size));
    verify_init(u);
    verify_draw(u);
    verify_pulse(u);
    /* The pulse starts exactly at the scroll oracle's stop address. Use a
     * fresh engine so cached translated blocks cannot cross that boundary. */
    check(uc_close(u)); check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,size));
    /* Tab jumps to the keyboard-drain boundary before the next update. */
    uc_hook hook; check(uc_hook_add(u,&hook,UC_HOOK_CODE,stop,NULL,0x31155,0x31155));
    unsigned cases=0;
    const short counts[]={0,1,3,10,100,101,127,128,255,256,512,1000,1560,2849},visible[]={1,7,11};
    for(unsigned c=0;c<sizeof counts/sizeof counts[0];++c) for(unsigned v=0;v<3;++v)
    for(unsigned variant=0;variant<16;++variant) for(unsigned scan=0;scan<256;++scan) {
        struct SlicksListDialog s={0};
        s.count=counts[c]; s.visible=visible[v]; s.actions=(short)(variant%4);
        s.selected=variant%3==0?0:variant%3==1?s.count-1:s.count/2;
        s.initial=s.count/2; s.top=variant%2?s.count/2:0;
        s.action=(unsigned char)(s.actions?variant%s.actions:0);
        s.focus_actions=(variant>>2)&1; s.redraw_actions=variant&1;
        s.redraw_list=7; s.pulse=41; s.flags=(variant>>3)&1;
        put(u,&s);
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xe000,bp=0xf000,ax=(uint16_t)(int16_t)(int8_t)scan;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp)); check(uc_reg_write(u,UC_X86_REG_AX,&ax));
        check(uc_emu_start(u,0x315b6,0x316cf,0,1000));
        uint16_t ip; check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        if(ip!=0x316cf-0x2e0f0 && ip!=0x31155-0x2e0f0) { fputs("DOS key boundary not reached\n",stderr); return 1; }
        slicks_list_dialog_key(&s,(unsigned char)scan);
        if(!equal(u,&s)) { fprintf(stderr,"List key mismatch count=%d visible=%d variant=%u scan=%02x\n",s.count,s.visible,variant,scan); return 1; }
        if(!s.done) {
            check(uc_emu_start(u,0x3115d,0x311b5,0,1000));
            check(uc_reg_read(u,UC_X86_REG_IP,&ip));
            if(ip!=0x311b5-0x2e0f0) { fputs("DOS scroll boundary not reached\n",stderr); return 1; }
            slicks_list_dialog_normalize(&s);
            if(!equal(u,&s)) { fprintf(stderr,"List scroll mismatch count=%d variant=%u scan=%02x\n",s.count,variant,scan); return 1; }
        }
        check(uc_emu_start(u,0x31760,0x31773,0,100));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_AX,&ax));
        if(ip!=0x31773-0x2e0f0 || (short)ax!=slicks_list_dialog_result(&s)) {
            fputs("DOS list result mismatch\n",stderr); return 1;
        }
        ++cases;
    }
    check(uc_close(u));
    printf("DOS list dialog: %u key/state, next-update scroll and encoded-result comparisons pass\n",cases);
    return 0;
}
