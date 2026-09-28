#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unicorn/unicorn.h>
static void ck(uc_err e){if(e){fprintf(stderr,"%s\n",uc_strerror(e));exit(2);}}
static unsigned seed=117;
static unsigned rnd(void){seed=seed*1664525u+1013904223u;return seed>>8;}
static void be16(unsigned char *p,unsigned x){p[0]=x>>8;p[1]=x;}
static void be32(unsigned char *p,unsigned x){p[0]=x>>24;p[1]=x>>16;p[2]=x>>8;p[3]=x;}
static unsigned word(const unsigned char *p){return ((unsigned)p[0]<<8)|p[1];}
static unsigned off(const char *path,const char *name){
    FILE *f=fopen(path,"r");char s[256];if(!f)exit(2);
    while(fgets(s,sizeof s,f))if(!strncmp(s,name,strlen(name))&&s[strlen(name)]==' '){
        unsigned v=strtoul(strstr(s,"equ")+3,0,0);fclose(f);return v;}
    fclose(f);fprintf(stderr,"Missing %s\n",name);exit(2);
}
int main(int argc,char **argv){
    if(argc!=3 && argc!=4)return 2;
    int compact=argc==4;
    if(compact && strcmp(argv[3],"compact"))return 2;
    unsigned char code[4096];FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    size_t n=fread(code,1,sizeof code,f);fclose(f);if(!n||n==sizeof code)return 2;
    enum{CODE=0x10000,BASE=0x20000,N=0x20000,PIX=32768,ROWS=100000,STACK=0x80000,STOP=0x90000};
    unsigned PART=off(argv[2],"RACE_TRAIL_PARTICLES"),NEXT=off(argv[2],"RACE_ACTOR_ORDER_NEXT"),
        INDEX=off(argv[2],"RACE_TRAIL_INDEX"),CHUNKY=off(argv[2],"RACE_CHUNKY");
    uc_engine *u;ck(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    ck(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));ck(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    ck(uc_mem_write(u,CODE,code,n));
    const int regs[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,
        UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,UC_M68K_REG_A3,UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6};
    unsigned restored=0,visited=0,boundaries=0;
    for(unsigned t=0;t<4096;++t){
        static unsigned char image[N],expected[N],got[N];unsigned char stack[12];
        for(unsigned i=0;i<N;++i)image[i]=rnd();
        unsigned order[199];for(unsigned i=0;i<199;++i)order[i]=i+1;
        if(t%2)for(unsigned i=198;i;i--){unsigned j=rnd()%(i+1),s=order[i];order[i]=order[j];order[j]=s;}
        unsigned count=t%200;
        for(unsigned i=0;i<count;++i){unsigned h=order[i];
            image[NEXT+h]=i+1<count?order[i+1]:0;
            int index=t%3==0&&i==count/2?-1:(int)((i*71)%256);
            be16(image+INDEX+h*2,(unsigned)index);
        }
        for(unsigned i=0;i<256;++i){unsigned char *p=image+PART+i*24;
            p[20]=rnd()%4;
            if(p[20]&1){
                unsigned x=t%4?rnd()%320:0,y=t%4?rnd()%200:199;
                if(t%7==0){x=319;y=199;}
                be16(p+12,x);be16(p+14,y);
            }else{be16(p+12,0xffff);be16(p+14,0xffff);}
        }
        for(unsigned y=0;y<200;++y)be32(image+ROWS+y*4,y*320);
        be32(image+CHUNKY,BASE+PIX);
        memcpy(expected,image,N);
        unsigned first=count?order[0]:0,h=first;
        while(h){unsigned index=word(expected+INDEX+h*2);if(index&0x8000){++boundaries;break;}
            unsigned char *p=expected+PART+index*24;++visited;
            if(p[20]&1){expected[PIX+word(p+14)*320+word(p+12)]=p[16];p[20]=2;++restored;}
            h=expected[NEXT+h];
        }
        /* Keep the canonical 24-byte reference above independent of the
         * compact layout. Project only redundant coordinate high words;
         * retain every represented byte and guard the unused pool tail. */
        if(compact){
            unsigned char canonical[256*24];
            unsigned char *buffers[]={image,expected};
            for(unsigned b=0;b<2;++b){
                unsigned char *pool=buffers[b]+PART;
                memcpy(canonical,pool,sizeof canonical);
                for(unsigned i=0;i<256;++i){
                    memcpy(pool+i*20,canonical+i*24+2,2);
                    memcpy(pool+i*20+2,canonical+i*24+6,2);
                    memcpy(pool+i*20+4,canonical+i*24+8,16);
                }
                memset(pool+256*20,0xcc,256*4);
            }
        }
        ck(uc_mem_write(u,BASE,image,N));be32(stack,STOP);
        be32(stack+4,BASE);be32(stack+8,first);
        ck(uc_mem_write(u,STACK,stack,12));unsigned sp=STACK;ck(uc_reg_write(u,UC_M68K_REG_A7,&sp));
        unsigned values[11];for(unsigned i=0;i<11;++i){values[i]=0xa5000000u+i*0x10101u+t;ck(uc_reg_write(u,regs[i],values+i));}
        ck(uc_emu_start(u,CODE,STOP,0,100000));ck(uc_mem_read(u,BASE,got,N));
        unsigned pc;ck(uc_reg_read(u,UC_M68K_REG_PC,&pc));if(pc!=STOP)return 1;
        if(memcmp(got,expected,N)){for(unsigned i=0;i<N;++i)if(got[i]!=expected[i]){fprintf(stderr,"Case %u byte %u got %u expected %u\n",t,i,got[i],expected[i]);break;}return 1;}
        unsigned result;ck(uc_reg_read(u,UC_M68K_REG_D0,&result));if(result!=h){fprintf(stderr,"Case %u returned %u instead of %u\n",t,result,h);return 1;}
        ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));if(sp!=STACK+4)return 1;
        for(unsigned i=0;i<11;++i){unsigned v;ck(uc_reg_read(u,regs[i],&v));if(v!=values[i])return 1;}
    }
    uc_close(u);printf("Point restore (%u-byte records): 4096 chains, %u visits, %u restores, %u sprite boundaries; saved flags, repeated pixels, invalid unsaved coordinates, full-image canaries and ABI pass\n",compact?20:24,visited,restored,boundaries);return 0;
}
