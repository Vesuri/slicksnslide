#ifndef SLICKS_RACE_RETURN_H
#define SLICKS_RACE_RETURN_H
#include "race_runtime.h"

/* Original 1c10b..1c24a, reached by nonzero-demo return at 259de.
 * Do not clear the whole race: positions, current/total clocks, inventory,
 * profile statistics and the random stream are not reset by this routine.
 * The saved-image restore and owner transition are separate caller work. */
static inline void slicks_race_reset_return(struct SlicksRaceRuntime *race)
{
    for(unsigned d=0;d<4;++d) {
        struct SlicksRaceCar *car=&race->cars[d];
        car->pending_damage_impact=0;
        car->damage_turn_sign=0;
        car->service_flags=0;
        car->special_drive_target=car->special_drive_state=0;
        car->speed_fixed=0; car->speed=0;
        car->last_lap_time_units=0; car->last_lap_centiseconds=0;
        car->checkpoint=0;
        for(unsigned j=0;j<4;++j) car->damage[j]=0;
        signed char role=race->participation_ready?race->participation[d]:
            (!d && race->human_control?-1:1);
        race->finish_ranks[d]=role?-1:0;
        car->finished=car->finish_position=0;
        car->ai_contact_threshold=350;
        car->old_x=car->old_y=0;
        car->velocity_x=car->velocity_y=0;
        car->ai_contact_ticks=60000;
        car->lap=1; /* Native lap = original DS:4bfe + 1. */
        car->previous_actor_contact=car->actor_contact=0;
        car->best_lap_time_units=30000;
        car->best_lap_centiseconds=9999; /* Original low-word formatter clamps. */
        car->oil_turn_sign=0;
        car->steering_scale=1000;
        car->touching_car=car->forward_drive_latch=0;
        car->maximum_speed=100;
        car->collision_sampling=0;
        race->car_display[d].direction=-1;
        race->car_display[d].ticks=0;
    }
    race->finish_ranks_ready=1;
    race->finished_count=0;
    race->boundary_level=5;
}
#endif
