#include "race_runtime.h"

#define SLICKS_SCREEN_WIDTH 320
#define SLICKS_TRACK_HEIGHT 190
#define SLICKS_SCREEN_HEIGHT 200
#define SLICKS_STRIDE 100U
#define SLICKS_PLANE_SIZE 0x10000UL
#define SLICKS_HEADING_STEP 0x4b0
#define SLICKS_HEADING_FULL 0x4b00
#define SLICKS_TIMER_COLOUR 254
#define SLICKS_START_LIGHT_X 164
#define SLICKS_START_LIGHT_Y 31
#define SLICKS_START_LIGHT_WIDTH 23
#define SLICKS_START_LIGHT_HEIGHT 38

/* These are the signed tables used by the original race engine at DS:06c3
 * and DS:06d3.  Position is kept in the original 100-units-per-pixel scale. */
static const signed char direction_x[16] = {
    100, 92, 71, 38, 0, -38, -71, -92,
    -100, -92, -71, -38, 0, 38, 71, 92
};
static const signed char direction_y[16] = {
    0, 38, 71, 92, 100, 92, 71, 38,
    0, -38, -71, -92, -100, -92, -71, -38
};

/* DS:017e in the original race engine.  A non-zero entry creates a
 * four-frame trail particle; its signed magnitude controls how quickly the
 * particle moves along the car's heading.  Dirt therefore throws the mark
 * backwards while grass throws it forwards. */
static const signed char trail_velocity_by_material[32] = {
    1, 1, 1, 0, -1, 0, 0, 0,
    20, 0, 30, 0, 15, 0, -6, 0,
    20, 0, -12, 1, 10, 0, -56, 0,
    10, 11, 12, 13, 14, 15, 15, 13
};

static unsigned char read_pixel(const unsigned char *logical,
                                unsigned short x, unsigned short y)
{
    unsigned long offset = (unsigned long)y * SLICKS_STRIDE + (x >> 2);
    offset += (unsigned long)(x & 3) * SLICKS_PLANE_SIZE;
    return logical[offset];
}

static void write_pixel(unsigned char *logical, unsigned char *chunky,
                        unsigned short x, unsigned short y,
                        unsigned char colour)
{
    unsigned long offset;
    if (x >= SLICKS_SCREEN_WIDTH || y >= SLICKS_SCREEN_HEIGHT)
        return;
    offset = (unsigned long)y * SLICKS_STRIDE + (x >> 2);
    offset += (unsigned long)(x & 3) * SLICKS_PLANE_SIZE;
    logical[offset] = colour;
    if (chunky)
        chunky[(unsigned long)y * SLICKS_SCREEN_WIDTH + x] = colour;
}

static void mark_dirty_rows(struct SlicksRaceRuntime *race,
                            short top, short bottom)
{
    struct SlicksDirtyRows rows;
    unsigned short at;
    if (top < 0)
        top = 0;
    if (bottom > SLICKS_SCREEN_HEIGHT)
        bottom = SLICKS_SCREEN_HEIGHT;
    if (top >= bottom)
        return;
    rows.top = (unsigned short)top;
    rows.bottom = (unsigned short)bottom;

    /* Repeatedly fold overlapping or touching half-open intervals.  Removing
     * a match can expose another overlap, so restart after every union. */
    for (;;) {
        unsigned char merged = 0;
        for (at = 0; at < race->dirty_row_count; ++at) {
            struct SlicksDirtyRows *existing = &race->dirty_rows[at];
            if (rows.bottom < existing->top || rows.top > existing->bottom)
                continue;
            if (existing->top < rows.top)
                rows.top = existing->top;
            if (existing->bottom > rows.bottom)
                rows.bottom = existing->bottom;
            *existing = race->dirty_rows[--race->dirty_row_count];
            merged = 1;
            break;
        }
        if (!merged)
            break;
    }
    if (race->dirty_row_count < SLICKS_DIRTY_ROW_MAX) {
        race->dirty_rows[race->dirty_row_count++] = rows;
        return;
    }

    /* Correctness fallback for an unexpectedly fragmented frame. */
    for (at = 0; at < race->dirty_row_count; ++at) {
        if (race->dirty_rows[at].top < rows.top)
            rows.top = race->dirty_rows[at].top;
        if (race->dirty_rows[at].bottom > rows.bottom)
            rows.bottom = race->dirty_rows[at].bottom;
    }
    race->dirty_rows[0] = rows;
    race->dirty_row_count = 1;
}

static int decode_sprite(struct SlicksCarSprite *sprite,
                         const unsigned char *source,
                         unsigned long source_size)
{
    unsigned long source_at = 0;
    unsigned short width;
    unsigned short height;
    unsigned short produced = 0;
    unsigned char format;
    unsigned char escape = 0;

    if (!sprite || !source || source_size < 3)
        return -1;
    format = source[source_at++];
    width = source[source_at++];
    height = source[source_at++];
    if (format > 3)
        return -1;
    width |= (unsigned short)(format & 1) << 8;
    if (!width || !height ||
        (unsigned long)width * height > SLICKS_CAR_PIXEL_MAX)
        return -1;

    if (format & 2) {
        if (source_at >= source_size)
            return -1;
        escape = source[source_at++];
        while (produced < width * height) {
            unsigned char value;
            unsigned short count = 1;
            if (source_at >= source_size)
                return -1;
            value = source[source_at++];
            if (value == escape) {
                if (source_at >= source_size)
                    return -1;
                count = source[source_at++];
                if (!count) {
                    count = 1;
                } else {
                    if (source_at >= source_size)
                        return -1;
                    value = source[source_at++];
                }
            }
            while (count-- && produced < width * height)
                sprite->pixels[produced++] = value;
        }
    } else {
        unsigned short count = width * height;
        if (source_at + count > source_size)
            return -1;
        while (produced < count) {
            sprite->pixels[produced] = source[source_at + produced];
            ++produced;
        }
    }
    sprite->width = (unsigned char)width;
    sprite->height = (unsigned char)height;
    sprite->ready = 1;
    return 0;
}

