#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unicorn/unicorn.h>
static void ck(uc_err e){if(e){fprintf(stderr,"%s\n",uc_strerror(e));exit(2);}}
static unsigned seed=17;
static unsigned rnd(void){seed=seed*1664525u+1013904223u;return seed>>8;}
static void longword(unsigned char *p,unsigned x){p[0]=x>>24;p[1]=x>>16;p[2]=x>>8;p[3]=x;}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    unsigned char code[4096];FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    size_t n=fread(code,1,sizeof code,f);fclose(f);if(!n||n==sizeof code)return 2;
    enum{CODE=0x10000,DST=0x20000,SRC=0x30000,STACK=0x80000,STOP=0x90000,SIZE=4608};
    uc_engine *u;ck(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    ck(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));ck(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    ck(uc_mem_write(u,CODE,code,n));
    const int regs[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,
        UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,UC_M68K_REG_A3,UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6};
    for(unsigned t=0;t<12000;++t){
        unsigned char dst[SIZE],src[SIZE],expected[SIZE],got[SIZE],stack[16];
        unsigned width=t&1?50:52,offset=16+t%4;
        for(unsigned i=0;i<SIZE;++i)dst[i]=src[i]=(unsigned char)rnd();
        for(unsigned y=0;y<14;++y)for(unsigned x=0;x<width;++x)
            if(t%4==0 || (t%4==1 && rnd()%41==0))dst[offset+y*320+x]^=1+rnd()%255;
        if(t%4==2)dst[offset+(t%14)*320+t%width]^=1;
        memcpy(expected,dst,SIZE);
        for(unsigned y=0;y<14;++y)memcpy(expected+offset+y*320,src+offset+y*320,width);
        ck(uc_mem_write(u,DST,dst,SIZE));ck(uc_mem_write(u,SRC,src,SIZE));
        longword(stack,STOP);longword(stack+4,DST+offset);longword(stack+8,SRC+offset);
        longword(stack+12,width);ck(uc_mem_write(u,STACK,stack,sizeof stack));
        unsigned sp=STACK;ck(uc_reg_write(u,UC_M68K_REG_A7,&sp));
        unsigned values[11];for(unsigned i=0;i<11;++i){values[i]=0xa5000000u+i*0x10101u+t;ck(uc_reg_write(u,regs[i],&values[i]));}
        ck(uc_emu_start(u,CODE,STOP,0,100000));
        ck(uc_mem_read(u,DST,got,SIZE));if(memcmp(got,expected,SIZE)){fprintf(stderr,"pixels trial %u\n",t);return 1;}
        ck(uc_mem_read(u,SRC,got,SIZE));if(memcmp(got,src,SIZE))return 1;
        ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));if(sp!=STACK+4)return 1;
        for(unsigned i=0;i<11;++i){unsigned v;ck(uc_reg_read(u,regs[i],&v));if(v!=values[i])return 1;}
    }
    uc_close(u);puts("HUD restore: 12000 dense/sparse/empty cells, 50/52 widths, four alignments, canaries and ABI pass");return 0;
}
