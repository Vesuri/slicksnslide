#include <stdio.h>
#include <string.h>

#include "../src/game/race_runtime.h"

static int check(int condition, const char *message)
{
    if (condition)
        return 1;
    fprintf(stderr, "car collision: %s\n", message);
    return 0;
}

int main(void)
{
    struct SlicksRaceRuntime race;
    struct SlicksRaceCar *current;
    struct SlicksRaceCar *other;
    int ok = 1;

    memset(&race, 0, sizeof(race));
    current = &race.cars[3];
    other = &race.cars[2];
    current->vehicle = 0;
    other->vehicle = 1;
    race.properties[0].collision_radius = 2;
    race.properties[0].collision_weight = 18;
    race.properties[1].collision_weight = 20;
    current->x = 20335;
    current->y = 14683;
    current->velocity_x = -860;
    current->velocity_y = 960;
    other->x = 20235;
    other->y = 14683;
    other->velocity_x = -883;
    other->velocity_y = 250;

    other->actor_layer = 1;
    slicks_race_resolve_car_collisions(&race, 3);
    ok &= check(current->velocity_x == -860 && current->velocity_y == 960 &&
                other->velocity_x == -883 && other->velocity_y == 250 &&
                !current->actor_contact && !other->actor_contact &&
                race.collision_count == 0,
                "cars on different bridge layers must not collide");
    other->actor_layer = 0;
    {
        /* Disabling the original car-pair gate must leave even existing
         * contact latches alone. Terrain collision code is independent. */
        struct SlicksRaceRuntime before;
        race.car_collisions_disabled=1;
        current->touching_car=1; current->actor_contact=1;
        before=race;
        for(unsigned driver=0;driver<4;++driver)
            slicks_race_resolve_car_collisions(&race,(unsigned short)driver);
        ok &= check(!memcmp(&race,&before,sizeof race),"disabled collisions changed race state");
        race.car_collisions_disabled=0;
        current->touching_car=0; current->actor_contact=0;
    }
    {
        signed char roles[4]={0,0,0,1};
        ok &= check(!slicks_race_set_participation(&race,roles),"participant setup failed");
        slicks_race_resolve_car_collisions(&race,3);
        ok &= check(current->velocity_x==-860 && other->velocity_x==-883 && !race.collision_count,
                    "inactive partner received a collision impulse");
        roles[2]=1; roles[3]=0;
        ok &= check(!slicks_race_set_participation(&race,roles),"participant replacement failed");
        slicks_race_resolve_car_collisions(&race,3);
        ok &= check(current->velocity_x==-860 && other->velocity_x==-883 && !race.collision_count,
                    "inactive current car applied a collision impulse");
        roles[3]=1;
        ok &= check(!slicks_race_set_participation(&race,roles),"active pair setup failed");
    }
    /* Finished entrants remain active collision participants. Their
     * response must equal the same original unequal-weight fixture. */
    current->finished = other->finished = 1;
    slicks_race_resolve_car_collisions(&race, 3);
    ok &= check(current->velocity_x == -885 &&
                current->velocity_y == 172,
                "unequal-weight current velocity differs from DOS oracle");
    ok &= check(other->velocity_x == -863 && other->velocity_y == 889,
                "unequal-weight other velocity differs from DOS oracle");
    ok &= check(current->collision_impact == 81 &&
                other->collision_impact == 65 && race.collision_impact == 81,
                "secondary impact magnitudes differ from DOS oracle");
    ok &= check(current->x == 20335 && current->y == 14683 &&
                other->x == 20235 && other->y == 14683,
                "contact must not separate car positions");
    ok &= check(current->touching_car && other->touching_car &&
                race.collision_count == 1,
                "first contact must latch both cars exactly once");
    ok &= check(current->actor_contact && other->actor_contact,
                "new collision must set both current-update contact flags");
    ok &= check(race.sound_event_count == 0,
                "pair resolver must leave sound dispatch to each car update tail");
    ok &= check(current->collision_partner == 2,
                "collision burst scratch must retain the scanned partner index");
    current->actor_contact = 0;
    other->actor_contact = 0;

    slicks_race_resolve_car_collisions(&race, 3);
    ok &= check(current->velocity_x == -885 &&
                current->velocity_y == 172 &&
                other->velocity_x == -863 && other->velocity_y == 889 &&
                race.collision_count == 1,
                "latched contact must suppress a repeated impulse");
    ok &= check(!current->actor_contact && !other->actor_contact,
                "latched overlap must not reassert current-update contact");

    current->x = 30000;
    current->y = 17000;
    slicks_race_resolve_car_collisions(&race, 3);
    ok &= check(!current->touching_car && other->touching_car,
                "only the scanned car clears its latch after separation");

    if (!ok)
        return 1;
    puts("car collision: DOS fixed-point oracle matched");
    return 0;
}
