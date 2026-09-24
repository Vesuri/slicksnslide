/* Execute original e204 and its real direction/division helpers. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/race_runtime.c"
#include "../src/ui/service_options.h"

/* Link guard for the countdown-only step test. Reaching the target-only
 * particle routine is a test failure, never an emulated successful call. */
unsigned short slicks_advance_particles(struct SlicksTrailParticle *particles,
    unsigned long count, unsigned char indices[SLICKS_TRAIL_PRIORITY_COUNT][SLICKS_TRAIL_PARTICLE_MAX],
    unsigned short counts[SLICKS_TRAIL_PRIORITY_COUNT],
    struct SlicksDirtyPixel *dirty_pixels, unsigned short *dirty_count,
    unsigned char *chunky, unsigned long actor_page)
{
    (void)particles; (void)count; (void)indices; (void)counts;
    (void)dirty_pixels; (void)dirty_count; (void)chunky; (void)actor_page;
    fputs("Countdown test unexpectedly entered particle gameplay\n",stderr);
    abort();
}

static void check(uc_err error)
{
    if(error) { fprintf(stderr,"%s\n",uc_strerror(error)); exit(1); }
}
static void word(uc_engine *uc,unsigned at,unsigned value)
{
    unsigned char b[2]={value,value>>8}; check(uc_mem_write(uc,at,b,2));
}
static void dword(uc_engine *uc,unsigned at,long value)
{
    word(uc,at,(uint32_t)value); word(uc,at+2,(uint32_t)value>>16);
}
static unsigned readword(uc_engine *uc,unsigned at)
{
    unsigned char b[2]; check(uc_mem_read(uc,at,b,2));
    return b[0]|(unsigned)b[1]<<8;
}
static void byte(uc_engine *uc,unsigned at,unsigned value)
{
    unsigned char b=value; check(uc_mem_write(uc,at,&b,1));
}
static int readbyte(uc_engine *uc,unsigned at)
{
    signed char b; check(uc_mem_read(uc,at,&b,1)); return b;
}
static void run_slice_data(uc_engine *uc,unsigned start,unsigned end,
                      unsigned car,unsigned ticks,uint16_t ds)
{
    uint16_t cs=0x1987,ss=0x8000,bp=0xf000,sp=0xef00,ip;
    word(uc,0x8f006,car); word(uc,0x8f008,ticks);
    check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
    check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
    check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
    check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
    check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
    check(uc_emu_start(uc,start,end,0,5000));
    check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
    check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
    if ((unsigned)cs*16+ip!=end || sp!=0xef00) {
        fprintf(stderr,"AI slice failed to reach %x: ip=%x sp=%x\n",end,ip,sp);
        exit(1);
    }
}
static void run_slice(uc_engine *uc,unsigned start,unsigned end,
                      unsigned car,unsigned ticks)
{
    run_slice_data(uc,start,end,car,ticks,0x6000);
}
static void forbidden_weapon_probe(uc_engine *uc,uint64_t address,uint32_t size,void *user)
{
    (void)uc; (void)address; (void)size; (void)user;
    fputs("Weapons-disabled AI called the weapon predictor\n",stderr); exit(1);
}

static void verify_disabled_weapon_gate(uc_engine *uc)
{
    static const unsigned counters[]={0,1,10,11,12,255};
    unsigned cases=0;
    uc_hook hook;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    check(uc_hook_add(uc,&hook,UC_HOOK_CODE,forbidden_weapon_probe,0,0x1ebbb,0x1ebbb));
    word(uc,0x63020,0);
    for(unsigned index=0;index<4;++index)
    for(unsigned service=0;service<256;++service)
    for(unsigned counter=0;counter<6;++counter) {
        byte(uc,0x66936+index,service); byte(uc,0x668e5+index,counters[counter]);
        byte(uc,0x62fb0+index,0xff);
        run_slice(uc,0x1f0a3,0x1f0d8,index,1);
        if(readbyte(uc,0x62fb0+index) ||
           (unsigned char)readbyte(uc,0x668e5+index)!=counters[counter]) {
            fputs("Disabled weapon request/counter mismatch\n",stderr); exit(1);
        }
        ++cases;
    }
    check(uc_hook_del(uc,hook));
    static unsigned char saved_data[65536],zero_data[65536];
    check(uc_mem_read(uc,0x60000,saved_data,sizeof saved_data));
    /* Isolate the common AI tail with a state outside 0/1/2: all actual
     * steering branches are covered separately. Crowding must not turn a
     * retained brake into acceleration after repeated updates. */
    for(unsigned index=0;index<4;++index)
    for(unsigned heading=0;heading<16;++heading) {
        static struct SlicksRaceRuntime race;
        memset(&race,0,sizeof race);
        for(unsigned other=0;other<4;++other)
            race.cars[other].x=race.cars[other].y=10300;
        struct SlicksRaceCar *car=&race.cars[index];
        car->ai_last_x=car->ai_last_y=10300; car->ai_stuck_ticks=700;
        car->ai_state=3; car->heading=heading*1200;
        car->ai_control_latch=SLICKS_CONTROL_BRAKE;
        check(uc_mem_write(uc,0x60000,zero_data,sizeof zero_data));
        byte(uc,0x6692a+index,3); word(uc,0x668fa+index*2,700);
        word(uc,0x653b6+index*2,100); word(uc,0x653be + index*2,100);
        word(uc,0x668ea+index*2,100); word(uc,0x668f2+index*2,100);
        for(unsigned bit=0;bit<4;++bit)
            byte(uc,0x65344+index*5+bit,(SLICKS_CONTROL_BRAKE>>bit)&1);
        for(unsigned tick=0;tick<32;++tick) {
            uint16_t cs=0x1987,ds=0x6000,ss=0x8000,sp=0xf000,ip;
            word(uc,0x8f000,0); word(uc,0x8f002,0x7000);
            word(uc,0x8f004,index); word(uc,0x8f006,1);
            check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
            check(uc_reg_write(uc,UC_X86_REG_SS,&ss)); check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
            check(uc_emu_start(uc,0x1f09d,0x70000,0,5000));
            check(uc_reg_read(uc,UC_X86_REG_CS,&cs)); check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
            check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
            unsigned expected=0;
            for(unsigned bit=0;bit<4;++bit)
                expected|=!!readbyte(uc,0x65344+index*5+bit)<<bit;
            if(cs*16U+ip!=0x70000 || sp!=0xf004 ||
               ai_controls(&race,index,1)!=expected || expected!=SLICKS_CONTROL_BRAKE ||
               car->ai_stuck_ticks!=(short)readword(uc,0x668fa+index*2)) {
                fputs("Weapons-disabled crowded AI differs from original tail\n",stderr); exit(1);
            }
        }
    }
    check(uc_mem_write(uc,0x60000,saved_data,sizeof saved_data));
    /* Later slice tests stop inside blocks translated by the full call. */
    check(uc_ctl_remove_cache(uc,0,0x100000));
    printf("DOS weapons-disabled AI: %u caller-gate cases and 2048 whole-call brake-tail comparisons match\n",cases);
}

