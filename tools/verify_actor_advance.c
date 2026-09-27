/* Native shared-pool advancement versus the DOS-verified scalar semantics. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/game/race_runtime.c"
#include <unicorn/unicorn.h>

static void ck(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e));exit(2); } }
static unsigned off(const char *path,const char *key)
{
    FILE *f=fopen(path,"r");char line[256];size_t n=strlen(key);
    if(!f)exit(2);
    while(fgets(line,sizeof line,f))if(!strncmp(line,key,n)&&line[n]==' ') {
        fclose(f);return (unsigned)strtoul(strstr(line,"equ")+3,0,0);
    }
    fprintf(stderr,"missing offset %s\n",key);exit(2);
}
static void w(unsigned char *p,unsigned v) {p[0]=v>>8;p[1]=v;}
static void l(unsigned char *p,unsigned v) {w(p,v>>16);w(p+2,v);}
static unsigned seed=127;
static unsigned rnd(void) {seed=seed*1664525u+1013904223u;return seed>>8;}
int main(int argc,char **argv)
{
    if(argc!=3)return 2;
    unsigned char code[8192];FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    size_t n=fread(code,1,sizeof code,f);fclose(f);if(n<4||n==sizeof code)return 2;
    unsigned entry=(unsigned)code[n-4]<<24|code[n-3]<<16|code[n-2]<<8|code[n-1];
    const char *keys[]={"RACE_WEAPON_SLOTS","SLOTS_HIGH_WATER","RACE_TRAIL_INDEX",
        "RACE_ACTORS","ACTOR_SIZE","ACTOR_KIND","RACE_ACTOR_PAGE",
        "ACTOR_MOTION_X","ACTOR_MOTION_Y","ACTOR_MOTION_VX","ACTOR_MOTION_VY",
        "ACTOR_MOTION_AX","ACTOR_MOTION_AY","ACTOR_MOTION_LIFETIME","ACTOR_MOTION_AGE",
        "ACTOR_MOTION_FRAME","ACTOR_MOTION_PERIOD","ACTOR_MOTION_FRAMES"};
    unsigned o[18];for(unsigned i=0;i<18;++i)o[i]=off(argv[2],keys[i]);
    enum {CODE=0x10000,RACE=0x100000,STACK=0x80000,STOP=0x9000,IMAGE=65536};
    static unsigned char before[IMAGE],expected[IMAGE],got[IMAGE];
    static struct SlicksRaceRuntime r;
    if(o[3]+200*o[4]>IMAGE)return 2;
    uc_engine *u;ck(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    ck(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));ck(uc_mem_map(u,0,0x200000,UC_PROT_ALL));
    ck(uc_mem_write(u,CODE,code,n));
    const int regs[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,
        UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,UC_M68K_REG_A3,
        UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6};
    for(unsigned trial=0;trial<8192;++trial) {
        memset(before,0xa7,sizeof before);
        r.weapons.slots.high_water=1+trial%200;r.actor_page=trial%4;
        w(before+o[0]+o[1],r.weapons.slots.high_water);before[o[6]]=r.actor_page;
        for(unsigned h=0;h<200;++h) {
            struct SlicksWeaponActor *a=&r.weapons.actors[h];
            short *fields[]={&a->motion.x,&a->motion.y,&a->motion.vx,&a->motion.vy,
                &a->motion.ax,&a->motion.ay,&a->motion.lifetime,&a->motion.age};
            for(unsigned k=0;k<8;++k)*fields[k]=(short)rnd();
            a->motion.frame=(signed char)rnd();a->motion.period=(signed char)rnd();
            a->motion.frames=(unsigned char)rnd();a->kind=rnd()%5;
            const signed char states[]={0,1,3,5,-1,-2,-3,-5};
            r.weapons.slots.state[h]=states[rnd()%8];
            r.weapons.trail_index[h]=rnd()%4?(short)-1:(short)(rnd()%200);
            /* Half the actors exercise exact inert state or one changed
             * field, including overflow, first-frame animation and expiry. */
            if((h+trial)%2==0) {
                a->motion.vx=a->motion.vy=a->motion.ax=a->motion.ay=0;
                a->motion.age=0;a->motion.frame=a->motion.period=0;
                a->motion.lifetime=(short)(trial%5-2);
                unsigned k=(h+trial/2)%12;
                if(k<6)*fields[k+2]=(short)(1u<<(trial%16));
                if(k==6)a->motion.frame=(signed char)(1u<<(trial%8));
                if(k==7)a->motion.period=(signed char)(1u<<(trial%8));
                if(k>=8)a->motion.lifetime=0;
            }
            unsigned char *p=before+o[3]+h*o[4];
            for(unsigned k=0;k<8;++k)w(p+o[7+k],(unsigned short)*fields[k]);
            p[o[15]]=a->motion.frame;p[o[16]]=a->motion.period;p[o[17]]=a->motion.frames;
            p[o[5]]=a->kind;before[o[0]+h]=r.weapons.slots.state[h];
            w(before+o[2]+2*h,(unsigned short)r.weapons.trail_index[h]);
        }
        memcpy(expected,before,sizeof expected);advance_weapon_actors_reference(&r);
        for(unsigned h=0;h<200;++h) {
            struct SlicksWeaponActor *a=&r.weapons.actors[h];
            const short fields[]={a->motion.x,a->motion.y,a->motion.vx,a->motion.vy,
                a->motion.ax,a->motion.ay,a->motion.lifetime,a->motion.age};
            unsigned char *p=expected+o[3]+h*o[4];
            for(unsigned k=0;k<8;++k)w(p+o[7+k],(unsigned short)fields[k]);
            p[o[15]]=a->motion.frame;p[o[16]]=a->motion.period;p[o[17]]=a->motion.frames;
            p[o[5]]=a->kind;expected[o[0]+h]=r.weapons.slots.state[h];
        }
        ck(uc_mem_write(u,RACE,before,sizeof before));
        unsigned char args[8];l(args,STOP);l(args+4,RACE);
        unsigned sp=STACK-8;ck(uc_mem_write(u,sp,args,8));ck(uc_reg_write(u,UC_M68K_REG_A7,&sp));
        for(unsigned i=0;i<11;++i){unsigned v=0xa5000000u+i+trial;ck(uc_reg_write(u,regs[i],&v));}
        ck(uc_emu_start(u,CODE+entry,STOP,0,100000));
        unsigned pc;ck(uc_reg_read(u,UC_M68K_REG_PC,&pc));ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));
        if(pc!=STOP||sp!=STACK-4)return 1;
        for(unsigned i=0;i<11;++i){unsigned v;ck(uc_reg_read(u,regs[i],&v));if(v!=0xa5000000u+i+trial)return 1;}
        ck(uc_mem_read(u,RACE,got,sizeof got));
        if(memcmp(got,expected,sizeof got)) {
            for(unsigned i=0;i<IMAGE;++i)if(got[i]!=expected[i]) {
                fprintf(stderr,"trial=%u offset=%u got=%u expected=%u\n",trial,i,got[i],expected[i]);break;
            }
            return 1;
        }
    }
    uc_close(u);puts("Native actor advance: 8192 pools, inert/near-inert, signed overflow, expiry, animation, canaries and ABI match reference");
    return 0;
}
