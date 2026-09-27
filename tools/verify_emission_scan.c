#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include "../src/game/actor_slots.h"

static void ck(uc_err e) {if(e){fprintf(stderr,"%s\n",uc_strerror(e));exit(2);}}
static unsigned seed=127;
static unsigned rnd(void) {seed=seed*1664525u+1013904223u;return seed>>8;}
static unsigned big(const unsigned char *p) {return (unsigned)p[0]<<24|p[1]<<16|p[2]<<8|p[3];}
static void word(unsigned char *p,unsigned v) {p[0]=v>>8;p[1]=v;}
struct Bounds {unsigned slots,high;int invalid;};
static void stop(uc_engine *u,uint64_t at,uint32_t size,void *p)
{(void)at;(void)size;(void)p;ck(uc_emu_stop(u));}
static void read_slot(uc_engine *u,uc_mem_type type,uint64_t at,int size,int64_t value,void *p)
{
    struct Bounds *b=p;(void)type;(void)value;
    if(at>=b->slots && at<b->slots+200 && at+(unsigned)size>b->slots+b->high) {
        b->invalid=1;ck(uc_emu_stop(u));
    }
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    unsigned char code[8192];FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    size_t n=fread(code,1,sizeof code,f);fclose(f);if(n<16||n==sizeof code)return 2;
    enum {CODE=0x10000,SLOTS=0x20000,STACK=0x80000};
    unsigned entry=CODE+big(code+n-16),found=CODE+big(code+n-12),extend=CODE+big(code+n-8);
    uc_engine *u;ck(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    ck(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));ck(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    ck(uc_mem_write(u,CODE,code,n));
    uc_hook h1,h2,h3;struct Bounds bounds={0};
    ck(uc_hook_add(u,&h1,UC_HOOK_CODE,stop,0,found,found));
    ck(uc_hook_add(u,&h2,UC_HOOK_CODE,stop,0,extend,extend));
    ck(uc_hook_add(u,&h3,UC_HOOK_MEM_READ,read_slot,&bounds,SLOTS,SLOTS+203));
    const int regs[]={UC_M68K_REG_D1,UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,
        UC_M68K_REG_D5,UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A0,
        UC_M68K_REG_A1,UC_M68K_REG_A2,UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6};
    unsigned found_count=0,full_count=0;
    for(unsigned t=0;t<100000;++t) {
        unsigned char slots[208],after[208];
        for(unsigned i=0;i<sizeof slots;++i)slots[i]=0xa7;
        unsigned high=1+t%200,cur=1+rnd()%205;
        if(t%7==0)cur=65535;
        if(t%7==1)cur=1;
        if(t%7==2)cur=high;
        for(unsigned i=0;i<200;++i) {
            unsigned v=rnd();
            slots[i]=t%5==0?1:t%5==1?0:t%5==2?(unsigned char)v:
                v%17? (unsigned char)(1+v%255):0;
        }
        /* A free byte in each longword lane, including at the live end. */
        if(t%5==0)slots[high-1]=0;
        word(slots+200,high);word(slots+202,200);
        unsigned expected=cur;while(expected<high && slots[expected])++expected;
        unsigned base=SLOTS+(t&1?2:0);
        bounds=(struct Bounds){base,high,0};ck(uc_mem_write(u,base,slots,sizeof slots));
        unsigned values[13];
        for(unsigned i=0;i<13;++i){values[i]=0xa5000000u+i*0x10101u+t; if(regs[i]==UC_M68K_REG_A0)values[i]=base;ck(uc_reg_write(u,regs[i],&values[i]));}
        unsigned sp=STACK;ck(uc_reg_write(u,UC_M68K_REG_A7,&sp));ck(uc_reg_write(u,UC_M68K_REG_D0,&cur));
        ck(uc_emu_start(u,entry,0,0,10000));
        unsigned actual,pc;ck(uc_reg_read(u,UC_M68K_REG_D0,&actual));ck(uc_reg_read(u,UC_M68K_REG_PC,&pc));ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));
        unsigned exit=expected<high?found:extend;
        if(bounds.invalid || actual!=expected || pc!=exit || sp!=STACK) {
            fprintf(stderr,"trial=%u cursor=%u high=%u result=%u/%u pc=%x/%x sp=%x bounds=%d\n",t,cur,high,actual,expected,pc,exit,sp,bounds.invalid);return 1;
        }
        for(unsigned i=0;i<13;++i){unsigned v;ck(uc_reg_read(u,regs[i],&v));if(v!=values[i]){fprintf(stderr,"trial=%u register=%u\n",t,i);return 1;}}
        ck(uc_mem_read(u,base,after,sizeof after));if(memcmp(slots,after,sizeof slots))return 1;
        if(expected<high)++found_count;else ++full_count;
    }
    uc_close(u);printf("Native emission scan: 100000 pools, %u first-free and %u end exits; signed bytes, tails, alignment, bounds, registers and stack match scalar\n",found_count,full_count);
    return 0;
}