static void draw_start_light(struct SlicksRaceRuntime *race,
                             unsigned char *logical,
                             unsigned short light)
{
    unsigned short x;
    unsigned short y;
    unsigned short at = 0;
    mark_dirty_rows(race, SLICKS_START_LIGHT_Y,
                    SLICKS_START_LIGHT_Y + SLICKS_START_LIGHT_HEIGHT);
    if (!race->start_light_visible) {
        for (y = 0; y < SLICKS_START_LIGHT_HEIGHT; ++y)
            for (x = 0; x < SLICKS_START_LIGHT_WIDTH; ++x)
                race->start_light_saved_under[at++] =
                    read_pixel(logical, SLICKS_START_LIGHT_X + x,
                               SLICKS_START_LIGHT_Y + y);
        race->start_light_visible = 1;
    }
    at = 0;
    for (y = 0; y < SLICKS_START_LIGHT_HEIGHT; ++y)
        for (x = 0; x < SLICKS_START_LIGHT_WIDTH; ++x)
            write_pixel(logical, race->chunky,
                        SLICKS_START_LIGHT_X + x,
                        SLICKS_START_LIGHT_Y + y,
                        race->start_lights[light].pixels[at++]);
    race->start_light_stage_mask |= (unsigned char)(1U << light);
}

static void restore_start_light(struct SlicksRaceRuntime *race,
                                unsigned char *logical)
{
    unsigned short x;
    unsigned short y;
    unsigned short at = 0;
    if (!race->start_light_visible)
        return;
    mark_dirty_rows(race, SLICKS_START_LIGHT_Y,
                    SLICKS_START_LIGHT_Y + SLICKS_START_LIGHT_HEIGHT);
    for (y = 0; y < SLICKS_START_LIGHT_HEIGHT; ++y)
        for (x = 0; x < SLICKS_START_LIGHT_WIDTH; ++x)
            write_pixel(logical, race->chunky,
                        SLICKS_START_LIGHT_X + x,
                        SLICKS_START_LIGHT_Y + y,
                        race->start_light_saved_under[at++]);
    race->start_light_visible = 0;
}

static void rotated_size(const struct SlicksCarSprite *sprite,
                         unsigned char rotation, unsigned char *width,
                         unsigned char *height)
{
    if (rotation & 1) {
        *width = sprite->height;
        *height = sprite->width;
    } else {
        *width = sprite->width;
        *height = sprite->height;
    }
}

static unsigned char sprite_pixel(const struct SlicksCarSprite *sprite,
                                  unsigned char rotation,
                                  unsigned char x, unsigned char y)
{
    unsigned char source_x;
    unsigned char source_y;
    rotation &= 3;
    if (rotation == 1) {
        source_x = y;
        source_y = sprite->height - 1 - x;
    } else if (rotation == 2) {
        source_x = sprite->width - 1 - x;
        source_y = sprite->height - 1 - y;
    } else if (rotation == 3) {
        source_x = sprite->width - 1 - y;
        source_y = x;
    } else {
        source_x = x;
        source_y = y;
    }
    return sprite->pixels[(unsigned short)source_y * sprite->width + source_x];
}

static void restore_car(struct SlicksRaceRuntime *race,
                        unsigned char *logical, struct SlicksRaceCar *car)
{
    unsigned short x;
    unsigned short y;
    unsigned short at = 0;
    if (!car->saved_valid)
        return;
    mark_dirty_rows(race, car->old_y, car->old_y + car->old_height);
    for (y = 0; y < car->old_height; ++y) {
        for (x = 0; x < car->old_width; ++x)
            write_pixel(logical, race->chunky,
                        car->old_x + x, car->old_y + y,
                        car->saved_under[at++]);
    }
    car->saved_valid = 0;
}

static void draw_car(struct SlicksRaceRuntime *race, unsigned char *logical,
                     unsigned short car_index)
{
    struct SlicksRaceCar *car = &race->cars[car_index];
    unsigned short direction = (unsigned short)car->heading / SLICKS_HEADING_STEP;
    unsigned char base_direction = direction & 3;
    unsigned char rotation = direction >> 2;
    const struct SlicksCarSprite *sprite =
        &race->sprites[car->vehicle][base_direction];
    unsigned char width;
    unsigned char height;
    unsigned short x;
    unsigned short y;
    unsigned short at = 0;
    short origin_x;
    short origin_y;

    rotated_size(sprite, rotation, &width, &height);
    origin_x = (short)(car->x / 100) - width / 2;
    origin_y = (short)(car->y / 100) - height / 2;
    if (origin_x < 0 || origin_y < 0 ||
        origin_x + width > SLICKS_SCREEN_WIDTH ||
        origin_y + height > SLICKS_TRACK_HEIGHT)
        return;
    mark_dirty_rows(race, origin_y, origin_y + height);

    car->old_x = (unsigned char)origin_x;
    car->old_y = (unsigned char)origin_y;
    car->old_width = width;
    car->old_height = height;
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x)
            car->saved_under[at++] =
                read_pixel(logical, origin_x + x, origin_y + y);
    }
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            unsigned char pixel = sprite_pixel(sprite, rotation, x, y);
            if (pixel) {
                /* Every original car image uses body shades 1..5.  The DOS
                 * renderer relocates that ramp to the five-colour slot
                 * selected for the racer. */
                if (pixel >= 1 && pixel <= 5)
                    pixel += car->style * 5;
                write_pixel(logical, race->chunky,
                            origin_x + x, origin_y + y, pixel);
            }
        }
    }
    car->saved_valid = 1;
}

static unsigned char nearest_direction(long dx, long dy)
{
    unsigned short direction;
    unsigned char best = 0;
    long best_dot = -2147483647L;
    for (direction = 0; direction < 16; ++direction) {
        long dot = dx * direction_x[direction] + dy * direction_y[direction];
        if (dot > best_dot) {
            best_dot = dot;
            best = (unsigned char)direction;
        }
    }
    return best;
}

static short heading_difference(short target, short current)
{
    short difference = target - current;
    if (difference > SLICKS_HEADING_FULL / 2)
        difference -= SLICKS_HEADING_FULL;
    if (difference < -SLICKS_HEADING_FULL / 2)
        difference += SLICKS_HEADING_FULL;
    return difference;
}

static long absolute_long(long value);

