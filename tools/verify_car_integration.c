/* Isolated non-blocking integration oracle. Blocked-ray responses are
 * separately covered by verify-car-probe and target-side motion shadows. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/game/race_runtime.c"
#include <unicorn/unicorn.h>
static void ck(uc_err e){if(e){fprintf(stderr,"%s\n",uc_strerror(e));exit(2);}}
static unsigned off(const char *path,const char *name){
    FILE *f=fopen(path,"r");char s[256];if(!f)exit(2);
    while(fgets(s,sizeof s,f))if(!strncmp(s,name,strlen(name))&&s[strlen(name)]==' '){
        unsigned v=strtoul(strstr(s,"equ")+3,0,0);fclose(f);return v;}
    fclose(f);fprintf(stderr,"Missing %s\n",name);exit(2);
}
static unsigned seed=97;
static unsigned rnd(void){seed=seed*1664525u+1013904223u;return seed>>8;}
static void be16(unsigned char *p,unsigned x){p[0]=x>>8;p[1]=x;}
static void be32(unsigned char *p,unsigned x){p[0]=x>>24;p[1]=x>>16;p[2]=x>>8;p[3]=x;}
static void blocked(uc_engine *u,uint64_t at,uint32_t size,void *data){
    (void)u;(void)at;(void)size;(void)data;fputs("Unexpected blocked-ray helper\n",stderr);exit(1);
}
int main(int argc,char **argv){
    if(argc!=3)return 2;
    unsigned char code[16384];FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    size_t n=fread(code,1,sizeof code,f);fclose(f);if(!n||n==sizeof code)return 2;
#define O(name) unsigned name=off(argv[2],#name)
    O(CAR_SIZE);O(CAR_X);O(CAR_Y);O(CAR_SPEED_FIXED);O(CAR_VELOCITY_X);O(CAR_VELOCITY_Y);
    O(CAR_HEADING);O(CAR_DAMAGE);O(CAR_DRIVE_BIAS);O(CAR_SPECIAL_DRIVE_STATE);
    O(CAR_DRIVE_COEFFICIENTS);O(CAR_POSITION_SCALE);O(CAR_TOUCHING_SOLID);
    O(CAR_ACTOR_LAYER);O(CAR_ACTOR_CONTACT);O(CAR_COLLISION_SAMPLING);
    O(RACE_COLLISION_ERROR);O(RACE_TRACK_COLLISION_COUNT);O(RACE_BOUNDARY_LEVEL);
    enum{CODE=0x10000,RACE=0x100000,CAR=0x180000,STACK=0x80000,STOP=0x90000};
    uc_engine *u;ck(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    ck(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));ck(uc_mem_map(u,0,0x200000,UC_PROT_ALL));
    ck(uc_mem_write(u,CODE,code,n));ck(uc_mem_write(u,0x30000,direction_x,16));ck(uc_mem_write(u,0x30020,direction_y,16));
    uc_hook hook;ck(uc_hook_add(u,&hook,UC_HOOK_CODE,blocked,0,0x90010,0x90010));
    static struct SlicksRaceRuntime race;
    const int regs[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,
        UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,UC_M68K_REG_A3,UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6};
    if(CAR_SIZE+32>512)return 2;
    for(unsigned t=0;t<24000;++t){
        unsigned char image[512],expected[512],got[512],stack[20],meta[4];
        for(unsigned i=0;i<512;++i)image[i]=rnd();
        struct SlicksRaceCar car={0};
        car.x=300+rnd()%31401;car.y=300+rnd()%17601;
        /* Reused pixel coordinates must be repaired by clamping even for
         * noncanonical initial positions; include both signed-long edges. */
        static const int edges[]={-2147483647-1,-101,-1,0,299,300,301,
            17899,17900,17901,31699,31700,31701,2147483647};
        if(t>=12000 && t%13==0) {
            car.x=edges[(t/13)%14];car.y=edges[(t/13+5)%14];
        }
        car.speed_fixed=(int)(rnd()%23001)-3000;
        car.velocity_x=(int)(rnd()%12001)-6000;car.velocity_y=(int)(rnd()%12001)-6000;
        car.heading=rnd()%19200;car.damage[0]=rnd()%1000;
        car.drive_bias=rnd()%2001;car.special_drive_state=t%3==0?0:t%3==1?1:-1;
        car.drive_coefficients[0]=80+rnd()%41;car.drive_coefficients[3]=80+rnd()%41;
        car.drive_coefficients[1]=(int)(rnd()%111)-40;
        car.position_scale=1+rnd()%255;car.touching_solid=rnd()%2;car.actor_contact=rnd()%2;
        car.actor_layer=rnd()%2;car.collision_sampling=t&1;
        unsigned ticks=t%8==7?65535:t%7,active=t%4;
        race.collision_error=0;race.track_collision_count=42;race.boundary_level=5;