static void verify_recovery(uc_engine *uc)
{
    static const short timers[]={-32768,-1,0,1,2,40,50,150,700,32767};
    static const unsigned ticks[]={0,1,2,3,127,128,255};
    static const signed char modes[]={-128,-1,0,1,2,127};
    unsigned watchdog_cases=0,escape_cases=0;
    for(unsigned index=0;index<4;++index)
    for(unsigned move=0;move<4;++move)
    for(unsigned mode=0;mode<6;++mode)
    for(unsigned timer=0;timer<10;++timer)
    for(unsigned tick=0;tick<7;++tick) {
        struct SlicksRaceCar car={0};
        car.ai_last_x=10300; car.ai_last_y=20300;
        car.x=10300+(move==1?99:move==2?100:0);
        car.y=20300+(move==3?-100:0);
        car.ai_stuck_ticks=timers[timer];
        car.ai_service_state=modes[mode];
        car.ai_state=1; car.ai_recovery_ticks=17;
        word(uc,0x653b6+index*2,car.x/100-3);
        word(uc,0x653be + index*2,car.y/100-3);
        word(uc,0x668ea+index*2,car.ai_last_x/100-3);
        word(uc,0x668f2+index*2,car.ai_last_y/100-3);
        word(uc,0x668fa+index*2,car.ai_stuck_ticks);
        word(uc,0x6691a+index*2,car.ai_recovery_ticks);
        byte(uc,0x6692a+index,car.ai_state);
        byte(uc,0x66936+index,car.ai_service_state);
        run_slice(uc,0x1f0d8,0x1f162,index,ticks[tick]);
        ai_watchdog(&car,ticks[tick]);
        if ((short)readword(uc,0x668fa+index*2)!=car.ai_stuck_ticks ||
            (short)readword(uc,0x6691a+index*2)!=car.ai_recovery_ticks ||
            readbyte(uc,0x6692a+index)!=car.ai_state ||
            readbyte(uc,0x66936+index)!=car.ai_service_state ||
            (short)readword(uc,0x668ea+index*2)!=car.ai_last_x/100-3 ||
            (short)readword(uc,0x668f2+index*2)!=car.ai_last_y/100-3) {
            fprintf(stderr,"watchdog mismatch car=%u move=%u mode=%d timer=%d ticks=%u\n",
                index,move,modes[mode],timers[timer],ticks[tick]); exit(1);
        }
        ++watchdog_cases;
    }
    for(unsigned index=0;index<4;++index)
    for(unsigned direction=0;direction<6;++direction)
    for(unsigned timer=0;timer<10;++timer)
    for(unsigned turn=0;turn<10;++turn)
    for(unsigned tick=0;tick<7;++tick) {
        static struct SlicksRaceRuntime race;
        unsigned char controls[5]={1,1,1,1,1};
        memset(&race,0,sizeof race);
        struct SlicksRaceCar *car=&race.cars[index];
        race.random_state=0x1fadec20u+escape_cases;
        car->ai_state=2;
        car->ai_recovery_ticks=timers[timer];
        car->ai_turn_ticks=timers[turn];
        car->ai_recovery_right=modes[direction];
        dword(uc,0x62aaa,race.random_state);
        word(uc,0x6691a+index*2,car->ai_recovery_ticks);
        word(uc,0x66922+index*2,car->ai_turn_ticks);
        byte(uc,0x6692a+index,car->ai_state);
        byte(uc,0x6692e + index,car->ai_recovery_right);
        check(uc_mem_write(uc,0x65344+index*5,controls,5));
        run_slice(uc,0x1f706,0x1f7dd,index,ticks[tick]);
        unsigned actual=ai_escape_controls(&race,car,ticks[tick]);
        check(uc_mem_read(uc,0x65344+index*5,controls,5));
        unsigned expected=controls[0]|controls[1]<<1|controls[2]<<2|controls[3]<<3;
        unsigned seed=readword(uc,0x62aaa)|readword(uc,0x62aac)<<16;
        if(actual!=expected || seed!=race.random_state || controls[4]!=1 ||
            (short)readword(uc,0x6691a+index*2)!=car->ai_recovery_ticks ||
            (short)readword(uc,0x66922+index*2)!=car->ai_turn_ticks ||
            readbyte(uc,0x6692a+index)!=car->ai_state ||
            readbyte(uc,0x6692e + index)!=car->ai_recovery_right) {
            fprintf(stderr,"escape mismatch car=%u direction=%d timer=%d turn=%d ticks=%u mask=%u/%u seed=%x/%lx\n",
                index,modes[direction],timers[timer],timers[turn],ticks[tick],actual,expected,seed,race.random_state);
            exit(1);
        }
        ++escape_cases;
    }
    printf("DOS AI recovery: %u watchdog and %u escape instruction-slice cases match\n",
           watchdog_cases,escape_cases);
}
static void stop_at_zone_entry(uc_engine *uc,uint64_t address,
                               uint32_t size,void *opaque)
{
    (void)address; (void)size; (void)opaque;
    check(uc_emu_stop(uc));
}
static void verify_zone_bounds(uc_engine *uc)
{
    static const short xs[]={-4,-3,-1,0,2,3,9,10,11,19,20,21,319,32767};
    static const short ys[]={-4,-3,-1,0,2,3,29,30,31,39,40,41,189,32767};
    unsigned cases=0;
    uc_hook hook;
    check(uc_hook_add(uc,&hook,UC_HOOK_CODE,stop_at_zone_entry,0,0x1f277,0x1f277));
    for(unsigned index=0;index<4;++index)
    for(unsigned zone_index=0;zone_index<3;++zone_index)
    for(unsigned xi=0;xi<sizeof xs/sizeof *xs;++xi)
    for(unsigned yi=0;yi<sizeof ys/sizeof *ys;++yi) {
        struct SlicksRaceCar car={0};
        struct SlicksTrackZone zone={{10,20,0},{30,40,0},0};
        uint16_t cs=0x1987,ds=0x6000,ss=0x8000,bp=0xf000,sp=0xef00,ip;
        /* Include zero-edge regions and inverted (empty) bounds. */
        if(zone_index==1) { zone.x[0]=0; zone.y[0]=0; }
        if(zone_index==2) { zone.x[0]=21; zone.y[0]=41; }
        car.x=(long)xs[xi]*100+99; car.y=(long)ys[yi]*100+99;
        word(uc,0x8f006,index);
        word(uc,0x653b6+index*2,car.x/100-3);
        word(uc,0x653be + index*2,car.y/100-3);
        word(uc,0x66902+index*2,zone_index);
        word(uc,0x65428+zone_index*2,zone.x[0]);
        word(uc,0x658d8+zone_index*2,zone.x[1]);
        word(uc,0x65680+zone_index*2,zone.y[0]);
        word(uc,0x65b30+zone_index*2,zone.y[1]);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
        check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
        check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x1f1fb,0x1f3bc,0,5000));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        unsigned end=cs*16u+ip;
        if((end!=0x1f277 && end!=0x1f3bc) ||
           ai_inside_zone(&car,&zone)!=(end==0x1f277)) {
            fprintf(stderr,"zone mismatch car=%u zone=%u x=%ld y=%ld end=%x\n",
                index,zone_index,car.x,car.y,end); exit(1);
        }
        ++cases;
    }
    check(uc_hook_del(uc,hook));
    printf("DOS AI zone bounds: %u instruction-slice cases match\n",cases);
}
static void verify_lap_flow(void)
{
    static struct SlicksRaceRuntime race;
    struct SlicksRaceCar *car=&race.cars[0];
    race.laps_to_run=4;
    race.navigation.zone_count=1;
    race.navigation.zones[0]=(struct SlicksTrackZone){{0,319,0},{0,189,0},0};
    race.navigation.checkpoint_count=2;
    race.navigation.checkpoints[0]=(struct SlicksTrackCheckpoint){{10,20},{10,20}};
    race.navigation.checkpoints[1]=(struct SlicksTrackCheckpoint){{30,40},{30,40}};
    car->x=car->y=5000; car->lap=1;
    advance_waypoint(&race,car);
    if(car->lap!=1 || car->waypoint!=0 || car->checkpoint) goto fail;
    car->selected_surface=17;
    advance_lap_checkpoints(&race,car); /* finish line before checkpoints */
    if(car->lap!=1 || car->checkpoint) goto fail;
    car->selected_surface=0;
    car->x=car->y=899; /* centre rounds to 8; +1 is still outside */
    advance_lap_checkpoints(&race,car);
    if(car->checkpoint) goto fail;
    car->x=car->y=900; /* exact first inclusive edge */
    advance_lap_checkpoints(&race,car);
    if(car->checkpoint!=1 || car->lap!=1) goto fail;
    car->x=car->y=3900; /* exact second inclusive edge */
    advance_lap_checkpoints(&race,car);
    if(car->checkpoint!=2 || car->lap!=1) goto fail;
    car->x=car->y=5000;
    car->current_lap_centiseconds=1234;
    car->current_lap_time_units=2222;
    car->best_lap_time_units=30000;
    car->selected_surface=17;
    advance_lap_checkpoints(&race,car);
    if(car->checkpoint || car->lap!=2 || car->last_lap_centiseconds!=1234 ||
       car->best_lap_centiseconds!=1234 || car->current_lap_centiseconds) goto fail;
    advance_lap_checkpoints(&race,car); /* staying on line cannot repeat lap */
    if(car->lap!=2) goto fail;
    car->checkpoint=2; car->selected_surface=0;
    race.material_map[50*320+50]=17; /* raw mode zero also qualifies */
    advance_lap_checkpoints(&race,car);
    if(car->lap!=3 || car->checkpoint) goto fail;
    puts("Native lap flow: route wrap, ordered checkpoints, finish-line gate and repeat suppression passed");
    return;
fail:
    fprintf(stderr,"lap flow mismatch lap=%u checkpoint=%u\n",car->lap,car->checkpoint);
    exit(1);
}
static void verify_lap_gate(uc_engine *uc)
{
    static const unsigned surfaces[]={0,16,17,19};
    unsigned cases=0;
    uc_hook hook;
    check(uc_hook_add(uc,&hook,UC_HOOK_CODE,stop_at_zone_entry,0,0x22a94,0x22a94));
    for(unsigned index=0;index<4;++index)
    for(unsigned done=0;done<4;++done)
    for(unsigned selected=0;selected<4;++selected)
    for(unsigned raw=0;raw<4;++raw) {
        static struct SlicksRaceRuntime race;
        memset(&race,0,sizeof race);
        struct SlicksRaceCar *car=&race.cars[index];
        uint16_t cs=0x1987,ds=0x6000,ss=0x8000,bp=0xf000,sp=0xef00,ip;
        race.navigation.checkpoint_count=2;
        race.laps_to_run=4;
        car->x=car->y=5000; car->lap=1; car->checkpoint=done;
        car->selected_surface=surfaces[selected];
        race.material_map[50*320+50]=surfaces[raw];
        word(uc,0x8ef98,index);
        byte(uc,0x6537c+index,surfaces[selected]);
        byte(uc,0x65384+index,surfaces[raw]);
        word(uc,0x6305c+index*54,done); word(uc,0x66364,2);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
        check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
        check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x22a61,0x22d23,0,5000));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        unsigned end=cs*16u+ip;
        advance_lap_checkpoints(&race,car);
        if((end!=0x22a94 && end!=0x22d23) ||
           (car->lap==2)!=(end==0x22a94) ||
           car->checkpoint!=readword(uc,0x6305c+index*54)) {
            fprintf(stderr,"lap gate mismatch driver=%u done=%u selected=%u raw=%u end=%x\n",
                index,done,surfaces[selected],surfaces[raw],end); exit(1);
        }
        ++cases;
    }
    check(uc_hook_del(uc,hook));
    printf("DOS finish-line gate: %u instruction-slice cases match\n",cases);
}
static void verify_checkpoints(uc_engine *uc)
{
    static const short coordinates[]={-4,-1,0,8,9,10,19,20,21,29,30,39,40,32767};
    unsigned cases=0;
    uc_hook hook;
    check(uc_hook_add(uc,&hook,UC_HOOK_CODE,stop_at_zone_entry,0,0x22844,0x22844));
    for(unsigned index=0;index<4;++index)
    for(unsigned checkpoint=0;checkpoint<3;++checkpoint)
    for(unsigned xi=0;xi<14;++xi)
    for(unsigned yi=0;yi<14;++yi) {
        static struct SlicksRaceRuntime race;
        memset(&race,0,sizeof race);
        struct SlicksRaceCar *car=&race.cars[index];
        uint16_t cs=0x1987,ds=0x6000,ss=0x8000,bp=0xf000,sp=0xef00,ip;
        race.navigation.checkpoint_count=3;
        struct SlicksTrackCheckpoint *point=&race.navigation.checkpoints[checkpoint];
        point->x[0]=checkpoint?10:0; point->x[1]=20;
        point->y[0]=checkpoint==2?41:0; point->y[1]=40;
        car->x=(long)coordinates[xi]*100+99;
        car->y=(long)coordinates[yi]*100+99;
        car->checkpoint=checkpoint;
        word(uc,0x8ef98,index);
        word(uc,0x653b6+index*2,car->x/100-3);
        word(uc,0x653be + index*2,car->y/100-3);
        word(uc,0x6305c+index*54,checkpoint);
        word(uc,0x66366+checkpoint*2,point->x[0]);
        word(uc,0x6642e + checkpoint*2,point->x[1]);
        word(uc,0x664f6+checkpoint*2,point->y[0]);
        word(uc,0x665be + checkpoint*2,point->y[1]);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
        check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
        check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x227b6,0x22981,0,5000));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        unsigned end=cs*16u+ip;
        advance_checkpoint(&race,car);
        if((end!=0x22844 && end!=0x22981) ||
           car->checkpoint!=readword(uc,0x6305c+index*54)) {
            fprintf(stderr,"checkpoint mismatch driver=%u checkpoint=%u x=%ld y=%ld end=%x\n",
                index,checkpoint,car->x,car->y,end); exit(1);
        }
        ++cases;
    }
    check(uc_hook_del(uc,hook));
    printf("DOS checkpoint advancement: %u instruction-slice cases match\n",cases);
}
static void verify_clock(uc_engine *uc)
{
    static const unsigned times[]={0,1,15000,30000,65535,65536,0x80000000u,0xffffffffu};
    for(unsigned now=0;now<8;++now)
    for(unsigned previous=0;previous<8;++previous)
    for(unsigned best=0;best<8;++best) {
        struct SlicksRaceCar car={0};
        uint16_t cs=0x1987,ds=0x6000,ss=0x8000,bp=0xf000,sp=0xef00,ip;
        car.current_lap_time_units=(times[now]-times[previous])*2u;
        car.best_lap_time_units=times[best]; car.lap=3;
        word(uc,0x8ef98,0); dword(uc,0x6685e,times[now]);
        dword(uc,0x6303b,times[previous]); dword(uc,0x64c06,times[best]);
        word(uc,0x64bfe,2);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
        check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
        check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x22a94,0x22b17,0,5000));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        record_lap_clock(&car);
        unsigned last=readword(uc,0x63037)|readword(uc,0x63039)<<16;
        unsigned fastest=readword(uc,0x64c06)|readword(uc,0x64c08)<<16;
        if(cs*16u+ip!=0x22b17 || car.last_lap_time_units!=last ||
           car.best_lap_time_units!=fastest || car.lap!=readword(uc,0x64bfe)+1 ||
           car.current_lap_time_units) {
            fputs("Lap clock arithmetic mismatch\n",stderr); exit(1);
        }
    }
    puts("DOS lap clock: 512 timestamp subtraction, wrap and signed best-time cases match");
    for(unsigned raw=0;raw<65536;++raw) {
        uint16_t cs=0x266c,ds=0x6000,ss=0x8000,sp=0xf000,bp,cx,ip;
        word(uc,0x8f008,raw);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
        check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x2aceb,0x2ad29,0,5000));
        check(uc_reg_read(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_read(uc,UC_X86_REG_CX,&cx));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        unsigned expected=readword(uc,0x80000+bp-2)*100+cx;
        if(cs*16u+ip!=0x2ad29 || display_time_centiseconds(raw)!=expected ||
           display_time_centiseconds(raw+0x10000u)!=expected) {
            fprintf(stderr,"clock formatter mismatch raw=%u native=%u DOS=%u\n",
                    raw,display_time_centiseconds(raw),expected); exit(1);
        }
    }
    struct SlicksRaceCar car={0};
    unsigned total=0;
    for(unsigned frame=0;frame<100000;++frame) {
        unsigned ticks=frame%3+1;
        total+=ticks*2;
        advance_car_clock(&car,ticks);
        if(car.elapsed_time_units!=total || car.current_lap_time_units!=total ||
           car.elapsed_centiseconds!=display_time_centiseconds(total)) exit(1);
    }
    puts("DOS clock: all 65536 formatter inputs match; 100000 variable-tick updates retain raw time");
    {
        static struct SlicksRaceRuntime race;
        static unsigned char logical[0x40000];
        race.started=1;
        race.countdown_ticks=120;
        for(unsigned frame=0;frame<20;++frame)
            slicks_race_step(&race,logical);
        unsigned expected_ticks=(20u*1193182u)/655350u;
        for(unsigned driver=0;driver<4;++driver)
            if(race.cars[driver].elapsed_time_units!=expected_ticks*2 ||
               race.cars[driver].current_lap_time_units!=expected_ticks*2 ||
               race.cars[driver].x || race.cars[driver].y) exit(1);
        if(race.racing || race.countdown_stage || race.frame_count!=20) exit(1);
        puts("Race clock epoch: all four stationary cars accumulate the first 20 countdown updates");
    }
}
static void verify_zone_expansion(uc_engine *uc)
{
    static const unsigned short xs[]={0,1,2,3,319,32767,32768,65534,65535};
    static const unsigned short ys[]={0,1,2,3,254,255};
    unsigned cases=0;
    for(unsigned a=0;a<9;++a)
    for(unsigned b=0;b<9;++b)
    for(unsigned c=0;c<6;++c)
    for(unsigned d=0;d<6;++d) {
        struct SlicksTrackZone zone={{xs[a],xs[b],123},{ys[c],ys[d],99},7};
        word(uc,0x8effe,0);
        word(uc,0x65428,zone.x[0]); word(uc,0x658d8,zone.x[1]);
        word(uc,0x65680,zone.y[0]); word(uc,0x65b30,zone.y[1]);
        run_slice(uc,0x1bb56,0x1bb96,0,0);
        slicks_expand_track_zone(&zone);
        if(zone.x[0]!=readword(uc,0x65428) || zone.x[1]!=readword(uc,0x658d8) ||
           zone.y[0]!=readword(uc,0x65680) || zone.y[1]!=readword(uc,0x65b30) ||
           zone.x[2]!=123 || zone.y[2]!=99 || zone.speed!=7) {
            fputs("Route loader expansion mismatch\n",stderr); exit(1);
        }
        ++cases;
    }
    printf("DOS route loader: %u expanded rectangle cases match\n",cases);
}
static void verify_alternate_selection(uc_engine *uc)
{
    static const long positions[]={-10001,-1,0,9900,10300,30000,3276700};
    unsigned cases=0;
    for(unsigned index=0;index<4;++index)
    for(unsigned count=0;count<5;++count)
    for(unsigned x=0;x<7;++x)
    for(unsigned y=0;y<7;++y)
    for(unsigned pattern=0;pattern<3;++pattern) {
        static struct SlicksRaceRuntime race;
        memset(&race,0,sizeof race);
        struct SlicksRaceCar *car=&race.cars[index];
        car->x=positions[x]; car->y=positions[y];
        car->ai_state=1; car->ai_target_x=-1; car->ai_target_y=13;
        car->ai_recovery_ticks=17;
        race.navigation.alternate_count=count;
        for(unsigned i=0;i<5;++i) {
            struct SlicksTrackPoint *point=&race.navigation.alternate[i];
            point->x=pattern==0?(short)(i*50):pattern==1?104:(short)(32767-i);
            point->y=pattern==0?(short)(i*30):pattern==1?104:(short)(-32768+i);
            word(uc,0x66688+i*2,point->x); word(uc,0x66750+i*2,point->y);
        }
        word(uc,0x66686,count);
        dword(uc,0x6538c+index*4,car->x); dword(uc,0x6539c+index*4,car->y);
        word(uc,0x6690a+index*2,-1); word(uc,0x66912+index*2,13);
        word(uc,0x6691a+index*2,17); byte(uc,0x6692a+index,1);
        run_slice(uc,0x1f3bc,0x1f4d3,index,1);
        select_ai_alternate(&race,car);
        if(car->ai_target_x!=(short)readword(uc,0x6690a+index*2) ||
           car->ai_target_y!=(short)readword(uc,0x66912+index*2) ||
           car->ai_recovery_ticks!=(short)readword(uc,0x6691a+index*2) ||
           car->ai_state!=readbyte(uc,0x6692a+index)) {
            fprintf(stderr,"Alternate selection mismatch car=%u count=%u pattern=%u x=%ld y=%ld\n",
                    index,count,pattern,car->x,car->y); exit(1);
        }
        ++cases;
    }
    printf("DOS alternate selection: %u instruction-slice cases match\n",cases);
}
static void verify_curve6_alternates(void)
{
    static unsigned char dat[65536],track[8192],arena[65536];
    static unsigned char logical[0x40000],material[60800],surface[60800];
    struct SlicksTrackNavigation navigation;
    FILE *file=fopen("ref/SLICKS.DAT","rb");
    if(!file) exit(2);
    size_t dat_size=fread(dat,1,sizeof dat,file);
    if(ferror(file) || fgetc(file)!=EOF) exit(2);
    fclose(file);
    file=fopen("ref/TRACKS/CURVE6.SS","rb");
    if(!file) exit(2);
    size_t track_size=fread(track,1,sizeof track,file);
    if(ferror(file) || fgetc(file)!=EOF) exit(2);
    fclose(file);
    if(slicks_build_track_scene(logical,material,surface,dat,dat_size,
            track,track_size,arena,sizeof arena,&navigation)!=468 ||
       navigation.alternate_count!=101 || navigation.alternate[100].x!=60 ||
       navigation.alternate[100].y!=80 || navigation.alternate[0].x!=0 ||
       navigation.alternate[0].y!=60) {
        fputs("CURVE6 alternate-array boundary regression\n",stderr); exit(1);
    }
    puts("CURVE6: all 100 alternate records and DOS fallback-slot alias decoded safely");
    if(!navigation.service_available || navigation.pit_count!=3 ||
       navigation.pits[0].x!=115 || navigation.pits[0].y!=33 ||
       navigation.pits[1].x!=85 || navigation.pits[1].y!=33 ||
       navigation.pits[2].x!=103 || navigation.pits[2].y!=33) {
        fputs("CURVE6 pit records mismatch\n",stderr); exit(1);
    }
    file=fopen("ref/TRACKS/BASIC.SS","rb");
    if(!file) exit(2);
    track_size=fread(track,1,sizeof track,file);
    if(ferror(file) || fgetc(file)!=EOF) exit(2);
    fclose(file);
    if(slicks_build_track_scene(logical,material,surface,dat,dat_size,
            track,track_size,arena,sizeof arena,&navigation)!=233 ||
       !navigation.service_available || navigation.pit_count!=2 ||
       navigation.pits[0].x!=114 || navigation.pits[0].y!=121 ||
       navigation.pits[1].x!=214 || navigation.pits[1].y!=64) {
        fputs("BASIC pit records/reload mismatch\n",stderr); exit(1);
    }
    puts("Pit assets: CURVE6 and BASIC destinations, order and reload count verified");
    if(navigation.actor_count!=2 || navigation.actors[0].kind!=0 ||
       navigation.actors[0].x!=48 || navigation.actors[0].y!=32 ||
       navigation.actors[1].kind!=2 || navigation.actors[1].x!=3440 ||
       navigation.actors[1].y!=992 || navigation.actors[0].layer!=1 ||
       navigation.actors[1].layer!=1) {
        fputs("BASIC actor asset coordinates/order mismatch\n",stderr); exit(1);
    }
    puts("BASIC: two track actor records retained in source order with Q4 positions");
    {
        static unsigned char fuel_off[0x40000], damage_on[0x40000];
        memset(logical,0,sizeof logical);
        if(slicks_build_track_scene_options(logical,material,surface,dat,dat_size,
                track,track_size,arena,sizeof arena,&navigation,0,0)!=233 ||
           navigation.service_available || navigation.pit_count!=2) exit(1);
        memcpy(fuel_off,logical,sizeof logical);
        memset(logical,0,sizeof logical);
        if(slicks_build_track_scene_options(logical,material,surface,dat,dat_size,
                track,track_size,arena,sizeof arena,&navigation,0,1)!=233 ||
           !navigation.service_available || navigation.pit_count!=2) exit(1);
        memcpy(damage_on,logical,sizeof logical);
        if(!memcmp(fuel_off,damage_on,sizeof logical)) {
            fputs("Pit options do not affect visible scenery\n",stderr); exit(1);
        }
        memset(logical,0,sizeof logical);
        if(slicks_build_track_scene_options(logical,material,surface,dat,dat_size,
                track,track_size,arena,sizeof arena,&navigation,10,0)!=233 ||
           memcmp(logical,damage_on,sizeof logical)) exit(1);
        /* Independent placement test: replacing only the two pit objects
         * with non-drawing control objects must equal the disabled setup. */
        unsigned at=6+0x165+15, replaced=0;
        for(unsigned i=0;i<2;++i) { while(track[at]) ++at; ++at; }
        at+=8;
        unsigned count=((unsigned)track[at]<<8)|track[at+1]; at+=2;
        for(unsigned i=0;i<count;++i,at+=5)
            if(track[at+3]==68 || track[at+3]==69) {
                track[at+3]=79; ++replaced;
            }
        memset(logical,0,sizeof logical);
        if(replaced!=2 || slicks_build_track_scene_options(logical,material,surface,
                dat,dat_size,track,track_size,arena,sizeof arena,&navigation,10,0)!=233 ||
           memcmp(logical,fuel_off,sizeof logical) || navigation.service_available ||
           navigation.pit_count) exit(1);
        puts("Pit visuals: disabled equals omitted objects; fuel/damage enable identical real graphics");
    }
}
static void verify_contact_transitions(uc_engine *uc)
{
    static const unsigned ages[]={0,49,50,349,350,32000,60000,65535};
    static const short timers[]={-32768,-1,0,1};
    unsigned cases=0;
    for(unsigned index=0;index<4;++index)
    for(unsigned state=0;state<3;++state)
    for(unsigned timer=0;timer<4;++timer)
    for(unsigned age=0;age<8;++age)
    for(unsigned threshold=0;threshold<8;++threshold)
    for(unsigned seen=0;seen<2;++seen) {
        struct SlicksRaceCar car={0}; uint16_t cx=index*2;
        car.ai_state=state; car.ai_recovery_ticks=timers[timer];
        car.ai_contact_ticks=ages[age]; car.ai_contact_threshold=ages[threshold];
        car.ai_route_seen=seen;
        word(uc,0x8effc,index);
        check(uc_reg_write(uc,UC_X86_REG_CX,&cx));
        byte(uc,0x6692a+index,state); byte(uc,0x66932+index,seen);
        word(uc,0x6691a+index*2,car.ai_recovery_ticks);
        word(uc,0x653c6+index*2,car.ai_contact_ticks);
        word(uc,0x62fa4+index*2,car.ai_contact_threshold);
        run_slice(uc,0x1f162,0x1f1de,index,1);
        ai_contact_transition(&car);
        if(car.ai_state!=readbyte(uc,0x6692a+index) ||
           car.ai_recovery_ticks!=(short)readword(uc,0x6691a+index*2) ||
           car.ai_contact_ticks!=readword(uc,0x653c6+index*2)) {
            fputs("AI contact transition mismatch\n",stderr); exit(1);
        }
        ++cases;
    }
    printf("DOS contact transitions: %u instruction-slice cases match\n",cases);
    cases=0;
    for(unsigned age=0;age<8;++age)
    for(unsigned ticks=0;ticks<256;ticks+=17)
    for(unsigned touching=0;touching<2;++touching)
    for(unsigned last_wall=0;last_wall<2;++last_wall)
    for(unsigned suppressed=0;suppressed<2;++suppressed) {
        struct SlicksRaceCar car={0};
        car.ai_contact_ticks=ages[age]; car.actor_contact=touching;
        car.touching_solid=last_wall;
        car.touching_car=suppressed;
        word(uc,0x8ef98,0); word(uc,0x8effe,ticks);
        byte(uc,0x64bc6,1); byte(uc,0x6536c,touching);
        byte(uc,0x64dae,suppressed); word(uc,0x653c6,car.ai_contact_ticks);
        run_slice(uc,0x23d3e,0x23d7b,0,ticks);
        ai_update_contact_age(&car,ticks);
        if(car.ai_contact_ticks!=readword(uc,0x653c6)) {
            fputs("AI contact age mismatch\n",stderr); exit(1);
        }
        ++cases;
    }
    printf("DOS contact age: %u instruction-slice cases match\n",cases);
}
static void verify_alternate_progress(uc_engine *uc)
{
    /* Discard previously translated slices before reusing this boundary;
     * otherwise Unicorn can continue through a stale translated block. */
    uc_hook hook;
    check(uc_hook_add(uc,&hook,UC_HOOK_CODE,stop_at_zone_entry,0,0x1f706,0x1f706));
    check(uc_ctl_remove_cache(uc,0,0x100000));
    static const short targets[]={0,100,32767};
    static const short offsets[]={-5,-4,-1,0,1,4,5};
    static const long speeds[]={-1,0,6999,7000,7001,2147483647L};
    static const unsigned ticks[]={0,1,2,127,128,255};
    static const short timers[]={-32768,-1,0,32767};
    unsigned cases=0;
    for(unsigned index=0;index<4;++index)
    for(unsigned target=0;target<3;++target)
    for(unsigned x=0;x<7;++x)
    for(unsigned y=0;y<7;++y)
    for(unsigned speed=0;speed<6;++speed)
    for(unsigned tick=0;tick<6;++tick)
    for(unsigned timer=0;timer<4;++timer) {
        struct SlicksRaceCar car={0};
        car.x=((long)targets[target]+offsets[x]+3)*100;
        car.y=((long)targets[target]+offsets[y]+3)*100;
        car.ai_target_x=car.ai_target_y=targets[target];
        car.ai_state=1; car.ai_route_seen=1; car.ai_recovery_ticks=timers[timer];
        car.measured_speed=speeds[speed];
        word(uc,0x653b6+index*2,car.x/100-3); word(uc,0x653be + index*2,car.y/100-3);
        word(uc,0x6690a+index*2,car.ai_target_x); word(uc,0x66912+index*2,car.ai_target_y);
        word(uc,0x6691a+index*2,car.ai_recovery_ticks);
        byte(uc,0x6692a+index,1); byte(uc,0x66932+index,1);
        dword(uc,0x6684e + index*4,car.measured_speed);
        run_slice(uc,0x1f679,0x1f706,index,ticks[tick]);
        ai_alternate_progress(&car,ticks[tick]);
        if(car.ai_state!=readbyte(uc,0x6692a+index) ||
           car.ai_route_seen!=readbyte(uc,0x66932+index) ||
           car.ai_target_x!=(short)readword(uc,0x6690a+index*2) ||
           car.ai_recovery_ticks!=(short)readword(uc,0x6691a+index*2)) {
            fputs("Alternate progress mismatch\n",stderr); exit(1);
        }
        ++cases;
    }
    check(uc_hook_del(uc,hook));
    printf("DOS alternate progress: %u instruction-slice cases match\n",cases);
}
static void verify_service_entry(uc_engine *uc)
{
    static const signed char states[]={-128,-1,0,1,2,127};
    unsigned cases=0;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    for(unsigned index=0;index<4;++index)
    for(unsigned state=0;state<6;++state)
    for(unsigned count=0;count<4;++count)
    for(unsigned zone=0;zone<5;++zone)
    for(unsigned enabled=0;enabled<2;++enabled) {
        static struct SlicksRaceRuntime race;
        memset(&race,0,sizeof race);
        struct SlicksRaceCar *car=&race.cars[index];
        car->ai_service_state=states[state]; car->waypoint=zone;
        car->ai_contact_threshold=123; car->ai_target_x=-1; car->ai_target_y=456;
        race.damage_enabled=enabled; race.navigation.pit_route_count=count;
        for(unsigned i=0;i<3;++i) {
            race.navigation.pit_route_zone[i]=i/2;
            race.navigation.pit_route_destination[i]=2-i;
            race.navigation.pits[i].x=10+i*20; race.navigation.pits[i].y=50+i*30;
            byte(uc,0x63642+i,i/2); byte(uc,0x63660+i,2-i);
            word(uc,0x6367e + i*2,race.navigation.pits[i].x);
            word(uc,0x63692+i*2,race.navigation.pits[i].y);
        }
        byte(uc,0x63640,count); byte(uc,0x636a6,enabled);
        byte(uc,0x66936+index,car->ai_service_state); byte(uc,0x6692a+index,0);
        word(uc,0x66902+index*2,zone); word(uc,0x62fa4+index*2,123);
        word(uc,0x6690a+index*2,-1); word(uc,0x66912+index*2,456);
        word(uc,0x8effc,index);
        uint16_t bx=index;
        check(uc_reg_write(uc,UC_X86_REG_BX,&bx));
        run_slice(uc,0x1f290,0x1f36b,index,1);
        ai_route_service_entry(&race,car);
        if(car->ai_service_state!=readbyte(uc,0x66936+index) ||
           car->ai_state!=readbyte(uc,0x6692a+index) ||
           car->ai_contact_threshold!=readword(uc,0x62fa4+index*2) ||
           car->ai_target_x!=(short)readword(uc,0x6690a+index*2) ||
           car->ai_target_y!=(short)readword(uc,0x66912+index*2)) {
            fputs("Service entry mismatch\n",stderr); exit(1);
        }
        ++cases;
    }
    printf("DOS service entry: %u original-instruction cases match\n",cases);
}

