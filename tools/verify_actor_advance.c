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
static void bound_check(int ok,const char *why)
{if(!ok){fprintf(stderr,"Sprite bound producer: %s\n",why);exit(1);}}
static void verify_bound_producers(void)
{
    static struct SlicksRaceRuntime r;
    const unsigned counts[]={0,5,18,32,100};
    for(unsigned n=0;n<5;++n)for(unsigned mask=0;mask<16;++mask)
    for(unsigned weapons=0;weapons<2;++weapons)for(unsigned tracks=0;tracks<2;++tracks){
        memset(&r,0,sizeof r);r.navigation.actor_count=counts[n];
        r.participation_ready=1;r.weapons_enabled=weapons;r.weapons.ready=1;
        r.track_actors_ready=tracks;
        for(unsigned i=0;i<counts[n];++i)r.navigation.actors[i].kind=i%5;
        unsigned active=0;
        for(unsigned d=0;d<4;++d){r.participation[d]=(mask&(1U<<d))?(d&1?-1:1):0;active+=r.participation[d]!=0;}
        initialize_weapon_actors(&r);
        unsigned initial=1+4+counts[n]+1+weapons+active;
        bound_check(r.weapons.sprite_high_water==initial && r.weapons.slots.high_water==initial,"initial slots, intro/notice and participation");
        for(unsigned i=0;i<9;++i)add_trail_component(&r,20+i,30,7,0,0,0,3);
        bound_check(r.trail_particle_count==9 && r.weapons.sprite_high_water==initial,"point allocations must not raise bound");
        short h=allocate_weapon_actor(&r,-1,1,12);
        bound_check(h==(short)(initial+9) && r.weapons.sprite_high_water==h+1,"sprite after point-only tail raises bound");
        configure_weapon_actor(&r,h,30,40,1,2,3,4,0);
        unsigned raised=r.weapons.sprite_high_water;
        r.weapons.slots.state[h]=0;
        add_trail_component(&r,30,40,7,0,0,0,3);
        bound_check(r.weapons.sprite_high_water==raised,"sprite slot reused by point preserves conservative bound");
        r.weapons.slots.state[h]=0;
        bound_check(allocate_weapon_actor(&r,2,4,1)==h && r.weapons.sprite_high_water==raised,"sprite reuse below bound");
        memset(r.weapons.slots.state,1,sizeof r.weapons.slots.state);
        r.weapons.slots.high_water=SLICKS_ACTOR_CAPACITY;
        bound_check(!allocate_weapon_actor(&r,2,4,1) && r.weapons.sprite_high_water==raised,"failed allocation leaves bound alone");
        r.weapons.slots.state[199]=0;
        bound_check(allocate_weapon_actor(&r,2,4,1)==199 && r.weapons.sprite_high_water==200,"highest handle fits byte bound");
        r.weapons.sprite_high_water=0;r.weapons.slots.state[199]=0;
        bound_check(allocate_weapon_actor(&r,2,4,1)==199 && !r.weapons.sprite_high_water,"legacy zero retains full-scan fallback");
        initialize_weapon_actors(&r);
        bound_check(r.weapons.sprite_high_water==initial,"race reinitialization resets stale bound");
    }
    puts("Sprite scan bound: 320 producer lifecycles pass, including loaded/unloaded track actors, point/sprite reuse, failure, capacity and reinitialization");
}
int main(int argc,char **argv)
{
    if(argc!=3)return 2;
    verify_bound_producers();
    unsigned char code[8192];FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    size_t n=fread(code,1,sizeof code,f);fclose(f);if(n<4||n==sizeof code)return 2;
    unsigned entry=(unsigned)code[n-4]<<24|code[n-3]<<16|code[n-2]<<8|code[n-1];
    const char *keys[]={"RACE_WEAPON_SLOTS","SLOTS_HIGH_WATER","RACE_TRAIL_INDEX",
        "RACE_ACTORS","ACTOR_SIZE","ACTOR_KIND","RACE_ACTOR_PAGE",
        "ACTOR_MOTION_X","ACTOR_MOTION_Y","ACTOR_MOTION_VX","ACTOR_MOTION_VY",
        "ACTOR_MOTION_AX","ACTOR_MOTION_AY","ACTOR_MOTION_LIFETIME","ACTOR_MOTION_AGE",
        "ACTOR_MOTION_FRAME","ACTOR_MOTION_PERIOD","ACTOR_MOTION_FRAMES"};
    unsigned o[18];for(unsigned i=0;i<18;++i)o[i]=off(argv[2],keys[i]);
    unsigned dirty_offset=off(argv[2],"RET_GEOMETRY_DIRTY");
    unsigned bound_offset=off(argv[2],"RACE_SPRITE_HIGH_WATER");
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
    for(unsigned trial=0;trial<8192+14;++trial) {
        memset(before,0xa7,sizeof before);
        r.weapons.slots.high_water=trial>=8192 || trial%17==0?2:1+trial%200;
        r.actor_page=trial>=8192?1:trial%4;
        unsigned bound=trial>=8192?2:trial%4==0?0:trial%4==1?255:
            1+rnd()%r.weapons.slots.high_water;
        before[bound_offset]=(unsigned char)bound;
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
            if(trial>=8192 && h==1) {
                /* Isolate each producer: another actor must not hide a
                 * missing dirty write by setting the shared flag first. */
                a->motion=(struct SlicksActorMotion){0};a->motion.frames=4;
                a->kind=3;r.weapons.slots.state[h]=1;r.weapons.trail_index[h]=-1;
                switch(trial-8192) {
                case 1:a->motion.vx=64;break;
                case 2:a->motion.vy=64;break;
                case 3:a->motion.ax=64;break;
                case 4:a->motion.ay=64;break;
                case 5:a->motion.frame=3;a->motion.age=2;a->motion.period=1;a->motion.frames=5;break;
                case 6:a->motion.frame=4;break;
                case 7:a->motion.lifetime=1;break;
                case 8:r.weapons.slots.state[h]=-2;break;
                case 9:a->motion.x=63;a->motion.vx=1;break;
                case 10:a->motion.x=32767;a->motion.vx=1;break;
                case 11:a->motion.frame=-1;a->motion.age=2;a->motion.period=1;break;
                case 12:a->motion.frame=3;a->motion.age=2;a->motion.period=1;break;
                case 13:a->motion.period=1;break;
                }
            }
            if(bound && h>=bound && h<r.weapons.slots.high_water){
                /* Producers guarantee no relevant non-point actor above the
                 * bound. Exercise all three reasons the full reference skips. */
                if(h%3==0)r.weapons.trail_index[h]=h;
                else if(h%3==1)r.weapons.slots.state[h]=0;
                else {a->kind=0;a->motion.lifetime=0;}
            }
            unsigned char *p=before+o[3]+h*o[4];
            for(unsigned k=0;k<8;++k)w(p+o[7+k],(unsigned short)*fields[k]);
            p[o[15]]=a->motion.frame;p[o[16]]=a->motion.period;p[o[17]]=a->motion.frames;
            p[o[5]]=a->kind;before[o[0]+h]=r.weapons.slots.state[h];
            w(before+o[2]+2*h,(unsigned short)r.weapons.trail_index[h]);
        }
        memcpy(expected,before,sizeof expected);advance_weapon_actors_reference(&r);
        unsigned must_invalidate=0;
        for(unsigned h=0;h<200;++h) {
            struct SlicksWeaponActor *a=&r.weapons.actors[h];
            const short fields[]={a->motion.x,a->motion.y,a->motion.vx,a->motion.vy,
                a->motion.ax,a->motion.ay,a->motion.lifetime,a->motion.age};
            unsigned char *p=expected+o[3]+h*o[4];
            for(unsigned k=0;k<8;++k)w(p+o[7+k],(unsigned short)fields[k]);
            p[o[15]]=a->motion.frame;p[o[16]]=a->motion.period;p[o[17]]=a->motion.frames;
            p[o[5]]=a->kind;expected[o[0]+h]=r.weapons.slots.state[h];
            const unsigned char *old=before+o[3]+h*o[4];
            unsigned was=old[o[5]]==3 && (signed char)before[o[0]+h]>0 && old[o[15]]<4;
            unsigned now=a->kind==3 && r.weapons.slots.state[h]>0 && (unsigned char)a->motion.frame<4;
            if(was!=now || (was && (old[o[7]]!=p[o[7]] ||
                (old[o[7]+1]&0xc0)!=(p[o[7]+1]&0xc0) || old[o[8]]!=p[o[8]] ||
                (old[o[8]+1]&0xc0)!=(p[o[8]+1]&0xc0))))must_invalidate=1;
        }
        ck(uc_mem_write(u,RACE,before,sizeof before));
        unsigned char dirty=0;
        ck(uc_mem_write(u,0x30000+dirty_offset,&dirty,1));
        unsigned char args[8];l(args,STOP);l(args+4,RACE);
        unsigned sp=STACK-8;ck(uc_mem_write(u,sp,args,8));ck(uc_reg_write(u,UC_M68K_REG_A7,&sp));
        for(unsigned i=0;i<11;++i){unsigned v=0xa5000000u+i+trial;ck(uc_reg_write(u,regs[i],&v));}
        ck(uc_emu_start(u,CODE+entry,STOP,0,100000));
        unsigned pc;ck(uc_reg_read(u,UC_M68K_REG_PC,&pc));ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));
        if(pc!=STOP||sp!=STACK-4)return 1;
        for(unsigned i=0;i<11;++i){unsigned v;ck(uc_reg_read(u,regs[i],&v));if(v!=0xa5000000u+i+trial)return 1;}
        ck(uc_mem_read(u,RACE,got,sizeof got));
        ck(uc_mem_read(u,0x30000+dirty_offset,&dirty,1));
        if(must_invalidate && dirty!=1) {
            fprintf(stderr,"trial=%u missed geometry invalidation\n",trial);return 1;
        }
        if((trial==8192 || trial>=8192+12) && dirty) {
            fprintf(stderr,"trial=%u unnecessarily invalidated stable geometry\n",trial);return 1;
        }
        if(memcmp(got,expected,sizeof got)) {
            for(unsigned i=0;i<IMAGE;++i)if(got[i]!=expected[i]) {
                fprintf(stderr,"trial=%u offset=%u got=%u expected=%u\n",trial,i,got[i],expected[i]);break;
            }
            return 1;
        }
    }
    uc_close(u);puts("Native actor advance: 8192 bounded/fallback pools plus 14 isolated geometry cases; inert/near-inert, signed overflow, expiry, animation, required geometry invalidations, canaries and ABI match full-scan reference");
    return 0;
}
