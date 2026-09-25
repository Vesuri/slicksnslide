#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Keep the arithmetic oracle in this translation unit so it can exercise the
 * internal Q15 helper without widening the production ABI. */
#include "../src/game/race_runtime.c"

static unsigned active_driver_count(const struct SlicksRaceRuntime *race)
{
    unsigned count=0;
    for(unsigned driver=0;driver<4;++driver) count+=driver_role(race,driver)!=0;
    return count;
}

/* Countdown-only step fixture: entering target assembly is a test failure. */
unsigned short slicks_advance_particles(struct SlicksTrailParticle *particles,
    unsigned long count, unsigned char indices[SLICKS_TRAIL_PRIORITY_COUNT][SLICKS_TRAIL_PARTICLE_MAX],
    unsigned short counts[SLICKS_TRAIL_PRIORITY_COUNT],
    struct SlicksDirtyPixel *dirty_pixels, unsigned short *dirty_count,
    unsigned char *chunky, unsigned long actor_page)
{
    (void)particles; (void)count; (void)indices; (void)counts;
    (void)dirty_pixels; (void)dirty_count; (void)chunky; (void)actor_page;
    abort();
}

/* Single-car fixtures intentionally isolate one driver; production uses
 * update_cars so all motion precedes every per-driver tail. */
static void update_car(struct SlicksRaceRuntime *race, unsigned short index,
                       unsigned short ticks)
{
    race->cars[index].position_scale=100; /* Fixture's original profile setting. */
    unsigned char controls=prepare_car_motion(race,index,ticks);
    finish_car_update(race,index,ticks,controls);
}

static void expect(long actual, long expected, const char *message)
{
    if (actual == expected)
        return;
    fprintf(stderr, "drive physics: %s: got %ld, expected %ld\n",
            message, actual, expected);
    exit(1);
}