static void verify_damage_service_request(uc_engine *uc)
{
    static const signed char states[]={-128,-1,0,1,2,127};
    static const short damage[]={-32768,-1,0,499,500,501,32767};
    unsigned cases=0;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    for(unsigned index=0;index<4;++index)
    for(unsigned state=0;state<6;++state)
    for(unsigned value=0;value<7;++value) {
        struct SlicksRaceCar car={0};
        car.ai_service_state=states[state]; car.damage[0]=damage[value];
        word(uc,0x8effc,index); word(uc,0x63024,0);
        byte(uc,0x66936+index,car.ai_service_state);
        word(uc,0x6304f+index*54,car.damage[0]);
        run_slice(uc,0x1f7dd,0x1f849,index,1);
        static struct SlicksRaceRuntime race;
        race.fuel_option=0;
        ai_request_service(&race,&car);
        if(car.ai_service_state!=readbyte(uc,0x66936+index)) {
            fputs("Damage service request mismatch\n",stderr); exit(1);
        }
        ++cases;
    }
    printf("DOS damage service request: %u fuel-disabled cases match\n",cases);
}

static void verify_service_completion(uc_engine *uc)
{
    static const short damage[]={-32768,-1,0,9,10,11,999,32767};
    static const unsigned amounts[]={0,1,89,90,100,0x7fffffffU,0x80000000U,0xffffffffU};
    unsigned cases=0;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    for(unsigned index=0;index<4;++index)
    for(unsigned value=0;value<8;++value)
    for(unsigned state=1;state<=2;++state)
    for(unsigned seen=0;seen<2;++seen)
    for(unsigned fuel_option=0;fuel_option<2;++fuel_option)
    for(unsigned fuel=0;fuel<8;++fuel)
    for(unsigned capacity=0;capacity<8;++capacity) {
        static struct SlicksRaceRuntime race;
        race.fuel_option=fuel_option;
        struct SlicksRaceCar car={0};
        car.fuel=amounts[fuel]; car.fuel_capacity=amounts[capacity];
        car.damage[0]=damage[value]; car.ai_state=1; car.ai_route_seen=seen;
        car.ai_service_state=state; car.ai_target_x=123; car.ai_target_y=456;
        car.ai_recovery_ticks=-1;
        word(uc,0x63024,fuel_option); word(uc,0x6304f+index*54,car.damage[0]);
        dword(uc,0x6305f+index*54,car.fuel); dword(uc,0x63063+index*54,car.fuel_capacity);
        byte(uc,0x6692a+index,1); byte(uc,0x66932+index,seen);
        byte(uc,0x66936+index,state); word(uc,0x6691a+index*2,-1);
        word(uc,0x6690a+index*2,123); word(uc,0x66912+index*2,456);
        run_slice(uc,0x1f5e5,0x1f706,index,1);
        ai_complete_service(&race,&car);
        if(car.ai_state!=readbyte(uc,0x6692a+index) ||
           car.ai_route_seen!=readbyte(uc,0x66932+index) ||
           car.ai_service_state!=readbyte(uc,0x66936+index) ||
           car.ai_recovery_ticks!=(short)readword(uc,0x6691a+index*2) ||
           car.ai_target_x!=(short)readword(uc,0x6690a+index*2) ||
           car.ai_target_y!=(short)readword(uc,0x66912+index*2)) {
            fputs("Service completion mismatch\n",stderr); exit(1);
        }
        ++cases;
    }
    printf("DOS service completion: %u damage/fuel/wrapped-product cases match\n",cases);
}