static unsigned char predicted_car_contact(struct SlicksRaceRuntime *race,
                                           unsigned short car_index)
{
    struct SlicksRaceCar *car = &race->cars[car_index];
    unsigned short direction;
    unsigned short step;
    long x;
    long y;
    if (++car->ai_probe_counter <= 10)
        return 0;
    car->ai_probe_counter = 0;
    direction = (unsigned short)car->heading / SLICKS_HEADING_STEP;
    x = car->x;
    y = car->y;
    for (step = 0; step < 12; ++step) {
        unsigned short other;
        x += (long)direction_x[direction] * 3L;
        y += (long)direction_y[direction] * 3L;
        for (other = 0; other < SLICKS_RACE_CAR_COUNT; ++other) {
            if (other != car_index &&
                absolute_long(x - race->cars[other].x) < 700L &&
                absolute_long(y - race->cars[other].y) < 700L)
                return (unsigned char)(other + 1);
        }
    }
    return 0;
}

static unsigned char ai_controls(struct SlicksRaceRuntime *race,
                                 unsigned short car_index)
{
    struct SlicksRaceCar *car = &race->cars[car_index];
    const struct SlicksTrackZone *zone =
        &race->navigation.zones[car->waypoint];
    long dx = (long)zone->x[2] * 100 - car->x;
    long dy = (long)zone->y[2] * 100 - car->y;
    short target_heading =
        (short)nearest_direction(dx, dy) * SLICKS_HEADING_STEP;
    short difference = heading_difference(target_heading, car->heading);
    unsigned char controls = 0;
    unsigned char contact = predicted_car_contact(race, car_index);

    /* f09d's stationary-position watchdog enters a timed recovery turn.
     * Keep the recovered 150/40 tick cadence; the original random side is
     * made deterministic from frame and racer so diagnostics remain stable. */
    if (car->x / 100 == car->ai_last_x / 100 &&
        car->y / 100 == car->ai_last_y / 100) {
        if (car->ai_stuck_ticks)
            --car->ai_stuck_ticks;
    } else {
        car->ai_last_x = car->x;
        car->ai_last_y = car->y;
        car->ai_stuck_ticks = 150;
    }
    if (!car->ai_recovery_ticks && car->ai_stuck_ticks == 0) {
        car->ai_recovery_ticks = 40;
        car->ai_recovery_right =
            (unsigned char)((race->frame_count + car_index) & 1);
        car->ai_stuck_ticks = 150;
    }
    if (car->ai_recovery_ticks) {
        --car->ai_recovery_ticks;
        controls = SLICKS_CONTROL_ACCELERATE |
            (car->ai_recovery_right ? SLICKS_CONTROL_RIGHT :
                                      SLICKS_CONTROL_LEFT);
        return controls;
    }

    if (contact) {
        const struct SlicksRaceCar *other = &race->cars[contact - 1];
        short away = heading_difference(car->heading, other->heading);
        if (away >= 0)
            difference = -SLICKS_HEADING_STEP * 3;
        else
            difference = SLICKS_HEADING_STEP * 3;
    }

    if (difference < 0)
        controls |= SLICKS_CONTROL_LEFT;
    if (difference > 0)
        controls |= SLICKS_CONTROL_RIGHT;
    if (!contact) {
        /* e204 unconditionally restores throttle when its avoidance angle is
         * zero, even while the route-heading correction is steering. */
        controls |= SLICKS_CONTROL_ACCELERATE;
        controls &= (unsigned char)~SLICKS_CONTROL_BRAKE;
    } else if (difference >= -SLICKS_HEADING_STEP &&
               difference <= SLICKS_HEADING_STEP) {
        controls |= SLICKS_CONTROL_ACCELERATE;
    } else if (difference < -5 * SLICKS_HEADING_STEP ||
               difference > 5 * SLICKS_HEADING_STEP) {
        if (car->speed_fixed >= 700L)
            controls |= SLICKS_CONTROL_BRAKE;
    }
    return controls;
}

static unsigned char material_at(const struct SlicksRaceRuntime *race,
                                 short x, short y)
{
    if (x < 0 || x >= SLICKS_SCREEN_WIDTH ||
        y < 0 || y >= SLICKS_TRACK_HEIGHT)
        return 31;
    return race->material_map[(unsigned long)y * SLICKS_SCREEN_WIDTH + x];
}

static int material_is_driveable(unsigned char material)
{
    /* The original collision reader returns palette_index >> 3.  BASIC.SS
     * uses 0 for tarmac, 14/15 for dirt, 20/21 for grass and 27 for the
     * bridge deck.  Codes 2/3/5/6 are track and start-line markings, 12
     * alternates with zero across the bridge texture, and 22 occurs beneath
     * the starting grid. Other classes are scenery or raised track boundaries.
     * Surface-specific friction is applied separately as its .omi triplets
     * are recovered. */
    return material == 0 || material == 2 || material == 3 ||
           material == 5 || material == 6 || material == 12 ||
           material == 14 || material == 15 || material == 20 ||
           material == 21 || material == 22 || material == 27;
}

static unsigned char surface_group_for_material(unsigned char material)
{
    if (material == 14 || material == 15)
        return 3;
    if (material == 20 || material == 21)
        return 2;
    if (material == 27)
        return 4;
    return 0;
}

static int position_touches_solid(const struct SlicksRaceRuntime *race,
                                  const struct SlicksCarProperties *properties,
                                  long car_x, long car_y)
{
    short x = (short)(car_x / 100);
    short y = (short)(car_y / 100);
    short rx = properties->body_radius_x;
    short ry = properties->body_radius_y;
    return !material_is_driveable(material_at(race, x - rx, y)) ||
           !material_is_driveable(material_at(race, x + rx, y)) ||
           !material_is_driveable(material_at(race, x, y - ry)) ||
           !material_is_driveable(material_at(race, x, y + ry));
}

static void advance_waypoint(struct SlicksRaceRuntime *race,
                             struct SlicksRaceCar *car)
{
    const struct SlicksTrackZone *zone =
        &race->navigation.zones[car->waypoint];
    short x = (short)(car->x / 100);
    short y = (short)(car->y / 100);

