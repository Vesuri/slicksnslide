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

    slicks_race_resolve_car_collisions(&race, 3);
    ok &= check(current->velocity_x == -885 &&
                current->velocity_y == 172 &&
                other->velocity_x == -863 && other->velocity_y == 889 &&
                race.collision_count == 1,
                "latched contact must suppress a repeated impulse");

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
