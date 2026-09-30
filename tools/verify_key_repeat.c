/* Original 36ce0 against src/ui/key_repeat.h over scripted latch/tick
 * sequences, persisting DS:174a/174c/174e between calls. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/ui/key_repeat.h"
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned a,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(u,a,b,2)); }
static unsigned readword(uc_engine *u,unsigned a)
{ unsigned char b[2]; check(uc_mem_read(u,a,b,2)); return b[0]|b[1]<<8; }
static unsigned long seed=12345;
static unsigned rnd(unsigned n) { seed=seed*1103515245UL+12345UL; return (unsigned)((seed>>16)%n); }
int main(void)
{
    static unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,n));
    const unsigned ds=0x3cbf0;
    unsigned long calls=0,returns=0;
    for(unsigned run=0;run<4000;++run) {
        struct SlicksKeyRepeat model={0,0};
        unsigned char arg=(unsigned char)(run%8);
        /* Start near the low-word boundary on some runs to exercise ADC. */
        unsigned long ticks=run%3==0?0xfff0UL+rnd(32):run%3==1?rnd(100000):0x7ffffff0UL+rnd(8);
        word(u,ds+0x174a,0); word(u,ds+0x174c,0); word(u,ds+0x174e,0);
        unsigned short latch=0x80;
        for(unsigned step=0;step<200;++step) {
            unsigned r=rnd(100);
            if(r<6) latch=(unsigned short)(0x80|rnd(128));           /* break */
            else if(r<12) latch=(unsigned short)rnd(128);            /* new make */
            else if(r<13) latch=0x2a;                                /* Shift make */
            ticks+=rnd(3);
            uint16_t cs=0x2e0f,ss=0x8000,sp=0xef00,dsr=0x3cbf,ax;
            check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_SS,&ss));
            check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_DS,&dsr));
            word(u,0x8ef04,arg); word(u,ds+0x1714,latch);
            word(u,0x46c,ticks&65535); word(u,0x46e,ticks>>16);
            check(uc_emu_start(u,0x36ce0,0x36d64,0,200));
            check(uc_reg_read(u,UC_X86_REG_AX,&ax));
            unsigned char expected=slicks_key_repeat_poll(&model,latch,ticks,arg);
            unsigned long last=readword(u,ds+0x174a)|(unsigned long)readword(u,ds+0x174c)<<16;
            unsigned armed=readword(u,ds+0x174e)&255;
            if((ax&255)!=expected || last!=model.last || armed!=model.armed) {
                fprintf(stderr,"mismatch run=%u step=%u arg=%u latch=%x ticks=%lx: DOS %x/%lx/%u native %x/%lx/%u\n",
                    run,step,arg,latch,ticks,ax&255,last,armed,expected,model.last,model.armed);
                return 1;
            }
            ++calls; returns+=expected!=0;
        }
    }
    check(uc_close(u));
    printf("Key repeat reader: %lu original 36ce0 calls match native return/last/armed (%lu scans returned)\n",calls,returns);
    return 0;
}
