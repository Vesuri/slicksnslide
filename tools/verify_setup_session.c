#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/setup_session.h"
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned a,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(u,a,b,2)); }
static unsigned rw(uc_engine *u,unsigned a)
{ unsigned char b[2]; check(uc_mem_read(u,a,b,2)); return b[0]|b[1]<<8; }
struct Calls { unsigned count; unsigned char driver[4],vehicle[4]; };
static void apply(void *p,unsigned driver,signed char vehicle)
{ struct Calls *c=p; if(c->count>=4) { fprintf(stderr,"Too many vehicle callbacks: driver=%u vehicle=%d\n",driver,vehicle); exit(1); } c->driver[c->count]=driver; c->vehicle[c->count++]=(unsigned char)vehicle; }
static void boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)address; (void)size;
    uint16_t ss,sp,cs,ip;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    apply(p,rw(u,stack+4),(signed char)rw(u,stack+6));
    ip=rw(u,stack); cs=rw(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void run(uc_engine *u,unsigned address,unsigned end,unsigned segment)
{
    uint16_t cs=segment,ds=0x3cbf,ss=0x8000,sp=0xf000,bp=0xef00;
    word(u,0x8f000,0); word(u,0x8f002,0x7000);
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    check(uc_reg_write(u,UC_X86_REG_BP,&bp));
    check(uc_emu_start(u,address,end,0,100000));
}
static int same(uc_engine *u,unsigned at,const void *p,unsigned size)
{ unsigned char b[64]; if(size>sizeof b) abort(); check(uc_mem_read(u,0x3cbf0+at,b,size)); return !memcmp(b,p,size); }
static void stop(uc_engine *u,uint64_t address,uint32_t size,void *p)
{ (void)address; (void)size; (void)p; check(uc_emu_stop(u)); }
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t size=fread(runtime,1,sizeof runtime,f); int error=ferror(f); fclose(f);
    if(error || size<200000 || size==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,size));
    struct Calls native,dos; uc_hook hook;
    check(uc_hook_add(u,&hook,UC_HOOK_CODE,boundary,&dos,0x1ccfc,0x1ccfc));
    check(uc_hook_add(u,&hook,UC_HOOK_CODE,stop,0,0x263f9,0x263f9));
    check(uc_hook_add(u,&hook,UC_HOOK_CODE,stop,0,0x26429,0x26429));
    unsigned cases=0;
    const short gates[]={-32768,-1,0,1,32767};
    for(unsigned seed=0;seed<32;++seed) for(unsigned mode=0;mode<6;++mode)
    for(unsigned gate_index=0;gate_index<5;++gate_index) {
        short gate=gates[gate_index];
        static unsigned char zero[65536]; check(uc_mem_write(u,0x3cbf0,zero,sizeof zero));
        struct SlicksConfiguration config={0};
        struct SlicksSetupProfile profiles[4]={{7,0,{0}},{7,11,{1,2,3,4,5,6}},
            {6,10,{0}},{0,3,{7,8,9,10,11,12}}};
        unsigned char fallback[4][6],weights[10],flags[13];
        for(unsigned i=0;i<24;++i) ((unsigned char *)fallback)[i]=(i+seed)&63;
        for(unsigned i=0;i<10;++i) weights[i]=(seed+i*7)&255;
        for(unsigned i=0;i<13;++i) flags[i]=(seed>> (i%5))&1;
        for(unsigned i=0;i<15;++i) { config.options[i]=(short)(seed*131+i*17); word(u,0x3cbf0+0x92+8*i,(unsigned short)config.options[i]); }
        config.options[0]=mode; word(u,0x3cbf0+0x92,mode);
        word(u,0x3cbf0+0x90,(unsigned short)gate);
        unsigned char mode_flags=(unsigned char)seed;
        check(uc_mem_write(u,0x3cbf0+0x1157+mode,&mode_flags,1));
        for(unsigned i=0;i<4;++i) {
            config.selected_profile[i]=i==0?2:i==1?1:i==2?3:(short)(seed%5)-1;
            word(u,0x3cbf0+0x44c+2*i,(unsigned short)config.selected_profile[i]);
            check(uc_mem_write(u,0x3cbf0+0x3ede +i,&profiles[i].flags,1));
            check(uc_mem_write(u,0x3cbf0+0x3fa6+i,&profiles[i].vehicle,1));
            check(uc_mem_write(u,0x3cbf0+0x1db+6*i,profiles[i].colours,6));
        }
        short override_count=(short)(seed%7)-1;
        word(u,0x3cbf0+0x36a8,4); word(u,0x3cbf0+0x4c6e,10);
        word(u,0x3cbf0+0xf1a,(unsigned short)override_count);
        check(uc_mem_write(u,0x3cbf0+0x433,fallback,24));
        check(uc_mem_write(u,0x3cbf0+0x1b4,weights,10));
        check(uc_mem_write(u,0x3cbf0+0x106f,flags,13));
        unsigned short initial_seed=(unsigned short)(seed*2179);
        word(u,0x3cbf0+0x2aaa,initial_seed); word(u,0x3cbf0+0x2aac,0);
        struct SlicksSetupSession session;
        struct SlicksSetupResources resources={profiles,4,10,override_count,fallback,weights,flags,apply,&native};
        for(unsigned stage=0;stage<4;++stage) {
            memset(&native,0,sizeof native); memset(&dos,0,sizeof dos);
            if(!stage) {
                slicks_setup_session_start(&session,&config,&resources,initial_seed);
                word(u,0x8f004,0); word(u,0x8f006,1);
                run(u,0x2bb70,0x70000,0x266c);
            } else if(stage==3) {
                for(unsigned d=0;d<4;++d) {
                    session.points[d]=(short)(seed*997+d*7000);
                    word(u,0x3cbf0+0x6826+2*d,(unsigned short)session.points[d]);
                }
                struct SlicksSetupSession before=session;
                slicks_setup_after_race(&session,&config,&resources);
                run(u,0x25937,0x25943,0x1987);
                if(memcmp(before.inventory,session.inventory,sizeof session.inventory) ||
                    memcmp(before.cash,session.cash,sizeof session.cash) ||
                    memcmp(before.points,session.points,sizeof session.points)) goto fail;
            } else {
                for(unsigned d=0;d<4;++d) {
                    session.points[d]=(short)(seed*997+d*7000+1);
                    word(u,0x3cbf0+0x6826+2*d,(unsigned short)session.points[d]);
                }
                slicks_setup_new_game(&session,&config,&resources,mode_flags,(short)gate);
                run(u,0x2a2ce,0x2a2dd,0x266c);
                run(u,0x2bdd8,0x70000,0x266c);
                run(u,0x26372,0x263e6,0x2468);
                /* This main-function slice uses the real CS, not a segment
                 * chosen only to cover its physical address. */
                run(u,0x263e6,0x70000,0x1987);
                uint16_t ip,cs;
                check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_CS,&cs));
                if(cs*16U+ip!=(gate>0?0x263f9U:0x26429U)) goto fail;
                const short resolved[]={session.options.weapons_enabled,session.options.inventory_mode,
                    session.options.fuel,session.options.damage,session.options.car_collisions,
                    session.options.starting_cash,session.options.field_302c,session.options.field_302e,
                    session.options.field_3030};
                for(unsigned i=0;i<9;++i)
                    if(rw(u,0x3cbf0+0x3020+2*i)!=(unsigned short)resolved[i]) goto fail;
                for(unsigned d=0;d<4;++d) {
                    if(rw(u,0x3cbf0+0x4bf6+2*d)!=(unsigned short)session.cash[d]) goto fail;
                    for(unsigned i=0;i<13;++i)
                        if(rw(u,0x3cbf0+0x6a7a+26*d+2*i)!=(unsigned short)session.inventory[d][i]) goto fail;
                }
            }
            for(unsigned d=0;d<4;++d)
                if(rw(u,0x3cbf0+0x44c+2*d)!=(unsigned short)config.selected_profile[d] ||
                   rw(u,0x3cbf0+0x6826+2*d)!=(unsigned short)session.points[d]) goto fail;
            if(!same(u,0x4bc2,session.players.vehicle,4) || !same(u,0x4bc6,session.players.participation,4) ||
                !same(u,0x310c,session.players.colours,24) || !same(u,0x4bf2,session.players.order,4) ||
                !same(u,0x4c16,&session.players.count,1) || memcmp(&native,&dos,sizeof native) ||
                (rw(u,0x3cbf0+0x2aaa)|((unsigned long)rw(u,0x3cbf0+0x2aac)<<16))!=session.random_state) goto fail;
            ++cases; continue;
fail:
            fprintf(stderr,"Setup session mismatch seed=%u mode=%u gate=%d stage=%u\n",seed,mode,gate,stage); return 1;
        }
    }
    check(uc_close(u));
    printf("DOS setup session: %u composed startup/new-game/post-race transitions match selections, colours, callback order, inventory, cash, points reset/preservation and shared RNG\n",cases);
    return 0;
}