static void verify_fuel_capacity(uc_engine *uc)
{
    static const short options[]={-32768,-1,0,1,10,100,32767};
    static const unsigned properties[]={0,1,9,10,100,255};
    static const short upgrades[]={-32768,-46,-1,0,1,10,32767};
    unsigned cases=0;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    for(unsigned index=0;index<4;++index)
    for(unsigned option=0;option<7;++option)
    for(unsigned property=0;property<6;++property)
    for(unsigned upgrade=0;upgrade<7;++upgrade) {
        static struct SlicksRaceRuntime race;
        struct SlicksRaceCar car={0};
        race.fuel_option=options[option]; race.properties[0].property_33=properties[property];
        car.fuel_upgrade=upgrades[upgrade];
        word(uc,0x8efd4,index); word(uc,0x63024,race.fuel_option);
        byte(uc,0x64efc+index,properties[property]);
        word(uc,0x3cbf0+0x6a7e + index*26,car.fuel_upgrade);
        run_slice(uc,0x24c49,0x24cce,index,1);
        initialize_car_fuel(&race,&car);
        unsigned capacity=readword(uc,0x63063+index*54)|readword(uc,0x63065+index*54)<<16;
        unsigned fuel=readword(uc,0x6305f+index*54)|readword(uc,0x63061+index*54)<<16;
        if(car.fuel_capacity!=capacity || car.fuel!=fuel) {
            fprintf(stderr,"Fuel capacity mismatch option=%d property=%u upgrade=%d: %u/%u\n",
                    race.fuel_option,properties[property],car.fuel_upgrade,car.fuel_capacity,capacity); exit(1);
        }
        ++cases;
    }
    printf("DOS fuel capacity: %u original setup cases match\n",cases);
}

static void verify_fuel_service_request(uc_engine *uc)
{
    static const unsigned amounts[]={0,1,24,25,0x7fffffffU,0xffffffffU};
    static const unsigned capacities[]={0,100,0x7fffffffU,0xffffffffU};
    static const signed char states[]={-1,0,1};
    static const short damage[]={0,500,501};
    static const unsigned laps[]={1,3,4,5};
    unsigned cases=0;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    for(unsigned index=0;index<4;++index)
    for(unsigned state=0;state<3;++state)
    for(unsigned value=0;value<3;++value)
    for(unsigned option=0;option<2;++option)
    for(unsigned amount=0;amount<6;++amount)
    for(unsigned capacity=0;capacity<4;++capacity)
    for(unsigned lap=0;lap<4;++lap) {
        static struct SlicksRaceRuntime race;
        struct SlicksRaceCar car={0};
        race.fuel_option=option; race.laps_to_run=4;
        car.ai_service_state=states[state]; car.damage[0]=damage[value];
        car.fuel=amounts[amount]; car.fuel_capacity=capacities[capacity]; car.lap=laps[lap];
        word(uc,0x8effc,index); word(uc,0x63024,option);
        word(uc,0x60092,0); word(uc,0x64c18,4); /* Actual 991f, fixed-lap mode. */
        word(uc,0x64bfe + index*2,car.lap-1);
        byte(uc,0x66936+index,car.ai_service_state);
        word(uc,0x6304f+index*54,car.damage[0]);
        dword(uc,0x6305f+index*54,car.fuel); dword(uc,0x63063+index*54,car.fuel_capacity);
        run_slice(uc,0x1f7dd,0x1f849,index,1);
        ai_request_service(&race,&car);
        if(car.ai_service_state!=readbyte(uc,0x66936+index)) {
            fputs("Fuel service request mismatch\n",stderr); exit(1);
        }
        ++cases;
    }
    printf("DOS fuel service request: %u fixed-lap cases match\n",cases);
}

static void verify_fuel_updates(uc_engine *uc)
{
    static const unsigned amounts[]={0,1,100,999,0x7fffffffU,0x80000000U,0xffffffffU};
    static const unsigned ticks[]={0,1,2,127,32767,65535};
    unsigned cases=0;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    for(unsigned index=0;index<4;++index)
    for(unsigned option=0;option<2;++option)
    for(unsigned master=0;master<2;++master)
    for(unsigned amount=0;amount<7;++amount)
    for(unsigned tick=0;tick<6;++tick)
    for(unsigned flags=0;flags<4;++flags) {
        static struct SlicksRaceRuntime race;
        struct SlicksRaceCar car={0}; uint16_t bx=index;
        race.fuel_option=option; race.damage_enabled=master;
        car.fuel=amounts[amount]; car.service_flags=flags;
        word(uc,0x8ef98,index); word(uc,0x8effe,ticks[tick]);
        word(uc,0x63024,option); byte(uc,0x636a6,master);
        dword(uc,0x6305f+index*54,car.fuel); byte(uc,0x6305e + index*54,flags);
        check(uc_reg_write(uc,UC_X86_REG_BX,&bx));
        run_slice(uc,0x202fd,0x2034f,index,ticks[tick]);
        consume_idle_fuel(&race,&car,ticks[tick]);
        unsigned fuel=readword(uc,0x6305f+index*54)|readword(uc,0x63061+index*54)<<16;
        if(car.fuel!=fuel || car.service_flags!=readbyte(uc,0x6305e + index*54)) {
            fputs("Idle fuel/flag mismatch\n",stderr); exit(1);
        }
        ++cases;
    }
    printf("DOS idle fuel: %u consumption/flag cases match\n",cases);
    cases=0;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    for(unsigned index=0;index<4;++index)
    for(unsigned amount=0;amount<7;++amount)
    for(unsigned capacity=0;capacity<7;++capacity)
    for(unsigned tick=0;tick<6;++tick)
    for(unsigned flags=0;flags<4;++flags) {
        struct SlicksRaceCar car={0};
        car.effective_surface=30; car.fuel=amounts[amount];
        car.fuel_capacity=amounts[capacity]; car.service_flags=flags;
        word(uc,0x8ef98,index); word(uc,0x8effe,ticks[tick]);
        dword(uc,0x6305f+index*54,car.fuel); dword(uc,0x63063+index*54,car.fuel_capacity);
        byte(uc,0x6305e + index*54,flags);
        run_slice(uc,0x2327e,0x238eb,index,ticks[tick]);
        refuel_car_at_pit(&car,ticks[tick]);
        unsigned fuel=readword(uc,0x6305f+index*54)|readword(uc,0x63061+index*54)<<16;
        if(car.fuel!=fuel || car.service_flags!=readbyte(uc,0x6305e + index*54)) {
            fputs("Pit refuel/flag mismatch\n",stderr); exit(1);
        }
        ++cases;
    }
    printf("DOS refuelling: %u wrapped-add/clamp/flag cases match\n",cases);
}

