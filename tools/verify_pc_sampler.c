/* Diagnostic handler: active-window tags, exclusion, exception-frame
 * validation, capacity limits and timer jitter against a scalar oracle. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unicorn/unicorn.h>
static void ck(uc_err e){if(e){fprintf(stderr,"%s\n",uc_strerror(e));exit(2);}}
static void be16(unsigned char *p,unsigned v){p[0]=v>>8;p[1]=v;}
static void be32(unsigned char *p,uint32_t v){be16(p,v>>16);be16(p+2,v);}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    unsigned char code[4096];FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    size_t n=fread(code,1,sizeof code,f);fclose(f);if(!n||n==sizeof code)return 2;
    enum{CODE=0x10000,DATA=0x20000,SAMPLES=0x30000,TIMER=0x60000,STACK=0x80000,STOP=0x90000};
    uc_engine *u;ck(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    ck(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));ck(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    ck(uc_mem_write(u,CODE,code,n));
    const unsigned frames[]={0,1,601,602,603,703,704,65535};
    const int regs[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,
        UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,UC_M68K_REG_A3,UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6};
    unsigned accepted=0,excluded=0,missed=0;
    for(unsigned t=0;t<1536;++t){
        unsigned char state[64],expected[64],actual[64],samples[128],wanted[128],got[128],stack[256],timer[512],got_timer[512];
        memset(state,0xa7,sizeof state);memset(samples,0x3c,sizeof samples);
        memset(stack,0xcc,sizeof stack);memset(timer,0xa9,sizeof timer);
        unsigned frame=frames[t%8],count=(t/8)%4,capacity=(t/32)%5,kind=(t/160)%4,scan=(t*17)%96;
        uint32_t seed=0x2545f491u+t,period=seed;
        period^=period<<7;period^=period>>9;period^=period<<8;
        be16(state,frame);be32(state+4,SAMPLES);be32(state+8,count);be32(state+12,capacity);
        be32(state+16,19);be32(state+20,seed);be32(state+24,TIMER);memcpy(expected,state,64);
        be32(expected+20,period);unsigned latch=(period&511)+900;
        unsigned offset=4+scan*2,pc=0x123456+t*2,sr=t&1?0x2000:0;
        be32(stack,STOP);be16(stack+offset,sr);be32(stack+offset+2,pc);be16(stack+offset+6,0x78);
        if(kind==1)be32(stack+offset+2,pc|1);
        if(kind==2)be16(stack+offset,0x8000);
        if(kind==3)be16(stack+offset+6,0);
        memcpy(wanted,samples,sizeof samples);
        if(frame<704){
            if(kind){be32(expected+16,20);++missed;}
            else if(count<capacity){unsigned char *p=wanted+count*8;
                be32(p,pc);be16(p+4,frame);p[6]=12+scan*2;p[7]=sr>>8;
                be32(expected+8,count+1);++accepted;
            }
        }else ++excluded;
        ck(uc_mem_write(u,DATA,state,64));ck(uc_mem_write(u,SAMPLES,samples,sizeof samples));
        ck(uc_mem_write(u,TIMER,timer,sizeof timer));ck(uc_mem_write(u,STACK,stack,sizeof stack));
        timer[0]=latch;timer[256]=latch>>8;
        unsigned sp=STACK;ck(uc_reg_write(u,UC_M68K_REG_A7,&sp));
        unsigned values[11];for(unsigned i=0;i<11;++i){values[i]=0xa5000000u+i*0x10101u+t;ck(uc_reg_write(u,regs[i],values+i));}
        ck(uc_emu_start(u,CODE,STOP,0,10000));
        ck(uc_mem_read(u,DATA,actual,64));ck(uc_mem_read(u,SAMPLES,got,sizeof got));ck(uc_mem_read(u,TIMER,got_timer,sizeof timer));
        if(memcmp(actual,expected,64)||memcmp(got,wanted,sizeof got)||memcmp(timer,got_timer,sizeof timer)){
            fprintf(stderr,"Sampler case %u frame %u kind %u count %u capacity %u failed\n",t,frame,kind,count,capacity);return 1;}
        ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));if(sp!=STACK+4)return 1;
        unsigned result;ck(uc_reg_read(u,UC_M68K_REG_D0,&result));if(result)return 1;
        for(unsigned i=0;i<11;++i){unsigned v;ck(uc_reg_read(u,regs[i],&v));if(v!=values[i])return 1;}
    }
    uc_close(u);printf("PC sampler: 1536 cases, %u accepted, %u inactive/outside, %u invalid frames; exact tags, capacity, timer writes, untouched bytes and ABI pass\n",accepted,excluded,missed);return 0;
}
