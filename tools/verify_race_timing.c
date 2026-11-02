#define main track_info_verifier_main
#include "verify_track_info.c"
#undef main
#include "../src/game/race_timing.h"
#include "../src/platform/amiga/video_timing.h"
static unsigned ports[3],port_count;
static void timer_out(uc_engine *u,uint32_t port,int size,uint32_t value,void *context)
{
    (void)u; (void)context;
    REQUIRE(size==1 && port_count<3 && port==(port_count?0x40:0x43));
    ports[port_count++]=value;
}
static void timer_vector(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size; (void)context;
    uint16_t ss,sp,cs,ip,ax=0,dx=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp; REQUIRE(get(u,stack+4)==8);
    if(address==0x142c9) { ax=0x1234; dx=0x5678; }
    else REQUIRE(address==0x142dc);
    ip=get(u,stack); cs=get(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); REQUIRE(f);
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f); REQUIRE(n && n<sizeof runtime);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,n));
    uc_hook hook;
    check(uc_hook_add(u,&hook,UC_HOOK_CODE,timer_vector,0,0x142c9,0x142c9));
    check(uc_hook_add(u,&hook,UC_HOOK_CODE,timer_vector,0,0x142dc,0x142dc));
    check(uc_hook_add(u,&hook,UC_HOOK_INSN,timer_out,0,1,0,UC_X86_INS_OUT));
    for(unsigned argument=0;argument<65536;++argument) {
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
        word(u,0x8f000,0); word(u,0x8f002,0x9000); word(u,0x8f004,argument);
        word(u,0x3cbf0+0x17b6,argument&1?500:0);
        word(u,0x3cbf0+0x74bc,0x1234); word(u,0x3cbf0+0x74be,0x5678);
        word(u,0x3cbf0+0x74c0,0xabcd); word(u,0x3cbf0+0x74c2,0xef01);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        port_count=0; check(uc_emu_start(u,0x37bc2,0x90000,0,10000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        unsigned long divisor=slicks_timer_divisor((unsigned short)argument);
        REQUIRE(ip==0 && sp==0xf004 && port_count==3 && ports[0]==0x34 &&
            ports[1]==(divisor&255) && ports[2]==((divisor>>8)&255));
        REQUIRE(get(u,0x3cbf0+0x74c4)==(divisor&65535) && get(u,0x3cbf0+0x74c6)==divisor>>16);
        unsigned enabled=(short)argument>=100;
        REQUIRE(get(u,0x3cbf0+0x17b6)==(enabled?argument:0));
        REQUIRE(get(u,0x3cbf0+0x74bc)==(enabled?0:0x1234) && get(u,0x3cbf0+0x74be)==(enabled?0:0x5678));
        REQUIRE(get(u,0x3cbf0+0x74c0)==(enabled?0:0xabcd) && get(u,0x3cbf0+0x74c2)==(enabled?0:0xef01));
    }
    /* Independent rational check over a minute of updates at every speed
     * offered by the original UI. Retain the exact PIT divisor, including
     * integer truncation (200% is not precisely twice 100%). */
    for(unsigned speed=50;speed<=200;++speed) {
        unsigned argument=slicks_speed_timer_argument((short)speed);
        unsigned long period=slicks_timer_divisor((unsigned short)argument)*50UL,phase=0,total=0;
        for(unsigned frame=1;frame<=3000;++frame) {
            total+=slicks_physics_clock_advance(&phase,period,0);
            uint64_t cycles=(uint64_t)frame*1193182;
            REQUIRE(total==cycles/period && phase==cycles%period);
        }
        unsigned long before=phase;
        REQUIRE(!slicks_physics_clock_advance(&phase,period,1) && phase==before);
    }
    /* Real-time line clock: PAL 15625 Hz (and the 0 fixture default), NTSC
     * and both cross-crystal line rates, exact against 64-bit arithmetic at
     * every UI speed, including multi-chunk (paused) reads. */
    static const unsigned long rates[]={0,15625,15734,15591,15769};
    for(unsigned r=0;r<sizeof rates/sizeof *rates;++r)
    for(unsigned speed=50;speed<=200;++speed) {
        unsigned argument=slicks_speed_timer_argument((short)speed);
        unsigned long divisor=slicks_timer_divisor((unsigned short)argument);
        unsigned long phase=0; uint64_t lines=0,total=0;
        for(unsigned read=1;read<=3000;++read) {
            unsigned long step=read%500==0?40000UL:1UL+(read*7919UL)%900UL;
            total+=slicks_physics_clock_lines(&phase,divisor*50UL,0,step,rates[r]); lines+=step;
            uint64_t cycles=lines*1193182ULL,tick=(uint64_t)divisor*(rates[r]?rates[r]:15625ULL);
            REQUIRE(total==cycles/tick && phase==cycles%tick);
        }
        unsigned long before=phase;
        REQUIRE(!slicks_physics_clock_lines(&phase,divisor*50UL,1,1000,rates[r]) && phase==before);
    }
    /* Display timing for each beam standard, frame parity and crystal. */
    static const unsigned short frames[]={313,312,263,262};
    for(unsigned b=0;b<4;++b) for(unsigned crystal=0;crystal<2;++crystal) {
        struct SlicksVideoTiming t; slicks_video_timing_init(&t,frames[b],(unsigned char)crystal);
        unsigned ntsc=b>=2; REQUIRE(t.ntsc_beam==ntsc);
        REQUIRE(t.display_start==(ntsc?0x2c:0x38) && t.display_end==t.display_start+200);
        /* The playfield and the line after it fit in the frame. */
        REQUIRE(t.display_end<frames[b]);
        double clock=crystal?3579545.0:3546895.0,half=ntsc?455.0:454.0;
        double line_rate=2.0*clock/half;
        REQUIRE(t.line_rate>=line_rate-0.5 && t.line_rate<=line_rate+0.5);
        if(b==0 && !crystal) REQUIRE(t.line_rate==15625 && t.tick_step==SLICKS_VIDEO_TICK_UNIT);
        /* Game fields over ten real minutes equal PAL's to one field. */
        unsigned long phase=0,fields=0; unsigned long n=(unsigned long)(600.0*line_rate/frames[b]);
        for(unsigned long i=0;i<n;++i) fields+=slicks_video_tick(&t,&phase);
        double seconds=n*frames[b]/line_rate,expected=seconds*2.0*3546895.0/142102.0;
        REQUIRE(fields+1.0>=expected && fields<=expected+1.0);
        /* At most one field per display frame. */
        REQUIRE(t.tick_step<=SLICKS_VIDEO_TICK_UNIT);
        /* The publication edge is the first display end strictly after now. */
        for(unsigned long at=0;at<4UL*frames[b];++at) {
            unsigned long edge=slicks_video_next_display_end(&t,at);
            REQUIRE(edge>at && edge%frames[b]==t.display_end && edge-at<=frames[b]);
        }
    }
    unsigned long legacy=0,current=0;
    for(unsigned i=0;i<3000;++i) REQUIRE(slicks_physics_clock_advance(&legacy,0,0)==
        slicks_physics_clock_advance(&current,655350,0) && legacy==current);
    check(uc_close(u)); puts("Original PIT setup: all 65536 arguments match divisor/ports/clock reset; 151 speeds pass 453000 exact-rational updates and unchanged 100% timing; real-time line clock exact at 151 speeds and 5 line rates; PAL/NTSC beam and crystal timing pass");
    return 0;
}