static void verify_pit_repair(uc_engine *uc)
{
    static const long speeds[]={-1,0,299,300,301};
    static const short timers[]={0,4,5,9,32767,-32768};
    static const unsigned ticks[]={0,1,2,5,127,65535};
    static const short damage[]={-32768,-1,0,7,8,9,999,32767};
    unsigned cases=0;
    uc_hook repaired_end, idle_end;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    check(uc_hook_add(uc,&repaired_end,UC_HOOK_CODE,stop_at_zone_entry,0,0x23276,0x23276));
    check(uc_hook_add(uc,&idle_end,UC_HOOK_CODE,stop_at_zone_entry,0,0x2327e,0x2327e));
    for(unsigned index=0;index<4;++index)
    for(unsigned speed=0;speed<5;++speed)
    for(unsigned timer=0;timer<6;++timer)
    for(unsigned tick=0;tick<6;++tick)
    for(unsigned value=0;value<8;++value)
    for(short scale=-1;scale<=1;++scale) {
        static struct SlicksRaceRuntime race;
        memset(&race,0,sizeof race);
        struct SlicksRaceCar *car=&race.cars[index];
        car->effective_surface=30; car->measured_speed=speeds[speed];
        race.damage_scale=scale; race.pit_repair_ticks=timers[timer];
        for(unsigned channel=0;channel<4;++channel) {
            car->damage[channel]=damage[(value+channel)%8];
            word(uc,0x6304f+index*54+channel*2,car->damage[channel]);
        }
        word(uc,0x8ef98,index); word(uc,0x8ef9c,race.pit_repair_ticks);
        word(uc,0x8effe,ticks[tick]); word(uc,0x63026,scale);
        dword(uc,0x6684e + index*4,car->measured_speed);
        int repaired=repair_car_at_pit(&race,car,ticks[tick]);
        unsigned end=car->measured_speed>=300 ? 0x238eb : repaired ? 0x23276 : 0x2327e;
        run_slice(uc,0x231fb,end,index,ticks[tick]);
        if(race.pit_repair_ticks!=(short)readword(uc,0x8ef9c)) {
            fputs("Pit repair timer mismatch\n",stderr); exit(1);
        }
        for(unsigned channel=0;channel<4;++channel)
            if(car->damage[channel]!=(short)readword(uc,0x6304f+index*54+channel*2)) {
                fputs("Pit repair damage mismatch\n",stderr); exit(1);
            }
        ++cases;
    }
    check(uc_hook_del(uc,repaired_end));
    check(uc_hook_del(uc,idle_end));
    printf("DOS pit repair: %u gate/timer/four-channel cases match\n",cases);
}

static void verify_service_order(uc_engine *uc)
{
    static const short targets[][2]={{200,100},{200,200},{100,200},{0,200},
        {0,100},{0,0},{100,0},{200,0}};
    static const long speeds[]={0,80,81,200,201,700,701,7001};
    const unsigned data=0x3cbf0;
    unsigned cases=0;
    uc_hook hook;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    check(uc_hook_add(uc,&hook,UC_HOOK_CODE,stop_at_zone_entry,0,0x1f5e5,0x1f5e5));
    for(unsigned index=0;index<4;++index)
    for(unsigned heading=0;heading<16;++heading)
    for(unsigned target=0;target<8;++target)
    for(unsigned speed=0;speed<8;++speed)
    for(unsigned initial=0;initial<16;++initial) {
        static struct SlicksRaceRuntime race;
        memset(&race,0,sizeof race);
        struct SlicksRaceCar *car=&race.cars[index];
        unsigned char controls[5];
        car->x=car->y=car->ai_last_x=car->ai_last_y=10300;
        car->ai_stuck_ticks=700; car->ai_service_state=-1;
        car->damage[0]=100; /* Keep completion false beyond this slice's end. */
        car->heading=heading*1200; car->measured_speed=speeds[speed];
        car->velocity_x=200; car->velocity_y=-300; car->ai_control_latch=initial;
        race.navigation.zone_count=2;
        race.navigation.zones[0]=(struct SlicksTrackZone){{90,110,200},{90,110,100},0};
        race.navigation.pit_route_count=1;
        race.navigation.pits[0].x=targets[target][0];
        race.navigation.pits[0].y=targets[target][1];
        for(unsigned bit=0;bit<4;++bit) controls[bit]=(initial>>bit)&1;
        controls[4]=1;
        check(uc_mem_write(uc,data+0x5344+index*5,controls,5));
        byte(uc,data+0x6936+index,-1); byte(uc,data+0x692a+index,0);
        word(uc,data+0x304f+index*54,100);
        word(uc,data+0x6902+index*2,0); word(uc,data+0x681c+index*2,car->heading);
        word(uc,data+0x53b6+index*2,100); word(uc,data+0x53be + index*2,100);
        dword(uc,data+0x684e + index*4,car->measured_speed);
        dword(uc,data+0x682e + index*4,car->velocity_x);
        dword(uc,data+0x683e + index*4,car->velocity_y);
        word(uc,data+0x5426,2);
        word(uc,data+0x5428,90); word(uc,data+0x58d8,110);
        word(uc,data+0x5680,90); word(uc,data+0x5b30,110);
        word(uc,data+0x5d88,200); word(uc,data+0x5fe0,100);
        byte(uc,data+0x3640,1); byte(uc,data+0x3642,0); byte(uc,data+0x3660,0);
        word(uc,data+0x367e,targets[target][0]); word(uc,data+0x3692,targets[target][1]);
        word(uc,0x8effc,index); uint16_t cx=index*2;
        check(uc_reg_write(uc,UC_X86_REG_CX,&cx));
        run_slice_data(uc,0x1f1de,0x1f5e5,index,1,0x3cbf);
        unsigned actual=ai_controls(&race,index,1);
        check(uc_mem_read(uc,data+0x5344+index*5,controls,5));
        unsigned expected=controls[0]|controls[1]<<1|controls[2]<<2|controls[3]<<3;
        if(actual!=expected || controls[4]!=1 ||
           car->ai_service_state!=readbyte(uc,data+0x6936+index) ||
           car->ai_state!=readbyte(uc,data+0x692a+index) ||
           car->ai_route_seen!=readbyte(uc,data+0x6932+index) ||
           car->ai_contact_threshold!=readword(uc,data+0x2fa4+index*2) ||
           car->ai_target_x!=(short)readword(uc,data+0x690a+index*2) ||
           car->ai_target_y!=(short)readword(uc,data+0x6912+index*2) ||
           car->waypoint!=readword(uc,data+0x6902+index*2) ||
           car->ai_recovery_ticks!=(short)readword(uc,data+0x691a+index*2)) {
            fprintf(stderr,"Service update order mismatch car=%u heading=%u target=%u speed=%ld mask=%u/%u\n",
                    index,heading,target,car->measured_speed,actual,expected); exit(1);
        }
        ++cases;
    }
    check(uc_hook_del(uc,hook));
    printf("DOS service update order: %u two-steering-call cases match\n",cases);
}

/* Captured A1200 CONFIGD frame 3600. Hold physical state fixed: this
 * proves the AI response to the stall, not how either game reaches it. */
static void verify_service_stall(uc_engine *uc)
{
    const unsigned data=0x3cbf0,index=2;
    static unsigned char saved[65536];
    check(uc_mem_read(uc,data,saved,sizeof saved));
    check(uc_ctl_remove_cache(uc,0,0x100000));
    unsigned cases=0;
    for(unsigned ticks=1;ticks<=2;++ticks) {
        static struct SlicksRaceRuntime race;
        memset(&race,0,sizeof race);
        struct SlicksRaceCar *car=&race.cars[index];
        car->x=21139; car->y=6850;
        car->ai_last_x=21151; car->ai_last_y=6895;
        car->heading=894; car->measured_speed=16; car->velocity_x=32;
        car->ai_stuck_ticks=100; car->ai_recovery_ticks=398;
        car->ai_target_x=214; car->ai_target_y=64;
        car->ai_contact_ticks=11; car->ai_contact_threshold=50;
        car->ai_route_seen=1; car->ai_state=1; car->ai_service_state=2;
        car->ai_control_latch=9; car->damage[0]=214;
        car->fuel=0xfffffffcUL; car->fuel_capacity=3312; car->lap=3;
        car->waypoint=8; race.navigation.zone_count=9;
        race.fuel_option=10; race.laps_to_run=4;
        check(uc_mem_write(uc,data,saved,sizeof saved));
        word(uc,data+0x3020,0); word(uc,data+0x3024,10);
        word(uc,data+0x0092,0); word(uc,data+0x4c18,4);
        word(uc,data+0x53b6+2*index,208); word(uc,data+0x53be +2*index,65);
        word(uc,data+0x68ea+2*index,208); word(uc,data+0x68f2+2*index,65);
        word(uc,data+0x68fa+2*index,100); word(uc,data+0x691a+2*index,398);
        word(uc,data+0x6922+2*index,0); word(uc,data+0x690a+2*index,214);
        word(uc,data+0x6912+2*index,64); word(uc,data+0x2fa4+2*index,50);
        word(uc,data+0x53c6+2*index,11); word(uc,data+0x6902+2*index,8);
        word(uc,data+0x681c+2*index,894); word(uc,data+0x304f+54*index,214);
        word(uc,data+0x4bfe +2*index,2);
        dword(uc,data+0x684e +4*index,16); dword(uc,data+0x682e +4*index,32);
        dword(uc,data+0x683e +4*index,0); dword(uc,data+0x305f+54*index,-4);
        dword(uc,data+0x3063+54*index,3312);
        byte(uc,data+0x692a+index,1); byte(uc,data+0x6936+index,2);
        byte(uc,data+0x6932+index,1); byte(uc,data+0x692e +index,0);
        for(unsigned bit=0;bit<4;++bit) byte(uc,data+0x5344+5*index+bit,(9>>bit)&1);
        byte(uc,data+0x5344+5*index+4,1);
        for(unsigned update=0;update<400;++update) {
            uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
            word(uc,0x8f000,0); word(uc,0x8f002,0x7000);
            word(uc,0x8f004,index); word(uc,0x8f006,ticks);
            check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
            check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
            check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
            check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
            check(uc_emu_start(uc,0x1f09d,0x70000,0,5000));
            check(uc_reg_read(uc,UC_X86_REG_CS,&cs));
            check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
            check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
            unsigned expected=0;
            for(unsigned bit=0;bit<4;++bit)
                expected|=!!readbyte(uc,data+0x5344+5*index+bit)<<bit;
            unsigned actual=ai_controls(&race,index,ticks);
            if(cs*16U+ip!=0x70000 || sp!=0xf004 || actual!=expected ||
               car->ai_state!=readbyte(uc,data+0x692a+index) ||
               car->ai_service_state!=readbyte(uc,data+0x6936+index) ||
               car->ai_route_seen!=readbyte(uc,data+0x6932+index) ||
               car->ai_stuck_ticks!=(short)readword(uc,data+0x68fa+2*index) ||
               car->ai_recovery_ticks!=(short)readword(uc,data+0x691a+2*index) ||
               car->ai_turn_ticks!=(short)readword(uc,data+0x6922+2*index) ||
               car->ai_target_x!=(short)readword(uc,data+0x690a+2*index) ||
               car->ai_target_y!=(short)readword(uc,data+0x6912+2*index)) {
                fprintf(stderr,"Service stall mismatch ticks=%u update=%u controls=%u/%u pc=%x:%x\n",
                        ticks,update,actual,expected,cs,ip); exit(1);
            }
            ++cases;
        }
    }
    check(uc_mem_write(uc,data,saved,sizeof saved));
    check(uc_ctl_remove_cache(uc,0,0x100000));
    printf("DOS captured pit stall: %u whole-AI calls match with physical state held fixed\n",cases);
}

static void verify_layer_contact_flags(uc_engine *uc)
{
    static struct SlicksRaceRuntime race;
    unsigned cases=0;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    for(unsigned index=0;index<4;++index)
    for(unsigned lower=0;lower<32;++lower)
    for(unsigned upper=0;upper<32;++upper)
    for(unsigned layer=0;layer<2;++layer)
    for(int special=-1;special<=1;++special)
    for(unsigned contact=0;contact<2;++contact)
    for(unsigned last_wall=0;last_wall<2;++last_wall)
    for(unsigned sampling=0;sampling<2;++sampling) {
        struct SlicksRaceCar car={0};
        uint16_t bx=index;
        car.x=10333; car.y=8377;
        car.actor_layer=layer; car.special_drive_state=special;
        car.actor_contact=contact; car.touching_solid=last_wall;
        car.collision_sampling=sampling;
        car.oil_active=last_wall;
        car.oil_turn_sign=-1;
        race.material_map[83*320+103]=lower;
        race.surface_map[83*320+103]=upper;
        byte(uc,0x65388+index,layer); byte(uc,0x65384+index,lower);
        byte(uc,0x65380+index,upper); byte(uc,0x6536c+index,contact);
        byte(uc,0x64daa+index,sampling); word(uc,0x63058+index*54,special);
        byte(uc,0x64c70+index,car.oil_active); byte(uc,0x64c78+index,255);
        word(uc,0x8ef98,index);
        check(uc_reg_write(uc,UC_X86_REG_BX,&bx));
        run_slice(uc,0x229d2,0x22a61,index,1);
        update_actor_layer(&race,&car);
        if(car.actor_layer!=(unsigned char)readbyte(uc,0x65388+index) ||
           car.selected_surface!=(unsigned char)readbyte(uc,0x6537c+index) ||
           car.effective_surface!=(unsigned char)readbyte(uc,0x65378+index) ||
           car.oil_active!=(unsigned char)readbyte(uc,0x64c70+index) ||
           (unsigned char)car.oil_turn_sign!=(unsigned char)readbyte(uc,0x64c78+index) ||
           car.collision_sampling!=(unsigned char)readbyte(uc,0x64daa+index)) {
            fprintf(stderr,"Layer flag mismatch car=%u maps=%u/%u layer=%u special=%d contact=%u last_wall=%u\n",
                    index,lower,upper,layer,special,contact,last_wall); exit(1);
        }
        ++cases;
    }
    check(uc_ctl_remove_cache(uc,0,0x100000));
    printf("DOS layer transitions: %u material/layer/special/independent-contact and oil-latch reset cases match\n",cases);
}

