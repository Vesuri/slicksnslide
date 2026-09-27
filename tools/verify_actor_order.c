#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/game/race_runtime.c"
#include <unicorn/unicorn.h>
static void ck(uc_err e){if(e){fprintf(stderr,"%s\n",uc_strerror(e));exit(2);}}
static unsigned off(const char *path,const char *name){
    FILE *f=fopen(path,"r");char s[256];if(!f)exit(2);
    while(fgets(s,sizeof s,f))if(!strncmp(s,name,strlen(name)) && s[strlen(name)]==' '){
        unsigned v=strtoul(strstr(s,"equ")+3,0,0);fclose(f);return v;}
    fclose(f);fprintf(stderr,"Missing offset %s\n",name);exit(2);
}
static unsigned seed=83;
static unsigned rnd(void){seed=seed*1664525u+1013904223u;return seed>>8;}
static void be16(unsigned char *p,unsigned x){p[0]=x>>8;p[1]=x;}
static void be32(unsigned char *p,unsigned x){p[0]=x>>24;p[1]=x>>16;p[2]=x>>8;p[3]=x;}
int main(int argc,char **argv){
    if(argc!=3)return 2;
    unsigned char code[4096];FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    size_t n=fread(code,1,sizeof code,f);fclose(f);if(!n || n==sizeof code)return 2;
#define O(name) unsigned name=off(argv[2],#name)
    O(RACE_WEAPON_SLOTS);O(SLOTS_HIGH_WATER);O(RACE_TRAIL_INDEX);
    O(RACE_TRAIL_PARTICLES);O(PARTICLE_SIZE);O(PARTICLE_PRIORITY);
    O(RACE_ACTORS);O(ACTOR_SIZE);O(ACTOR_KIND);O(ACTOR_PRIORITY);
    O(RACE_ACTOR_ORDER_HEAD);O(RACE_ACTOR_ORDER_NEXT);O(RACE_ACTOR_ORDER_MAX);
    O(RACE_ACTOR_ORDER_READY);O(RACE_ACTOR_ORDER_DRAWN);
    enum{CODE=0x10000,RACE=0x20000,STACK=0x80000,STOP=0x90000,N=65536};
    uc_engine *u;ck(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    ck(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));ck(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    ck(uc_mem_write(u,CODE,code,n));
    static struct SlicksRaceRuntime race;
    const int regs[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,
        UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,UC_M68K_REG_A3,UC_M68K_REG_A4,
        UC_M68K_REG_A5,UC_M68K_REG_A6};
    unsigned linked=0;
    for(unsigned t=0;t<12000;++t){
        unsigned char image[N],expected[N],got[N],stack[8];
        for(unsigned i=0;i<N;++i)image[i]=rnd();
        unsigned count=t%201;
        race.weapons.slots.high_water=count;
        be16(image+RACE_WEAPON_SLOTS+SLOTS_HIGH_WATER,count);
        for(unsigned i=0;i<256;++i){
            unsigned p=t%4?rnd()%256:3;
            race.trail_particles[i].priority=p;
            image[RACE_TRAIL_PARTICLES+i*PARTICLE_SIZE+PARTICLE_PRIORITY]=p;
        }
        for(unsigned h=0;h<200;++h){
            signed char state=t%5?(signed char)rnd():1;
            short index=t%3==0?-1:(short)(rnd()%257)-1;
            race.weapons.slots.state[h]=state;
            race.weapons.trail_index[h]=index;
            image[RACE_WEAPON_SLOTS+h]=(unsigned char)state;
            be16(image+RACE_TRAIL_INDEX+h*2,(unsigned short)index);
            struct SlicksWeaponActor *a=&race.weapons.actors[h];
            a->kind=rnd()%5;a->priority=t%4?rnd()%256:3;
            image[RACE_ACTORS+h*ACTOR_SIZE+ACTOR_KIND]=a->kind;
            image[RACE_ACTORS+h*ACTOR_SIZE+ACTOR_PRIORITY]=a->priority;
        }
        memcpy(race.actor_order_next,image+RACE_ACTOR_ORDER_NEXT,sizeof race.actor_order_next);
        memcpy(expected,image,N);
        build_actor_order(&race,0);
        memcpy(expected+RACE_ACTOR_ORDER_HEAD,race.actor_order_head,sizeof race.actor_order_head);
        memcpy(expected+RACE_ACTOR_ORDER_NEXT,race.actor_order_next,sizeof race.actor_order_next);
        expected[RACE_ACTOR_ORDER_MAX]=race.actor_order_max;
        expected[RACE_ACTOR_ORDER_READY]=1;expected[RACE_ACTOR_ORDER_DRAWN]=0;
        for(unsigned p=0;p<128;++p)for(unsigned h=race.actor_order_head[p];h;h=race.actor_order_next[h])++linked;
        ck(uc_mem_write(u,RACE,image,N));be32(stack,STOP);be32(stack+4,RACE);
        ck(uc_mem_write(u,STACK,stack,8));unsigned sp=STACK;ck(uc_reg_write(u,UC_M68K_REG_A7,&sp));
        unsigned values[11];for(unsigned i=0;i<11;++i){values[i]=0xa5000000u+i*0x10101u+t;ck(uc_reg_write(u,regs[i],values+i));}
        ck(uc_emu_start(u,CODE,STOP,0,100000));ck(uc_mem_read(u,RACE,got,N));
        if(memcmp(got,expected,N)){for(unsigned i=0;i<N;++i)if(got[i]!=expected[i]){fprintf(stderr,"Case %u byte %u got %u expected %u\n",t,i,got[i],expected[i]);break;}return 1;}
        ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));if(sp!=STACK+4)return 1;
        for(unsigned i=0;i<11;++i){unsigned v;ck(uc_reg_read(u,regs[i],&v));if(v!=values[i])return 1;}
    }
    uc_close(u);printf("Actor order: 12000 mixed/empty/full pools, %u linked handles; all priorities, signed states, stable chains, untouched bytes and ABI pass\n",linked);return 0;
}
