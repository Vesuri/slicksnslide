#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include "../src/game/actor_slots.h"
static void ck(uc_err e){if(e){fprintf(stderr,"Unicorn: %s\n",uc_strerror(e));exit(2);}}
static unsigned word(const unsigned char *p){return (unsigned)p[0]<<8|p[1];}
static uint32_t big(const unsigned char *p){return (uint32_t)word(p)<<16|word(p+2);}
static void w(unsigned char *p,unsigned v){p[0]=v>>8;p[1]=v;}
static void l(unsigned char *p,uint32_t v){w(p,v>>16);w(p+2,v);}
static unsigned seed=773;
static unsigned rnd(void){seed=seed*1664525u+1013904223u;return seed;}
static unsigned off(const char *path,const char *name){
    FILE *f=fopen(path,"r");char line[256];if(!f)exit(2);
    while(fgets(line,sizeof line,f))if(!strncmp(line,name,strlen(name))&&line[strlen(name)]==' '){
        unsigned v=strtoul(strstr(line,"equ")+3,0,0);fclose(f);return v;}
    fprintf(stderr,"Missing %s\n",name);exit(2);
}
static size_t load(const char *path,unsigned char *p,size_t cap){FILE *f=fopen(path,"rb");if(!f)exit(2);size_t n=fread(p,1,cap,f);fclose(f);if(!n||n==cap)exit(2);return n;}
int main(int argc,char **argv){
    if(argc!=5)return 2;
    unsigned stride=atoi(argv[4]);if(stride!=20&&stride!=24)return 2;
#define O(n) unsigned n=off(argv[2],#n)
    O(RACE_TRAIL_PARTICLES);O(RACE_TRAIL_PARTICLE_COUNT);O(RACE_WEAPON_SLOTS);
    O(RACE_EMISSION_SLOT_CURSOR);O(RACE_TRAIL_HANDLE);O(RACE_TRAIL_INDEX);
    O(RACE_ACTORS);O(ACTOR_SIZE);O(ACTOR_KIND);O(ACTOR_SAVED);O(RACE_SKIDMARK_COUNT);
    enum{CODE=0x10000,BASE=0x30000,N=65536,STACK=0x150000,STOP=0x180000,ALLOC=0x190000};
    unsigned char code[8192],allocator[2048];size_t n=load(argv[1],code,sizeof code),an=load(argv[3],allocator,sizeof allocator);
    if(n<8)return 2;unsigned entry=CODE+big(code+n-8),disabled=big(code+n-4);
    uc_engine *u;ck(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    ck(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));ck(uc_mem_map(u,0,0x200000,UC_PROT_ALL));
    ck(uc_mem_write(u,CODE,code,n));ck(uc_mem_write(u,ALLOC,allocator,an));
    const int regs[]={UC_M68K_REG_D1,UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,UC_M68K_REG_D7,UC_M68K_REG_A2,UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6};
    unsigned allocations=0,rejections=0;
    for(unsigned t=0;t<8192;++t){
        static unsigned char image[N],expected[N],got[N];
        for(unsigned i=0;i<N;++i)image[i]=(unsigned char)(rnd()>>24);
        struct SlicksActorSlots pool;
        pool.capacity=t%13==0?0:200;pool.high_water=1+t%200;
        for(unsigned h=0;h<200;++h)pool.state[h]=t%3==0?1:t%3==1?(signed char)(rnd()>>24):0;
        memcpy(image+RACE_WEAPON_SLOTS,pool.state,200);
        w(image+RACE_WEAPON_SLOTS+200,pool.high_water);w(image+RACE_WEAPON_SLOTS+202,pool.capacity);
        unsigned count=t%257;w(image+RACE_TRAIL_PARTICLE_COUNT,count);
        unsigned short cursor=t%3==0?0:(unsigned short)(1+t%205);w(image+RACE_EMISSION_SLOT_CURSOR,cursor);
        unsigned char disable=t%31==0;ck(uc_mem_write(u,disabled,&disable,1));
        unsigned x=(t*40503+32767)&65535,y=(t*257+32768)&65535;
        unsigned colour=t&255,priority=t%4==0?0:t%4==1?3:t%4==2?5:255;
        unsigned life=t%3==0?3:(t*71)&255,occ=(t%16)*15;
        uint32_t velocity=rnd();
        memcpy(expected,image,N);
        short h=0;
        if(!disable && count<256){h=cursor?slicks_actor_allocate_batch(&pool,&cursor):slicks_actor_allocate(&pool,1);
            memcpy(expected+RACE_WEAPON_SLOTS,pool.state,200);
            w(expected+RACE_WEAPON_SLOTS+200,pool.high_water);w(expected+RACE_EMISSION_SLOT_CURSOR,cursor);}
        if(h){
            ++allocations;expected[RACE_TRAIL_HANDLE+count]=(unsigned char)h;
            w(expected+RACE_TRAIL_INDEX+h*2,count);
            expected[RACE_ACTORS+h*ACTOR_SIZE+ACTOR_KIND]=0;expected[RACE_ACTORS+h*ACTOR_SIZE+ACTOR_SAVED]=0;
            w(expected+RACE_TRAIL_PARTICLE_COUNT,count+1);
            unsigned char *p=expected+RACE_TRAIL_PARTICLES+count*24;
            l(p,(uint32_t)(int32_t)(int16_t)x*64);l(p+4,(uint32_t)(int32_t)(int16_t)y*64);
            l(p+8,velocity);p[17]=life;p[18]=colour;p[19]=priority;
            p[20]=0;p[21]=priority==0&&life==3;p[22]=occ;p[23]=p[21]?5:1;
            l(expected+RACE_SKIDMARK_COUNT,big(expected+RACE_SKIDMARK_COUNT)+1);
        }else ++rejections;
        if(stride==20){
            unsigned char canonical[256*24];unsigned char *buffers[]={image,expected};
            for(unsigned b=0;b<2;++b){unsigned char *p=buffers[b]+RACE_TRAIL_PARTICLES;memcpy(canonical,p,sizeof canonical);
                for(unsigned i=0;i<256;++i){memcpy(p+i*20,canonical+i*24+2,2);memcpy(p+i*20+2,canonical+i*24+6,2);memcpy(p+i*20+4,canonical+i*24+8,16);}
                memset(p+256*20,0xcc,1024);}
        }
        ck(uc_mem_write(u,BASE,image,N));unsigned char stack[32];memset(stack,0xa7,sizeof stack);l(stack,STOP);w(stack+12,occ<<8);
        ck(uc_mem_write(u,STACK,stack,sizeof stack));uint32_t sp=STACK;ck(uc_reg_write(u,UC_M68K_REG_A7,&sp));
        uint32_t values[]={colour<<8|priority,velocity,life,x,y,0xa5000077,0xa5000088,BASE,0xa5000099,0xa50000aa};
        for(unsigned i=0;i<10;++i)ck(uc_reg_write(u,regs[i],values+i));
        ck(uc_emu_start(u,entry,STOP,0,100000));uint32_t pc;ck(uc_reg_read(u,UC_M68K_REG_PC,&pc));if(pc!=STOP)return 1;
        ck(uc_mem_read(u,BASE,got,N));if(memcmp(got,expected,N)){for(unsigned i=0;i<N;++i)if(got[i]!=expected[i]){fprintf(stderr,"layout=%u trial=%u byte=%u got=%u expected=%u\n",stride,t,i,got[i],expected[i]);break;}return 1;}
        for(unsigned i=0;i<10;++i){uint32_t v;ck(uc_reg_read(u,regs[i],&v));if(v!=values[i]){fprintf(stderr,"trial=%u register=%u\n",t,i);return 1;}}
        ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));if(sp!=STACK+4)return 1;
    }
    ck(uc_close(u));printf("Emission creation (%u-byte): 8192 complete pools, %u allocations, %u rejections; full records, mapping, slot state, cursor, guards and ABI pass\n",stride,allocations,rejections);return 0;
}