static void verify_track_sampling_lifecycle(uc_engine *uc)
{
    static struct SlicksRaceRuntime race;
    static const short levels[]={-32768,-1,0,1,3,5,32767};
    unsigned cases=0;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    word(uc,0x605b8,0); word(uc,0x605ba,0x9000);
    word(uc,0x605bc,0); word(uc,0x605be,0xa000);
    for(unsigned index=0;index<4;++index)
    for(unsigned material=0;material<32;++material)
    for(unsigned layer=0;layer<2;++layer)
    for(int special=-1;special<=1;++special)
    for(unsigned contact=0;contact<2;++contact)
    for(unsigned previous=0;previous<2;++previous)
    for(unsigned sampling=0;sampling<2;++sampling)
    for(unsigned level=0;level<7;++level) {
        struct SlicksRaceCar *car=&race.cars[index];
        unsigned x=100+index,y=80+index,upper=(material+7)&31,offset=y*320+x;
        uint16_t cs=0x1987,ds=0x6000,ss=0x8000,bp=0xf000,sp=0xef00,ip;
        race.boundary_level=levels[level];
        race.material_map[offset]=material; race.surface_map[offset]=upper;
        car->x=x*100+33; car->y=y*100+77; car->actor_layer=layer;
        car->special_drive_state=special; car->actor_contact=contact;
        car->previous_actor_contact=previous; car->collision_sampling=sampling;
        car->collision_safe_x=456; car->collision_safe_y=233;
        byte(uc,0x90000+offset,(material<<3)|(upper&7));
        byte(uc,0xa0000+y*80+x/4,(upper>>3)<<((x&3)*2));
        byte(uc,0x65388+index,layer); byte(uc,0x64daa+index,sampling);
        byte(uc,0x6536c+index,contact); byte(uc,0x65370+index,previous);
        word(uc,0x63058+index*54,special); word(uc,0x64c6c,levels[level]);
        word(uc,0x653ce + index*2,456); byte(uc,0x66818+index,233);
        dword(uc,0x6538c+index*4,car->x); dword(uc,0x6539c+index*4,car->y);
        word(uc,0x8ef98,index);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss)); check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x238eb,0x2399f,0,1000));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip)); check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
        if(update_track_sampling(&race,car) || ip+0x19870!=0x2399f || sp!=0xef00 ||
           car->collision_sampling!=(unsigned char)readbyte(uc,0x64daa+index) ||
           car->collision_safe_x!=(short)readword(uc,0x653ce + index*2) ||
           car->collision_safe_y!=(unsigned char)readbyte(uc,0x66818+index)) {
            fprintf(stderr,"Sampling lifecycle mismatch car=%u material=%u layer=%u special=%d contact=%u/%u latch=%u level=%d\n",
                    index,material,layer,special,contact,previous,sampling,levels[level]); exit(1);
        }
        ++cases;
    }
    printf("DOS collision sampling lifecycle: %u contact/latch/layer/material/level cases match\n",cases);
}

static void verify_car_material_sample(uc_engine *uc)
{
    static unsigned char lower[60800],upper[60800];
    static const short levels[]={-32768,-1,0,1,3,5,32767};
    unsigned cases=0;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    word(uc,0x605b8,0); word(uc,0x605ba,0x9000);
    word(uc,0x605bc,0); word(uc,0x605be,0xa000);
    for(unsigned index=0;index<4;++index)
    for(unsigned low=0;low<32;++low)
    for(unsigned high=0;high<32;++high)
    for(unsigned layer=0;layer<2;++layer)
    for(int special=-1;special<=1;++special)
    for(unsigned enabled=0;enabled<2;++enabled)
    for(unsigned level=0;level<sizeof levels/sizeof levels[0];++level) {
        unsigned x=100+index,y=80+index,offset=y*320+x;
        uint16_t cs=0x1987,ds=0x6000,ss=0x8000,sp=0xf000,ip,ax;
        lower[offset]=low; upper[offset]=high;
        byte(uc,0x90000+offset,(low<<3)|(high&7));
        byte(uc,0xa0000+y*80+x/4,(high>>3)<<((x&3)*2));
        byte(uc,0x65388+index,layer); byte(uc,0x64daa+index,enabled);
        word(uc,0x63058+index*54,special); word(uc,0x64c6c,levels[level]);
        word(uc,0x8f000,0); word(uc,0x8f002,0x7000);
        word(uc,0x8f004,x); word(uc,0x8f006,y); word(uc,0x8f008,index);
        word(uc,0x8f00a,!layer); /* Actual car layer overrides this argument. */
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss)); check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x1c5a0,0x70000,0,500));
        check(uc_reg_read(uc,UC_X86_REG_CS,&cs)); check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        check(uc_reg_read(uc,UC_X86_REG_SP,&sp)); check(uc_reg_read(uc,UC_X86_REG_AX,&ax));
        int actual=slicks_track_car_sample(lower,upper,x,y,layer,special,enabled,levels[level]);
        if(cs*16U+ip!=0x70000 || sp!=0xf004 || actual!=(ax&255)) {
            fprintf(stderr,"Car sample mismatch car=%u maps=%u/%u layer=%u special=%d enabled=%u level=%d\n",
                    index,low,high,layer,special,enabled,levels[level]); exit(1);
        }
        ++cases;
    }
    if(slicks_track_car_sample(0,0,0,0,0,0,0,5)!=0 ||
       slicks_track_car_sample(0,0,0,0,1,1,1,5)!=0 ||
       slicks_track_car_sample(lower,upper,-1,80,1,0,1,5)!=-1 ||
       slicks_track_car_sample(lower,upper,320,80,1,0,1,5)!=-1 ||
       slicks_track_car_sample(lower,upper,0,190,0,0,1,5)!=-1) exit(1);
    printf("DOS car material predicate: %u packed-map/layer/state/latch/level cases match\n",cases);
}

static void verify_pit_routes(uc_engine *uc)
{
    static unsigned char material[60800],raw[65536];
    static const short xs[]={10,300,10,300,100,150,200,100};
    static const short ys[]={10,10,180,180,80,80,120,80};
    unsigned cases=0;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    for(unsigned kind=0;kind<32;++kind)
    for(unsigned level=0;level<=5;++level)
    for(unsigned pattern=0;pattern<4;++pattern) {
        struct SlicksTrackNavigation navigation={0};
        uint16_t cs=0x1987,ds=0x6000,ss=0x8000,sp=0xf000,ip;
        for(unsigned i=0;i<60800;++i) {
            unsigned x=i%320,y=i/320;
            material[i]=(pattern==0 || (pattern==1 && x==155) ||
                (pattern==2 && x==10 && y==10) ||
                (pattern==3 && (x+y)%13==0)) ? kind : 0;
            raw[i]=material[i]<<3;
        }
        check(uc_mem_write(uc,0x90000,raw,sizeof raw));
        word(uc,0x605b8,0); word(uc,0x605ba,0x9000);
        word(uc,0x64c6c,level);
        navigation.zone_count=8; navigation.pit_count=4;
        word(uc,0x65426,8); byte(uc,0x63641,4);
        for(unsigned i=0;i<8;++i) {
            navigation.zones[i].x[0]=xs[i]; navigation.zones[i].y[0]=ys[i];
            word(uc,0x65428+i*2,xs[i]); word(uc,0x65680+i*2,ys[i]);
            if(i<4) {
                navigation.pits[i].x=xs[i]; navigation.pits[i].y=ys[i];
                word(uc,0x6367e + i*2,xs[i]); word(uc,0x63692+i*2,ys[i]);
            }
        }
        word(uc,0x8f000,0); word(uc,0x8f002,0x7000);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
        check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x1b103,0x70000,0,2000000));
        check(uc_reg_read(uc,UC_X86_REG_CS,&cs));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
        if(cs*16u+ip!=0x70000 || sp!=0xf004 ||
           slicks_build_pit_routes(&navigation,material,level) ||
           navigation.pit_route_count!=readbyte(uc,0x63640)) {
            fprintf(stderr,"Pit routing mismatch material=%u level=%u pattern=%u native=%u DOS=%d pc=%x\n",
                kind,level,pattern,navigation.pit_route_count,readbyte(uc,0x63640),cs*16u+ip);
            exit(1);
        }
        for(unsigned i=0;i<navigation.pit_route_count;++i)
            if(navigation.pit_route_zone[i]!=readbyte(uc,0x63642+i) ||
               navigation.pit_route_destination[i]!=readbyte(uc,0x63660+i)) {
                fputs("Pit route ordering mismatch\n",stderr); exit(1);
            }
        ++cases;
    }
    printf("DOS pit routing: %u full original visibility/selection cases match\n",cases);
}
static void verify_track_actor_records(uc_engine *uc)
{
    static const unsigned counts[]={0,1,98,99};
    static const unsigned positions[]={0,1,255,32767,65535};
    unsigned cases=0;
    unsigned char tables[10];
    uc_hook hook;
    check(uc_mem_read(uc,0x3cbf0+0x1be,tables,sizeof tables));
    check(uc_mem_write(uc,0x601be,tables,sizeof tables));
    check(uc_ctl_remove_cache(uc,0,0x100000));
    check(uc_hook_add(uc,&hook,UC_HOOK_CODE,stop_at_zone_entry,0,0x1b9b0,0x1b9b0));
    for(unsigned c=0;c<4;++c)
    for(unsigned type=0;type<110;++type)
    for(unsigned p=0;p<5;++p) {
        struct SlicksTrackNavigation navigation={0};
        uint16_t cs=0x1987,ds=0x6000,ss=0x8000,bp=0xf000,sp=0xef00,ip;
        navigation.actor_count=counts[c];
        word(uc,0x63124,counts[c]);
        for(unsigned i=0;i<100;++i) {
            struct SlicksTrackActor *a=&navigation.actors[i];
            a->x=123+i; a->y=-234-(int)i; a->velocity_x=345+i;
            a->velocity_y=-456-(int)i; a->kind=77; a->layer=88;
            word(uc,0x63126+2*i,(unsigned short)a->x);
            word(uc,0x631ee + 2*i,(unsigned short)a->y);
            word(uc,0x632b6+2*i,(unsigned short)a->velocity_x);
            word(uc,0x6337e + 2*i,(unsigned short)a->velocity_y);
            byte(uc,0x63446+i,a->kind); byte(uc,0x634aa+i,a->layer);
        }
        word(uc,0x8effa,positions[p]); word(uc,0x8eff8,positions[4-p]);
        word(uc,0x8eff6,type);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
        check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
        check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x1b908,0x1b9f9,0,500));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
        int recognized=slicks_record_track_actor(&navigation,positions[p],positions[4-p],type);
        if(sp!=0xef00 || (unsigned)cs*16+ip!=(recognized?0x1b9f9:0x1b9b0) ||
           navigation.actor_count!=readword(uc,0x63124)) {
            fputs("Track actor dispatch/count mismatch\n",stderr); exit(1);
        }
        for(unsigned i=0;i<100;++i) {
            const struct SlicksTrackActor *a=&navigation.actors[i];
            if(a->x!=(short)readword(uc,0x63126+2*i) ||
               a->y!=(short)readword(uc,0x631ee + 2*i) ||
               a->velocity_x!=(short)readword(uc,0x632b6+2*i) ||
               a->velocity_y!=(short)readword(uc,0x6337e + 2*i) ||
               a->kind!=(unsigned char)readbyte(uc,0x63446+i) ||
               a->layer!=(unsigned char)readbyte(uc,0x634aa+i)) {
                fprintf(stderr,"Track actor record mismatch type=%u count=%u position=%u slot=%u\n",
                    type,counts[c],p,i); exit(1);
            }
        }
        ++cases;
    }
    check(uc_hook_del(uc,hook));
    printf("DOS track actor records: %u dispatch/Q4/order/overflow cases match\n",cases);
}