    /* f1eb..f277 compares the centre coordinates against the four recovered
     * navigation arrays before advancing DS:6902. */
    if (x >= (short)zone->x[0] && x <= (short)zone->x[1] &&
        y >= (short)zone->y[0] && y <= (short)zone->y[1]) {
        ++car->waypoint;
        if (car->waypoint >= race->navigation.zone_count) {
            car->waypoint = 0;
            car->last_lap_centiseconds = car->current_lap_centiseconds;
            if (!car->best_lap_centiseconds ||
                car->last_lap_centiseconds < car->best_lap_centiseconds)
                car->best_lap_centiseconds = car->last_lap_centiseconds;
            car->current_lap_centiseconds = 0;
            ++car->lap;
            if (!car->finished && car->lap > race->laps_to_run) {
                car->finished = 1;
                car->finish_position = ++race->finished_count;
                car->finish_time_centiseconds = car->elapsed_centiseconds;
                if (race->finished_count >= SLICKS_RACE_CAR_COUNT)
                    race->race_complete = 1;
            }
        }
    }
}

static void restore_trail_particles(struct SlicksRaceRuntime *race,
                                    unsigned char *logical)
{
    unsigned short at = race->trail_particle_count;
    while (at) {
        struct SlicksTrailParticle *particle =
            &race->trail_particles[--at];
        if (particle->saved_valid) {
            const struct SlicksCarSprite *sprite =
                &race->trail_sprites[particle->lifetime < 2 ? 2 :
                    4 - particle->lifetime];
            unsigned short x;
            unsigned short y;
            unsigned short saved_at = 0;
            mark_dirty_rows(race, particle->old_y,
                            particle->old_y + sprite->height);
            for (y = 0; y < sprite->height; ++y)
                for (x = 0; x < sprite->width; ++x)
                    write_pixel(logical, race->chunky,
                                (unsigned short)(particle->old_x + x),
                                (unsigned short)(particle->old_y + y),
                                particle->saved_under[saved_at++]);
            particle->saved_valid = 0;
        }
    }
}

static void advance_trail_particles(struct SlicksRaceRuntime *race)
{
    unsigned short source;
    unsigned short destination = 0;
    for (source = 0; source < race->trail_particle_count; ++source) {
        struct SlicksTrailParticle particle = race->trail_particles[source];
        if (!particle.lifetime)
            continue;
        particle.x += particle.velocity_x;
        particle.y += particle.velocity_y;
        --particle.lifetime;
        if (particle.lifetime)
            race->trail_particles[destination++] = particle;
    }
    race->trail_particle_count = (unsigned char)destination;
}

static void add_trail_particle(struct SlicksRaceRuntime *race,
                               const struct SlicksRaceCar *car)
{
    unsigned short direction;
    unsigned char material;
    signed char strength;
    struct SlicksTrailParticle *particle;
    if (race->trail_particle_count >= SLICKS_TRAIL_PARTICLE_MAX)
        return;
    material = material_at(race, (short)(car->x / 100),
                           (short)(car->y / 100));
    strength = trail_velocity_by_material[material];
    if (!strength)
        return;
    direction = (unsigned short)car->heading / SLICKS_HEADING_STEP;
    particle = &race->trail_particles[race->trail_particle_count++];
    particle->x = car->x * 64L / 100L;
    particle->y = car->y * 64L / 100L;
    particle->velocity_x =
        (short)strength * (short)direction_x[direction] * 2;
    particle->velocity_y =
        (short)strength * (short)direction_y[direction] * 2;
    particle->lifetime = 4;
    particle->saved_valid = 0;
    ++race->skidmark_count;
}

static void draw_trail_particles(struct SlicksRaceRuntime *race,
                                 unsigned char *logical)
{
    unsigned short at;
    for (at = 0; at < race->trail_particle_count; ++at) {
        struct SlicksTrailParticle *particle = &race->trail_particles[at];
        unsigned short frame = particle->lifetime < 2 ? 2 :
            4 - particle->lifetime;
        const struct SlicksCarSprite *sprite = &race->trail_sprites[frame];
        short x = (short)(particle->x / 64L);
        short y = (short)(particle->y / 64L);
        unsigned short sprite_x;
        unsigned short sprite_y;
        unsigned short saved_at = 0;
        if (x < 0 || x + sprite->width > SLICKS_SCREEN_WIDTH ||
            y < 0 || y + sprite->height > SLICKS_TRACK_HEIGHT)
            continue;
        particle->old_x = x;
        particle->old_y = y;
        for (sprite_y = 0; sprite_y < sprite->height; ++sprite_y) {
            for (sprite_x = 0; sprite_x < sprite->width; ++sprite_x) {
                unsigned char pixel = sprite->pixels[
                    sprite_y * sprite->width + sprite_x];
                particle->saved_under[saved_at++] =
                    read_pixel(logical, (unsigned short)(x + sprite_x),
                               (unsigned short)(y + sprite_y));
                if (pixel)
                    write_pixel(logical, race->chunky,
                                (unsigned short)(x + sprite_x),
                                (unsigned short)(y + sprite_y), pixel);
            }
        }
        particle->saved_valid = 1;
        mark_dirty_rows(race, y, y + sprite->height);
    }
}

