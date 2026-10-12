/* Audit reproducer, NOT a passing fidelity gate: reports known differences.
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
    uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,bp=0xf000,sp=0xe000,ip;
    audit_check(uc_reg_write(u,UC_X86_REG_CS,&cs));audit_check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    audit_check(uc_reg_write(u,UC_X86_REG_SS,&ss));audit_check(uc_reg_write(u,UC_X86_REG_BP,&bp));
    audit_check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    audit_check(uc_emu_start(u,start,end,0,10000));
    audit_check(uc_reg_read(u,UC_X86_REG_IP,&ip));assert(ip==end-0x19870);
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
    if(address==0x330ab){ax=next_actor++;audit_check(uc_reg_write(u,UC_X86_REG_AX,&ax));}
    else {assert(configurations<2);for(unsigned i=0;i<13;++i)configured[configurations][i]=audit_read(u,stack+4+2*i);++configurations;}
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
    /* Original startup configures two drawable sprite actors, not just slots. */
    audit_word(u,ds+0x16ce,0);audit_word(u,ds+0x16d0,0xb000);
    audit_word(u,ds+0x3020,1);audit_word(u,0x8efd4,100);audit_word(u,0x8efd2,80);
    next_actor=5;uc_hook hook;audit_check(uc_hook_add(u,&hook,UC_HOOK_CODE,actor_boundary,0,1,0));
    audit_run(u,0x251f3,0x252cf);
    assert(configurations==2 && configured[0][0]==5 && configured[1][0]==6);
    assert(configured[0][11]==1 && configured[1][11]==1);
    memset(&race,0,sizeof race);race.weapons_enabled=1;initialize_weapon_actors(&race);
    assert(!race.weapons.actors[5].kind && !race.weapons.actors[6].kind);
    printf("DIFFERENCE: DOS configures visible intro/notice actors (lifetimes %u/%u); native actor kinds %u/%u are rejected by its painter\n",audit_read(u,0xb0000+5*64+0x3e),audit_read(u,0xb0000+6*64+0x3e),race.weapons.actors[5].kind,race.weapons.actors[6].kind);
    audit_check(uc_hook_add(u,&hook,UC_HOOK_CODE,cue_boundary,0,1,0));
    audit_run(u,0x25358,0x253f4);
    assert(cue_calls==4 && draws_at_cue[0]==1 && draws_at_cue[1]==2 && draws_at_cue[2]==2 && draws_at_cue[3]==2);
    puts("DOS cue caller: light draw before cue 1 and another before cue 2 (compare diag_audit_startup.gdb target trace)");
    audit_check(uc_close(u));return 0;
}