static void verify_pit_records(uc_engine *uc)
{
    static const unsigned positions[]={0,1,255,32767,65535};
    unsigned cases=0;
    uc_hook hook;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    check(uc_hook_add(uc,&hook,UC_HOOK_CODE,stop_at_zone_entry,0,0x1b908,0x1b908));
    for(unsigned count=0;count<=10;++count)
    for(unsigned type=0;type<110;++type)
    for(unsigned position=0;position<5;++position)
    for(unsigned enabled=0;enabled<2;++enabled) {
        struct SlicksTrackNavigation navigation={0};
        navigation.pit_count=count; navigation.service_available=enabled;
        for(unsigned i=0;i<10;++i) {
            navigation.pits[i].x=123+i; navigation.pits[i].y=234+i;
            word(uc,0x6367e + i*2,navigation.pits[i].x);
            word(uc,0x63692+i*2,navigation.pits[i].y);
        }
        byte(uc,0x63641,count); byte(uc,0x636a6,enabled);
        word(uc,0x8effa,positions[position]);
        word(uc,0x8eff8,positions[4-position]);
        word(uc,0x8eff6,type);
        run_slice(uc,0x1b8cb,0x1b908,0,0);
        slicks_record_track_pit(&navigation,positions[position],positions[4-position],type);
        if(navigation.pit_count!=readbyte(uc,0x63641) ||
           navigation.service_available!=readbyte(uc,0x636a6)) {
            fputs("Pit count/availability mismatch\n",stderr); exit(1);
        }
        for(unsigned i=0;i<10;++i)
            if(navigation.pits[i].x!=(short)readword(uc,0x6367e + i*2) ||
               navigation.pits[i].y!=(short)readword(uc,0x63692+i*2)) {
                fputs("Pit coordinate/order mismatch\n",stderr); exit(1);
            }
        ++cases;
    }
    check(uc_hook_del(uc,hook));
    printf("DOS pit records: %u original-instruction cases match\n",cases);
}
static void verify_service_approach(uc_engine *uc)
{
    static const short offsets[]={-32768,-81,-20,-1,0,8,9,10,19,20,79,80,81,32767};
    static const long speeds[]={-1,0,80,81,200,201,700,701};
    unsigned cases=0;
    uc_hook hook;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    check(uc_hook_add(uc,&hook,UC_HOOK_CODE,stop_at_zone_entry,0,0x1f5e5,0x1f5e5));
    for(unsigned index=0;index<4;++index)
    for(unsigned x=0;x<14;++x)
    for(unsigned y=0;y<14;++y)
    for(unsigned speed=0;speed<8;++speed)
    for(unsigned initial=0;initial<16;++initial) {
        struct SlicksRaceCar car={0};
        unsigned char controls[5];
        car.x=((long)offsets[x]+103)*100;
        car.y=((long)offsets[y]+103)*100;
        car.ai_target_x=car.ai_target_y=100;
        car.ai_service_state=1;
        car.measured_speed=speeds[speed];
        for(unsigned bit=0;bit<4;++bit) controls[bit]=(initial>>bit)&1;
        controls[4]=1;
        check(uc_mem_write(uc,0x65344+index*5,controls,5));
        word(uc,0x653b6+index*2,car.x/100-3);
        word(uc,0x653be + index*2,car.y/100-3);
        word(uc,0x6690a+index*2,100); word(uc,0x66912+index*2,100);
        byte(uc,0x66936+index,1);
        dword(uc,0x6684e + index*4,car.measured_speed);
        run_slice(uc,0x1f504,0x1f5e5,index,1);
        unsigned actual=ai_service_approach(&car,initial);
        check(uc_mem_read(uc,0x65344+index*5,controls,5));
        unsigned expected=controls[0]|controls[1]<<1|controls[2]<<2|controls[3]<<3;
        if(actual!=expected || controls[4]!=1 ||
           car.ai_service_state!=readbyte(uc,0x66936+index)) {
            fprintf(stderr,"Service approach mismatch car=%u dx=%d dy=%d speed=%ld controls=%u/%u\n",
                    index,offsets[x],offsets[y],car.measured_speed,actual,expected);
            exit(1);
        }
        ++cases;
    }
    check(uc_hook_del(uc,hook));
    printf("DOS service approach: %u original-instruction cases match\n",cases);
}
struct HudCapture {
    unsigned count;
    short words[6][6];
};
static void verify_service_option_keys(uc_engine *uc)
{
    static const unsigned scans[]={0x4b,0x4d,0x47,0x49,0x4f,0x51,0x1c};
    unsigned cases=0;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    for(unsigned damage=0;damage<2;++damage)
    for(unsigned key=0;key<sizeof scans/sizeof scans[0];++key)
    for(unsigned value=0;value<65536;++value) {
        uint16_t cs=0x266c,ss=0x8000,bp=0xf000,sp=0xef00,ip;
        unsigned record=0x3cbf0+0xda+damage*8;
        /* Real records: signed minimum byte +3, maximum word +4, step +6. */
        word(uc,record,value); word(uc,record+2,1);
        word(uc,record+4,300); word(uc,record+6,0x9000+(damage?20:5));
        byte(uc,0x8effb,9+damage); word(uc,0x8efee,scans[key]);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
        check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x294d1,0x29659,0,150));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
        short native=slicks_service_option_key((short)value,damage,scans[key]);
        if(ip+0x266c0!=0x29659 || sp!=0xef00 ||
           (unsigned short)native!=readword(uc,record)) {
            fprintf(stderr,"Option key mismatch damage=%u scan=%x value=%u\n",damage,scans[key],value);
            exit(1);
        }
        ++cases;
    }
    puts("DOS service option keys: 917504 signed-value/arrow/extrema/toggle cases match");
    if(cases!=917504) exit(1);
    {
        short fuel=0,damage=0;
        unsigned short selection=0;
        static const unsigned sequence[]={0x4d,0x4d,0x50,0x4d,0x50};
        for(unsigned i=0;i<sizeof sequence/sizeof sequence[0];++i)
            if(slicks_service_menu_key(&selection,&fuel,&damage,sequence[i])!=1) exit(1);
        if(selection!=2 || fuel!=10 || damage!=20 ||
           slicks_service_menu_key(&selection,&fuel,&damage,0x4d)!=0 ||
           slicks_service_menu_key(&selection,&fuel,&damage,0x1c)!=2 ||
           slicks_service_menu_key(&selection,&fuel,&damage,0x01)!=2) exit(1);
        if(slicks_service_menu_key(&selection,&fuel,&damage,0x50)!=1 || selection!=0 ||
           slicks_service_menu_key(&selection,&fuel,&damage,0x48)!=1 || selection!=2 ||
           slicks_service_menu_key(&selection,&fuel,&damage,0x39)!=0 || fuel!=10 || damage!=20) exit(1);
        puts("Native service menu: selection wrapping, adjustments, return and ignored keys pass");
    }
}

static void verify_service_setup(uc_engine *uc)
{
    static struct SlicksRaceRuntime race;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    for(unsigned value=0;value<65536;++value) {
        uint16_t cs=0x266c,ss=0x8000,sp=0xf000,ip;
        short fuel=(short)value,damage=(short)(value^0x5a5a);
        word(uc,0x3cbf0+0xda,fuel); word(uc,0x3cbf0+0xe2,damage);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss)); check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x2be0b,0x2be47,0,100));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip)); check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
        slicks_race_set_service_options(&race,fuel,damage);
        if(ip+0x266c0!=0x2be47 || sp!=0xf000 ||
           (unsigned short)race.fuel_option!=readword(uc,0x3cbf0+0x3024) ||
           (unsigned short)race.damage_scale!=readword(uc,0x3cbf0+0x3026)) exit(1);
    }
    puts("DOS custom service setup: all 65536 signed fuel/damage inputs match");
    {
        static unsigned char logical[0x40000],chunky[128000];
        struct SlicksTrackNavigation navigation={0};
        navigation.zone_count=1; navigation.start_x=100; navigation.start_y=80;
        for(int level=-1;level<=20;++level)
        for(unsigned selected=0;selected<SLICKS_VEHICLE_COUNT;++selected) {
            slicks_race_initialize(&race,&navigation);
            if(race.cars[0].vehicle!=5 || race.cars[1].vehicle!=2 ||
               race.cars[2].vehicle || race.cars[3].vehicle) exit(1);
            slicks_race_set_service_options(&race,10,100);
            slicks_race_set_vehicle(&race,0,selected);
            short inventory[4][13];
            for(unsigned car=0;car<4;++car)
                for(unsigned slot=0;slot<13;++slot) inventory[car][slot]=(short)(level<0?0:level);
            if(level>=0) {
                inventory[3][1]=21;
                if(slicks_race_set_inventory(&race,inventory)!=-1 || race.setup_inventory_ready) exit(1);
                inventory[3][1]=(short)level;
                if(slicks_race_set_inventory(&race,inventory) ||
                    memcmp(race.weapon_inventory,inventory,sizeof inventory)) exit(1);
            }
            /* Synthetic ready assets isolate setup order, not asset fidelity.
             * Host padding accommodates legacy target-long framebuffer clears. */
            race.font.ready=1;
            for(unsigned v=0;v<SLICKS_VEHICLE_COUNT;++v) {
                race.properties[v].ready=1; race.properties[v].property_33=100+v;
                for(unsigned d=0;d<SLICKS_CAR_BASE_DIRECTIONS;++d) race.sprites[v][d].ready=1;
            }
            for(unsigned i=0;i<SLICKS_START_LIGHT_COUNT;++i) race.start_lights[i].ready=1;
            if(slicks_race_start(&race,logical,chunky)) exit(1);
            for(unsigned car=0;car<4;++car)
                if(race.cars[car].steering_property!=100+(level<0?4:level) ||
                   race.cars[car].steering_property!=race.cars[car].drive_coefficients[6]) exit(1);
            unsigned capacity=(100+selected)*(46+(level<0?0:level))*72/100;
            if(race.cars[0].vehicle!=selected || race.cars[0].fuel_capacity!=capacity ||
               race.cars[0].fuel!=capacity-1 || race.fuel_option!=10 || race.damage_scale!=100) exit(1);
            if(level>=0) {
                for(unsigned car=0;car<4;++car)
                    if(memcmp(race.cars[car].drive_setup,inventory[car],sizeof inventory[car]) ||
                        race.cars[car].fuel_upgrade!=level) exit(1);
                if(slicks_race_set_inventory(&race,inventory)!=-1) exit(1);
            }
            slicks_race_set_vehicle(&race,0,(selected+1)%SLICKS_VEHICLE_COUNT);
            slicks_race_set_service_options(&race,60,20);
            if(race.cars[0].vehicle!=selected || race.fuel_option!=10 || race.damage_scale!=100) exit(1);
        }
    }
    puts("Native setup: ten vehicles x legacy/all 21 inventory upgrade levels initialize fuel and drive state; invalid/live mutations rejected");
}

