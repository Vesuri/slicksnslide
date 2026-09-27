#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unicorn/unicorn.h>
enum { CODE=0x10000,SRC=0x20000,DST=0x40000,BM=0x60000,STACK=0x80000,STOP=0x90000,N=64064 };
static unsigned seed=71,left,top,width,height,writes;
static unsigned char expected[N];
static void ck(uc_err e){if(e){fprintf(stderr,"%s\n",uc_strerror(e));exit(2);}}
static unsigned rnd(void){seed=seed*1664525u+1013904223u;return seed>>8;}
static void be32(unsigned char *p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static void access_hook(uc_engine *u,uc_mem_type type,uint64_t address,int size,int64_t value,void *data)
{
    (void)u;(void)data;
    if(address>=DST && address<DST+N && type==UC_MEM_WRITE){
        unsigned offset=address-DST-32,row=offset/320,col=offset%40;
        if(address<DST+32 || size!=2 || row<top || row>=top+height ||
           col<left/8 || col>= (left+width)/8 ||
           (unsigned short)value!=(unsigned)(expected[address-DST]*256+expected[address-DST+1])){
            fprintf(stderr,"Invalid/non-final plane store %llx size %d\n",address,size);exit(1);
        }
        ++writes;
    }
    if(address>=SRC && address<SRC+N){
        unsigned offset=address-SRC-32,row=offset/320,col=offset%320;
        if(type==UC_MEM_WRITE || address<SRC+32 || row<top || row>=top+height ||
           col<left || col+(unsigned)size>left+width){fputs("Invalid source access\n",stderr);exit(1);}
    }
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    unsigned char code[4096];FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    size_t n=fread(code,1,sizeof code,f);fclose(f);if(!n || n==sizeof code)return 2;
    uc_engine *u;ck(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    ck(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));ck(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    ck(uc_mem_write(u,CODE,code,n));uc_hook hook;
    ck(uc_hook_add(u,&hook,UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,access_hook,NULL,1,0));
    const int regs[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,
        UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,UC_M68K_REG_A3,UC_M68K_REG_A4,
        UC_M68K_REG_A5,UC_M68K_REG_A6};
    for(unsigned t=0;t<1696;++t){
        unsigned char src[N],got[N],bm[40]={0},ret[4];
        left=(rnd()%20)*16;top=rnd()%200;width=(1+rnd()%((320-left)/16))*16;height=1+rnd()%(200-top);
        if(t<128){left=0;top=0;width=16;height=1;}
        if(t>=1664){if(t&1)width=0;else height=0;}
        for(unsigned i=0;i<N;++i){src[i]=t<128?0:rnd();expected[i]=rnd();}
        if(t<128)src[32+t/8]=1<<(t%8);
        ck(uc_mem_write(u,SRC,src,N));ck(uc_mem_write(u,DST,expected,N));
        for(unsigned y=top;y<top+height;++y)for(unsigned x=left;x<left+width;++x){
            unsigned v=src[32+y*320+x],mask=0x80>>(x&7);
            for(unsigned p=0;p<8;++p){
                unsigned off=32+y*320+p*40+x/8;
                expected[off]=(expected[off]&~mask)|((v&(1<<p))?mask:0);
            }
        }
        be32(bm+8,DST+32);ck(uc_mem_write(u,BM,bm,sizeof bm));be32(ret,STOP);ck(uc_mem_write(u,STACK,ret,4));
        unsigned values[11];for(unsigned i=0;i<11;++i){values[i]=0xa5000000u+i*0x10101u+t;ck(uc_reg_write(u,regs[i],values+i));}
        values[0]=left;values[1]=top*320;
        unsigned sp=STACK,a0=SRC+32+top*320+left,a1=BM,d0=width,d1=height;
        ck(uc_reg_write(u,UC_M68K_REG_A7,&sp));ck(uc_reg_write(u,UC_M68K_REG_A0,&a0));ck(uc_reg_write(u,UC_M68K_REG_A1,&a1));
        ck(uc_reg_write(u,UC_M68K_REG_D0,&d0));ck(uc_reg_write(u,UC_M68K_REG_D1,&d1));
        ck(uc_reg_write(u,UC_M68K_REG_D2,&left));ck(uc_reg_write(u,UC_M68K_REG_D3,&values[1]));
        writes=0;ck(uc_emu_start(u,CODE,STOP,0,2000000));
        ck(uc_mem_read(u,DST,got,N));if(memcmp(got,expected,N)){fprintf(stderr,"Pixels case %u\n",t);return 1;}
        if(writes!=width*height/2){fputs("Store coverage\n",stderr);return 1;}
        ck(uc_mem_read(u,SRC,got,N));if(memcmp(got,src,N))return 1;
        ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));if(sp!=STACK+4)return 1;
        for(unsigned i=0;i<11;++i){unsigned v;ck(uc_reg_read(u,regs[i],&v));if(v!=values[i])return 1;}
    }
    uc_close(u);puts("C2P16: 128 single-bit + 1536 random + 32 empty rectangles; final-value-only stores, source bounds, canaries and ABI pass");return 0;
}
