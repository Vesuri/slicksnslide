#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/actor_slots.h"
static void ck(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e));exit(1); } }
static void word(uc_engine *u,unsigned a,unsigned v) { unsigned char b[2]={v,v>>8};ck(uc_mem_write(u,a,b,2)); }
static unsigned rd(uc_engine *u,unsigned a) { unsigned char b[2];ck(uc_mem_read(u,a,b,2));return b[0]|b[1]<<8; }
static void stop(uc_engine *u,uint64_t at,uint32_t size,void *p) { (void)at;(void)size;(void)p;ck(uc_emu_stop(u)); }
int main(void)
{
    unsigned char bytes[300000];FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    size_t n=fread(bytes,1,sizeof bytes,f);fclose(f);
    uc_engine *u;ck(uc_open(UC_ARCH_X86,UC_MODE_16,&u));ck(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    ck(uc_mem_write(u,0x10100,bytes,n));
    unsigned seed=127;
    for(unsigned t=0;t<8192;++t) {
        struct SlicksActorSlots pool; slicks_actor_slots_init(&pool);
        pool.capacity=t%17?200:0;pool.high_water=(t%200)+1;
        unsigned char actors[201*64]={0};
        for(unsigned i=1;i<200;++i) {
            seed=seed*1664525U+1013904223U;
            const signed char states[]={0,1,3,5,-1,-2,-3,-5,-6};
            pool.state[i]=states[(seed>>8)%9];
            if(t&8) pool.state[i]=1;
            actors[i*64+0x1a]=(unsigned char)pool.state[i];
        }
        ck(uc_mem_write(u,0x90000,actors,sizeof actors));
        word(u,0x3cbf0+0x16be,pool.capacity);word(u,0x3cbf0+0x16c0,pool.high_water);
        word(u,0x3cbf0+0x16ce,0);word(u,0x3cbf0+0x16d0,0x9000);
        unsigned resource=t%3;word(u,0x8f004,0);word(u,0x8f006,resource?0xa000:0);
        unsigned char sentinel=255;ck(uc_mem_write(u,0xa0000,&sentinel,1));
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,ax;
        word(u,0x8f000,0);word(u,0x8f002,0x7000);
        ck(uc_reg_write(u,UC_X86_REG_CS,&cs));ck(uc_reg_write(u,UC_X86_REG_DS,&ds));
        ck(uc_reg_write(u,UC_X86_REG_SS,&ss));ck(uc_reg_write(u,UC_X86_REG_SP,&sp));
        ck(uc_emu_start(u,0x330ab,0x70000,0,100000));ck(uc_reg_read(u,UC_X86_REG_AX,&ax));
        short handle=slicks_actor_allocate(&pool,resource);
        if(handle!=(short)ax || pool.high_water!=rd(u,0x3cbf0+0x16c0)) {
            fprintf(stderr,"Actor allocation mismatch trial=%u original=%d native=%d\n",t,(short)ax,handle);return 1;
        }
        ck(uc_mem_read(u,0x90000,actors,sizeof actors));
        for(unsigned i=1;i<200;++i) if(pool.state[i]!=(signed char)actors[i*64+0x1a]) return 1;
    }
    puts("Original actor allocation: 8192 returns, high-water marks and slot states match");
    uc_hook hook;ck(uc_hook_add(u,&hook,UC_HOOK_CODE,stop,0,0x33a9a,0x33a9a));
    for(unsigned t=0;t<8192;++t) {
        struct SlicksActorMotion a={(short)(t*47),(short)(t*97),(short)(t*107),(short)(t*193),
            (short)(t*11),(short)(t*13),(short)(t%9-3),(short)(t%17-3),
            (signed char)(t%8),(signed char)(t%11-3),(unsigned char)(1+t%8)};
        signed char state=(t&1)?1:5;
        const unsigned offsets[]={0,2,0x1b,0x1d,0x1f,0x21,0x3e,0x3c};
        const short values[]={a.x,a.y,a.vx,a.vy,a.ax,a.ay,a.lifetime,a.age};
        for(unsigned i=0;i<8;++i) word(u,0x90040+offsets[i],values[i]);
        unsigned char v=(unsigned char)state;ck(uc_mem_write(u,0x9005a,&v,1));
        v=(unsigned char)a.period;ck(uc_mem_write(u,0x90063,&v,1));
        v=(unsigned char)a.frame;ck(uc_mem_write(u,0x90064,&v,1));
        v=a.frames;ck(uc_mem_write(u,0x9007a,&v,1));
        v=t&1;ck(uc_mem_write(u,0x3cbf0+0x16c6,&v,1));
        word(u,0x90058,0);word(u,0x8effe,1);
        uint16_t cs=0x2e0f,ds=0x3cbf,es=0x9000,bx=64,bp=0xf000,ss=0x8000,sp=0xe000;
        ck(uc_reg_write(u,UC_X86_REG_CS,&cs));ck(uc_reg_write(u,UC_X86_REG_DS,&ds));
        ck(uc_reg_write(u,UC_X86_REG_ES,&es));ck(uc_reg_write(u,UC_X86_REG_BX,&bx));
        ck(uc_reg_write(u,UC_X86_REG_BP,&bp));ck(uc_reg_write(u,UC_X86_REG_SS,&ss));ck(uc_reg_write(u,UC_X86_REG_SP,&sp));
        ck(uc_emu_start(u,0x33918,0x33a9a,0,10000));
        slicks_actor_advance(&a,&state,t&1);
        const short result[]={a.x,a.y,a.vx,a.vy,a.ax,a.ay,a.lifetime,a.age};
        for(unsigned i=0;i<8;++i) if(result[i]!=(short)rd(u,0x90040+offsets[i])) { fprintf(stderr,"Actor motion trial=%u field=%u\n",t,i);return 1; }
        ck(uc_mem_read(u,0x9005a,&v,1));if((signed char)v!=state)return 1;
        ck(uc_mem_read(u,0x90064,&v,1));if((signed char)v!=a.frame)return 1;
    }
    puts("Original actor motion/animation: 8192 state/lifetime/position/velocity/frame cases match");
    ck(uc_close(u));return 0;
}
