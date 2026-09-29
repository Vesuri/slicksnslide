#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include <unicorn/m68k.h>
#include "../src/ui/palette_remap.h"
#include "../src/ui/player_menu_prepare.h"
#include "../src/ui/player_menu_renderer.h"
#include "../src/ui/font_resource.h"
#include "../src/ui/menu_bitmap.h"
#include "../src/ui/menu_icon.h"
#include "host_archive.h"
#include "../src/ui/language_table.h"
static void check(uc_err e)
{ if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned at,unsigned value)
{ unsigned char bytes[2]={value,value>>8}; check(uc_mem_write(u,at,bytes,2)); }
static unsigned getword(uc_engine *u,unsigned at)
{ unsigned char b[2]; check(uc_mem_read(u,at,b,2)); return b[0]|b[1]<<8; }
/* The decoder has a separate original-instruction oracle. These painting
 * checks run real DOS lookup code over its independently verified output. */
static const unsigned char *menu_language_title(uc_engine *u,unsigned language,
    const unsigned char *key)
{
    static unsigned char table[2000];unsigned used=0;
    if(language){
        unsigned char resource[2000];char name[10];
        if(slicks_language_resource(name,language))abort();
        long size=host_archive_load("ref/SLICKS.000",name,resource,sizeof resource);
        if(size<=0 || slicks_language_table_load(resource,(unsigned)size,table,sizeof table,&used))abort();
        check(uc_mem_write(u,0x58000,table,used));
    }
    word(u,0x3cbf0+0x1722,0);word(u,0x3cbf0+0x1724,language?0x5800:0);
    return slicks_language_lookup(language?table:0,used,key,key);
}
struct Vga { unsigned char pixels[64000]; unsigned read_plane,write_plane,mask;
    unsigned allow_margin; unsigned char margin[16000]; };