static void verify_status_palette(uc_engine *uc)
{
    static const unsigned char rgb[4][3]={{15,15,25},{50,50,15},{60,20,5},{55,55,10}};
    struct SlicksRaceRuntime race={0};
    unsigned char palette[768];
    unsigned random=1;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    for(unsigned sample=0;sample<320;++sample) {
        for(unsigned i=0;i<sizeof palette;++i) {
            random=random*1664525U+1013904223U;
            palette[i]=sample<64?(unsigned char)sample:(unsigned char)((random>>24)&63);
        }
        slicks_race_set_status_palette(&race,palette);
        check(uc_mem_write(uc,0xa0000,palette,sizeof palette));
        for(unsigned colour=0;colour<4;++colour) {
            uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,ip,ax;
            word(uc,0x8f000,0); word(uc,0x8f002,0x9000);
            for(unsigned c=0;c<3;++c) word(uc,0x8f004+c*2,rgb[colour][c]);
            word(uc,0x8f00a,0); word(uc,0x8f00c,0xa000);
            check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
            check(uc_reg_write(uc,UC_X86_REG_SS,&ss)); check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
            check(uc_emu_start(uc,0x36fae,0x90000,0,30000));
            check(uc_reg_read(uc,UC_X86_REG_AX,&ax)); check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
            check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
            unsigned actual=colour<3?race.status_colours[colour]:race.collision_colour;
            if(ip || sp!=0xf004 || (ax&255)!=actual) {
                fprintf(stderr,"HUD palette mismatch sample=%u slot=%u DOS=%u native=%u\n",
                        sample,colour,ax&255,actual); exit(1);
            }
        }
    }
    puts("DOS status/collision palette: 1280 original nearest-colour comparisons including ties match");
}
static void capture_hud_rectangle(uc_engine *uc, uint64_t address,
                                  uint32_t size, void *opaque)
{
    struct HudCapture *capture=opaque;
    uint16_t ss,sp,cs,ip;
    (void)address; (void)size;
    check(uc_reg_read(uc,UC_X86_REG_SS,&ss));
    check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
    unsigned stack=(unsigned)ss*16+sp;
    if(capture->count>=6) { fputs("Unexpected extra HUD draw\n",stderr); exit(1); }
    for(unsigned i=0;i<6;++i)
        capture->words[capture->count][i]=(short)readword(uc,stack+4+i*2);
    ++capture->count;
    /* Capture the graphics boundary, not an implementation of its pixels. */
    ip=readword(uc,stack); cs=readword(uc,stack+2); sp+=4;
    check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
    check(uc_reg_write(uc,UC_X86_REG_IP,&ip));
    check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
}
static void verify_status_rects(uc_engine *uc)
{
    static const unsigned amounts[]={0,1,24,25,99,3311,0xffffffffU,0x80000001U};
    static const unsigned capacities[]={1,2,100,3312,0x7fffffffU,0x80000000U};
    static const short damages[]={-1,0,1,39,40,799,800,32767};
    struct HudCapture capture;
    uc_hook hook;
    unsigned cases=0;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    check(uc_hook_add(uc,&hook,UC_HOOK_CODE,capture_hud_rectangle,&capture,0x39ed8,0x39ed8));
    for(unsigned index=0;index<4;++index)
    for(unsigned option=0;option<4;++option)
    for(unsigned amount=0;amount<8;++amount)
    for(unsigned capacity=0;capacity<6;++capacity)
    for(unsigned damage=0;damage<8;++damage)
    for(unsigned flags=0;flags<2;++flags)
    for(unsigned phase=0;phase<2;++phase) {
        struct SlicksRaceRuntime race={0};
        struct SlicksStatusRect rectangles[3];
        struct SlicksRaceCar *car=&race.cars[index];
        uint16_t cs=0x1987,ds=0x6000,ss=0x8000,sp=0xf000,ip;
        race.fuel_option=option&1; race.damage_scale=option&2;
        car->fuel=amounts[amount]; car->fuel_capacity=capacities[capacity];
        car->damage[0]=damages[damage]; car->service_flags=flags;
        byte(uc,0x64bc6+index,1); byte(uc,0x60799,1);
        byte(uc,0x668e2,10); byte(uc,0x668e4,11); byte(uc,0x668e3,12);
        word(uc,0x63020,0); word(uc,0x63024,race.fuel_option); word(uc,0x63026,race.damage_scale);
        dword(uc,0x6305f+index*54,car->fuel); dword(uc,0x63063+index*54,car->fuel_capacity);
        byte(uc,0x6305e + index*54,flags); word(uc,0x6304f+index*54,car->damage[0]);
        word(uc,0x3cbf0+0x1d87,0x1234); word(uc,0x3cbf0+0x1d89,0x5678);
        word(uc,0x3cbf0+0x1716,0); word(uc,0x3cbf0+0x1718,0xb000); word(uc,0xb0000,phase);
        word(uc,0x8f000,0); word(uc,0x8f002,0x9000); word(uc,0x8f004,index);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss)); check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        capture.count=0;
        check(uc_emu_start(uc,0x1d9b6,0x90000,0,5000));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip)); check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
        int count=slicks_race_status_rects(&race,index,phase,rectangles);
        if(ip || sp!=0xf004 || count<0 || capture.count!=(unsigned)count*2) exit(1);
        for(int i=0;i<count;++i) for(unsigned page=0;page<2;++page) {
            short *draw=capture.words[i*2+page];
            struct SlicksStatusRect *r=&rectangles[i];
            if(draw[0]!=r->left || draw[1]!=r->top || draw[2]!=r->right || draw[3]!=r->bottom ||
               draw[4]!=10+r->colour || draw[5]!=(page?0x5678:0x1234)) {
                fprintf(stderr,"HUD command mismatch car=%u option=%u fuel=%x capacity=%x damage=%d flags=%u phase=%u rect=%d\n",
                        index,option,car->fuel,car->fuel_capacity,car->damage[0],flags,phase,i); exit(1);
            }
        }
        ++cases;
    }
    check(uc_hook_del(uc,hook));
    printf("DOS fuel/damage HUD: %u complete command-sequence cases match both pages\n",cases);
}

static void verify_service_visual_gate(uc_engine *uc)
{
    static const short options[]={-32768,-1,0,1,10,32767};
    unsigned cases=0;
    uc_hook draw_hook;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    check(uc_hook_add(uc,&draw_hook,UC_HOOK_CODE,stop_at_zone_entry,0,0x1b9ca,0x1b9ca));
    for(unsigned type=0;type<256;++type)
    for(unsigned fuel=0;fuel<6;++fuel)
    for(unsigned damage=0;damage<6;++damage)
    for(unsigned flag=0;flag<2;++flag) {
        uint16_t cs=0x1987,ds=0x6000,ss=0x8000,bp=0xf000,sp=0xef00,ip;
        word(uc,0x8eff6,type);
        word(uc,0x63024,options[fuel]); word(uc,0x63026,options[damage]);
        byte(uc,0x636a6,flag);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
        check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
        check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x1b9b0,0x1b9f9,0,100));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        unsigned enabled=ip+0x19870==0x1b9ca;
        if((!enabled && ip+0x19870!=0x1b9f9) ||
           enabled!=(unsigned)slicks_track_object_enabled(type,options[fuel],options[damage]) ||
           readbyte(uc,0x636a6)!=(enabled?(int)flag:0)) {
            fprintf(stderr,"Pit visual gate mismatch type=%u fuel=%d damage=%d ip=%x\n",
                    type,options[fuel],options[damage],ip); exit(1);
        }
        ++cases;
    }
    check(uc_hook_del(uc,draw_hook));
    printf("DOS pit visual gate: %u signed-option/type cases match\n",cases);
}

static void verify_material_masks(uc_engine *uc)
{
    unsigned char tables[76];
    unsigned cases = 0;
    uc_hook hook;
    check(uc_mem_read(uc,0x3cbf0+0x70b,tables,sizeof tables));
    check(uc_mem_write(uc,0x6070b,tables,sizeof tables));
    word(uc,0x605b8,0); word(uc,0x605ba,0x9000);
    word(uc,0x605bc,0); word(uc,0x605be,0xa000);
    check(uc_ctl_remove_cache(uc,0,0x100000));
    check(uc_hook_add(uc,&hook,UC_HOOK_CODE,stop_at_zone_entry,0,0x1b426,0x1b426));
    for (unsigned pixel=0;pixel<38;++pixel)
    for (unsigned lower=0;lower<32;++lower)
    for (unsigned upper=0;upper<32;++upper)
    for (unsigned bridge=0;bridge<2;++bridge) {
        unsigned char a=lower,b=upper;
        uint16_t dx=pixel;
        byte(uc,0x90000,(lower<<3)|(upper&7));
        byte(uc,0xa0000,0xfc|(upper>>3));
        word(uc,0x8eff2,0); word(uc,0x8eff0,0);
        word(uc,0x8f00e,bridge);
        check(uc_reg_write(uc,UC_X86_REG_DX,&dx));
        run_slice(uc,0x1b2c5,0x1b426,0,0);
        unsigned raw=(unsigned char)readbyte(uc,0x90000);
        unsigned extra=(unsigned char)readbyte(uc,0xa0000);
        unsigned expected_lower=raw>>3, expected_upper=(raw&7)|((extra&3)<<3);
        if(slicks_apply_material_mask(pixel,bridge,&a,&b) ||
           a!=expected_lower || b!=expected_upper || (extra&0xfc)!=0xfc) {
            fprintf(stderr,"Mask mismatch pixel=%u old=%u/%u bridge=%u native=%u/%u DOS=%u/%u\n",
                    pixel,lower,upper,bridge,a,b,expected_lower,expected_upper); exit(1);
        }
        ++cases;
    }
    check(uc_hook_del(uc,hook));
    printf("DOS material masks: %u category/layer combinations match original compositor\n",cases);
}

int main(int argc,char **argv)
{
    static unsigned char runtime[300000];
    static const short vectors[][2]={{100,0},{100,100},{0,100},{-100,100},
        {-100,0},{-100,-100},{0,-100},{100,-100}};
    static const short fractions[]={0,1,600,1199};
    static const long speeds[]={0,700,701,709,1200};
    FILE *file=fopen(argc>1?argv[1]:"disasm/runtime.bin","rb");
    if(!file) return 2;
    size_t size=fread(runtime,1,sizeof runtime,file);
    if(ferror(file) || !feof(file)) return 2;
    fclose(file);
    uc_engine *uc; unsigned cases=0;
    check(uc_open(UC_ARCH_X86,UC_MODE_16,&uc));
    check(uc_mem_map(uc,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(uc,0x10100,runtime,size));
    verify_service_stall(uc);
    verify_material_masks(uc);
    verify_service_visual_gate(uc);
    verify_status_rects(uc);
    verify_status_palette(uc);
    verify_service_option_keys(uc);
    verify_service_setup(uc);
    for(unsigned target=0;target<8;++target)
    for(unsigned heading=0;heading<16;++heading)
    for(unsigned fraction=0;fraction<4;++fraction)
    for(unsigned speed=0;speed<5;++speed)
    for(unsigned velocity=0;velocity<8;++velocity)
    for(unsigned initial=0;initial<16;++initial)
    for(unsigned alternate=0;alternate<2;++alternate) {
        static struct SlicksRaceRuntime race;
        uint16_t cs=0x1987,ds=0x6000,ss=0x8000,sp=0xf000,ip,final_sp;
        unsigned char controls[5]={0};
        memset(&race,0,sizeof race);
        struct SlicksRaceCar *car=&race.cars[0];
        car->x=car->y=car->ai_last_x=car->ai_last_y=10300;
        car->ai_stuck_ticks=700;
        car->ai_control_latch=initial;
        for(unsigned bit=0;bit<4;++bit) controls[bit]=(initial>>bit)&1;
        car->heading=heading*1200+fractions[fraction];
        car->measured_speed=speeds[speed];
        if(velocity<4) {
            car->velocity_x=vectors[velocity*2][0]*speeds[speed]/50;
            car->velocity_y=vectors[velocity*2][1]*speeds[speed]/50;
        } else {
            /* Independent velocity/saved-speed pairs cover post-collision
             * state and quantization boundaries. At speed 709, (910,1330)
             * becomes (13,19), NOT (12,18) from vx*10/(speed+1). */
            car->velocity_x=(velocity&1)?-910:910;
            car->velocity_y=(velocity&2)?-1330:1330;
        }
        race.navigation.zones[0].x[2]=100+vectors[target][0];
        race.navigation.zones[0].y[2]=100+vectors[target][1];
        car->ai_state=alternate;
        car->ai_target_x=race.navigation.zones[0].x[2];
        car->ai_target_y=race.navigation.zones[0].y[2];
        word(uc,0x8f000,0); word(uc,0x8f002,0x9000);
        word(uc,0x8f004,0); word(uc,0x8f006,alternate?0xffff:0);
        word(uc,0x3cbf0+0x690a,car->ai_target_x);
        word(uc,0x3cbf0+0x6912,car->ai_target_y);
        word(uc,0x65d88,race.navigation.zones[0].x[2]);
        word(uc,0x65fe0,race.navigation.zones[0].y[2]);
        word(uc,0x653b6,100); word(uc,0x653be,100);
        word(uc,0x6681c,car->heading);
        dword(uc,0x6684e,car->measured_speed);
        dword(uc,0x6682e,car->velocity_x); dword(uc,0x6683e,car->velocity_y);
        check(uc_mem_write(uc,0x65344,controls,sizeof controls));
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
        check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        /* Real caller-side input bookkeeping for an AI driver. It must
         * retain controls, unlike the human input branches of 99cf. */
        byte(uc,0x64bc6,1);
        check(uc_emu_start(uc,0x199cf,0x90000,0,5000));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        check(uc_reg_read(uc,UC_X86_REG_SP,&final_sp));
        if(ip || final_sp!=0xf004) return 1;
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x1e204,0x90000,0,5000));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        check(uc_reg_read(uc,UC_X86_REG_SP,&final_sp));
        check(uc_mem_read(uc,0x65344,controls,sizeof controls));
        unsigned expected=(controls[0]?1:0)|(controls[1]?2:0)|
                          (controls[2]?4:0)|(controls[3]?8:0);
        unsigned actual=ai_controls(&race,0,1);
        if(ip || final_sp!=0xf004 || expected!=actual || car->ai_control_latch!=actual) {
            fprintf(stderr,"AI mismatch target=%u heading=%d speed=%ld velocity=%ld,%ld DOS=%u native=%u ip=%x sp=%x\n",
                target,car->heading,car->measured_speed,car->velocity_x,car->velocity_y,expected,actual,ip,final_sp);
            return 1;
        }
        ++cases;
    }
    verify_disabled_weapon_gate(uc);
    verify_recovery(uc);
    verify_zone_bounds(uc);
    verify_lap_flow();
    verify_lap_gate(uc);
    verify_checkpoints(uc);
    verify_clock(uc);
    verify_zone_expansion(uc);
    verify_alternate_selection(uc);
    verify_curve6_alternates();
    verify_contact_transitions(uc);
    verify_alternate_progress(uc);
    verify_service_approach(uc);
    verify_pit_records(uc);
    verify_track_actor_records(uc);
    verify_car_material_sample(uc);
    verify_layer_contact_flags(uc);
    verify_track_sampling_lifecycle(uc);
    verify_pit_routes(uc);
    verify_service_entry(uc);
    verify_damage_service_request(uc);
    verify_service_order(uc);
    verify_service_completion(uc);
    verify_pit_repair(uc);
    verify_fuel_capacity(uc);
    verify_fuel_updates(uc);
    verify_fuel_service_request(uc);
    check(uc_close(uc));
    printf("DOS route AI: %u full-routine cases match native decisions and return stack\n",cases);
    return 0;
}
