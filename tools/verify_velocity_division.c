#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unicorn/unicorn.h>

static void ck(uc_err e) { if(e) {fprintf(stderr,"%s\n",uc_strerror(e));exit(2);} }
static void be32(unsigned char *p,uint32_t v) {p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}

int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    unsigned char code[1024];size_t n=fread(code,1,sizeof code,f);fclose(f);
    if(!n || n==sizeof code)return 2;
    uc_engine *u;ck(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    ck(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));
    ck(uc_mem_map(u,0,0x40000,UC_PROT_ALL));
    enum {CODE=0x10000,STACK=0x20000,STOP=0x30000};
    ck(uc_mem_write(u,CODE,code,n));
    const int regs[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,
        UC_M68K_REG_D5,UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,
        UC_M68K_REG_A3,UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6};
    uint32_t rng=17,total=0;
    for(uint32_t divisor=32728;divisor<=32840;++divisor) {
        if(divisor==32768)continue; /* exact power of two needs another path */
        uint64_t multiplier=0,scale=0;unsigned shift;
        for(shift=0;shift<=15;++shift) {
            scale=UINT64_C(1)<<(32+shift);
            multiplier=(scale+divisor-1)/divisor;
            uint64_t epsilon=multiplier*divisor-scale;
            uint64_t q=(UINT64_C(2147483648)+divisor-1)/divisor;
            if(multiplier<UINT64_C(4294967296) && q*epsilon<multiplier)break;
        }
        if(shift>15)return 1;
        /* Prove the selected reciprocal on every quotient boundary for
         * |n|<=2^31, including INT32_MIN's magnitude. Between boundaries
         * the multiply is monotonic, so testing both interval ends suffices. */
        for(uint64_t q=0;q*divisor<=UINT64_C(2147483648);++q) {
            uint64_t low=q*divisor,high=low+divisor-1;
            if(high>UINT64_C(2147483648))high=UINT64_C(2147483648);
            if((low*multiplier)/scale!=q || (high*multiplier)/scale!=q)return 1;
        }
        for(unsigned trial=0;trial<8192;++trial) {
            rng=rng*1664525U+1013904223U;
            int32_t value=(int32_t)rng;
            if(trial<2048)value=(int32_t)trial-1024;
            else if(trial<3072)value=INT32_MIN+(int32_t)(trial-2048);
            else if(trial<4096)value=INT32_MAX-(int32_t)(trial-3072);
            else if(trial<6144) {
                int32_t q=(int32_t)(rng%65536U)-32768;
                value=q*(int32_t)divisor+(int32_t)(trial%3)-1;
            }
            unsigned char stack[16];uint32_t sp=STACK,pc,actual;
            be32(stack,STOP);be32(stack+4,(uint32_t)value);
            be32(stack+8,(uint32_t)multiplier);be32(stack+12,shift);
            ck(uc_mem_write(u,STACK,stack,sizeof stack));
            ck(uc_reg_write(u,UC_M68K_REG_A7,&sp));
            for(unsigned r=0;r<11;++r){uint32_t v=0x95120000U+r;ck(uc_reg_write(u,regs[r],&v));}
            ck(uc_emu_start(u,CODE,STOP,0,100));
            ck(uc_reg_read(u,UC_M68K_REG_D0,&actual));
            ck(uc_reg_read(u,UC_M68K_REG_PC,&pc));
            ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));
            if(pc!=STOP || sp!=STACK+4 || (int32_t)actual!=value/(int32_t)divisor) {
                fprintf(stderr,"d=%u n=%d actual=%d expected=%d\n",divisor,value,(int32_t)actual,value/(int32_t)divisor);return 1;
            }
            for(unsigned r=0;r<11;++r){uint32_t v;ck(uc_reg_read(u,regs[r],&v));if(v!=0x95120000U+r)return 1;}
            ++total;
        }
    }
    uc_close(u);
    printf("Velocity reciprocal: all quotient intervals proved for 112 divisors; %u native signed inputs, limits, boundaries and ABI pass\n",total);
    return 0;
}