static void select_plane(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size; struct Vga *v=context; uint16_t ss,sp,cs,ip;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp,plane=getword(u,stack+4)&3;
    if(address==0x3b4f6) { v->write_plane=plane; v->mask=1U<<plane; } else v->read_plane=plane;
    ip=getword(u,stack); cs=getword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void vga_access(uc_engine *u,uc_mem_type type,uint64_t address,int size,int64_t value,void *context)
{
    struct Vga *v=context; unsigned offset=(unsigned)(address-0xa0000);
    for(int byte=0;byte<size;++byte,++offset) {
        if(offset>=20000 || offset%100>=80) {
            if(v->allow_margin && offset<20000) {
                unsigned pixel=((offset/100)*20+offset%100-80)*4;
                if(type==UC_MEM_READ) check(uc_mem_write(u,address+byte,v->margin+pixel+v->read_plane,1));
                else for(unsigned p=0;p<4;++p) if(v->mask&(1U<<p))
                    v->margin[pixel+p]=(unsigned char)((uint64_t)value>>(8*byte));
            } else if(type==UC_MEM_READ) abort();
            continue;
        }
        unsigned pixel=(offset/100)*320+(offset%100)*4;
        if(type==UC_MEM_READ) check(uc_mem_write(u,address+byte,v->pixels+pixel+v->read_plane,1));
        else for(unsigned p=0;p<4;++p) if(v->mask&(1U<<p))
            v->pixels[pixel+p]=(unsigned char)((uint64_t)value>>(8*byte));
    }
}
struct Dirty { unsigned count; short bounds[4]; };
static void dirty(void *context,short left,short top,short right,short bottom)
{
    struct Dirty *d=context; ++d->count;
    d->bounds[0]=left; d->bounds[1]=top; d->bounds[2]=right; d->bounds[3]=bottom;
}
static void verify_rectangles(uc_engine *u)
{
    struct Vga v; uc_hook hooks[3];
    check(uc_hook_add(u,&hooks[0],UC_HOOK_CODE,select_plane,&v,0x3b52b,0x3b52b));
    check(uc_hook_add(u,&hooks[1],UC_HOOK_CODE,select_plane,&v,0x3b4f6,0x3b4f6));
    check(uc_hook_add(u,&hooks[2],UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,vga_access,&v,0xa0000,0xaffff));
    const short rectangles[][4]={{0,0,80,11},{0,11,80,12},{48,29,225,40},
        {48,45,225,56},{48,61,225,72},{48,77,225,88},{50,105,120,157},
        {0,0,320,200},{319,199,320,200},{1,1,2,2},{2,2,5,3},{3,3,7,4},
        {5,7,5,8},{0,200,320,200}};
    unsigned cases=0;
    for(unsigned pattern=0;pattern<8;++pattern)
    for(unsigned r=0;r<sizeof rectangles/sizeof rectangles[0];++r) {
        unsigned char native[64000],table[256]; struct Dirty d={0,{0,0,0,0}};
        for(unsigned i=0;i<256;++i) table[i]=(unsigned char)(i*(pattern+1)+37);
        for(unsigned i=0;i<64000;++i) native[i]=v.pixels[i]=(unsigned char)(i*13+(i>>8)*17+pattern);
        struct SlicksChunkyUi ui={native,0,dirty,&d}; const short *rect=rectangles[r];
        if(slicks_ui_remap(&ui,rect[0],rect[1],rect[2],rect[3],table)) abort();
        unsigned nonempty=rect[0]<rect[2] && rect[1]<rect[3];
        if(d.count!=nonempty || (nonempty && memcmp(d.bounds,rect,sizeof d.bounds))) abort();
        check(uc_mem_write(u,0x61000,table,sizeof table)); word(u,0x3cbf0+0x1d7b,100);
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
        word(u,0x8f000,0); word(u,0x8f002,0x9000);
        for(unsigned i=0;i<4;++i) word(u,0x8f004+2*i,(unsigned short)rect[i]);
        word(u,0x8f00c,0); word(u,0x8f00e,0x6100); word(u,0x8f010,0);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x34599,0x90000,0,5000000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        if(ip || sp!=0xf004 || memcmp(native,v.pixels,sizeof native)) {
            fprintf(stderr,"Remap mismatch pattern=%u rectangle=%u ip=%x\n",pattern,r,ip); exit(1);
        }
        ++cases;
    }
    for(unsigned i=0;i<3;++i) check(uc_hook_del(u,hooks[i]));
    printf("Original remap painter: %u full-frame comparisons and exact dirty bounds pass\n",cases);
}
struct TextCall { unsigned font; short x,y; unsigned char flags,colour; unsigned char text[128]; };
struct TextTrace { struct TextCall calls[2]; unsigned count; unsigned char *fonts[3]; };
static void text_call(void *context,unsigned font,const unsigned char *text,short x,short y,unsigned char flags)
{
    struct TextTrace *trace=context; if(trace->count>=2 || strlen((const char *)text)>=128) abort();
    struct TextCall *call=&trace->calls[trace->count++];
    call->font=font; call->x=x; call->y=y; call->flags=flags; call->colour=trace->fonts[font][6];
    strcpy((char *)call->text,(const char *)text);
}
static void prepare_boundary(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size; struct TextTrace *trace=context; uint16_t ss,sp,cs,ip,ax=0,dx=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    if(address==0x301ab) {
        unsigned font_at=getword(u,stack+12)+16*getword(u,stack+14);
        unsigned font=(font_at-0x62000)/32;
        if(font>2) abort();
        check(uc_mem_read(u,font_at,trace->fonts[font],16));
        unsigned char text[128]; unsigned at=getword(u,stack+8)+16*getword(u,stack+10);
        check(uc_mem_read(u,at,text,sizeof text)); text[127]=0;
        text_call(trace,font,text,(short)getword(u,stack+4),(short)getword(u,stack+6),(unsigned char)getword(u,stack+16));
    }
    ip=getword(u,stack); cs=getword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void verify_prepare(uc_engine *u)
{
    struct Vga v; struct TextTrace dos; uc_hook hooks[5];
    check(uc_hook_add(u,&hooks[0],UC_HOOK_CODE,select_plane,&v,0x3b52b,0x3b52b));
    check(uc_hook_add(u,&hooks[1],UC_HOOK_CODE,select_plane,&v,0x3b4f6,0x3b4f6));
    check(uc_hook_add(u,&hooks[2],UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,vga_access,&v,0xa0000,0xaffff));
    check(uc_hook_add(u,&hooks[3],UC_HOOK_CODE,prepare_boundary,&dos,0x301ab,0x301ab));
    check(uc_hook_add(u,&hooks[4],UC_HOOK_CODE,prepare_boundary,&dos,0x2e0f8,0x2e0f8));
    unsigned char title[128],footer[128];
    check(uc_mem_read(u,0x3cbf0+0x1365,title,sizeof title));
    check(uc_mem_read(u,0x3cbf0+0x136d,footer,sizeof footer));
    for(unsigned pattern=0;pattern<24;++pattern) {
        unsigned char native[64000],palette[768],fonts[3][16],dos_fonts[3][16];
        struct TextTrace trace={0}; memset(&dos,0,sizeof dos);
        for(unsigned f=0;f<3;++f) {
            for(unsigned i=0;i<16;++i) fonts[f][i]=dos_fonts[f][i]=(unsigned char)(i+f+20);
            fonts[f][5]=dos_fonts[f][5]=2;
            trace.fonts[f]=fonts[f]; dos.fonts[f]=dos_fonts[f];
            check(uc_mem_write(u,0x62000+32*f,fonts[f],16));
            word(u,0x3cbf0+0x680+4*f,32*f); word(u,0x3cbf0+0x682+4*f,0x6200);
        }
        for(unsigned i=0;i<768;++i) palette[i]=(unsigned char)((i*19+pattern*37+(i>>4)*7)&63);
        for(unsigned i=0;i<64000;++i) native[i]=v.pixels[i]=(unsigned char)(i*7+(i>>9)+pattern);
        check(uc_mem_write(u,0x3cbf0+0x71bc,palette,sizeof palette));
        word(u,0x3cbf0+0x71b8,0x71bc); word(u,0x3cbf0+0x71ba,0x3cbf);
        word(u,0x3cbf0+0x1d7b,100); word(u,0x3cbf0+0x1d87,0);
        unsigned char percent=pattern?pattern*11:66;
        word(u,0x3cbf0+0x16fd,percent);
        struct SlicksChunkyUi ui={native,palette,0,0};
        const struct SlicksPlayerMenuPrepareOps ops={text_call,&trace};
        slicks_prepare_player_menu(&ui,trace.fonts,title,footer,percent,&ops);
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xebb0,bp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_emu_start(u,0x283c3,0x285ec,0,30000000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        for(unsigned f=0;f<3;++f) check(uc_mem_read(u,0x62000+32*f,dos_fonts[f],16));
        if(ip!=0x1f2c || sp!=0xebb0 || memcmp(native,v.pixels,sizeof native) ||
           trace.count!=dos.count || memcmp(trace.calls,dos.calls,sizeof trace.calls) || memcmp(fonts,dos_fonts,sizeof fonts)) {
            fprintf(stderr,"Player-menu preparation mismatch pattern=%u ip=%x sp=%x texts=%u/%u pixels=%d fonts=%d calls=%d\n",
                pattern,ip,sp,trace.count,dos.count,memcmp(native,v.pixels,sizeof native),memcmp(fonts,dos_fonts,sizeof fonts),memcmp(trace.calls,dos.calls,sizeof trace.calls)); exit(1);
        }
    }
    for(unsigned i=0;i<5;++i) check(uc_hook_del(u,hooks[i]));
    puts("Original player-menu preparation: 24 complete background/font-state/text-command comparisons pass (text rasterization excluded)");
}
#include "../src/ui/profile_editor_renderer.h"

struct NativeText {
    uc_engine *cpu,*icon_cpu; unsigned entry,bridge,records_bridge;
    unsigned char *pixels,*fonts[3]; unsigned sizes[3];
    unsigned spacing,tab,highlight,shadow;
};
static void native_text(void *context,unsigned font,const unsigned char *text,short x,short y,unsigned char flags)
{
    struct NativeText *n=context; uc_engine *u=n->cpu;
    check(uc_mem_write(u,0x100000,n->pixels,64000));
    check(uc_mem_write(u,0x50000,n->fonts[font],n->sizes[font]));
    check(uc_mem_write(u,0x60000,text,strlen((const char *)text)+1));
    const int regs[]={UC_M68K_REG_D0,UC_M68K_REG_D1,UC_M68K_REG_D2,
        UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,UC_M68K_REG_D6,
        UC_M68K_REG_A0,UC_M68K_REG_A1,UC_M68K_REG_A2};
    uint32_t values[]={(unsigned short)x,(unsigned short)y,flags,n->spacing,n->tab,n->highlight,n->shadow,
        0x100000,0x50000,0x60000};
    for(unsigned i=0;i<10;++i) check(uc_reg_write(u,regs[i],&values[i]));
    uint32_t stack=0x300000,sr=0,pc;
    uint32_t arguments[]={0x380000,0x100000,0x50000,0x60000,(unsigned short)x,(unsigned short)y,flags,n->highlight};
    unsigned char argument_bytes[32];
    for(unsigned i=0;i<8;++i) for(unsigned b=0;b<4;++b) argument_bytes[4*i+b]=(unsigned char)(arguments[i]>>(24-8*b));
    if(n->spacing!=1 || n->tab!=10 || (!n->records_bridge && n->highlight) || (n->shadow!=0x0100 && n->shadow!=0x0101)) abort();
    check(uc_reg_write(u,UC_M68K_REG_SR,&sr)); check(uc_reg_write(u,UC_M68K_REG_A7,&stack));
    check(uc_mem_write(u,stack,argument_bytes,sizeof argument_bytes));
    check(uc_emu_start(u,n->records_bridge?n->records_bridge:n->bridge,0x380000,0,1000000));
    check(uc_reg_read(u,UC_M68K_REG_PC,&pc)); check(uc_reg_read(u,UC_M68K_REG_A7,&stack));
    if(pc!=0x380000 || stack!=0x300004) abort();
    for(unsigned i=2;i<10;++i) if(i!=7 && i!=8) {
        uint32_t actual; check(uc_reg_read(u,regs[i],&actual)); if(actual!=values[i]) abort();
    }
    check(uc_mem_read(u,0x100000,n->pixels,64000));
}
static void font_port(uc_engine *u,uint32_t port,int size,uint32_t value,void *context)
{
    (void)u; struct Vga *v=context;
    if(port==0x3ce && size==2 && (value&255)==4 && (value>>8)<4) {
        v->read_plane=value>>8; return;
    }
    if(port==0x3ce && size==1 && value==4) return;
    if(port==0x3cf && size==1 && value<4) { v->read_plane=value; return; }
    if(port==0x3c4 && size==1 && value==2) return;
    if(port==0x3c5 && size==1) { v->mask=value&15; return; }
    if(port==0x3c4 && size==2 && (value&255)==2) { v->mask=(value>>8)&15; return; }
    abort();
}
static void renderer_text(void *context,struct SlicksChunkyUi *ui,unsigned char *font,
    const unsigned char *text,short x,short y,unsigned char flags)
{
    struct NativeText *n=context;
    if(ui->pixels!=n->pixels) abort();
    for(unsigned i=0;i<3;++i) if(n->fonts[i]==font) { native_text(n,i,text,x,y,flags); return; }
    abort();
}
static void renderer_icon(void *context,struct SlicksChunkyUi *ui,const struct SlicksMenuIcon *icon,short x,short y)
{
    struct NativeText *n=context; uc_engine *m=n->icon_cpu;
    check(uc_mem_write(m,0x100000,ui->pixels,64000)); check(uc_mem_write(m,0x50000,icon->pixels,icon->width*icon->height));
    const int regs[]={UC_M68K_REG_D0,UC_M68K_REG_D1,UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_A0,UC_M68K_REG_A1};
    uint32_t values[]={(unsigned short)x,(unsigned short)y,icon->width,icon->height,0x100000,0x50000},stack=0x300000,sr=0,pc;
    for(unsigned i=0;i<6;++i) check(uc_reg_write(m,regs[i],&values[i]));
    unsigned char ret[]={0,0x38,0,0}; check(uc_mem_write(m,stack,ret,4));
    check(uc_reg_write(m,UC_M68K_REG_SR,&sr)); check(uc_reg_write(m,UC_M68K_REG_A7,&stack));
    check(uc_emu_start(m,0,0x380000,0,100000)); check(uc_reg_read(m,UC_M68K_REG_PC,&pc)); if(pc!=0x380000) abort();
    check(uc_mem_read(m,0x100000,ui->pixels,64000));
}
static void verify_editor_pixels(uc_engine *u,struct Vga *v,struct SlicksPlayerMenuRenderer *r)
{
    unsigned char rows[48],roles[18],percent[64],random[64],each[64],unavailable[64];
    check(uc_mem_read(u,0x3cbf0+0x10ce,rows,sizeof rows));
    check(uc_mem_read(u,0x3cbf0+0x10fe,roles,sizeof roles));
    check(uc_mem_read(u,0x3cbf0+0x1280,percent,sizeof percent));
    check(uc_mem_read(u,0x3cbf0+0x1300,random,sizeof random));
    check(uc_mem_read(u,0x3cbf0+0x1314,each,sizeof each));
    check(uc_mem_read(u,0x3cbf0+0x132a,unavailable,sizeof unavailable));
    struct SlicksProfileEditorLabels labels={{rows,rows+8,rows+16,rows+24,rows+32,rows+40},
        {roles,roles+9},percent,random,each,unavailable};
    const unsigned char vehicles[]={0,1,9,10,11,12,127,128,255};
    for(unsigned variant=0;variant<18;++variant) for(unsigned row=0;row<6;++row) {
        struct SlicksPlayerProfiles p={0}; unsigned index=3;
        p.setup[index].vehicle=vehicles[variant%9]; p.setup[index].flags=(unsigned char)variant;
        p.setting[index]=(unsigned char)(variant*15);
        for(unsigned j=0;j<6;++j) p.setup[index].colours[j]=(unsigned char)((variant*17+j*31)%64);
        unsigned char name[21]={0}; for(unsigned j=0;j<variant%21;++j) name[j]='A'+j;
        struct SlicksProfileEditor e={(unsigned char)row,(unsigned char)(row?255:1),0};
        check(uc_mem_write(u,0x8efa8,rows,sizeof rows)); check(uc_mem_write(u,0x8ef96,roles,sizeof roles));
        check(uc_mem_write(u,0x8efd8,name,sizeof name));
        check(uc_mem_write(u,0x8effc,&e.row,1)); check(uc_mem_write(u,0x8effb,&e.redraw,1));
        unsigned char colour=slicks_ui_nearest(&r->ui,60,60,40); check(uc_mem_write(u,0x8effe,&colour,1));
        colour=slicks_ui_nearest(&r->ui,50,50,30); check(uc_mem_write(u,0x8effd,&colour,1));
        word(u,0x8f006,index); word(u,0x3cbf0+0x4c6e,10);
        check(uc_mem_write(u,0x3cbf0+0x3ede +index,&p.setup[index].flags,1));
        check(uc_mem_write(u,0x3cbf0+0x3fa6+index,&p.setup[index].vehicle,1));
        check(uc_mem_write(u,0x3cbf0+0x3f42+index,&p.setting[index],1));
        check(uc_mem_write(u,0x3cbf0+0x1db+6*index,p.setup[index].colours,6));
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xebb0,bp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_emu_start(u,0x27bea,0x27f53,0,10000000)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        if(slicks_profile_editor_renderer_draw(r,&e,&p,index,name,10,&labels)) abort();
        unsigned char font[8192]; struct NativeText *n=r->context; check(uc_mem_read(u,0x60000,font,n->sizes[0]));
        if(ip!=0x27f53-0x266c0 || memcmp(r->ui.pixels,v->pixels,64000) || memcmp(font,r->fonts[0],n->sizes[0])) {
            fprintf(stderr,"Editor pixels/font mismatch variant=%u row=%u ip=%x\n",variant,row,ip);
            for(unsigned i=0;i<64000;++i) if(r->ui.pixels[i]!=v->pixels[i]) { fprintf(stderr,"pixel %u,%u native=%u DOS=%u\n",i%320,i/320,r->ui.pixels[i],v->pixels[i]); break; }
            exit(1);
        }
    }
    puts("Original profile editor: 108 sequential full-frame/font-state comparisons with real DOS/68020 text and icons pass");
}
static void verify_dynamic(uc_engine *u,struct Vga *v,struct SlicksPlayerMenuRenderer *r)
{
    struct NativeText *n=r->context; unsigned char code[4096]; FILE *f=fopen("build/hud_icon_test.bin","rb"); if(!f) abort();
    size_t size=fread(code,1,sizeof code,f); fclose(f); if(!size || size==sizeof code) abort();
    check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&n->icon_cpu)); check(uc_ctl_set_cpu_model(n->icon_cpu,UC_CPU_M68K_M68020));
    check(uc_mem_map(n->icon_cpu,0,0x400000,UC_PROT_ALL)); check(uc_mem_write(n->icon_cpu,0,code,size));
    unsigned char icon_pixels[11][256]; struct SlicksMenuIcon icons[11];
    for(unsigned i=0;i<11;++i) {
        char name[32]; if(!i) strcpy(name,"computer.@16"); else if(i==1) strcpy(name,"carimage16"); else snprintf(name,sizeof name,"auto%02u.@16",i-1);
        unsigned char resource[1024],runtime[1024]; long bytes=host_archive_load("ref/SLICKS.000",name,resource,sizeof resource);
        if(bytes<0 || slicks_decode_menu_icon(resource,(unsigned long)bytes,r->ui.palette,icon_pixels[i],256,&icons[i].width,&icons[i].height)) abort();
        icons[i].pixels=icon_pixels[i];
        /* Original unconverted B1 pixels, independent of native palette conversion. */
        runtime[0]=0; runtime[1]=0xb1; runtime[2]=resource[5]; runtime[3]=0; runtime[4]=resource[7]; runtime[5]=0;
        memcpy(runtime+6,resource+8,(size_t)bytes-8); check(uc_mem_write(u,0x68000+0x800*i,runtime,(size_t)bytes-2));
        if(i) { word(u,0x3cbf0+0x4e44+4*(i-1),0x800*i); word(u,0x3cbf0+0x4e46+4*(i-1),0x6800); }
    }
    r->icons=icons; r->icon_count=11; r->icon=renderer_icon;
    unsigned char snapshot[65536]; memset(snapshot,0xd7,sizeof snapshot); snapshot[0]=80; snapshot[1]=200;
    for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;++x) snapshot[2+(x&3)*16000+y*80+x/4]=r->saved[y*320+x];
    check(uc_mem_write(u,0x70000,snapshot,sizeof snapshot)); word(u,0x3cbf0+0x5b8,0); word(u,0x3cbf0+0x5ba,0x7000);
    unsigned char action_bytes[44],random[4]; check(uc_mem_read(u,0x3cbf0+0x1110,action_bytes,44)); check(uc_mem_read(u,0x3cbf0+0x1393,random,4));
    struct SlicksPlayerMenuLabels labels={random,random+2,{action_bytes,action_bytes+11,action_bytes+22,action_bytes+33}};
    unsigned cases=0;
    for(unsigned variant=0;variant<13;++variant) for(unsigned row=0;row<8;++row) {
        struct SlicksPlayerProfiles profiles={0}; profiles.count=5;
        short selected[4]; signed char roles[4];
        for(unsigned i=0;i<4;++i) {
            unsigned index=i+1; selected[i]=(variant+i)%5?index:0; roles[i]=(signed char)((variant+i)%3-1);
            profiles.names[index][0]='A'+i; profiles.names[index][1]='0'+variant%10;
            profiles.setup[index].vehicle=(unsigned char)((variant+i)%13);
            check(uc_mem_write(u,0x3cbf0+0x36aa+21*index,profiles.names[index],21));
            check(uc_mem_write(u,0x3cbf0+0x3fa6+index,&profiles.setup[index].vehicle,1)); word(u,0x3cbf0+0x44c+2*i,selected[i]);
        }
        check(uc_mem_write(u,0x3cbf0+0x4bc6,roles,4)); word(u,0x3cbf0+0x4c6e,10);
        word(u,0x8effc,0); word(u,0x8effe,0x6800); unsigned char selection=(unsigned char)row; check(uc_mem_write(u,0x8eff9,&selection,1));
        /* Keep the previous frame: exercise restoration between selections. */
        if(slicks_player_renderer_draw(r,row,selected,roles,&profiles,10,&labels)) abort();
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xebb0,bp=0xf000,ip;
        word(u,0x3cbf0+0x1d8d,0); word(u,0x3cbf0+0x1d8f,200); word(u,0x3cbf0+0x1d91,0); word(u,0x3cbf0+0x1d93,79);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_emu_start(u,0x286ab,0x28981,0,10000000)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        if(ip!=0x28981-0x266c0 || memcmp(r->ui.pixels,v->pixels,64000)) {
            fprintf(stderr,"Dynamic menu mismatch variant=%u row=%u ip=%x\n",variant,row,ip);
            for(unsigned i=0;i<64000;++i) if(r->ui.pixels[i]!=v->pixels[i]) { fprintf(stderr,"pixel %u,%u native=%u DOS=%u\n",i%320,i/320,r->ui.pixels[i],v->pixels[i]); break; }
            exit(1);
        }
        ++cases;
    }
    verify_editor_pixels(u,v,r);
    check(uc_close(n->icon_cpu)); n->icon_cpu=0;
    puts("Original complete player-menu redraw: 104 sequential full-frame comparisons with real assets, 68020 text/icons, all selection rows and restoration pass");
    if(cases!=104) abort();
}
static void verify_prepare_pixels(uc_engine *u)
{
    static unsigned char source[70000],base[64000],palette[768],fonts[3][8192],native[64000],saved[64000];
    unsigned sizes[3],width,height; unsigned long consumed;
    long loaded=host_archive_load("ref/SLICKS.000","players.bmp",source,sizeof source);
    if(loaded<0 || slicks_decode_menu_bitmap(source,(unsigned long)loaded,base,palette,&width,&height,&consumed) || width!=320 || height!=200) abort();
    const char *names[]={"kirj.@f","pieni.@f","iso.@f"};
    for(unsigned i=0;i<3;++i) {
        loaded=host_archive_load("ref/SLICKS.000",names[i],source,sizeof source);
        long size=loaded<0?-1:slicks_decode_font_resource(source,(unsigned long)loaded,fonts[i],sizeof fonts[i]);
        if(size<0) abort(); sizes[i]=(unsigned)size;
    }
    unsigned char code[8192]; FILE *f=fopen("build/font_string_test.bin","rb"); if(!f) abort();
    size_t bytes=fread(code,1,sizeof code,f); fclose(f); if(bytes<4 || bytes==sizeof code) abort();
    struct NativeText n={0}; check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&n.cpu));
    check(uc_ctl_set_cpu_model(n.cpu,UC_CPU_M68K_M68020)); check(uc_mem_map(n.cpu,0,0x400000,UC_PROT_ALL));
    check(uc_mem_write(n.cpu,0,code,bytes));
    n.entry=(unsigned)code[0]<<24|(unsigned)code[1]<<16|(unsigned)code[2]<<8|code[3];
    n.bridge=(unsigned)code[4]<<24|(unsigned)code[5]<<16|(unsigned)code[6]<<8|code[7];
    n.pixels=native; n.spacing=getword(u,0x3cbf0+0x1604)&255; n.tab=getword(u,0x3cbf0+0x15fe);
    n.highlight=getword(u,0x3cbf0+0x1600)&255;
    unsigned shadow=getword(u,0x3cbf0+0x1602); n.shadow=((shadow&255)<<8)|(shadow>>8);
    struct Vga v; struct TextTrace unused={0}; uc_hook hooks[3];
    check(uc_hook_add(u,&hooks[0],UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,vga_access,&v,0xa0000,0xaffff));
    check(uc_hook_add(u,&hooks[1],UC_HOOK_CODE,prepare_boundary,&unused,0x2e0f8,0x2e0f8));
    check(uc_hook_add(u,&hooks[2],UC_HOOK_INSN,font_port,&v,1,0,UC_X86_INS_OUT));
    unsigned char title[128],footer[128];
    check(uc_mem_read(u,0x3cbf0+0x1365,title,sizeof title)); check(uc_mem_read(u,0x3cbf0+0x136d,footer,sizeof footer));
    const unsigned percentages[]={66,0,50,100};
    for(unsigned language=0;language<=8;++language){
    const unsigned char *heading=menu_language_title(u,language,title);
    for(unsigned test=0;test<4;++test) {
        memcpy(native,base,sizeof native); memcpy(v.pixels,base,sizeof base);
        for(unsigned i=0;i<3;++i) {
            n.fonts[i]=fonts[i]; n.sizes[i]=sizes[i];
            check(uc_mem_write(u,0x60000+0x2000*i,fonts[i],sizes[i]));
            word(u,0x3cbf0+0x680+4*i,0x2000*i); word(u,0x3cbf0+0x682+4*i,0x6000);
        }
        check(uc_mem_write(u,0x3cbf0+0x71bc,palette,sizeof palette));
        word(u,0x3cbf0+0x71b8,0x71bc); word(u,0x3cbf0+0x71ba,0x3cbf);
        word(u,0x3cbf0+0x1d7b,100); word(u,0x3cbf0+0x1d87,0); word(u,0x3cbf0+0x16fd,percentages[test]);
        struct SlicksPlayerMenuRenderer renderer={0};
        renderer.ui=(struct SlicksChunkyUi){native,palette,0,0}; renderer.saved=saved;
        for(unsigned i=0;i<3;++i) renderer.fonts[i]=n.fonts[i];
        renderer.text=renderer_text; renderer.context=&n;
        if(slicks_player_renderer_prepare(&renderer,heading,footer,(unsigned char)percentages[test]) ||
           memcmp(saved,native,sizeof saved)) abort();
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xebb0,bp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_emu_start(u,0x283c3,0x285ec,0,30000000)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        if(ip!=0x1f2c || memcmp(native,v.pixels,sizeof native)) {
            for(unsigned fi=0;fi<3;++fi) {
                unsigned char actual[8192]; check(uc_mem_read(u,0x60000+0x2000*fi,actual,sizes[fi]));
                fprintf(stderr,"font %u equal=%d colours=%u,%u vs %u,%u\n",fi,!memcmp(actual,fonts[fi],sizes[fi]),actual[6],actual[7],fonts[fi][6],fonts[fi][7]);
            }
            fprintf(stderr,"Full menu preparation pixels differ test=%u ip=%x spacing=%u/%u tab=%u/%u\n",test,ip,n.spacing,getword(u,0x3cbf0+0x1604)&255,n.tab,getword(u,0x3cbf0+0x15fe));
            for(unsigned i=0;i<64000;++i) if(native[i]!=v.pixels[i]) { fprintf(stderr,"pixel %u,%u native=%u DOS=%u\n",i%320,i/320,native[i],v.pixels[i]); break; }
            exit(1);
        }
        if(!test && !language) verify_dynamic(u,&v,&renderer);
    }
    }
    for(unsigned i=0;i<3;++i) check(uc_hook_del(u,hooks[i])); check(uc_close(n.cpu));
    puts("Original player-menu preparation: 36 full-screen comparisons across eight languages and fallback using actual players.bmp and real DOS/68020 text rasterizers pass");
}
int main(int argc,char **argv)
{
    unsigned char runtime[300000];
    FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f); fclose(f);
    if(bytes<200000 || bytes==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,bytes));
    if(argc==2 && !strcmp(argv[1],"--prepare-pixels")) {
        verify_prepare_pixels(u); check(uc_close(u)); return 0;
    }
    const short colours[][3]={{15,15,45},{0,0,0},{63,63,63},{-20,70,127},
        {-32768,32767,-1},{255,128,-128}};
    unsigned cases=0;
    for(unsigned test=0;test<288;++test) {
        unsigned char palette[768],native[258],original[258];
        unsigned pattern=test%6;
        for(unsigned i=0;i<768;++i)
            palette[i]=pattern==0?0:pattern==1?63:
                (unsigned char)((i*61+(i>>3)*17+test*43)&(pattern<4?63:255));
        unsigned colour=test<256?0:test%6;
        unsigned char percent=test<256?(unsigned char)test:50;
        memset(native,0xa5,sizeof native);
        check(uc_mem_write(u,0x60000,palette,sizeof palette));
        check(uc_mem_write(u,0x61000,native,sizeof native));
        slicks_ui_tint_table(palette,native+1,colours[colour][0],colours[colour][1],colours[colour][2],percent);
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
        word(u,0x8f000,0); word(u,0x8f002,0x9000);
        word(u,0x8f004,0); word(u,0x8f006,0x6000);
        word(u,0x8f008,1); word(u,0x8f00a,0x6100);
        for(unsigned c=0;c<3;++c) word(u,0x8f00c+2*c,(unsigned short)colours[colour][c]);
        word(u,0x8f012,percent);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x344c5,0x90000,0,10000000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        check(uc_mem_read(u,0x61000,original,sizeof original));
        if(ip || sp!=0xf004 || memcmp(native,original,sizeof native)) {
            fprintf(stderr,"Tint mismatch test=%u percent=%u ip=%x sp=%x\n",test,percent,ip,sp);
            for(unsigned i=0;i<258;++i) if(native[i]!=original[i]) {
                fprintf(stderr,"byte %u native=%u DOS=%u\n",i,native[i],original[i]); break;
            }
            return 1;
        }
        ++cases;
    }
    verify_rectangles(u);
    verify_prepare(u);
    verify_prepare_pixels(u);
    check(uc_close(u));
    printf("Original palette tint: %u complete 256-entry table comparisons, all percentage bytes, signed overflow, ties and output guards pass\n",cases);
    return 0;
}
