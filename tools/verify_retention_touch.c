/* Raw target-layout comparison against the original per-cell rectangle walk. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unicorn/unicorn.h>
static void ck(uc_err e){if(e){fprintf(stderr,"Unicorn: %s\n",uc_strerror(e));exit(2);}}
static void p16(unsigned char *p,unsigned v){p[0]=v>>8;p[1]=v;}
static void p32(unsigned char *p,uint32_t v){p16(p,v>>16);p16(p+2,v);}
static int s16(const unsigned char *p){unsigned v=p[0]*256U+p[1];return v<32768?(int)v:(int)v-65536;}
static unsigned off(const char *path,const char *name){
    FILE *f=fopen(path,"r");char line[256];if(!f)exit(2);
    while(fgets(line,sizeof line,f))if(!strncmp(line,name,strlen(name)) && line[strlen(name)]==' '){
        unsigned v=strtoul(strstr(line,"equ")+3,0,0);fclose(f);return v;}
    fprintf(stderr,"Missing offset %s\n",name);exit(2);
}
static uint32_t rng=971;
static unsigned next(void){rng=rng*1664525U+1013904223U;return rng;}
int main(int argc,char **argv){
    if(argc!=3 && argc!=4)return 2;
    unsigned char code[4096];FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    size_t n=fread(code,1,sizeof code,f);fclose(f);if(!n || n==sizeof code)return 2;
    /* Corrupt only the loaded test image, never the production source/build.
     * Exact unique instruction patterns fail closed if the assembly changes. */
    if(argc==4){
        static const unsigned char patterns[6][6]={
            {0x52,0x44}, {0x08,0xeb,0,1,0,13},
            {0xb0,0x6b,0,4,0x6c,0x18}, {0x26,0x3c,0,0,0,184},
            {0xce,0xfc,0,40}, {0x08,0x2b,0,0,0,13}};
        static const unsigned lengths[]={2,6,6,6,4,6};
        static const unsigned byte[]={0,3,4,5,3,3};
        static const unsigned char replacements[]={0x4e,0,0x6e,183,39,2};
        unsigned m=(unsigned)strtoul(argv[3],0,10),matches=0,at=0;
        if(m>=6)return 2;
        for(unsigned i=0;i+lengths[m]<=n;i+=2)if(!memcmp(code+i,patterns[m],lengths[m])){++matches;at=i;}
        if(matches!=1){fprintf(stderr,"Mutation %u pattern count=%u\n",m,matches);return 2;}
        code[at+byte[m]]=replacements[m];if(m==0)code[at+1]=0x71;
    }
    unsigned ent=off(argv[2],"RET_ENTRIES"),cells=off(argv[2],"RET_CELLS");
    unsigned stride=off(argv[2],"ENTRY_SIZE"),flags=off(argv[2],"ENTRY_FLAGS");
    unsigned prio=off(argv[2],"ENTRY_PRIORITY"),countoff=off(argv[2],"RET_COUNT");
    unsigned left=off(argv[2],"ENTRY_LEFT"),top=off(argv[2],"ENTRY_TOP");
    unsigned right=off(argv[2],"ENTRY_RIGHT"),bottom=off(argv[2],"ENTRY_BOTTOM");
    enum{CODE=0x10000,STATE=0x40000,SP=0xf0000,STOP=0x18000,SIZE=8192};
    unsigned char before[SIZE],want[SIZE],got[SIZE],stack[32];
    uc_engine *u;ck(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    ck(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));ck(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    ck(uc_mem_write(u,CODE,code,n));
    const int regs[]={UC_M68K_REG_D0,UC_M68K_REG_D1,UC_M68K_REG_D2,UC_M68K_REG_D3,
        UC_M68K_REG_D4,UC_M68K_REG_D5,UC_M68K_REG_D6,UC_M68K_REG_D7,
        UC_M68K_REG_A0,UC_M68K_REG_A1,UC_M68K_REG_A2,UC_M68K_REG_A3,
        UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6,UC_M68K_REG_A7};
    for(unsigned t=0;t<4096;++t){
        for(unsigned i=0;i<SIZE;++i)before[i]=next()>>24;
        unsigned count=t%101;before[countoff]=count;
        for(unsigned i=0;i<count;++i){unsigned char *e=before+ent+i*stride;
            int x=(int)(next()%400)-40,y=(int)(next()%240)-30;
            p16(e+left,x);p16(e+top,y);p16(e+right,x+(int)(next()%80)-4);
            p16(e+bottom,y+(int)(next()%48)-4);e[flags]=next()>>24;e[prio]=next()>>24;}
        for(unsigned i=0;i<920;++i)before[cells+i]=!count?0:(next()%7==0?255:next()%(count+1));
        int32_t args[5]={(int)(next()%400)-40,(int)(next()%240)-30,0,0,(int)(next()%260)-2};
        args[2]=args[0]+(int)(next()%180)-8;args[3]=args[1]+(int)(next()%100)-4;
        /* Signed clipping extremes and a full-screen shared scan. */
        if(t%16==0){args[0]=args[1]=INT32_MIN;args[2]=args[3]=INT32_MAX;args[4]=-1;}
        if(t%16==1)args[0]=INT32_MAX;
        if(t%16==2)args[3]=INT32_MIN;
        if(t%16==3)args[4]=INT32_MAX;
        if(t%16==4)args[4]=INT32_MIN;
        memcpy(want,before,SIZE);
        int32_t l=args[0]<0?0:args[0],a=args[1]<0?0:args[1];
        int32_t r=args[2]>320?320:args[2],b=args[3]>184?184:args[3];
        if(l<r && a<b)for(int y=a/8;y<=(b-1)/8;++y)for(int x=l/8;x<=(r-1)/8;++x){
            unsigned cell=want[cells+y*40+x];if(!cell)continue;
            for(unsigned i=cell==255?0:cell-1;i<(cell==255?count:cell);++i){
                unsigned char *e=want+ent+i*stride;
                if((e[flags]&1) && (int32_t)e[prio]>args[4] &&
                   s16(e+left)<r && l<s16(e+right) && s16(e+top)<b && a<s16(e+bottom))e[flags]|=2;
            }
        }
        memset(stack,0xa7,sizeof stack);p32(stack,STOP);
        for(unsigned i=0;i<5;++i)p32(stack+4+i*4,args[i]);
        ck(uc_mem_write(u,STATE,before,SIZE));ck(uc_mem_write(u,SP,stack,sizeof stack));
        uint32_t rr[16];for(unsigned k=0;k<16;++k){rr[k]=0xabc10000+k*0x101+t;if(k==15)rr[k]=SP;ck(uc_reg_write(u,regs[k],rr+k));}
        ck(uc_emu_start(u,CODE,STOP,0,200000));
        uint32_t pc;ck(uc_reg_read(u,UC_M68K_REG_PC,&pc));
        ck(uc_mem_read(u,STATE,got,SIZE));
        if(pc!=STOP || memcmp(got,want,SIZE)){
            unsigned at=0;while(at<SIZE && got[at]==want[at])++at;
            fprintf(stderr,"case=%u pc=%x offset=%u bounds=%d,%d,%d,%d priority=%d\n",t,pc,at,args[0],args[1],args[2],args[3],args[4]);return 1;}
        for(unsigned k=2;k<16;++k){if(k==8 || k==9)continue;uint32_t v;ck(uc_reg_read(u,regs[k],&v));
            if(v!=(k==15?SP+4:rr[k])){fprintf(stderr,"ABI case=%u reg=%u\n",t,k);return 1;}}
        unsigned char sg[32];ck(uc_mem_read(u,SP,sg,sizeof sg));if(memcmp(stack,sg,sizeof sg))return 1;
    }
    ck(uc_close(u));puts("Native retention touch: 4096 original-traversal whole-state/ABI cases pass");return 0;
}
