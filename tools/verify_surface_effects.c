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
    car->actor_layer = 1;
    car->forward_drive_latch = 1;
    race->properties[0].effect_profile = 88;
    race->sprites[0][0].wheel_x[0][0]=2;
    race->sprites[0][0].wheel_x[0][1]=2;
    race->sprites[0][0].wheel_y[0][0]=1;
    race->sprites[0][0].wheel_y[0][1]=4;
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
    static unsigned char pixels[SLICKS_SCREEN_WIDTH * SLICKS_SCREEN_HEIGHT];
    static unsigned char logical[4 * SLICKS_PLANE_SIZE];

    /* Branch fixtures from 2000:29d2..2a61, including nonzero negative
     * special state and the entry-then-exit ordering for surface 19.
     * The full original-code layer oracle proves last-wall does not gate entry. */
    {
        static const short cases[][7] = {
            {0,0,0,0,0,0,1}, {0,1,0,0,0,0,0},
            {0,0,0,0,1,0,1}, {0,0,0,0,0,1,0},
            {0,0,19,0,0,0,0}, {1,31,0,0,0,0,1},
            {1,0,19,0,0,0,0}, {1,0,0,1,0,0,0},
            {1,0,0,-1,0,0,0}, {0,0,0,-1,0,0,0},
            {0,19,0,0,0,0,0}, {1,19,0,0,0,0,1}
        };
        for (unsigned int i=0; i<sizeof cases/sizeof *cases; ++i) {
            prepare(&race, &car, 0, 0, 0);
            car.actor_layer = cases[i][0];
            race.material_map[32100] = cases[i][1];
            race.surface_map[32100] = cases[i][2];
            car.special_drive_state = cases[i][3];
            car.touching_solid = cases[i][4];
            car.actor_contact = cases[i][5];
            update_actor_layer(&race, &car);
            if (car.actor_layer != cases[i][6])
                fail("bridge actor layer transition");
        }
    }
    for (unsigned int raw=0; raw<256; ++raw) {
        for (unsigned int limit=0; limit<=15; limit+=15) {
            prepare(&race, &car, 0, 0, 0);
            race.chunky = pixels;
            pixels[32100] = 40;
            race.material_map[32100] = raw >> 3;
            race.surface_map[32100] = raw & 7;
            add_trail_component(&race, 100, 100, 71, 0, 0, 0, 3);
            race.trail_particles[0].occlusion_limit = limit;
            draw_trail_particles(&race, 0);
            unsigned char visible = !limit || raw <= limit;
            if (pixels[32100] != (visible ? 71 : 40))
                fail("raw foreground mask comparison");
            restore_trail_particles(&race, 0);
            race.trail_particles[0].lifetime = 1;
            commit_expiring_trails(&race);
            if (pixels[32100] != 40)
                fail("page-zero deferred mark committed early");
            race.actor_page = 1;
            commit_expiring_trails(&race);
            if (pixels[32100] != (visible ? 71 : 40))
                fail("masked permanent mark painted on retirement");
            pixels[32100] = 40;
            race.cars[0] = car;
            race.cars[0].actor_layer = limit ? 1 : 0;
            race.sprites[0][0].width = 1;
            race.sprites[0][0].height = 1;
            race.sprites[0][0].pixels[0] = 1;
            draw_car(&race, 0, 0);
            if (pixels[32100] != (visible ? 1 : 40))
                fail("car sprite foreground masking");
            restore_car(&race, 0, &race.cars[0]);
            if (pixels[32100] != 40)
                fail("masked car saved-under restoration");
        }
    }

    /* Optimization regression: the chunky path must match the retained
     * scalar VGA renderer for every rotation, body ramp and masking mode.
     * This is supplemental to the DOS instruction oracles, not a new oracle. */
    for(unsigned trial=0;trial<256;++trial) {
        static unsigned char expected[64000],saved[100];
        prepare(&race,&car,0,0,0);
        race.chunky=pixels;
        race.cars[0]=car;
        race.cars[0].heading=(trial&15)*SLICKS_HEADING_STEP;
        race.cars[0].style=(trial>>4)&7;
        race.cars[0].actor_layer=trial>>7;
        for(unsigned base=0;base<4;++base) {
            race.sprites[0][base].width=7;race.sprites[0][base].height=9;
            for(unsigned i=0;i<63;++i)
                race.sprites[0][base].pixels[i]=(i+base)%11;
        }
        for(unsigned i=0;i<64000;++i) {
            pixels[i]=(unsigned char)(i*19+trial);
            if(i<sizeof race.material_map) {
                race.material_map[i]=(i+trial)%4;
                race.surface_map[i]=(i+trial)%8;
            }
            write_pixel(logical,0,i%320,i/320,pixels[i]);
        }
        draw_car(&race,logical,0);
        memcpy(expected,pixels,sizeof expected);
        memcpy(saved,race.cars[0].saved_under,63);
        restore_car(&race,logical,&race.cars[0]);
        draw_car(&race,0,0);
        if(memcmp(expected,pixels,sizeof expected) ||
           memcmp(saved,race.cars[0].saved_under,63))
            fail("chunky car draw differs from scalar renderer");
        restore_car(&race,0,&race.cars[0]);
        for(unsigned i=0;i<64000;++i)
            if(pixels[i]!=(unsigned char)(i*19+trial))
                fail("chunky car restoration differs");
    }

    /* Every mixed car-layer combination: priority 4 must cover priority 3,
     * irrespective of player number, and restoration must recover scenery. */
    for (unsigned int test = 0; test < 32; ++test) {
        unsigned int layers = test & 15;
        unsigned char *planes = test & 16 ? logical : 0;
        unsigned int top = 3;
        prepare(&race, &car, 0, 0, 0);
        memset(pixels, 40, sizeof pixels);
        memset(logical, 40, sizeof logical);
        race.chunky = pixels;
        race.sprites[0][0].width = 1;
        race.sprites[0][0].height = 1;
        race.sprites[0][0].pixels[0] = 1;
        for (unsigned int i = 0; i < 4; ++i) {
            race.cars[i] = car;
            race.cars[i].actor_layer = (layers >> i) & 1;
            race.cars[i].style = i;
            if (!race.cars[i].actor_layer) top = i;
        }
        draw_layered_cars(&race, planes);
        if (pixels[32100] != 1 + top * 5 ||
            read_pixel(planes, planes ? 0 : pixels, 100, 100) != 1 + top * 5)
            fail("mixed car layer draw order");
        restore_layered_cars(&race, planes);
        if (pixels[32100] != 40 ||
            read_pixel(planes, planes ? 0 : pixels, 100, 100) != 40)
            fail("mixed car layer restoration order");
    }

    /* A priority-3 point covers cars at priority 3 (lower actor slots),
     * but is covered by any priority-4 car. Check the reverse unwind too. */
    for (unsigned int layers = 0; layers < 16; ++layers) {
        unsigned int expected = 71;
        prepare(&race, &car, 0, 0, 0);
        memset(pixels, 40, sizeof pixels);
        race.chunky = pixels;
        race.sprites[0][0].width = 1;
        race.sprites[0][0].height = 1;
        race.sprites[0][0].pixels[0] = 1;
        for (unsigned int i = 0; i < 4; ++i) {
            race.cars[i] = car;
            race.cars[i].actor_layer = (layers >> i) & 1;
            race.cars[i].style = i;
            if (!race.cars[i].actor_layer) expected = 1 + i * 5;
        }
        add_trail_component(&race, 100, 100, 71, 3, 0, 0, 15);
        draw_layered_cars(&race, 0);
        if (pixels[32100] != expected)
            fail("equal-priority point/car ordering");
        restore_layered_cars(&race, 0);
        if (pixels[32100] != 40)
            fail("equal-priority point/car restoration");
    }

    /* Unwind overlapping actors before committing an expired permanent
     * mark. A surviving transient must save the new background on redraw. */
    prepare(&race, &car, 0, 0, 0);
    memset(pixels, 40, sizeof(pixels));
    race.chunky = pixels;
    add_trail_component(&race, 100, 100, 65, 0, 0, 0, 40);
    add_trail_component(&race, 100, 100, 71, 0, 0, 0, 3);
    if (race.trail_particles[0].permanent ||
        !race.trail_particles[1].permanent)
        fail("DOS permanent/transient actor classification");
    draw_trail_particles(&race, 0);
    restore_trail_particles(&race, 0);
    commit_expiring_trails(&race);
    if (pixels[32100] != 40)
        fail("mark committed before actor expiry");
    draw_trail_particles(&race, 0);
    race.trail_particles[1].lifetime = 1;
    restore_trail_particles(&race, 0);
    commit_expiring_trails(&race);
    if (pixels[32100] != 40)
        fail("page-zero overlapping mark committed early");
    race.actor_page = 1;
    commit_expiring_trails(&race);
    if (pixels[32100] != 71)
        fail("expired skid mark was erased with saved-under");
    /* Remove expired actor from the draw bucket, as the compactor does. */
    race.trail_priority_counts[0] = 1;
    draw_trail_particles(&race, 0);
    restore_trail_particles(&race, 0);
    if (pixels[32100] != 71)
        fail("overlapping transient erased committed skid mark");
    race.trail_priority_counts[0] = 2;
    race.dirty_pixel_count = SLICKS_DIRTY_PIXEL_MAX;
    race.dirty_row_count = 0;
    commit_expiring_trails(&race);
    if (!race.dirty_row_count)
        fail("permanent mark lost dirty coverage at sparse-list capacity");

    /* DOS SAR must clip fractional positions just outside either edge. */
    for (short fraction = -63; fraction < 0; ++fraction) {
        for (unsigned int axis = 0; axis < 2; ++axis) {
            prepare(&race, &car, 0, 0, 0);
            memset(pixels, 40, sizeof(pixels));
            race.chunky = pixels;
            add_trail_component(&race, 0, 0, 218, 6, 0, 0, 5);
            if (axis) race.trail_particles[0].y = fraction;
            else race.trail_particles[0].x = fraction;
            draw_trail_particles(&race, 3);
            if (pixels[0] != 40 || race.dirty_pixel_count ||
                race.trail_particles[0].saved_valid)
                fail("negative fractional particle leaked onto border");
        }
    }

    /* The same pixel has different effects on the two bridge layers. */
    prepare(&race, &car, 5, 600, 0);
    memset(race.material_map, 2, sizeof race.material_map);
    car.actor_layer = 0;
    emit_wheel_surface(&race, &car, 1, SLICKS_CONTROL_ACCELERATE);
    if (race.trail_particle_count || race.random_state != 0x1fadec20UL)
        fail("layer-zero wheel sampling used the layer-one surface");
    for (short special=-1; special<=1; special+=2) {
        prepare(&race, &car, 5, 600, 0);
        car.special_drive_state = special;
        emit_wheel_surface(&race, &car, 1, SLICKS_CONTROL_ACCELERATE);
        if (race.trail_particle_count || race.random_state != 0x1fadec20UL)
            fail("special state emitted wheel effects or consumed RNG");
    }

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

    prepare(&race, &car, 0, 1000, 0);
    race.sprites[0][0].wheel_x[0][1]=-1;
    emit_wheel_surface(&race, &car, 1, SLICKS_CONTROL_ACCELERATE);
    if(race.trail_particle_count!=2)
        fail("missing second wheel emitted a false trail");
    prepare(&race, &car, 0, 1000, 0);
    race.sprites[0][0].wheel_x[0][0]=race.sprites[0][0].wheel_x[0][1]=-1;
    emit_wheel_surface(&race, &car, 1, SLICKS_CONTROL_ACCELERATE);
    if(race.trail_particle_count || race.random_state!=0x1fadec20UL)
        fail("no-wheel vehicle emitted particles or consumed RNG");

    prepare(&race, &car, 0, 1760, 0);
    emit_wheel_surface(&race, &car, 1, SLICKS_CONTROL_ACCELERATE);
    if (race.trail_particle_count != 0 ||
        race.random_state != 0x1fadec20UL)
        fail("road acceleration threshold must be strict");

    /* Driver 1 stays fixed while its selected vehicle changes. Its former
     * slot-based threshold (88) must not override the selected .omi value. */
    prepare(&race, &car, 0, 1760, 0);
    car.vehicle = 9;
    race.properties[9].effect_profile = 100;
    race.sprites[9][0]=race.sprites[0][0];
    emit_wheel_surface(&race, &car, 1, SLICKS_CONTROL_ACCELERATE);
    if (race.trail_particle_count != 4 || race.random_state != 0x09f9dc4eUL)
        fail("road threshold must follow selected vehicle, not driver slot");

    prepare(&race, &car, 0, 354, 0);
    emit_wheel_surface(&race, &car, 1, SLICKS_CONTROL_BRAKE);
    if (race.trail_particle_count != 4 || race.random_state != 0x09f9dc4eUL)
        fail("road braking above threshold must emit");
    prepare(&race, &car, 0, 352, 0);
    emit_wheel_surface(&race, &car, 1, SLICKS_CONTROL_BRAKE);
    if(race.trail_particle_count || race.random_state!=0x1fadec20UL)
        fail("road braking at threshold must not emit");

    prepare(&race, &car, 5, 600, 0);
    emit_wheel_surface(&race, &car, 1, SLICKS_CONTROL_ACCELERATE);
    if (race.trail_particle_count != 4 || race.random_state != 0xc393ca14UL)
        fail("class-5 actor count or random state");
    expect_particle(&race.trail_particles[0], 98, 97, 62, 0, 0, 0, 3);
    expect_particle(&race.trail_particles[1], 99, 98, 62, 5, -4, 3, 21);
    expect_particle(&race.trail_particles[2], 98, 101, 63, 0, 0, 0, 3);
    expect_particle(&race.trail_particles[3], 99, 101, 63, 5, 1, 9, 15);

    prepare(&race, &car, 11, 600, 0);
    emit_wheel_surface(&race, &car, 1, SLICKS_CONTROL_ACCELERATE);
    if (race.trail_particle_count != 4 || race.random_state != 0x611c93aaUL)
        fail("class-11 actor count or random state");
    expect_particle(&race.trail_particles[0], 98, 97, 65, 0, 0, 0, 43);
    expect_particle(&race.trail_particles[1], 99, 98, 65, 5, 6, -4, 21);
    expect_particle(&race.trail_particles[2], 99, 100, 65, 0, 0, 0, 48);
    expect_particle(&race.trail_particles[3], 99, 101, 65, 5, 6, -5, 20);

    prepare(&race, &car, 7, 602, 0);
    emit_wheel_surface(&race, &car, 1, SLICKS_CONTROL_ACCELERATE);
    if (race.trail_particle_count != 2 || race.random_state != 0xd5e076dcUL)
        fail("class-7 actor count or random state");
    expect_particle(&race.trail_particles[0], 99, 98, 55, 3, -3, -10, 20);
    expect_particle(&race.trail_particles[1], 99, 101, 55, 3, -10, 3, 20);

    /* The visual A/B switch must remove actors without changing the wheel
     * dispatch's random-number consumption or touching the pixel surface. */
    {
        const unsigned char surfaces[] = {0, 5, 7, 11};
        const long velocities[] = {1000, 600, 602, 600};
        unsigned int i;
        for (i = 0; i < sizeof(surfaces); ++i) {
            unsigned long random_after;
            prepare(&race, &car, surfaces[i], velocities[i], 0);
            emit_wheel_surface(&race, &car, 1, SLICKS_CONTROL_ACCELERATE);
            random_after = race.random_state;
            if (!race.trail_particle_count)
                fail("disable comparison requires an emitting control");
            prepare(&race, &car, surfaces[i], velocities[i], 0);
            slicks_race_disable_particles = 1;
            emit_wheel_surface(&race, &car, 1, SLICKS_CONTROL_ACCELERATE);
            if (race.trail_particle_count || race.skidmark_count ||
                race.random_state != random_after)
                fail("disabled particles changed RNG or inserted actors");
            for (unsigned int bucket = 0; bucket < 4; ++bucket) {
                if (race.trail_priority_counts[bucket])
                    fail("disabled particles inserted priority entries");
                restore_trail_particles(&race, bucket);
                draw_trail_particles(&race, bucket);
            }
            if (race.dirty_pixel_count || race.dirty_row_count)
                fail("disabled particles marked display dirty");
            slicks_race_disable_particles = 0;
        }
    }
    puts("surface effects: DOS dispatch, actor tuples, RNG, and particle-disable control matched");
    return 0;
}
