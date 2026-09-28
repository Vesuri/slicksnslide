#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unicorn/unicorn.h>
static void ck(uc_err err) { if(err){fprintf(stderr,"%s\n",uc_strerror(err));exit(2);} }
static void be32(unsigned char *p,uint32_t v) {p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    unsigned char code[4096],stack[8];size_t n=fread(code,1,sizeof code,f);fclose(f);
    if(n<=256 || n==sizeof code)return 2;
    uc_engine *u;ck(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    ck(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));
    ck(uc_mem_map(u,0,0x40000,UC_PROT_ALL));
    enum { CODE=0x10000, STACK=0x20000, STOP=0x30000 };
    ck(uc_mem_write(u,CODE,code,n));
    const int regs[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,
      UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,UC_M68K_REG_A3,UC_M68K_REG_A4,
      UC_M68K_REG_A5,UC_M68K_REG_A6};
    uint32_t seed=17;
    for(unsigned t=0;t<262144;++t) {
        seed=seed*1664525U+1013904223U;
        int32_t value=t<131072?(int32_t)t-65536:(int32_t)seed;
        if(t>=131072 && t<132072)value=INT32_MIN+(int32_t)(t-131072);
        if(t>=132072 && t<133072)value=INT32_MAX-(int32_t)(t-132072);
        for(unsigned variant=0;variant<2;++variant) {
            uint32_t sp=STACK,pc,actual;
            be32(stack,STOP);be32(stack+4,(uint32_t)value);
            ck(uc_mem_write(u,STACK,stack,sizeof stack));
            ck(uc_reg_write(u,UC_M68K_REG_A7,&sp));
            for(unsigned r=0;r<11;++r){uint32_t v=0xa5310000U+r*0x101U;ck(uc_reg_write(u,regs[r],&v));}
            ck(uc_emu_start(u,CODE+variant*256,STOP,0,100));
            ck(uc_reg_read(u,UC_M68K_REG_D0,&actual));
            ck(uc_reg_read(u,UC_M68K_REG_PC,&pc));
            ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));
            if(pc!=STOP || sp!=STACK+4 || (int32_t)actual!=value/100) {
                fprintf(stderr,"variant %u value %d got %d expected %d pc=%x sp=%x\n",variant,value,(int32_t)actual,value/100,pc,sp);
                return 1;
            }
            for(unsigned r=0;r<11;++r){uint32_t v;ck(uc_reg_read(u,regs[r],&v));if(v!=0xa5310000U+r*0x101U)return 1;}
        }
    }
    uc_close(u);
    puts("C /100 and direct DIVS.L: 262144 signed inputs each; near-zero range, INT32 edges, random full-domain values, return/stack/callee-save ABI pass");
    return 0;
}