static void update_car(struct SlicksRaceRuntime *race,
                       unsigned short car_index)
{
    struct SlicksRaceCar *car = &race->cars[car_index];
    const struct SlicksCarProperties *properties =
        &race->properties[car->vehicle];
    unsigned char controls =
        car_index == 0 && race->human_control
            ? race->controls
            : ai_controls(race, car_index);
    unsigned char turning = controls & (SLICKS_CONTROL_LEFT |
                                        SLICKS_CONTROL_RIGHT);
    unsigned char steering_input =
        (unsigned char)(car_index == 0 && race->human_control ? 100 : 140);
    long steering_step;
    unsigned short direction;
    short velocity_response;
    long target_velocity_x;
    long target_velocity_y;
    long previous_x = car->x;
    long previous_y = car->y;

    if (car->finished)
        controls = 0;
    car->surface_group = surface_group_for_material(
        material_at(race, (short)(car->x / 100),
                    (short)(car->y / 100)));

    /* The DOS car state stores speed as a signed 32-bit fixed quantity.
     * Throttle adds 0xa0 per simulation quantum and .omi byte four supplies
     * the limit in hundreds; neither value is a C-era tuning estimate. */
    if ((controls & SLICKS_CONTROL_ACCELERATE) &&
        car->speed_fixed < (long)properties->top_speed * 100L) {
        car->speed_fixed += 0xa0L;
    } else if (car->speed_fixed > 0) {
        /* 2000:0e5e multiplies car-state +10h by the Q15 factor held in
         * DS:53fe.  aeb6 constructs that factor as 7bddh minus twice the
         * signed displacement of .omi byte 23 from 100.  This reproduces
         * captured coast sequences such as 10000, 9676, 9363 exactly. */
        long coast_factor = 0x7bddL - properties->balance_bias;
        car->speed_fixed = car->speed_fixed * coast_factor / 0x8000L;
    }
    if (controls & SLICKS_CONTROL_BRAKE) {
        car->speed_fixed -= 0x21L * properties->top_speed;
        if (car->speed_fixed < 0)
            car->speed_fixed = 0;
    }
    car->speed = (short)(car->speed_fixed / 100L);

    /* 2000:0c79..0d54 performs these divisions separately with signed IDIV;
     * preserving their order is observable.  The semantic DOS trace proves
     * this recurrence for 9,329 turns.  The +26h penalty is zero in the
     * captured normal race but remains explicit for the recovered state. */
    car->steering_amount = steering_input;
    steering_step = (long)steering_input *
        (car->steering_scale / 10);
    steering_step /= 155;
    steering_step *= 80 - car->steering_penalty / 25;
    steering_step /= 100;
    steering_step *= car->steering_property;
    steering_step /= 50;
    if (controls & SLICKS_CONTROL_LEFT)
        car->heading -= (short)steering_step;
    if (controls & SLICKS_CONTROL_RIGHT)
        car->heading += (short)steering_step;
    while (car->heading < 0)
        car->heading += SLICKS_HEADING_FULL;
    while (car->heading >= SLICKS_HEADING_FULL)
        car->heading -= SLICKS_HEADING_FULL;

    direction = (unsigned short)car->heading / SLICKS_HEADING_STEP;
    target_velocity_x =
        (long)direction_x[direction] * car->speed_fixed / 100L;
    target_velocity_y =
        (long)direction_y[direction] * car->speed_fixed / 100L;
    velocity_response = (short)(
        properties->drive_response *
        (short)properties->surface[car->surface_group][0] / 100);
    if (velocity_response < 10)
        velocity_response = 10;
    if (velocity_response > 100)
        velocity_response = 100;
    car->velocity_x +=
        (target_velocity_x - car->velocity_x) * velocity_response / 100L;
    car->velocity_y +=
        (target_velocity_y - car->velocity_y) * velocity_response / 100L;
    car->x += car->velocity_x / 100L;
    car->y += car->velocity_y / 100L;
    if (position_touches_solid(race, properties, car->x, car->y)) {
        long proposed_x = car->x;
        long proposed_y = car->y;
        if (!position_touches_solid(race, properties,
                                    proposed_x, previous_y)) {
            car->y = previous_y;
            car->speed_fixed = car->speed_fixed * 3L / 4L;
            car->velocity_y = -car->velocity_y / 2L;
        } else if (!position_touches_solid(race, properties,
                                           previous_x, proposed_y)) {
            car->x = previous_x;
            car->speed_fixed = car->speed_fixed * 3L / 4L;
            car->velocity_x = -car->velocity_x / 2L;
        } else {
            car->x = previous_x;
            car->y = previous_y;
            car->speed_fixed /= 2;
            car->velocity_x = -car->velocity_x / 2L;
            car->velocity_y = -car->velocity_y / 2L;
            if ((car_index + race->frame_count) & 1)
                car->heading += SLICKS_HEADING_FULL / 8;
            else
                car->heading -= SLICKS_HEADING_FULL / 8;
        }
        if (!car->touching_solid)
            ++race->track_collision_count;
        car->touching_solid = 1;
    } else
        car->touching_solid = 0;
    car->speed = (short)(car->speed_fixed / 100L);
    while (car->heading < 0)
        car->heading += SLICKS_HEADING_FULL;
    while (car->heading >= SLICKS_HEADING_FULL)
        car->heading -= SLICKS_HEADING_FULL;
    if (turning && car->speed > 65 && !(race->frame_count & 1))
        add_trail_particle(race, car);
    else if ((car->surface_group == 2 || car->surface_group == 3) &&
             car->speed > 20 && !(race->frame_count & 3))
        add_trail_particle(race, car);
    if (!car->finished) {
        car->elapsed_centiseconds += 2;
        car->current_lap_centiseconds += 2;
        advance_waypoint(race, car);
    }
}

static long absolute_long(long value)
{
    return value < 0 ? -value : value;
}

