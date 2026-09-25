/* Execute the original finish branch as a whole, not isolated rank/deadline
 * helpers. Feed successive line crossings to the native runtime. Only the
 * DOS sound-player boundary is replaced; no racing state is injected there. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/race_runtime.c"
#include "../src/game/setup_session.h"
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned at,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(u,at,b,2)); }
static unsigned get(uc_engine *u,unsigned at)
{ unsigned char b[2]; check(uc_mem_read(u,at,b,2)); return b[0]|b[1]<<8; }
static void dword(uc_engine *u,unsigned at,uint32_t v)
{ word(u,at,v); word(u,at+2,v>>16); }
static uint32_t get32(uc_engine *u,unsigned at)
{ return get(u,at)|((uint32_t)get(u,at+2)<<16); }
static unsigned sounds;
static struct SlicksSoundEvent sound_calls[4];
static void sound_boundary(uc_engine *u,uint64_t address,uint32_t size,void *opaque)
{
    (void)address; (void)size; (void)opaque;
    uint16_t ss,sp,ip,cs;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    if(sounds>=4) abort();
    sound_calls[sounds].sample_block=(unsigned char)(get(u,ss*16U+sp+4)-1);
    sound_calls[sounds].flags=(unsigned char)get(u,ss*16U+sp+6);
    sound_calls[sounds].priority=(unsigned char)get(u,ss*16U+sp+8);
    ip=get(u,ss*16U+sp); cs=get(u,ss*16U+sp+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp)); ++sounds;
}
static struct SlicksSetupSession session;
static const unsigned char points_by_rank[4]={10,6,3,1};
static void reward(struct SlicksRaceRuntime *race,unsigned driver,signed char rank)
{ (void)race; slicks_setup_finish_reward(&session,driver,rank,points_by_rank); }
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); int error=ferror(f); fclose(f);
    if(error || n<200000 || n==sizeof runtime) return 2;
    uc_engine *u; uc_hook hook;
    check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n));
    for(unsigned i=0;i<27;++i) {
        unsigned char handle=(unsigned char)(i+1);
        check(uc_mem_write(u,0x3cbf0+0x4c4a+i,&handle,1));
    }
    check(uc_hook_add(u,&hook,UC_HOOK_CODE,(void *)sound_boundary,0,0x3989b,0x3989b));
    unsigned cases=0,events=0,sound_mask=0;
    for(unsigned mode=0;mode<6;++mode) for(unsigned mask=1;mask<16;++mask)
    for(unsigned order=0;order<24;++order) for(unsigned spread=0;spread<3;++spread) {
        static struct SlicksRaceRuntime race;
        memset(&race,0,sizeof race); memset(&session,0,sizeof session);
        race.participation_ready=1; race.finish_reward=reward;
        race.race_mode=(short)mode; race.arcade_seconds=5;
        race.laps_to_run=(unsigned short)(mode==5?9999:4);
        word(u,0x3cbf0+0x92,mode); word(u,0x3cbf0+0xfa,5);
        word(u,0x3cbf0+0x4c18,race.laps_to_run); dword(u,0x3cbf0+0x6862,0);
        session.options.field_302e=20; word(u,0x3cbf0+0x302e,20);
        check(uc_mem_write(u,0x3cbf0+0x454,points_by_rank,4));
        for(unsigned i=0;i<4;++i) {
            signed char role=(mask>>i)&1?(i&1?-1:1):0,rank=role?-1:0;
            race.participation[i]=role; race.cars[i].lap=role?(unsigned short)(5-spread*(i%2)):1;
            race.cars[i].selected_surface=17;
            session.players.count+=role!=0;
            session.cash[i]=(short)(200+17*i); session.points[i]=(short)(7*i);
            check(uc_mem_write(u,0x3cbf0+0x4bc6+i,&role,1));
            check(uc_mem_write(u,0x3cbf0+0x4bce +i,&rank,1));
            word(u,0x3cbf0+0x4bf6+2*i,session.cash[i]); word(u,0x3cbf0+0x6826+2*i,session.points[i]);
            word(u,0x3cbf0+0x044c+2*i,0);
        }
        check(uc_mem_write(u,0x3cbf0+0x4c16,&session.players.count,1));
        unsigned permutation[4],pool[4]={0,1,2,3},digits=order;
        for(unsigned i=0;i<4;++i) {
            unsigned at=digits%(4-i); digits/=4-i; permutation[i]=pool[at];
            for(unsigned j=at;j<3-i;++j) pool[j]=pool[j+1];
        }
        /* Several crossings include pre-expiry, lazy timed target selection,
         * winner, lapped entrants, last entrant and repeated finished passes. */
        for(unsigned event=0;event<16;++event) {
            unsigned driver=permutation[event%4];
            if(!race.participation[driver]) continue;
            race.game_clock_ticks=event<4?100:600+event*31;
            dword(u,0x3cbf0+0x685e,race.game_clock_ticks);
            dword(u,0x3cbf0+0x74bc,race.game_clock_ticks);
            for(unsigned i=0;i<4;++i)
                word(u,0x3cbf0+0x4bfe +2*i,race.cars[i].lap-1+(i==driver));
            word(u,0x8ef98,driver);
            uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,bp=0xf000,sp=0xe000,ip;
            check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
            check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
            check(uc_reg_write(u,UC_X86_REG_SP,&sp));
            sounds=0; race.sound_event_count=0;
            check(uc_emu_start(u,0x22b17,0x22d1b,0,100000));
            check(uc_reg_read(u,UC_X86_REG_IP,&ip));
            if(ip!=0x22d1b-0x19870) { fprintf(stderr,"Unexpected finish exit %04x\n",ip); return 1; }
            advance_lap_checkpoints(&race,&race.cars[driver]);
            for(unsigned i=0;i<sounds;++i) sound_mask|=1U<<sound_calls[i].sample_block;
            unsigned native_sounds=race.sound_event_count;
            int sound_mismatch=sounds!=native_sounds;
            for(unsigned i=0;i<sounds && i<native_sounds;++i)
                sound_mismatch|=sound_calls[i].sample_block!=race.sound_events[i].sample_block ||
                    sound_calls[i].flags!=race.sound_events[i].flags ||
                    sound_calls[i].priority!=race.sound_events[i].priority;
            signed char ranks[4]; check(uc_mem_read(u,0x3cbf0+0x4bce,ranks,4));
            int failed= get(u,0x3cbf0+0x4c18)!=race.laps_to_run ||
                get32(u,0x3cbf0+0x6862)!=race.finish_deadline || sound_mismatch;
            for(unsigned i=0;i<4;++i) {
                signed char rank=race.finish_ranks_ready?race.finish_ranks[i]:(race.participation[i]?-1:0);
                failed|=rank!=ranks[i] || get(u,0x3cbf0+0x4bf6+2*i)!=(unsigned short)session.cash[i] ||
                    get(u,0x3cbf0+0x6826+2*i)!=(unsigned short)session.points[i];
            }
            if(failed) {
                fprintf(stderr,"Finish sequence mismatch mode=%u mask=%x order=%u spread=%u event=%u driver=%u deadline=%u/%u limit=%u/%u sound=%u/%u\n",
                    mode,mask,order,spread,event,driver,get32(u,0x3cbf0+0x6862),race.finish_deadline,
                    get(u,0x3cbf0+0x4c18),race.laps_to_run,sounds,native_sounds); return 1;
            }
            ++events;
        }
        ++cases;
    }
    check(uc_close(u));
    if(sound_mask!=((1U<<8)|(1U<<9)|(1U<<25))) return 1;
    printf("Original composed race completion: %u sequences, %u line crossings; six modes, all active masks and finish orders, lapped/repeated finishers, awards, lap/final-lap/winner sample IDs, flags, priorities and deadlines match\n",cases,events);
    return 0;
}
