#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/ui/list_captions.h"
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned a,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(u,a,b,2)); }
static unsigned rd(uc_engine *u,unsigned a)
{ unsigned char b[2]; check(uc_mem_read(u,a,b,2)); return b[0]|b[1]<<8; }
struct Call { unsigned kind; short args[3]; unsigned char text[256]; };
struct Trace { struct Call calls[32]; unsigned count; unsigned char colour; };
static struct Call *add(struct Trace *t,unsigned kind)
{ if(t->count>=32) abort(); struct Call *c=&t->calls[t->count++]; c->kind=kind; return c; }
static short measure(void *p,const unsigned char *s)
{
    struct Call *c=add(p,0); strcpy((char *)c->text,(const char *)s);
    short width=0; while(*s) width+=(short)(1+*s++%7); return width;
}
static unsigned char colour(void *p,unsigned char value)
{
    struct Trace *t=p; struct Call *c=add(t,1); c->args[0]=value;
    unsigned char old=t->colour; t->colour=value; return old;
}
static unsigned char nearest(void *p,unsigned char r,unsigned char g,unsigned char b)
{ struct Call *c=add(p,2); c->args[0]=r; c->args[1]=g; c->args[2]=b; return 97; }
static void text(void *p,const unsigned char *s,short x,short y,unsigned char flags)
{ struct Call *c=add(p,3); c->args[0]=x; c->args[1]=y; c->args[2]=flags; strcpy((char *)c->text,(const char *)s); }
static void boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)size; uint16_t ss,sp,cs,ip,ax=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=16U*ss+sp; unsigned a[8]; for(unsigned i=0;i<8;++i) a[i]=rd(u,stack+4+2*i);
    unsigned char s[256];
    if(address==0x300e5 || address==0x301ab) {
        unsigned at=address==0x300e5?a[0]+16U*a[1]:a[2]+16U*a[3];
        check(uc_mem_read(u,at,s,sizeof s)); if(!memchr(s,0,sizeof s)) abort();
        if(address==0x300e5) ax=(uint16_t)measure(p,s);
        else text(p,s,(short)a[0],(short)a[1],(unsigned char)a[6]);
    } else if(address==0x2fe63) ax=colour(p,(unsigned char)a[0]);
    else ax=nearest(p,(unsigned char)a[0],(unsigned char)a[1],(unsigned char)a[2]);
    ip=rd(u,stack); cs=rd(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_CS,&cs));
    check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t size=fread(runtime,1,sizeof runtime,f); int error=ferror(f); fclose(f);
    if(error || size<200000 || size==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,size));
    struct Trace dos; uc_hook hooks[4]; unsigned addresses[]={0x300e5,0x301ab,0x2fe63,0x36fae};
    for(unsigned i=0;i<4;++i) check(uc_hook_add(u,&hooks[i],UC_HOOK_CODE,boundary,&dos,addresses[i],addresses[i]));
    const char *strings[]={"SELECT","MODIFY,REMOVE,CANCEL","","-",",",",A,,B,","-YES,-NO,CANCEL","WIDE LABEL,i"};
    unsigned cases=0;
    for(unsigned i=0;i<8;++i) for(unsigned selection=0;selection<256;++selection) for(unsigned font=5;font<=9;font+=2) {
        unsigned char input[256]={0}; strcpy((char *)input,strings[i]);
        check(uc_mem_write(u,0x60000,input,sizeof input)); unsigned char height=(unsigned char)font;
        check(uc_mem_write(u,0x61002,&height,1));
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,ip,ax;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        word(u,0x8f000,0); word(u,0x8f002,0x7000);
        unsigned args[]={0,0x6000,160,30,0,0x6100,selection,211};
        for(unsigned j=0;j<8;++j) word(u,0x8f004+2*j,args[j]);
        memset(&dos,0,sizeof dos); dos.colour=73;
        check(uc_emu_start(u,0x30b5b,0x70000,0,20000));
        struct Trace native={0}; native.colour=73;
        struct SlicksListCaptionOps ops={measure,colour,nearest,text,&native};
        int result=slicks_list_captions(input,160,30,height,(unsigned char)selection,211,&ops);
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_AX,&ax));
        check(uc_reg_read(u,UC_X86_REG_SP,&sp)); unsigned char restored[256]; check(uc_mem_read(u,0x60000,restored,256));
        if(ip || sp!=0xf004 || result!=ax || memcmp(&native,&dos,sizeof dos) || memcmp(restored,input,256)) {
            fprintf(stderr,"Caption mismatch string=%u selection=%u font=%u DOS=%u native=%d\n",i,selection,font,ax,result); return 1;
        }
        ++cases;
    }
    unsigned layouts=0;
    const char *layout_strings[]={"SELECT","MODIFY,REMOVE,CANCEL","","-",",","-YES,NO","YES,-NO",",-YES,NO","\002YES,NO","\377YES,NO"};
    for(unsigned i=0;i<10;++i) for(unsigned action=0;action<4;++action) for(unsigned font=5;font<=9;++font) {
        unsigned char input[256]={0}; strcpy((char *)input,layout_strings[i]);
        check(uc_mem_write(u,0x60000,input,256)); unsigned char height=(unsigned char)font;
        check(uc_mem_write(u,0x61002,&height,1));
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xe000,bp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        word(u,0x8f00e,0); word(u,0x8f010,0x6000); word(u,0x8f01c,0); word(u,0x8f01e,0x6100);
        word(u,0x8f006,160); word(u,0x8f008,30);
        word(u,0x8eff4,0); word(u,0x8efee,0); word(u,0x8efbc,0);
        unsigned char initial=(unsigned char)action; check(uc_mem_write(u,0x8efff,&initial,1));
        memset(&dos,0,sizeof dos); dos.colour=73;
        unsigned end=input[0]?0x30dc1:0x30f39;
        check(uc_emu_start(u,0x30d28,end,0,20000));
        struct Trace native={0}; native.colour=73;
        struct SlicksListCaptionOps ops={measure,colour,nearest,text,&native};
        struct SlicksListCaptionLayout layout;
        int result=slicks_list_caption_layout(&layout,input,height,initial,&ops);
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        unsigned char actual_action; check(uc_mem_read(u,0x8efff,&actual_action,1));
        unsigned char restored[256]; check(uc_mem_read(u,0x60000,restored,256));
        if(result || ip!=end-0x2e0f0 || sp!=0xe000 || memcmp(&dos,&native,sizeof dos) ||
           layout.labels-input!=rd(u,0x8f00e) || layout.count!=(short)rd(u,0x8eff4) ||
           layout.width!=(short)rd(u,0x8efee) || (input[0] && layout.height!=(short)rd(u,0x8efbc)) ||
           layout.action!=actual_action || memcmp(input,restored,256)) {
            fprintf(stderr,"Caption layout mismatch string=%u action=%u font=%u\n",i,action,font); return 1;
        }
        ++layouts;
    }
    unsigned char unterminated[256]; memset(unterminated,'X',sizeof unterminated);
    struct Trace untouched={0}; untouched.colour=73;
    struct SlicksListCaptionOps ops={measure,colour,nearest,text,&untouched};
    if(slicks_list_captions(unterminated,160,30,6,0,211,&ops)!=-1 ||
       untouched.count || untouched.colour!=73) abort();
    check(uc_close(u));
    printf("DOS action captions: %u complete measure/draw/colour/return/input-restoration comparisons pass\n",cases);
    printf("DOS caption layout: %u control-byte/hyphen/measurement comparisons pass\n",layouts);
    return 0;
}