static void resolve_car_collisions(struct SlicksRaceRuntime *race)
{
    unsigned short first;
    unsigned char pair_bit = 1;
    unsigned char next_pairs = 0;
    for (first = 0; first < SLICKS_RACE_CAR_COUNT; ++first) {
        unsigned short second;
        for (second = first + 1; second < SLICKS_RACE_CAR_COUNT; ++second) {
            struct SlicksRaceCar *a = &race->cars[first];
            struct SlicksRaceCar *b = &race->cars[second];
            const struct SlicksCarProperties *pa =
                &race->properties[a->vehicle];
            const struct SlicksCarProperties *pb =
                &race->properties[b->vehicle];
            long dx = b->x - a->x;
            long dy = b->y - a->y;
            long minimum =
                (long)(pa->collision_radius + pb->collision_radius) * 50L;
            long delta_vx;
            long delta_vy;

            /* Finished entrants no longer participate in the race contact
             * set; otherwise a stopped winner can permanently blockade the
             * checkpoint line for the remaining cars. */
            if (a->finished || b->finished) {
                pair_bit <<= 1;
                continue;
            }

            /* The DOS resolver performs two extent comparisons, not a radial
             * distance test. Its impulse then transfers each relative vector
             * component using the other car's .omi weight divided by this
             * car's weight. */
            if (absolute_long(dx) >= minimum || absolute_long(dy) >= minimum) {
                pair_bit <<= 1;
                continue;
            }
            next_pairs |= pair_bit;

            if (absolute_long(dx) >= absolute_long(dy)) {
                long overlap = minimum - absolute_long(dx);
                long direction = dx < 0 ? -1 : 1;
                a->x -= direction * ((overlap + 1) / 2);
                b->x += direction * (overlap / 2);
            } else {
                long overlap = minimum - absolute_long(dy);
                long direction = dy < 0 ? -1 : 1;
                a->y -= direction * ((overlap + 1) / 2);
                b->y += direction * (overlap / 2);
            }

            if (!(race->active_collision_pairs & pair_bit)) {
                delta_vx = a->velocity_x - b->velocity_x;
                delta_vy = a->velocity_y - b->velocity_y;
                a->velocity_x -= delta_vx * pb->collision_weight /
                                 pa->collision_weight;
                a->velocity_y -= delta_vy * pb->collision_weight /
                                 pa->collision_weight;
                b->velocity_x += delta_vx * pa->collision_weight /
                                 pb->collision_weight;
                b->velocity_y += delta_vy * pa->collision_weight /
                                 pb->collision_weight;
                a->speed_fixed =
                    (absolute_long(a->velocity_x) +
                     absolute_long(a->velocity_y)) * 55L;
                b->speed_fixed =
                    (absolute_long(b->velocity_x) +
                     absolute_long(b->velocity_y)) * 55L;
                if (a->speed_fixed > (long)pa->top_speed * 100L)
                    a->speed_fixed = (long)pa->top_speed * 100L;
                if (b->speed_fixed > (long)pb->top_speed * 100L)
                    b->speed_fixed = (long)pb->top_speed * 100L;
                ++race->collision_count;
            }
            pair_bit <<= 1;
        }
    }
    race->active_collision_pairs = next_pairs;
}

static void clear_timer_strip(struct SlicksRaceRuntime *race,
                              unsigned char *logical)
{
    unsigned short x;
    unsigned short y;
    mark_dirty_rows(race, SLICKS_TRACK_HEIGHT, SLICKS_SCREEN_HEIGHT);
    for (y = SLICKS_TRACK_HEIGHT; y < SLICKS_SCREEN_HEIGHT; ++y)
        for (x = 0; x < SLICKS_SCREEN_WIDTH; ++x)
            write_pixel(logical, race->chunky, x, y, 0);
}

static unsigned char draw_character(const struct SlicksRaceFont *font,
                                    unsigned char *logical,
                                    unsigned char *chunky,
                                    unsigned short x, unsigned short y,
                                    unsigned char character)
{
    unsigned short glyph;
    unsigned short pixel_at = 0;
    unsigned short row;
    unsigned short column;
    for (glyph = 0; glyph < font->glyph_count; ++glyph) {
        if (font->codes[glyph] == character)
            break;
        pixel_at += font->widths[glyph] * font->height;
    }
    if (glyph >= font->glyph_count)
        return 0;
    for (row = 0; row < font->height; ++row)
        for (column = 0; column < font->widths[glyph]; ++column)
            if (font->pixels[pixel_at + row * font->widths[glyph] + column])
                write_pixel(logical, chunky, x + column, y + row,
                            SLICKS_TIMER_COLOUR);
    return font->widths[glyph];
}

static void draw_timers(struct SlicksRaceRuntime *race, unsigned char *logical)
{
    unsigned short car;
    clear_timer_strip(race, logical);
    for (car = 0; car < SLICKS_RACE_CAR_COUNT; ++car) {
        unsigned short x = 4 + car * 80;
        unsigned short time = race->cars[car].elapsed_centiseconds;
        unsigned short seconds = time / 100;
        unsigned short hundredths = time % 100;
        x += draw_character(&race->font, logical, race->chunky, x, 192,
                            (unsigned char)('1' + car)) + 3;
        x += draw_character(&race->font, logical, race->chunky, x, 192,
                            (unsigned char)('0' + (seconds / 10) % 10)) + 1;
        x += draw_character(&race->font, logical, race->chunky, x, 192,
                            (unsigned char)('0' + seconds % 10)) + 1;
        x += draw_character(&race->font, logical, race->chunky,
                            x, 192, ':') + 1;
        x += draw_character(&race->font, logical, race->chunky, x, 192,
                            (unsigned char)('0' + hundredths / 10)) + 1;
        x += draw_character(&race->font, logical, race->chunky, x, 192,
                            (unsigned char)('0' + hundredths % 10)) + 2;
        if (race->cars[car].finished) {
            x += draw_character(&race->font, logical, race->chunky, x, 192,
                                '#') + 1;
            (void)draw_character(
                &race->font, logical, race->chunky, x, 192,
                (unsigned char)('0' + race->cars[car].finish_position));
        } else {
            x += draw_character(
                &race->font, logical, race->chunky, x, 192,
                (unsigned char)('0' + race->cars[car].lap)) + 1;
            x += draw_character(&race->font, logical, race->chunky, x, 192,
                                '/') + 1;
            (void)draw_character(
                &race->font, logical, race->chunky, x, 192,
                (unsigned char)('0' + race->laps_to_run));
        }
    }
}

static void draw_results(struct SlicksRaceRuntime *race,
                         unsigned char *logical)
{
    static const char heading[] = "RESULTS";
    unsigned short at;
    unsigned short x;
    unsigned short y;
    mark_dirty_rows(race, 68, 133);
    for (y = 68; y < 133; ++y)
        for (x = 105; x < 215; ++x)
            write_pixel(logical, race->chunky, x, y, 0);
    x = 139;
    for (at = 0; heading[at]; ++at)
        x += draw_character(&race->font, logical, race->chunky, x, 73,
                            (unsigned char)heading[at]) + 1;
    for (at = 0; at < SLICKS_RACE_CAR_COUNT; ++at) {
        unsigned short car;
        unsigned short row = 88 + at * 10;
        for (car = 0; car < SLICKS_RACE_CAR_COUNT; ++car) {
            if (race->cars[car].finish_position == at + 1) {
                unsigned short time = race->cars[car].finish_time_centiseconds;
                x = 130;
                x += draw_character(&race->font, logical, race->chunky, x,
                                    row, (unsigned char)('1' + at)) + 5;
                x += draw_character(&race->font, logical, race->chunky, x,
                                    row, (unsigned char)('1' + car)) + 8;
                x += draw_character(&race->font, logical, race->chunky, x,
                                    row,
                                    (unsigned char)('0' +
                                        (time / 1000) % 10)) + 1;
                x += draw_character(&race->font, logical, race->chunky, x,
                                    row,
                                    (unsigned char)('0' +
                                        (time / 100) % 10)) + 1;
                x += draw_character(&race->font, logical, race->chunky, x,
                                    row, ':') + 1;
                x += draw_character(&race->font, logical, race->chunky, x,
                                    row,
                                    (unsigned char)('0' +
                                        (time / 10) % 10)) + 1;
                (void)draw_character(&race->font, logical, race->chunky, x,
                                     row,
                                     (unsigned char)('0' + time % 10));
                break;
            }
        }
    }
    race->results_drawn = 1;
}