int main(void)
{
    for(unsigned combination=0;combination<81;++combination) {
        static struct SlicksRaceRuntime race;
        memset(&race,0,sizeof race);
        signed char roles[4]; unsigned digits=combination,active=0;
        for(unsigned i=0;i<4;++i) {
            roles[i]=(signed char)((int)(digits%3)-1); digits/=3;
            active+=roles[i]!=0;
            race.cars[i].x=5000+i*5000; race.cars[i].y=8000;
            race.cars[i].special_drive_state=-1;
            race.cars[i].position_scale=100;
            race.cars[i].vehicle=0;
        }
        int result=slicks_race_set_participation(&race,roles);
        if(!active) { expect(result,-1,"empty participant guard"); continue; }
        expect(result,0,"participant setup");
        expect(active_driver_count(&race),active,"active participant count");
        struct SlicksRaceCar before[4]; memcpy(before,race.cars,sizeof before);
        update_cars(&race,0);
        for(unsigned i=0;i<4;++i) {
            if(!roles[i] && memcmp(&before[i],&race.cars[i],sizeof before[i])) {
                fputs("Inactive car changed during update passes\n",stderr); exit(1);
            }
            if(roles[i]) expect(race.cars[i].steering_amount,roles[i]<0?100:140,"selected role steering");
        }
        race.started=1;
        expect(slicks_race_set_participation(&race,roles),-1,"live participation mutation rejected");
    }
    for(unsigned scale=0;scale<256;++scale)
    for(unsigned human=0;human<2;++human) {
        static struct SlicksRaceRuntime race;
        memset(&race,0,sizeof race);
        race.human_control=human;
        race.cars[0].position_scale=(unsigned char)scale;
        race.cars[0].special_drive_state=-1;
        /* Zero ticks isolates the production profile-to-steering handoff
         * from movement; unlike update_car(), do not force normal scale. */
        (void)prepare_car_motion(&race,0,0);
        expect(race.cars[0].steering_amount,human?scale:scale*7/5,
               "profile steering word handoff");
    }
    for(unsigned mask=1;mask<16;++mask) {
        static struct SlicksRaceRuntime race;
        static unsigned char logical[0x40000],chunky[128000];
        struct SlicksTrackNavigation navigation={0};
        navigation.zone_count=1; navigation.start_x=100; navigation.start_y=80;
        slicks_race_initialize(&race,&navigation);
        signed char roles[4];
        for(unsigned i=0;i<4;++i) roles[i]=(mask&(1U<<i))?(i&1?-1:1):0;
        expect(slicks_race_set_participation(&race,roles),0,"render participant setup");
        race.font.ready=1;
        for(unsigned v=0;v<SLICKS_VEHICLE_COUNT;++v) {
            race.properties[v].ready=1;
            for(unsigned d=0;d<SLICKS_CAR_BASE_DIRECTIONS;++d) {
                race.sprites[v][d].ready=1;
                race.sprites[v][d].width=race.sprites[v][d].height=1;
                race.sprites[v][d].pixels[0]=7;
            }
        }
        for(unsigned i=0;i<SLICKS_START_LIGHT_COUNT;++i) race.start_lights[i].ready=1;
        memset(logical,0,sizeof logical); memset(chunky,0,sizeof chunky);
        expect(slicks_race_start(&race,logical,chunky),0,"participant grid start");
        for(unsigned i=0;i<4;++i)
            expect(race.cars[i].saved_valid,roles[i]!=0,"inactive car absent from grid");
        slicks_race_step(&race,logical);
        for(unsigned i=0;i<4;++i) {
            expect(race.cars[i].elapsed_time_units!=0,roles[i]!=0,"active countdown clock");
            if(!roles[i]) {
                struct SlicksStatusRect rects[3];
                race.fuel_option=10;
                expect(slicks_race_status_rects(&race,i,0,rects),0,"inactive HUD skips zero fuel divisor");
            }
        }
        race.fuel_option=0;
        unsigned finished=0;
        for(unsigned i=0;i<4;++i) if(roles[i]) {
            race.cars[i].lap=4; race.cars[i].selected_surface=17;
            advance_lap_checkpoints(&race,&race.cars[i]);
            ++finished;
            expect(race.race_complete,0,"finish waits for original grace deadline");
            expect(race.finish_deadline!=0,finished==active_driver_count(&race),"deadline excludes absent entrants");
        }
        race.game_clock_ticks=race.finish_deadline;
        (void)apply_finish_gate(&race,0,15);
        expect(race.race_complete,0,"deadline comparison is strict");
        ++race.game_clock_ticks;
        (void)apply_finish_gate(&race,0,15);
        expect(race.race_complete,1,"race ends after grace deadline");
    }
    {
        static struct SlicksRaceRuntime race;
        struct SlicksRaceCar *car=&race.cars[0];
        race.human_control=1; race.controls=SLICKS_CONTROL_ACCELERATE;
        car->finished=1; car->finish_position=1;
        car->x=car->y=10000; car->speed_fixed=10000; car->maximum_speed=100;
        car->drive_coefficients[0]=car->drive_coefficients[3]=100;
        car->elapsed_time_units=1234; car->lap=5;
        update_car(&race,0,1);
        expect(car->speed_fixed,7000,"finished car throttle is capped, not suppressed");
        if(car->x<=10000) { fputs("finished car failed to keep moving\n",stderr); return 1; }
        expect(car->elapsed_time_units,1234,"finished time remains frozen while driving");
        expect(car->lap,5,"finished driving does not add laps");
    }
    {
        static struct SlicksRaceRuntime actual,expected,interleaved;
        actual.properties[0].collision_radius=8;
        actual.properties[0].collision_weight=21;
        for(unsigned i=0;i<4;++i) {
            struct SlicksRaceCar *car=&actual.cars[i];
            car->position_scale=100;
            car->x=i==0?10000:i==1?10480:20000+i*1000;
            car->y=10000; car->ai_last_x=car->x; car->ai_last_y=car->y;
            car->velocity_x=i==0?960:i==1?-960:0;
            car->ai_state=3; car->ai_stuck_ticks=700;
            car->actor_layer=1;
            car->drive_coefficients[0]=car->drive_coefficients[3]=100;
        }
        expected=actual; interleaved=actual;
        unsigned char controls[4];
        for(unsigned i=0;i<4;++i) controls[i]=prepare_car_motion(&expected,i,1);
        for(unsigned i=0;i<4;++i) finish_car_update(&expected,i,1,controls[i]);
        update_cars(&actual,1);
        for(unsigned i=0;i<4;++i) update_car(&interleaved,i,1);
        if(memcmp(&actual,&expected,sizeof actual)) {
            fputs("driver phases: production differs from all-motion/all-tail ordering\n",stderr);
            return 1;
        }
        if(!memcmp(actual.cars,interleaved.cars,sizeof actual.cars)) {
            fputs("driver phases: collision fixture does not distinguish old ordering\n",stderr);
            return 1;
        }
        puts("Native driver phases: approaching-car fixture distinguishes and rejects interleaved ordering");
    }
    {
        static struct SlicksRaceRuntime race;
        unsigned long long total = 0;
        for (unsigned long frame = 1; frame <= 1000000; ++frame) {
            unsigned short ticks = next_physics_ticks(&race);
            if (ticks < 1 || ticks > 2)
                expect(ticks, 1, "PAL tick batch range");
            total += ticks;
            expect((long)total, (long)(1193182ULL * frame / 655350ULL),
                   "rational PIT tick total");
            expect(race.physics_tick_phase,
                   (long)(1193182ULL * frame % 655350ULL), "PIT phase remainder");
        }
    }
    /* Factor seven is 7ffch at zero bias. */
    expect(multiply_q15_unsigned(-860, 0x7ffc), -860,
           "negative special-state velocity");
    expect(multiply_q15_unsigned(960, 0x7ffc), 959,
           "positive special-state velocity");

    /* A -30 driver bias produces 801ah. It remains an unsigned factor, as
     * proved by the forced DOS fixture, rather than becoming -32742. */
    expect(multiply_q15_unsigned(-860, 0x801a), -861,
           "unsigned factor above 7fffh");
    expect(multiply_q15_unsigned(960, 0x801a), 960,
           "unsigned positive factor");

    /* The DOS helper keeps the low product dword before SAR 15. */
    expect(multiply_q15_unsigned(-123456, 0x7ffc), 7631,
           "wrapped signed 32-bit product");
    expect(multiply_q15_unsigned(78901, 0x7ffc), -52181,
           "wrapped positive 32-bit product");

    {
        static struct SlicksRaceRuntime race;
        struct SlicksRaceCar *car = &race.cars[0];
        race.human_control = 1;
        car->x = car->y = 10000;
        car->velocity_x = 960;
        car->velocity_y = -860;
        car->special_drive_state = 1;
        car->speed_fixed = 10000;
        update_car(&race, 0, 2);
        expect(car->velocity_x, 958, "two special integrations");
        expect(car->velocity_y, -860, "two signed special integrations");
        expect(car->x, 10094, "position after each integration");
        expect(car->y, 9914, "negative position after each integration");
        expect(car->speed_fixed, 10000, "special branch bypasses coast");
        expect(car->elapsed_centiseconds, 2, "timer advances once per batch");
        memset(&race, 0, sizeof race);
        race.human_control = 1;
        car->x = 31700; car->y = 10000;
        car->velocity_x = 1000; car->velocity_y = -860;
        car->special_drive_state = 1;
        update_car(&race, 0, 1);
        expect(car->x, 31700, "right boundary clamps after movement");
        expect(car->y, 9957, "boundary does not undo other-axis movement");
        expect(car->velocity_x, 982, "boundary damping after special integration");
        expect(car->velocity_y, -846, "right boundary also damps vertical velocity");
        memset(&race, 0, sizeof race);
        race.human_control = 1;
        car->x = car->y = 10000;
        car->speed_fixed = 10000;
        car->drive_coefficients[0] = 100;
        car->drive_coefficients[3] = 100;
        update_car(&race, 0, 2);
        /* DOS signed divides: coast 10000->9676->9363; force 84 then 81;
         * second velocity includes trunc(84*31703/32768)=81. */
        expect(car->speed_fixed, 9363, "coast decay on each tick");
        expect(car->velocity_x, 162, "normal force repeated on each tick");
        expect(car->x, 10012, "normal positions use intermediate velocity");
        expect(car->elapsed_centiseconds, 2, "normal timer once per batch");
        memset(&race, 0, sizeof race);
        race.human_control = 1;
        car->x = car->y = 10000;
        car->speed_fixed = 10000;
        car->drive_coefficients[0] = 100;
        car->drive_coefficients[3] = 100;
        update_car(&race, 0, 1);
        expect(car->speed_fixed, 9676, "single-tick coast decay");
        expect(car->velocity_x, 84, "single-tick force");
        expect(car->x, 10004, "single-tick position");
        expect(car->elapsed_time_units, 2, "single tick accumulates two DOS units");
        expect(car->elapsed_centiseconds, 1, "DOS two-unit display truncation");
    }
    /* DOS 2000:0cd6/0d4c multiplies AFTER the last signed divide.
     * This fixture rounds 79*51/50 down to 80 before tick scaling. */
    for (unsigned ticks = 1; ticks <= 45; ++ticks) {
        for (unsigned keys = 0; keys < 4; ++keys) {
            static struct SlicksRaceRuntime race;
            struct SlicksRaceCar *car = &race.cars[0];
            memset(&race, 0, sizeof race);
            race.human_control = 1;
            race.controls = ((keys & 1) ? SLICKS_CONTROL_LEFT : 0) |
                            ((keys & 2) ? SLICKS_CONTROL_RIGHT : 0);
            car->x = car->y = 10000;
            car->steering_scale = 1550;
            car->damage[3] = 26;
            car->steering_property = 51;
            car->heading = (keys == 2) ? SLICKS_HEADING_FULL - 10 : 10;
            long expected = car->heading;
            if (keys & 1) expected -= 80 * ticks;
            if (keys & 2) expected += 80 * ticks;
            expected = (expected + SLICKS_HEADING_FULL) % SLICKS_HEADING_FULL;
            update_car(&race, 0, ticks);
            expect(car->heading, expected, "elapsed-tick steering and wrap");
        }
    }
    {
        static const short damage[] = {0,69,70,999};
        static const long coast_force[] = {84,84,76,35};
        static const long drive_force[] = {53,53,48,22};
        for (unsigned at=0; at<4; ++at) {
            for (unsigned powered=0; powered<2; ++powered) {
                static struct SlicksRaceRuntime race;
                struct SlicksRaceCar *car=&race.cars[0];
                memset(&race,0,sizeof race);
                race.human_control=1;
                race.controls=powered ? SLICKS_CONTROL_ACCELERATE : 0;
                car->x=car->y=10000;
                car->speed_fixed=10000;
                car->maximum_speed=200;
                car->drive_coefficients[0]=100;
                car->drive_coefficients[3]=100;
                car->damage[0]=damage[at];
                update_car(&race,0,1);
                expect(car->velocity_x,powered ? drive_force[at] : coast_force[at],
                       "damage channel zero weakens powered/coasting force");
                expect(car->velocity_y,0,"damage preserves force direction");
            }
        }
    }
    {
        static struct SlicksRaceRuntime race;
        struct SlicksRaceCar *car=&race.cars[0];
        race.human_control=1;
        race.controls=SLICKS_CONTROL_BRAKE;
        race.properties[0].engine_sound=1;
        race.properties[0].property_4=100; /* Tail refreshes the vehicle's speed limit. */
        car->x=car->y=10000;
        car->maximum_speed=100;
        car->drive_coefficients[0]=car->drive_coefficients[3]=100;
        car->forward_drive_latch=1;
        car->measured_speed=99;
        update_car(&race,0,1);
        expect(car->speed_fixed,0,"transition frame brakes, not reverse");
        expect(car->forward_drive_latch,0,"low measured speed arms reverse");
        update_car(&race,0,1);
        expect(car->speed_fixed,-3300,"next held brake selects reverse force");
        expect(car->velocity_x,-17,"reverse force reaches velocity integration");
        race.controls=SLICKS_CONTROL_ACCELERATE;
        update_car(&race,0,1);
        expect(car->forward_drive_latch,1,"throttle restores forward latch");
        expect(car->speed_fixed,-3140,"throttle increments signed reverse scalar");
    }
    for (unsigned previous = 30; previous <= 31; ++previous) {
        for (unsigned moving = 0; moving < 2; ++moving) {
            static struct SlicksRaceRuntime race;
            struct SlicksRaceCar *car = &race.cars[0];
            memset(&race, 0, sizeof race);
            race.human_control = 1;
            car->x = car->y = 10000;
            car->heading = SLICKS_HEADING_FULL - 1;
            car->special_drive_state = 10000;
            car->velocity_x = moving ? 100 : 0;
            car->measured_speed = previous;
            car->damage[2] = 70;
            car->damage_turn_sign = 1;
            update_car(&race, 0, 2);
            long heading = previous > 30 ? 1 : SLICKS_HEADING_FULL - 1;
            expect(car->heading, heading, "yaw uses previous measured speed and wraps");
            expect(car->measured_speed, moving ? 49 : 0,
                   "measured speed refreshed after integration");
            update_car(&race, 0, 2);
            heading = (heading + (moving ? 2 : 0)) % SLICKS_HEADING_FULL;
            expect(car->heading, heading, "next yaw uses refreshed measured speed");
        }
    }
    for (int state=-1; state<=1; state+=2) {
        for (unsigned keys=0; keys<16; ++keys) {
            static struct SlicksRaceRuntime race;
            struct SlicksRaceCar *car=&race.cars[0];
            memset(&race,0,sizeof race);
            race.human_control=1;
            race.controls=((keys&1)?SLICKS_CONTROL_ACCELERATE:0) |
                          ((keys&2)?SLICKS_CONTROL_BRAKE:0) |
                          ((keys&4)?SLICKS_CONTROL_LEFT:0) |
                          ((keys&8)?SLICKS_CONTROL_RIGHT:0);
            car->special_drive_state=state;
            car->x=car->y=10000;
            car->velocity_x=960; car->velocity_y=-860;
            car->speed_fixed=10000;
            car->heading=100;
            car->steering_scale=1000; car->steering_property=104;
            car->damage[2]=70; car->damage_turn_sign=1;
            car->measured_speed=31;
            update_car(&race,0,2);
            expect(car->heading,102,"special state bypasses input steering, retains yaw");
            expect(car->forward_drive_latch,0,"special state bypasses throttle latch");
            expect(car->speed_fixed,state>0?10000:9363,"signed special-state drive branch");
            expect(car->velocity_x,state>0?958:960,"negative special state bypasses damping");
            expect(car->velocity_y,-860,"special signed velocity");
            expect(car->x,state>0?10094:10096,"special-state position integration");
        }
    }
    {
        static struct SlicksRaceRuntime race;
        race.human_control=1;
        race.cars[0].special_drive_state=-1;
        race.cars[0].x=race.cars[0].y=10000;
        race.cars[0].speed_fixed=-10000;
        update_car(&race,0,1);
        expect(race.cars[0].speed_fixed,-9677,"negative special coast uses arithmetic shift");
    }
    {
        struct SlicksRaceCar car={0};
        static const short expected[]={266,532,798,532,266,0};
        car.special_drive_target=700;
        for(unsigned at=0;at<6;++at) {
            advance_special_state(&car,1);
            expect(car.special_drive_state,expected[at],"special rise, overshoot and decay");
            expect(car.special_drive_target,at<2?700:0,"special target clears at overshoot");
        }
    }
    for (unsigned surface=13; surface<=14; ++surface) {
        static struct SlicksRaceRuntime race;
        struct SlicksRaceCar *car=&race.cars[0];
        memset(&race,0,sizeof race);
        memset(race.material_map,surface,sizeof race.material_map);
        race.human_control=1;
        race.properties[0].model_class=30;
        car->x=car->y=10000;
        car->velocity_x=1000;
        car->drive_coefficients[0]=car->drive_coefficients[3]=100;
        update_car(&race,0,1);
        expect(car->effective_surface,surface,"jump uses selected track surface");
        expect(car->special_drive_state,266,"surface trigger enters lifecycle same update");
        expect(car->special_drive_target,surface==13?720:480,"surface-specific target divisor");
        expect(car->velocity_x,surface==13?967:483,"surface 14 halves post-collision velocity");
        expect(race.sound_event_totals[7],1,"jump requests sample block seven");
        expect(race.sound_events[0].sample_block,7,"jump sound queued");
        expect(race.sound_events[0].flags,2,"jump sound duplicate suppression");
        expect(race.sound_events[0].priority,12,"jump sound priority");
        update_car(&race,0,1);
        expect(car->effective_surface,0,"active special state suppresses retrigger");
        expect(car->special_drive_state,532,"triggered state advances next update");
        expect(car->special_drive_target,surface==13?720:0,"target clears on overshoot");
        expect(race.sound_event_totals[7],1,"airborne update does not repeat jump sound");
    }
    for (unsigned hits = 0; hits < 4; ++hits) {
        static struct SlicksRaceRuntime race;
        struct SlicksRaceCar *car = &race.cars[0];
        memset(&race, 0, sizeof race);
        car->x = car->y = 10000;
        record_track_contact(&race, car, hits & 1);
        record_track_contact(&race, car, hits & 2);
        expect(car->actor_contact, hits != 0, "contact latch survives clear substep");
        expect(car->touching_solid, (hits & 2) != 0, "last-substep contact distinct from latch");
        expect(race.track_collision_count, hits != 0, "adjacent contact counting");
        expect(race.sound_event_count, 0, "substeps do not emit contact sound early");
        emit_contact_sound(&race, car);
        expect(race.sound_event_count, hits != 0, "one update-wide contact sound");
        if (hits) {
            expect(race.sound_events[0].sample_block, 5, "wall impact uses sample five");
            car->previous_actor_contact = car->actor_contact;
            emit_contact_sound(&race, car);
            expect(race.sound_event_count, 1, "persistent contact suppresses repeat sound");
        }
        update_actor_layer(&race, car);
        expect(car->actor_layer, hits == 0, "any contact suppresses layer entry");
        car->actor_contact = 0;
        record_track_contact(&race, car, 0);
        update_actor_layer(&race, car);
        expect(car->actor_layer, 1, "cleared next batch permits layer entry");
    }
    puts("drive physics: PIT schedule, Q15, integration, 180 steering batches, yaw timing and contact latch matched");
    return 0;
}
