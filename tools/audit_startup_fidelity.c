/* Original-code startup regression checks for F20 and F22.
 * Reuse the host runtime fixture and its target-only particle guard. */
#define main lap_limit_fixture_main
#include "verify_race_lap_limit.c"
#undef main
#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
static void audit_check(uc_err e) {if(e){fprintf(stderr,"%s\n",uc_strerror(e));exit(1);}}
static void audit_word(uc_engine *u,unsigned at,unsigned v)
{unsigned char b[2]={v,v>>8};audit_check(uc_mem_write(u,at,b,2));}
static unsigned audit_read(uc_engine *u,unsigned at)
{unsigned char b[2];audit_check(uc_mem_read(u,at,b,2));return b[0]|b[1]<<8;}
static void audit_run(uc_engine *u,unsigned start,unsigned end)
{
    uint16_t cs=start>=0x2e0f0?0x2e0f:0x1987,ds=0x3cbf,ss=0x8000,bp=0xf000,sp=0xe000,ip;
    audit_check(uc_reg_write(u,UC_X86_REG_CS,&cs));audit_check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    audit_check(uc_reg_write(u,UC_X86_REG_SS,&ss));audit_check(uc_reg_write(u,UC_X86_REG_BP,&bp));
    audit_check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    audit_check(uc_emu_start(u,start,end,0,10000));
    audit_check(uc_reg_read(u,UC_X86_REG_IP,&ip));assert(ip==end-16U*cs);
}
static unsigned polls,next_actor,configurations;
static jmp_buf first_driver;
static unsigned configured[2][13];
static void poll_devices(struct SlicksRaceRuntime *race,unsigned short ticks)
{(void)race;assert(ticks==1);++polls;longjmp(first_driver,1);}
static void actor_boundary(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size;(void)context;
    if(address!=0x330ab && address!=0x32ef2)return;
    uint16_t sp,ss,cs,ip,ax;
    audit_check(uc_reg_read(u,UC_X86_REG_SP,&sp));audit_check(uc_reg_read(u,UC_X86_REG_SS,&ss));
    unsigned stack=ss*16U+sp;
    if(address==0x330ab){
        ax=next_actor++;audit_check(uc_reg_write(u,UC_X86_REG_AX,&ax));
        unsigned char actor[64]={0};actor[0x3a]=1;actor[0x18]=1;
        audit_check(uc_mem_write(u,0xb0000+ax*64,actor,sizeof actor));
    } else {
        assert(configurations<2);
        for(unsigned i=0;i<13;++i)configured[configurations][i]=audit_read(u,stack+4+2*i);
        ++configurations;return; /* Execute the original configuration body. */
    }
    ip=audit_read(u,stack);cs=audit_read(u,stack+2);sp+=4;
    audit_check(uc_reg_write(u,UC_X86_REG_CS,&cs));audit_check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    audit_check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static unsigned draw_calls,cue_calls,draws_at_cue[4];
static void cue_boundary(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size;(void)context;
    if(address!=0x3aa7c && address!=0x3989b && address!=0x19878 && address!=0x395b8)return;
    uint16_t sp,ss,cs,ip,ax=1;
    audit_check(uc_reg_read(u,UC_X86_REG_SP,&sp));audit_check(uc_reg_read(u,UC_X86_REG_SS,&ss));
    unsigned stack=ss*16U+sp;
    if(address==0x3aa7c)++draw_calls;
    if(address==0x3989b){assert(cue_calls<4);draws_at_cue[cue_calls++]=draw_calls;audit_check(uc_reg_write(u,UC_X86_REG_AX,&ax));}
    ip=audit_read(u,stack);cs=audit_read(u,stack+2);sp+=4;
    audit_check(uc_reg_write(u,UC_X86_REG_CS,&cs));audit_check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    audit_check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    static unsigned char runtime[300000],logical[0x40000],chunky[64000];
    static struct SlicksRaceRuntime race;
    FILE *f=fopen("disasm/runtime.bin","rb");assert(f);size_t n=fread(runtime,1,sizeof runtime,f);fclose(f);
    uc_engine *u;audit_check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    audit_check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));audit_check(uc_mem_write(u,0x10100,runtime,n));
    const unsigned ds=0x3cbf0;
    audit_word(u,0x8effe,1);audit_word(u,0x8efa0,120); /* tick, boundary timer */
    audit_word(u,ds+0x3124,0); /* no track actors */
    audit_word(u,ds+0x4bc6,1);audit_word(u,ds+0x305f,100);audit_word(u,ds+0x3061,0);
    audit_word(u,ds+0x685e,0);audit_word(u,ds+0x6860,0);
    audit_run(u,0x1fe9b,0x2034f); /* Reaches per-driver update on first tick. */
    assert(audit_read(u,0x8efa0)==119 && audit_read(u,ds+0x685e)==1);
    race.navigation.zone_count=1;race.font.ready=1;
    race.participation_ready=1; /* No car artwork needed before the polling boundary. */
    for(unsigned v=0;v<SLICKS_VEHICLE_COUNT;++v) {
        race.properties[v].ready=1;
        for(unsigned d=0;d<SLICKS_CAR_BASE_DIRECTIONS;++d)race.sprites[v][d].ready=1;
    }
    for(unsigned l=0;l<SLICKS_START_LIGHT_COUNT;++l)race.start_lights[l].ready=1;
    assert(!slicks_race_start(&race,logical,chunky));
    assert(race.racing && !race.game_clock_ticks);
    race.poll_driver_devices=poll_devices;
    if(!setjmp(first_driver)) { slicks_race_step(&race,logical);abort(); }
    assert(polls==1 && race.game_clock_ticks==1 && race.boundary_timer==119);
    puts("F20 MATCH: first native tick reaches driver polling, clock=1 and boundary timer=119, as in DOS");
    /* Compare original constructor state, then every motion/lifetime update
     * through retirement, with weapons disabled and enabled. */
    audit_word(u,ds+0x16ce,0);audit_word(u,ds+0x16d0,0xb000);
    uc_hook hook;audit_check(uc_hook_add(u,&hook,UC_HOOK_CODE,actor_boundary,0,1,0));
    const short positions[]={80,129,130,200};unsigned actor_cases=0;
    for(unsigned weapons=0;weapons<2;++weapons)for(unsigned pos=0;pos<4;++pos) {
        audit_word(u,ds+0x3020,weapons);audit_word(u,0x8efd4,positions[pos]);audit_word(u,0x8efd2,80);
        next_actor=5;configurations=0;audit_run(u,0x251f3,0x252cf);
        assert(configurations==1+weapons && configured[0][0]==5);
        memset(&race,0,sizeof race);race.weapons_enabled=weapons;race.track_actors_ready=1;
        race.start_light_x=positions[pos];race.start_light_y=80;initialize_weapon_actors(&race);
        assert(race.weapons.actors[5].kind==4 && race.weapons.actors[5].asset==3);
        if(weapons)assert(race.weapons.actors[6].kind==5);
        audit_word(u,ds+0x16c0,next_actor);
        for(unsigned frame=0;frame<=205;++frame) {
            for(unsigned h=5;h<next_actor;++h) {
                const struct SlicksWeaponActor *a=&race.weapons.actors[h];unsigned at=0xb0000+h*64;
                const short *fields[]={&a->motion.x,&a->motion.y,&a->motion.vx,&a->motion.vy,&a->motion.ax,&a->motion.ay,&a->motion.lifetime,&a->motion.age};
                const unsigned offsets[]={0,2,0x1b,0x1d,0x1f,0x21,0x3e,0x3c};
                for(unsigned k=0;k<8;++k)assert(*fields[k]==(short)audit_read(u,at+offsets[k]));
                assert(a->priority==(audit_read(u,at+0x25)&255));
                assert(a->occlusion==(audit_read(u,at+0x3b)&255));
                assert((race.weapons.slots.state[h]>0)==((signed char)audit_read(u,at+0x1a)>0));
                ++actor_cases;
            }
            /* Setup PRE uses page one; first racing PRE uses page zero. */
            race.actor_page=(unsigned char)((frame+1)&1);
            audit_word(u,ds+0x16c6,race.actor_page);audit_word(u,0x8e004,0);
            audit_run(u,0x338e7,0x33aa9);advance_weapon_actors(&race);
        }
    }
    printf("F22 MATCH: %u original intro/notice constructor and motion/lifetime states, both directions and weapons off/on\n",actor_cases);
    audit_check(uc_hook_add(u,&hook,UC_HOOK_CODE,cue_boundary,0,1,0));
    audit_run(u,0x25358,0x253f4);
    assert(cue_calls==4 && draws_at_cue[0]==1 && draws_at_cue[1]==2 && draws_at_cue[2]==2 && draws_at_cue[3]==2);
    puts("DOS cue caller: light draw before cue 1 and another before cue 2 (compare diag_audit_startup.gdb target trace)");
    audit_check(uc_close(u));return 0;
}