void slicks_race_initialize(struct SlicksRaceRuntime *race,
                            const struct SlicksTrackNavigation *navigation)
{
    unsigned char *destination = (unsigned char *)race;
    const unsigned char *source = (const unsigned char *)navigation;
    unsigned long at;
    for (at = 0; at < sizeof(*race); ++at)
        destination[at] = 0;
    for (at = 0; at < sizeof(*navigation); ++at)
        ((unsigned char *)&race->navigation)[at] = source[at];
}

int slicks_race_add_car_sprite(struct SlicksRaceRuntime *race,
                               unsigned short vehicle,
                               unsigned short base_direction,
                               const unsigned char *resource,
                               unsigned long resource_size)
{
    if (!race || vehicle >= SLICKS_VEHICLE_COUNT ||
        base_direction >= SLICKS_CAR_BASE_DIRECTIONS)
        return -1;
    return decode_sprite(&race->sprites[vehicle][base_direction], resource,
                         resource_size);
}

int slicks_race_add_car_properties(struct SlicksRaceRuntime *race,
                                   unsigned short vehicle,
                                   const unsigned char *resource,
                                   unsigned long resource_size)
{
    struct SlicksCarProperties *properties;
    unsigned short at;
    if (!race || !resource || vehicle >= SLICKS_VEHICLE_COUNT ||
        resource_size != SLICKS_CAR_PROPERTY_SIZE)
        return -1;
    properties = &race->properties[vehicle];
    for (at = 0; at < SLICKS_CAR_PROPERTY_SIZE; ++at)
        properties->raw[at] = resource[at];

    /* The original loader transposes the 34-byte .omi record into its
     * per-car property tables.  These fields have been traced through the
     * original collision and driving routines; the remaining bytes stay in
     * raw[] until their behaviour is recovered. */
    properties->body_radius_x = resource[0];
    properties->body_radius_y = resource[1];
    properties->collision_radius = resource[2];
    properties->model_class = resource[3];
    properties->top_speed = resource[4];
    properties->drive_response = resource[5];
    properties->steering = resource[6];
    properties->collision_weight = resource[22];
    for (at = 0; at < SLICKS_SURFACE_GROUP_COUNT; ++at) {
        unsigned short source_at = 7 + at * 3;
        properties->surface[at][0] = (signed char)resource[source_at];
        properties->surface[at][1] = (signed char)resource[source_at + 2];
        properties->surface[at][2] = (signed char)resource[source_at + 1];
    }
    properties->balance_bias = (short)((resource[23] - 100) * 2);
    properties->effect_profile = resource[24];
    properties->engine_sound = (signed char)resource[25];
    properties->collision_sound = resource[26];
    properties->surface_sound = resource[27];
    properties->smoke_profile = resource[28];
    properties->engine_volume = resource[29];
    properties->auxiliary_accumulator = (signed char)resource[31];
    properties->ai_speed = resource[32];
    properties->ai_aggression = resource[33];
    if (!properties->top_speed || !properties->steering ||
        !properties->collision_weight)
        return -1;
    properties->ready = 1;
    return 0;
}

int slicks_race_add_font(struct SlicksRaceRuntime *race,
                         const unsigned char *resource,
                         unsigned long resource_size)
{
    struct SlicksRaceFont *font;
    unsigned short glyph;
    unsigned short pixel_count = 0;
    unsigned long data_at;
    if (!race || !resource || resource_size < 12)
        return -1;
    font = &race->font;
    font->glyph_count = resource[4];
    font->height = resource[6];
    if (!font->glyph_count || font->glyph_count > SLICKS_FONT_GLYPH_MAX ||
        !font->height)
        return -1;
    data_at = 12UL + (unsigned long)font->glyph_count * 2UL;
    if (data_at > resource_size)
        return -1;
    for (glyph = 0; glyph < font->glyph_count; ++glyph) {
        unsigned short glyph_pixels;
        font->codes[glyph] = resource[12 + glyph];
        font->widths[glyph] = resource[12 + font->glyph_count + glyph];
        glyph_pixels = font->widths[glyph] * font->height;
        if ((unsigned long)pixel_count + glyph_pixels >
            SLICKS_FONT_PIXEL_MAX)
            return -1;
        pixel_count += glyph_pixels;
    }
    if (data_at + pixel_count != resource_size)
        return -1;
    for (glyph = 0; glyph < pixel_count; ++glyph)
        font->pixels[glyph] = resource[data_at + glyph];
    font->pixel_count = pixel_count;
    font->ready = 1;
    return 0;
}

int slicks_race_add_start_light(struct SlicksRaceRuntime *race,
                                unsigned short light,
                                const unsigned char *resource,
                                unsigned long resource_size)
{
    unsigned short at;
    if (!race || !resource || light >= SLICKS_START_LIGHT_COUNT ||
        resource_size != 3UL + SLICKS_START_LIGHT_PIXEL_COUNT ||
        resource[0] != 0 || resource[1] != SLICKS_START_LIGHT_WIDTH ||
        resource[2] != SLICKS_START_LIGHT_HEIGHT)
        return -1;
    for (at = 0; at < SLICKS_START_LIGHT_PIXEL_COUNT; ++at)
        race->start_lights[light].pixels[at] = resource[3 + at];
    race->start_lights[light].ready = 1;
    return 0;
}

