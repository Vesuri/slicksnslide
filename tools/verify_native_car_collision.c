/* Native pair loop versus the existing DOS-backed scalar resolver.
 * Inputs here keep signed arithmetic within range; wrap extremes need a
 * separate explicit-width oracle before production acceptance. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../src/game/race_runtime.c"
#include <unicorn/unicorn.h>
static void ck(uc_err e){if(e){fprintf(stderr,"%s\n",uc_strerror(e));exit(2);}}
static unsigned off(const char *path,const char *name){
    FILE *f=fopen(path,"r");char line[256];if(!f)exit(2);
    while(fgets(line,sizeof line,f))if(!strncmp(line,name,strlen(name))&&line[strlen(name)]==' '){unsigned v=strtoul(strstr(line,"equ")+3,0,0);fclose(f);return v;}
    fprintf(stderr,"Missing offset %s\n",name);exit(2);
}
static void b16(unsigned char *p,unsigned v){p[0]=v>>8;p[1]=v;}
static void b32(unsigned char *p,unsigned v){b16(p,v>>16);b16(p+2,v);}
static unsigned seed=918;static unsigned rnd(void){seed=seed*1664525U+1013904223U;return seed>>8;}
static void write_guard(uc_engine *u,uc_mem_type type,uint64_t address,int size,int64_t value,void *opaque){
    (void)u;(void)type;(void)value;(void)opaque;
    if((address>=0x100000 && address+(unsigned)size<=0x110000) ||
       (address>=0x80000-48 && address+(unsigned)size<=0x80000))return;
    fprintf(stderr,"Out-of-state/stack write %llx size=%d\n",address,size);exit(1);
}
int main(int argc,char **argv){
    if(argc!=3)return 2;
    unsigned char code[4096];FILE *f=fopen(argv[1],"rb");if(!f)return 2;size_t n=fread(code,1,sizeof code,f);fclose(f);if(!n||n==sizeof code)return 2;
#define O(x) unsigned x=off(argv[2],#x)
    O(RACE_CARS);O(RACE_PROPERTIES);O(RACE_PAIR_DISABLED);O(RACE_PARTICIPATION_READY);O(RACE_PARTICIPATION);O(RACE_PAIR_COUNT);O(RACE_PAIR_IMPACT);
    O(CAR_SIZE);O(PROPERTY_SIZE);O(PROPERTY_COLLISION_RADIUS);O(PROPERTY_COLLISION_WEIGHT);
    O(CAR_X);O(CAR_Y);O(CAR_VELOCITY_X);O(CAR_VELOCITY_Y);O(CAR_ACTOR_LAYER);O(CAR_ACTOR_CONTACT);O(CAR_VEHICLE);O(CAR_TOUCHING_CAR);O(CAR_COLLISION_PARTNER);O(CAR_PAIR_IMPACT);O(CAR_PENDING_DAMAGE_IMPACT);
    enum{CODE=0x10000,RACE=0x100000,N=65536,STACK=0x80000,STOP=0x90000};
    if(RACE_PROPERTIES+10*PROPERTY_SIZE>N || RACE_PARTICIPATION+4>N)return 2;
    static struct SlicksRaceRuntime race;
    static unsigned char image[N],expected[N],got[N];
    uc_engine *u;ck(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));ck(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));ck(uc_mem_map(u,0,0x200000,UC_PROT_ALL));ck(uc_mem_write(u,CODE,code,n));
    uc_hook guard;ck(uc_hook_add(u,&guard,UC_HOOK_MEM_WRITE,write_guard,0,1,0));
    const int regs[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,UC_M68K_REG_A3,UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6};
    unsigned impulses=0,disabled=0,inactive=0,boundary_hits=0,boundary_misses=0;
    const unsigned boundary_cases=4*3*4*3*2*4*3;
    for(unsigned t=0;t<12000+boundary_cases;++t){
        memset(&race,0,sizeof race);memset(image,0xa5,sizeof image);
        race.participation_ready=t%5!=0;race.car_collisions_disabled=t%17==0;
        race.collision_count=19;race.collision_impact=rnd()%500;
        unsigned current=t%4;
        for(unsigned i=0;i<10;++i){race.properties[i].collision_radius=1+rnd()%10;race.properties[i].collision_weight=1+rnd()%255;}
        for(unsigned i=0;i<4;++i){struct SlicksRaceCar *c=&race.cars[i];
            race.participation[i]=(signed char)((int)(rnd()%3)-1);
            c->x=10000+(int)(rnd()%(t&1?300:10000))-150;c->y=8000+(int)(rnd()%(t&1?300:8000))-150;
            c->velocity_x=(int)(rnd()%4001)-2000;c->velocity_y=(int)(rnd()%4001)-2000;
            c->actor_layer=rnd()%2;c->actor_contact=rnd()%2;c->vehicle=rnd()%10;
            c->touching_car=t%3==0?1:0;c->collision_partner=rnd()%4;
            c->collision_impact=rnd()%100;c->pending_damage_impact=rnd()%100;
        }
        if(t%11==0)for(unsigned i=0;i<4;++i){race.cars[i].x=10000;race.cars[i].y=8000;race.cars[i].velocity_x=race.cars[i].velocity_y=0;race.cars[i].actor_layer=0;race.participation[i]=1;}
        int boundary_hit=-1,boundary_latch=0,boundary_disabled=0;
        unsigned boundary_other=0;
        if(t>=12000){
            unsigned q=t-12000;current=q%4;q/=4;
            unsigned other=q%3;q/=3;if(other>=current)++other;
            unsigned edge=q%4;q/=4;int distance=(int)(q%3)-1;q/=3;
            unsigned latch=q%2;q/=2;unsigned gate=q%4;q/=4;unsigned speed=q%3;
            for(unsigned i=0;i<4;++i){memset(&race.cars[i],0,sizeof race.cars[i]);race.participation[i]=0;}
            race.participation_ready=1;race.participation[current]=-1;race.participation[other]=gate==1?0:1;
            race.car_collisions_disabled=gate==3;
            struct SlicksRaceCar *a=&race.cars[current],*b=&race.cars[other];
            a->vehicle=0;b->vehicle=1;a->touching_car=latch;a->collision_partner=255;
            a->x=10000;a->y=8000;a->velocity_x=speed==1?860:speed==2?-860:0;a->velocity_y=speed?-960:0;
            b->actor_layer=gate==2;
            race.properties[0].collision_radius=2;
            race.properties[0].collision_weight=18;race.properties[1].collision_weight=20;
            long denominator=(labs(a->velocity_x)+labs(a->velocity_y))/2+1;
            b->x=a->x+a->velocity_x*10/denominator;b->y=a->y+a->velocity_y*10/denominator;
            if(edge<2)b->x+=(edge==0?-1:1)*(100+distance);
            else b->y+=(edge==2?-1:1)*(100+distance);
            boundary_hit=gate==0 && distance<=0;boundary_latch=latch;boundary_disabled=gate==3;boundary_other=other;
        }
#define PACK(buf) do { \
    (buf)[RACE_PAIR_DISABLED]=race.car_collisions_disabled;(buf)[RACE_PARTICIPATION_READY]=race.participation_ready; \
    b32((buf)+RACE_PAIR_COUNT,race.collision_count);b32((buf)+RACE_PAIR_IMPACT,race.collision_impact); \
    for(unsigned j=0;j<10;++j){unsigned char *p=(buf)+RACE_PROPERTIES+j*PROPERTY_SIZE;p[PROPERTY_COLLISION_RADIUS]=race.properties[j].collision_radius;p[PROPERTY_COLLISION_WEIGHT]=race.properties[j].collision_weight;} \
    for(unsigned j=0;j<4;++j){struct SlicksRaceCar *c=&race.cars[j];unsigned char *p=(buf)+RACE_CARS+j*CAR_SIZE; \
        (buf)[RACE_PARTICIPATION+j]=(unsigned char)race.participation[j]; \
        b32(p+CAR_X,c->x);b32(p+CAR_Y,c->y);b32(p+CAR_VELOCITY_X,c->velocity_x);b32(p+CAR_VELOCITY_Y,c->velocity_y); \
        p[CAR_ACTOR_LAYER]=c->actor_layer;p[CAR_ACTOR_CONTACT]=c->actor_contact;p[CAR_VEHICLE]=c->vehicle;p[CAR_TOUCHING_CAR]=c->touching_car;p[CAR_COLLISION_PARTNER]=c->collision_partner; \
        b32(p+CAR_PAIR_IMPACT,c->collision_impact);b32(p+CAR_PENDING_DAMAGE_IMPACT,c->pending_damage_impact); } \
}while(0)
        PACK(image);memcpy(expected,image,N);ck(uc_mem_write(u,RACE,image,N));
        disabled+=race.car_collisions_disabled;inactive+=!driver_role(&race,current);
        slicks_race_resolve_car_collisions(&race,(unsigned short)current);impulses+=race.collision_count-19;PACK(expected);
        if(boundary_hit>=0){
            unsigned expected_touch=boundary_disabled?(unsigned)boundary_latch:(unsigned)boundary_hit;
            unsigned expected_count=19+(boundary_hit && !boundary_latch);
            if(race.cars[current].touching_car!=expected_touch || race.collision_count!=expected_count ||
               race.cars[current].collision_partner!=(boundary_hit?boundary_other:255)){
                fprintf(stderr,"Independent boundary result failed case=%u\n",t);return 1;
            }
            if(boundary_hit)++boundary_hits;else ++boundary_misses;
        }
        unsigned char stack[12];b32(stack,STOP);b32(stack+4,RACE);b32(stack+8,current);ck(uc_mem_write(u,STACK,stack,12));
        unsigned sp=STACK,values[11];ck(uc_reg_write(u,UC_M68K_REG_A7,&sp));
        for(unsigned k=0;k<11;++k){values[k]=0xa5000000+k*123+t;ck(uc_reg_write(u,regs[k],values+k));}
        ck(uc_emu_start(u,CODE,STOP,0,100000));unsigned pc;ck(uc_reg_read(u,UC_M68K_REG_PC,&pc));if(pc!=STOP)return 1;
        ck(uc_mem_read(u,RACE,got,N));if(memcmp(expected,got,N)){for(unsigned i=0;i<N;++i)if(expected[i]!=got[i]){fprintf(stderr,"case=%u offset=%u got=%u expected=%u\n",t,i,got[i],expected[i]);break;}return 1;}
        ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));if(sp!=STACK+4)return 1;
        for(unsigned k=0;k<11;++k){unsigned v;ck(uc_reg_read(u,regs[k],&v));if(v!=values[k])return 1;}
    }
    if(!impulses||!disabled||!inactive||!boundary_hits||!boundary_misses)return 1;
    ck(uc_close(u));printf("Native car pairs: 12000 random + %u boundary full-image/ABI cases, %u impulses, %u disabled and %u inactive; boundary hits=%u misses=%u pass\n",boundary_cases,impulses,disabled,inactive,boundary_hits,boundary_misses);return 0;
}
