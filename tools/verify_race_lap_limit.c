#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/game/race_runtime.c"
#include "../src/gen/setup_defaults.h"

/* The stationary-grid test must never enter target-only particle assembly. */
unsigned short slicks_advance_particles(struct SlicksTrailParticle *particles,
    unsigned long count,unsigned char indices[SLICKS_TRAIL_PRIORITY_COUNT][SLICKS_TRAIL_PARTICLE_MAX],
    unsigned short counts[SLICKS_TRAIL_PRIORITY_COUNT],struct SlicksDirtyPixel *pixels,
    unsigned short *dirty_count,unsigned char *chunky,unsigned long page)
{
    (void)particles; (void)count; (void)indices; (void)counts; (void)pixels;
    (void)dirty_count; (void)chunky; (void)page;
    fputs("Unexpected particle gameplay in countdown test\n",stderr); abort();
}

static unsigned track_awards;
static void award_track(struct SlicksRaceRuntime *race) { (void)race; ++track_awards; }

int main(void)
{
    static struct SlicksRaceRuntime race;
    /* Exercise the public runtime setter, including the original Arcade
     * sentinel. This is a storage regression test, not an x86 oracle. */
    for (unsigned value = 0; value <= 65535; ++value) {
        race.finished_count = 37;
        race.race_complete = 41;
        slicks_race_set_laps(&race, (unsigned short)value);
        if (race.laps_to_run != (value ? value : 1) ||
            race.finished_count != 37 || race.race_complete != 41) {
            fprintf(stderr, "lap-limit storage failed: %u\n", value);
            return 1;
        }
    }
    slicks_race_set_laps(0, 9999);
    puts("Race lap-limit word storage: 65536 values passed");
    for(short speed=50;speed<=200;++speed) {
        static unsigned char logical[0x40000];
        memset(&race,0,sizeof race); race.started=1; race.countdown_ticks=10000;
        slicks_race_set_timer(&race,slicks_speed_timer_argument(speed));
        unsigned long period=(6553600UL/(unsigned)(speed*5))*50;
        if(race.physics_timer_disabled || race.physics_tick_period!=period) return 1;
        for(unsigned frame=1;frame<=300;++frame) {
            slicks_race_step(&race,logical);
            unsigned long expected=(unsigned long)((unsigned long long)frame*1193182/period);
            if(race.game_clock_ticks!=expected || race.cars[0].elapsed_time_units!=expected*2 ||
               race.frame_count!=frame || race.countdown_ticks!=10000-(long)expected) return 1;
        }
        unsigned long ticks=race.game_clock_ticks,elapsed=race.cars[0].elapsed_time_units;
        slicks_race_set_timer(&race,530);
        if(race.physics_tick_phase || race.physics_tick_period!=618250 ||
           race.game_clock_ticks!=ticks || race.cars[0].elapsed_time_units!=elapsed) return 1;
        slicks_race_step(&race,logical);
        if(race.game_clock_ticks!=ticks+1 || race.cars[0].elapsed_time_units!=elapsed+2) return 1;
        slicks_race_set_timer(&race,0);
        slicks_race_step(&race,logical);
        if(race.game_clock_ticks!=ticks+1 || race.cars[0].elapsed_time_units!=elapsed+2) return 1;
    }
    puts("Actual race-step clock: all 151 UI speeds, 45300 grid updates, live reprogram preserving elapsed time and disabled timer passed");
    /* Run the actual frame path while the grid is stationary. This checks
     * inclusion of countdown time and the existing PIT-to-PAL accumulator. */
    for(unsigned mode=0;mode<6;++mode) {
        static unsigned char logical[0x40000];
        memset(&race,0,sizeof race);
        slicks_race_set_mode(&race,(short)mode,5);
        slicks_race_set_laps(&race,4);
        if(race.laps_to_run!=(mode==5?9999:4)) return 1;
        race.started=1;
        race.countdown_ticks=10000;
        for(unsigned i=0;i<4;++i) race.cars[i].lap=1;
        slicks_race_set_mode(&race,17,99);
        if(race.race_mode!=(short)mode || race.arcade_seconds!=5) return 1;
        for(unsigned frame=1;frame<=300;++frame) {
            slicks_race_step(&race,logical);
            unsigned expected=(unsigned)((unsigned long long)frame*1193182/655350);
            if(race.game_clock_ticks!=expected || race.frame_count!=frame ||
                race.cars[0].elapsed_time_units!=2*expected) return 1;
        }
        /* Expiry is lazy; crossing a lap increments it before querying the
         * target. In Arcade this must not mark that driver finished yet. */
        if(race.laps_to_run!=(mode==5?9999:4)) return 1;
        race.cars[0].lap=4;
        race.cars[0].selected_surface=17;
        advance_lap_checkpoints(&race,&race.cars[0]);
        if(race.laps_to_run!=(mode==5?5:0) ||
            race.cars[0].finished!=(mode==5?0:1)) return 1;
        if(mode==5) {
            advance_lap_checkpoints(&race,&race.cars[0]);
            if(race.laps_to_run!=0 || !race.cars[0].finished ||
                race.cars[0].finish_position!=1) return 1;
        }
        race.cars[1].lap=1; race.cars[1].selected_surface=17;
        advance_lap_checkpoints(&race,&race.cars[1]);
        if(!race.cars[1].finished || race.cars[1].finish_position!=2) return 1;
    }
    puts("Race runtime: six modes, 1800 countdown frames, expiry/lap crossing passed");
    /* The winner's lap adjustment is observable even for inactive slots.
     * A lapped entrant starts at -3 here, so next crossing takes rank 3,
     * not merely the second increment of a finish counter. */
    memset(&race,0,sizeof race);
    race.participation_ready=1;
    race.participation[0]=race.participation[1]=race.participation[2]=1;
    race.cars[0].lap=5; race.cars[1].lap=2; race.cars[2].lap=4;
    if(assign_race_finish(&race,0)!=1 || race.finish_ranks[1]!=-3 ||
       race.finish_ranks[2]!=-2 || race.finish_ranks[3]!=-3) return 1;
    if(assign_race_finish(&race,1)!=3 || race.cars[1].finish_position!=3 ||
       race.finished_count!=2) return 1;
    if(assign_race_finish(&race,2)!=2 || race.finished_count!=3 ||
       race.cars[3].finished) return 1;
    puts("Native finish ranking: lapped entrants take original ranks 1,3,2; inactive slot excluded from finish count");
    for(unsigned finished=0;finished<2;++finished)
    for(unsigned remaining=269;remaining<=271;++remaining) {
        memset(&race,0,sizeof race);
        race.game_clock_ticks=1000; race.finish_deadline=1000+remaining;
        race.cars[0].finished=finished;
        race.driver_controls[0]=race.cars[0].ai_control_latch=race.controls=15;
        unsigned suppressed=finished || remaining<270;
        if(apply_finish_gate(&race,0,15)!=(suppressed?0:15) ||
            race.driver_controls[0]!=(suppressed?12:15) ||
            race.cars[0].ai_control_latch!=(suppressed?12:15) || race.race_complete) return 1;
        race.game_clock_ticks=race.finish_deadline;
        if(apply_finish_gate(&race,0,15) || race.race_complete) return 1;
        ++race.game_clock_ticks;
        if(apply_finish_gate(&race,0,15) || !race.race_complete) return 1;
    }
    puts("Finish runtime: suppression threshold, cleared drive latches and strict expiry passed");
    /* Integration guard: buy real weapons/ammunition through the translated
     * shop, then apply input at the runtime's finish/special-state boundary.
     * Zero elapsed physics isolates dispatch from unrelated track movement. */
    unsigned weapon_gates=0;
    for(unsigned driver=0;driver<4;++driver) for(unsigned weapon=0;weapon<8;++weapon)
    for(unsigned finished=0;finished<2;++finished) for(int special=-1;special<=1;++special)
    for(unsigned remaining=269;remaining<=271;++remaining) {
        memset(&race,0,sizeof race);
        race.participation_ready=1;race.participation[driver]=-1;
        race.game_clock_ticks=1000;race.finish_deadline=1000+remaining;
        race.weapons.ready=1;race.weapons_enabled=1;
        race.weapons.rules=slicks_original_weapon_rules;
        initialize_weapon_actors(&race);
        struct SlicksRaceOptions options={.weapons_enabled=1};
        short cash=30000;
        short *inventory=race.weapon_inventory[driver];
        if(!slicks_shop_buy(&slicks_original_shop_rules,&options,inventory,&cash,-1,6,weapon+5,1) ||
           !slicks_shop_buy(&slicks_original_shop_rules,&options,inventory,&cash,-1,6,weapon+5,1)) return 1;
        short before=inventory[weapon+5];
        race.selected_weapon[driver]=(signed char)weapon;
        race.driver_controls[driver]=SLICKS_CONTROL_BRAKE;
        race.cars[driver].finished=finished;
        race.cars[driver].special_drive_state=(short)special;
        /* A stale AI request must not bypass the outer caller gate either. */
        race.weapons.controls[driver].request=1;
        prepare_car_motion(&race,(unsigned short)driver,0);
        unsigned suppressed=finished || special || remaining<270;
        if(inventory[weapon+5]!=before-(suppressed?0:1) ||
           race.weapons.shots!=(suppressed?0:(unsigned)slicks_original_weapon_rules.shots[weapon])) {
            fprintf(stderr,"Weapon caller gate failed driver=%u weapon=%u finished=%u special=%d remaining=%u\n",
                driver,weapon,finished,special,remaining);return 1;
        }
        /* Inactive dispatch must ignore held fire and retained inventory. */
        race.participation[driver]=0;race.weapons.shots=0;
        memset(race.weapons.projectiles,0,sizeof race.weapons.projectiles);
        before=inventory[weapon+5];race.weapons.controls[driver].request=1;
        update_cars(&race,0);
        if(inventory[weapon+5]!=before || race.weapons.shots) return 1;
        ++weapon_gates;
    }
    printf("Weapon runtime caller: %u bought-inventory finish/special/expiry and inactive dispatch gates pass\n",weapon_gates);
    memset(&race,0,sizeof race);
    race.track_reward=award_track; race.started=1; race.race_complete=1;
    race.game_clock_ticks=12345; race.frame_count=567; race.sound_event_count=3;
    slicks_race_award_track(&race); slicks_race_award_track(&race);
    if(track_awards!=1 || !race.track_rewarded) return 1;
    static struct SlicksRaceRuntime frozen;
    frozen=race; frozen.sound_event_count=0;
    unsigned char unused_pixel=0;
    for(unsigned i=0;i<100;++i) slicks_race_step(&race,&unused_pixel);
    if(memcmp(&race,&frozen,sizeof race) || track_awards!=1) return 1;
    puts("Completed race: track award once; 100 further steps preserve all race state and clear stale sound events");
    return 0;
}