int slicks_race_add_trail_sprite(struct SlicksRaceRuntime *race,
                                 unsigned short frame,
                                 const unsigned char *resource,
                                 unsigned long resource_size)
{
    struct SlicksCarSprite *sprite;
    if (!race || frame >= SLICKS_TRAIL_SPRITE_COUNT)
        return -1;
    sprite = &race->trail_sprites[frame];
    if (decode_sprite(sprite, resource, resource_size) != 0 ||
        (unsigned short)sprite->width * sprite->height >
            SLICKS_TRAIL_PIXEL_MAX) {
        sprite->ready = 0;
        return -1;
    }
    return 0;
}

int slicks_race_start(struct SlicksRaceRuntime *race, unsigned char *logical,
                      unsigned char *chunky)
{
    static const unsigned char default_vehicle[SLICKS_RACE_CAR_COUNT] = {
        5, 2, 0, 0
    };
    static const short default_steering_scale[SLICKS_RACE_CAR_COUNT] = {
        700, 1000, 850, 1000
    };
    unsigned short car;
    unsigned short direction;
    unsigned short x;
    unsigned short y;
    if (!race || !logical || !chunky || !race->navigation.zone_count ||
        !race->font.ready)
        return -1;
    race->chunky = chunky;
    for (y = 0; y < SLICKS_TRACK_HEIGHT; ++y)
        for (x = 0; x < SLICKS_SCREEN_WIDTH; ++x)
            race->material_map[(unsigned long)y * SLICKS_SCREEN_WIDTH + x] =
                read_pixel(logical, x, y) >> 3;
    for (car = 0; car < SLICKS_VEHICLE_COUNT; ++car)
        if (!race->properties[car].ready)
            return -1;
    for (car = 0; car < SLICKS_START_LIGHT_COUNT; ++car)
        if (!race->start_lights[car].ready)
            return -1;
    for (car = 0; car < SLICKS_TRAIL_SPRITE_COUNT; ++car)
        if (!race->trail_sprites[car].ready)
            return -1;
    for (car = 0; car < SLICKS_VEHICLE_COUNT; ++car)
        for (direction = 0; direction < SLICKS_CAR_BASE_DIRECTIONS;
             ++direction)
            if (!race->sprites[car][direction].ready)
                return -1;

    for (car = 0; car < SLICKS_RACE_CAR_COUNT; ++car) {
        struct SlicksRaceCar *state = &race->cars[car];
        short side = car < 2 ? 426 : -426;
        short forward = (car == 0 || car == 3) ? -426 : 426;
        unsigned short start_direction;
        state->heading =
            (short)race->navigation.start_heading * 0x78;
        while (state->heading >= SLICKS_HEADING_FULL)
            state->heading -= SLICKS_HEADING_FULL;
        start_direction =
            (unsigned short)state->heading / SLICKS_HEADING_STEP;
        state->x = (long)race->navigation.start_x * 100L +
            (long)side * direction_y[start_direction] / 100L +
            (long)forward * direction_x[start_direction] / 100L;
        state->y = (long)race->navigation.start_y * 100L -
            (long)side * direction_x[start_direction] / 100L +
            (long)forward * direction_y[start_direction] / 100L;
        state->style = (unsigned char)car;
        state->vehicle = default_vehicle[car];
        state->waypoint = 0;
        state->lap = 1;
        state->speed = 0;
        state->speed_fixed = 0;
        state->steering_amount = (unsigned char)(car ? 140 : 100);
        state->steering_scale = default_steering_scale[car];
        state->steering_penalty = 0;
        state->steering_property = 104;
        state->ai_last_x = state->x;
        state->ai_last_y = state->y;
        state->ai_stuck_ticks = 150;
        draw_car(race, logical, car);
    }
    draw_timers(race, logical);
    draw_start_light(race, logical, 0);
    race->countdown_ticks = 0x78;
    race->countdown_stage = 0;
    race->racing = 0;
    race->laps_to_run = 4;
    race->started = 1;
    return 0;
}

void slicks_race_set_controls(struct SlicksRaceRuntime *race,
                              unsigned char controls,
                              unsigned char human_control)
{
    race->controls = controls;
    race->human_control = human_control;
}

void slicks_race_set_vehicle(struct SlicksRaceRuntime *race,
                            unsigned short car, unsigned short vehicle)
{
    if (race && car < SLICKS_RACE_CAR_COUNT &&
        vehicle < SLICKS_VEHICLE_COUNT)
        race->cars[car].vehicle = (unsigned char)vehicle;
}

void slicks_race_set_laps(struct SlicksRaceRuntime *race,
                         unsigned short laps)
{
    if (race)
        race->laps_to_run = (unsigned char)(laps ? laps : 1);
}

void slicks_race_step(struct SlicksRaceRuntime *race, unsigned char *logical)
{
    unsigned short car;
    if (!race || !logical || !race->started)
        return;
    if (!race->racing) {
        race->countdown_ticks -= 2;
        if (race->countdown_ticks < 0) {
            ++race->countdown_stage;
            race->countdown_ticks = 10;
            if (race->countdown_stage < SLICKS_START_LIGHT_COUNT)
                draw_start_light(race, logical, race->countdown_stage);
            else if (race->countdown_stage == SLICKS_START_LIGHT_COUNT)
                restore_start_light(race, logical);
            if (race->countdown_stage > 5)
                race->racing = 1;
        }
        ++race->frame_count;
        return;
    }
    /* Saved-under images contain any cars drawn before them.  Restore in the
     * opposite order so the final restore exposes the real track surface. */
    for (car = SLICKS_RACE_CAR_COUNT; car > 0; --car)
        restore_car(race, logical, &race->cars[car - 1]);
    restore_trail_particles(race, logical);
    advance_trail_particles(race);
    for (car = 0; car < SLICKS_RACE_CAR_COUNT; ++car)
        update_car(race, car);
    resolve_car_collisions(race);
    draw_timers(race, logical);
    draw_trail_particles(race, logical);
    for (car = 0; car < SLICKS_RACE_CAR_COUNT; ++car)
        draw_car(race, logical, car);
    if (race->race_complete && !race->results_drawn)
        draw_results(race, logical);
    ++race->frame_count;
}

void slicks_race_clear_dirty_rows(struct SlicksRaceRuntime *race)
{
    if (race)
        race->dirty_row_count = 0;
}
