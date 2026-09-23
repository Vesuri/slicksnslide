#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Keep this focused oracle in the same translation unit so it can exercise
 * the recovered wheel dispatcher without widening the production API. */
#include "../src/game/race_runtime.c"

static void fail(const char *message)
{
    fprintf(stderr, "surface effects: %s\n", message);
    exit(1);
}

static void prepare(struct SlicksRaceRuntime *race,
                    struct SlicksRaceCar *car, unsigned char surface,
                    long velocity_x, long velocity_y)
{
    memset(race, 0, sizeof(*race));
    memset(car, 0, sizeof(*car));
    memset(race->surface_map, surface, sizeof(race->surface_map));
    race->random_state = 0x1fadec20UL;
    car->x = 10000;
    car->y = 10000;
    car->velocity_x = velocity_x;
    car->velocity_y = velocity_y;
}

static void expect_particle(const struct SlicksTrailParticle *particle,
                            short x, short y, unsigned char colour,
                            unsigned char priority, short velocity_x,
                            short velocity_y, unsigned char lifetime)
{
    if (particle->x != (long)x * 64L ||
        particle->y != (long)y * 64L ||
        particle->colour != colour || particle->priority != priority ||
        particle->velocity_x != velocity_x ||
        particle->velocity_y != velocity_y ||
        particle->lifetime != lifetime)
        fail("particle tuple differs from DOS instruction oracle");
}

int main(void)
{
    struct SlicksRaceRuntime race;
    struct SlicksRaceCar car;

    prepare(&race, &car, 0, 1000, 0);
    emit_wheel_surface(&race, &car, 1, SLICKS_CONTROL_ACCELERATE);
    if (race.trail_particle_count != 4 || race.random_state != 0x09f9dc4eUL) {
        fprintf(stderr, "road count=%u random=%08lx\n",
                race.trail_particle_count, race.random_state);
        fail("road actor count or random state");
    }
    expect_particle(&race.trail_particles[0], 99, 98, 218, 6, -11, 3, 5);
    expect_particle(&race.trail_particles[1], 99, 98, 71, 0, 0, 0, 3);
    expect_particle(&race.trail_particles[2], 99, 101, 218, 6, -4, 3, 16);
    expect_particle(&race.trail_particles[3], 99, 101, 70, 0, 0, 0, 3);

    prepare(&race, &car, 0, 1760, 0);
    emit_wheel_surface(&race, &car, 1, SLICKS_CONTROL_ACCELERATE);
    if (race.trail_particle_count != 0 ||
        race.random_state != 0x1fadec20UL)
        fail("road acceleration threshold must be strict");

    prepare(&race, &car, 0, 352, 0);
    emit_wheel_surface(&race, &car, 1, SLICKS_CONTROL_BRAKE);
    if (race.trail_particle_count != 4 || race.random_state != 0x09f9dc4eUL)
        fail("road brake threshold must be inclusive");

    prepare(&race, &car, 5, 600, 0);
    emit_wheel_surface(&race, &car, 1, SLICKS_CONTROL_ACCELERATE);
    if (race.trail_particle_count != 4 || race.random_state != 0xc393ca14UL)
        fail("class-5 actor count or random state");
    expect_particle(&race.trail_particles[0], 98, 97, 62, 0, 0, 0, 3);
    expect_particle(&race.trail_particles[1], 99, 98, 62, 5, 3, -4, 21);
    expect_particle(&race.trail_particles[2], 98, 101, 63, 0, 0, 0, 3);
    expect_particle(&race.trail_particles[3], 99, 101, 63, 5, 9, 1, 15);

    prepare(&race, &car, 11, 600, 0);
    emit_wheel_surface(&race, &car, 1, SLICKS_CONTROL_ACCELERATE);
    if (race.trail_particle_count != 4 || race.random_state != 0x611c93aaUL)
        fail("class-11 actor count or random state");
    expect_particle(&race.trail_particles[0], 98, 97, 65, 0, 0, 0, 43);
    expect_particle(&race.trail_particles[1], 99, 98, 65, 5, -4, 6, 21);
    expect_particle(&race.trail_particles[2], 99, 100, 65, 0, 0, 0, 48);
    expect_particle(&race.trail_particles[3], 99, 101, 65, 5, -5, 6, 20);

    prepare(&race, &car, 7, 602, 0);
    emit_wheel_surface(&race, &car, 1, SLICKS_CONTROL_ACCELERATE);
    if (race.trail_particle_count != 2 || race.random_state != 0xd5e076dcUL)
        fail("class-7 actor count or random state");
    expect_particle(&race.trail_particles[0], 99, 98, 55, 3, -3, -10, 20);
    expect_particle(&race.trail_particles[1], 99, 101, 55, 3, -10, 3, 20);

    puts("surface effects: DOS dispatch, actor tuples, and RNG matched");
    return 0;
}