#define SERIALIZE(buf) do { \
    unsigned char *p=(buf)+16; \
    be32(p+CAR_X,car.x);be32(p+CAR_Y,car.y);be32(p+CAR_SPEED_FIXED,car.speed_fixed); \
    be32(p+CAR_VELOCITY_X,car.velocity_x);be32(p+CAR_VELOCITY_Y,car.velocity_y); \
    be16(p+CAR_HEADING,car.heading);be16(p+CAR_DRIVE_BIAS,car.drive_bias); \
    be16(p+CAR_SPECIAL_DRIVE_STATE,car.special_drive_state); \
    for(unsigned j=0;j<4;++j)be16(p+CAR_DAMAGE+j*2,car.damage[j]); \
    for(unsigned j=0;j<7;++j)be16(p+CAR_DRIVE_COEFFICIENTS+j*2,car.drive_coefficients[j]); \
    p[CAR_POSITION_SCALE]=car.position_scale;p[CAR_TOUCHING_SOLID]=car.touching_solid; \
    p[CAR_ACTOR_LAYER]=car.actor_layer;p[CAR_ACTOR_CONTACT]=car.actor_contact;p[CAR_COLLISION_SAMPLING]=car.collision_sampling; \
}while(0)
        SERIALIZE(image);memcpy(expected,image,512);
        ck(uc_mem_write(u,CAR,image,512));
        be32(meta,42);ck(uc_mem_write(u,RACE+RACE_TRACK_COLLISION_COUNT,meta,4));
        meta[0]=0;ck(uc_mem_write(u,RACE+RACE_COLLISION_ERROR,meta,1));
        be16(meta,5);ck(uc_mem_write(u,RACE+RACE_BOUNDARY_LEVEL,meta,2));
        integrate_car_motion_reference(&race,&car,ticks,active);SERIALIZE(expected);
        be32(stack,STOP);be32(stack+4,RACE);be32(stack+8,CAR+16);be32(stack+12,ticks);be32(stack+16,active);
        ck(uc_mem_write(u,STACK,stack,20));unsigned sp=STACK;ck(uc_reg_write(u,UC_M68K_REG_A7,&sp));
        unsigned values[11];for(unsigned i=0;i<11;++i){values[i]=0xa5000000u+i*0x10101u+t;ck(uc_reg_write(u,regs[i],values+i));}
        uc_err e=uc_emu_start(u,CODE,STOP,0,2000000);
        if(e){unsigned pc;uc_reg_read(u,UC_M68K_REG_PC,&pc);fprintf(stderr,"case %u pc=%x ",t,pc);ck(e);}
        unsigned pc;ck(uc_reg_read(u,UC_M68K_REG_PC,&pc));
        if(pc!=STOP){fprintf(stderr,"Case %u did not return: pc=%x (instruction limit)\n",t,pc);return 1;}
        ck(uc_mem_read(u,CAR,got,512));
        if(memcmp(got,expected,512)){for(unsigned i=0;i<512;++i)if(got[i]!=expected[i]){fprintf(stderr,"Case %u byte %u got %u expected %u\n",t,i,got[i],expected[i]);break;}return 1;}
        ck(uc_mem_read(u,RACE+RACE_COLLISION_ERROR,meta,1));if(meta[0]!=race.collision_error)return 1;
        ck(uc_mem_read(u,RACE+RACE_TRACK_COLLISION_COUNT,meta,4));if(memcmp(meta,"\0\0\0\52",4))return 1;
        ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));if(sp!=STACK+4)return 1;
        for(unsigned i=0;i<11;++i){unsigned v;ck(uc_reg_read(u,regs[i],&v));if(v!=values[i])return 1;}
    }
    uc_close(u);puts("Car integration: 24000 no-wall normal/coast/special, signed ticks, sampling, signed-long position edges, clamps, full-car canaries, completed returns and ABI cases pass");return 0;
}
