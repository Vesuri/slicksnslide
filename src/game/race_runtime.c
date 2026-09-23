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

static long absolute_long(long value);

/* DS:4ca4/4d24: the two wheel sample points generated for each of the four
 * entrants and sixteen headings.  Coordinates are relative to the original
 * centre-minus-three car anchor. */
static const signed char wheel_x[4][2][16] = {
    {{2,1,2,1,1,3,4,4,5,4,4,3,1,1,2,1},
     {2,3,4,4,4,6,6,6,5,6,6,6,4,4,4,3}},
    {{2,1,2,1,1,3,4,4,5,4,4,3,1,1,2,1},
     {2,3,4,4,4,6,6,6,5,6,6,6,4,4,4,3}},
    {{2,1,1,0,0,2,3,4,5,4,3,2,0,0,1,1},
     {2,3,4,4,4,6,6,6,5,6,6,6,4,4,4,3}},
    {{2,1,2,1,1,3,4,4,5,4,4,3,1,1,2,1},
     {2,3,4,4,4,6,6,6,5,6,6,6,4,4,4,3}}
};
static const signed char wheel_y[4][2][16] = {
    {{1,4,4,3,2,1,2,1,1,6,6,6,5,4,4,3},
     {4,1,2,1,2,3,4,4,4,3,4,4,5,6,6,6}},
    {{1,4,4,3,2,1,2,1,1,6,6,6,5,4,4,3},
     {4,1,2,1,2,3,4,4,4,3,4,4,5,6,6,6}},
    {{0,4,4,3,2,1,1,0,0,6,6,6,5,4,3,2},
     {4,0,1,1,2,3,4,4,4,2,3,4,5,6,6,6}},
    {{1,4,4,3,2,1,2,1,1,6,6,6,5,4,4,3},
     {4,1,2,1,2,3,4,4,4,3,4,4,5,6,6,6}}
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

static unsigned char dos_vector_direction(long dx, long dy)
{
    long ratio;
    unsigned char direction;
    if (!dy)
        direction = dx < 0 ? 12 : 4;
    else {
        if (dx < -900L || dx > 900L) {
            dx >>= 6;
            dy >>= 6;
        }
        ratio = (dx * 64L) / dy;
        if (dy < 0) {
            if (ratio < -322) direction = 4;
            else if (ratio < -96) direction = 3;
            else if (ratio < -43) direction = 2;
            else if (ratio < -13) direction = 1;
            else if (ratio < 13) direction = 0;
            else if (ratio < 43) direction = 15;
            else if (ratio < 96) direction = 14;
            else if (ratio < 322) direction = 13;
            else direction = 12;
        } else {
            if (ratio < -322) direction = 12;
            else if (ratio < -96) direction = 11;
            else if (ratio < -43) direction = 10;
            else if (ratio < -13) direction = 9;
            else if (ratio < 13) direction = 8;
            else if (ratio < 43) direction = 7;
            else if (ratio < 96) direction = 6;
            else if (ratio < 322) direction = 5;
            else direction = 4;
        }
    }
    return (unsigned char)((direction + 12) & 15);
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
    long dx = (long)zone->x[2] - (car->x / 100L - 3L);
    long dy = (long)zone->y[2] - (car->y / 100L - 3L);
    short target_heading =
        (short)dos_vector_direction(dx, dy) * SLICKS_HEADING_STEP;
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
        long speed = (absolute_long(car->velocity_x) +
                      absolute_long(car->velocity_y)) / 2L;
        unsigned char target_direction = dos_vector_direction(
            (long)zone->x[2] - (car->x / 100L - 3L),
            (long)zone->y[2] - (car->y / 100L - 3L));
        unsigned char velocity_direction = target_direction;
        if (speed > 700L)
            velocity_direction = dos_vector_direction(
                car->velocity_x * 10L / (speed + 1L),
                car->velocity_y * 10L / (speed + 1L));

        /* e204 coasts through ordinary corrections, brakes only beyond five
         * direction sectors, and restores throttle whenever the velocity is
         * already aligned with the route target.  The captured BASIC trace
         * matches this drive decision on 98.81% of ordinary AI samples; the
         * remainder enter f09d's optional recovery/avoidance states. */
        if (speed <= 700L || velocity_direction == target_direction ||
            (difference >= -SLICKS_HEADING_STEP &&
             difference <= SLICKS_HEADING_STEP)) {
            controls |= SLICKS_CONTROL_ACCELERATE;
            controls &= (unsigned char)~SLICKS_CONTROL_BRAKE;
        } else if (difference < -5 * SLICKS_HEADING_STEP ||
                   difference > 5 * SLICKS_HEADING_STEP) {
            controls |= SLICKS_CONTROL_BRAKE;
            controls &= (unsigned char)~SLICKS_CONTROL_ACCELERATE;
        }
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
        return 0;
    return race->material_map[(unsigned long)y * SLICKS_SCREEN_WIDTH + x];
}

static int material_blocks_car(unsigned char material)
{
    /* 1000:c5a0 compares palette_index >> 3 with the six-word table at
     * CS:2db6. Its literal entries are 2,22,23,24,25,26. Material 2 takes the
     * non-contact return used by the start/finish and bridge markings. The
     * latter five compare material-22 with the animated boundary-level word;
     * every other class is also non-blocking. */
    return material >= 22 && material <= 26;
}

static long collision_decay(long velocity)
{
    return velocity * 10L / 16L;
}

static void resolve_track_velocity(const struct SlicksRaceRuntime *race,
                                   struct SlicksRaceCar *car,
                                   short x, short y)
{
    unsigned char left = (unsigned char)material_blocks_car(
        material_at(race, (short)(x - 1), y));
    unsigned char up = (unsigned char)material_blocks_car(
        material_at(race, x, (short)(y - 1)));
    unsigned char right = (unsigned char)material_blocks_car(
        material_at(race, (short)(x + 1), y));
    unsigned char down = (unsigned char)material_blocks_car(
        material_at(race, x, (short)(y + 1)));
    long old_x = car->velocity_x;
    long old_y = car->velocity_y;

    /* 1000:c63e classifies the four neighbouring collision samples.  Its
     * multiply/divide sequences are signed component * 10 / 16. */
    if (left && right) {
        car->velocity_x = -collision_decay(old_x);
        car->velocity_y = collision_decay(old_y);
    } else if (up && down) {
        car->velocity_x = collision_decay(old_x);
        car->velocity_y = -collision_decay(old_y);
    } else if ((left && up) || (right && down)) {
        car->velocity_x = -collision_decay(old_y);
        car->velocity_y = collision_decay(old_x);
    } else if ((up && right) || (left && down)) {
        car->velocity_x = collision_decay(old_y);
        car->velocity_y = -collision_decay(old_x);
    } else if (up || down) {
        car->velocity_x = collision_decay(old_x);
        car->velocity_y = -collision_decay(old_y);
    } else if (left || right) {
        car->velocity_x = -collision_decay(old_x);
        car->velocity_y = collision_decay(old_y);
    } else {
        car->velocity_x = -collision_decay(old_x);
        car->velocity_y = -collision_decay(old_y);
    }
}

static int move_car_through_track(const struct SlicksRaceRuntime *race,
                                  struct SlicksRaceCar *car,
                                  long previous_x, long previous_y)
{
    short old_x = (short)(previous_x / 100L);
    short old_y = (short)(previous_y / 100L);
    short new_x = (short)(car->x / 100L);
    short new_y = (short)(car->y / 100L);
    short delta_x = (short)(old_x - new_x);
    short delta_y = (short)(old_y - new_y);
    short abs_x = delta_x < 0 ? (short)-delta_x : delta_x;
    short abs_y = delta_y < 0 ? (short)-delta_y : delta_y;
    short sign_x = delta_x < 0 ? 1 : -1;
    short sign_y = delta_y < 0 ? 1 : -1;
    short clear_x = old_x;
    short clear_y = old_y;
    unsigned short step;
    unsigned short count = abs_y > abs_x ? (unsigned short)abs_y :
                                           (unsigned short)abs_x;
    unsigned char saw_clear = 0;

    for (step = 0; step <= count; ++step) {
        short x;
        short y;
        if (abs_y > abs_x) {
            x = (short)(old_x -
                (abs_y ? (short)((long)step * delta_x / abs_y) : 0));
            y = (short)(old_y + sign_y * (short)step);
        } else if (abs_x) {
            x = (short)(old_x + sign_x * (short)step);
            y = (short)(old_y - (short)((long)step * delta_y / abs_x));
        } else {
            x = old_x;
            y = old_y;
        }
        if (material_blocks_car(material_at(race, x, y))) {
            if (!saw_clear)
                continue;
            car->x = (long)clear_x * 100L + 50L;
            car->y = (long)clear_y * 100L + 50L;
            resolve_track_velocity(race, car, clear_x, clear_y);
            return 1;
        }
        clear_x = x;
        clear_y = y;
        saw_clear = 1;
    }
    return 0;
}

static void interpolate_drive_coefficients(struct SlicksRaceCar *car)
{
    static const signed char source_index[7] = {1, 1, 0, 0, 0, 3, 1};
    static const short table[7][6] = {
        {99, 103, 107, 111, 115, 118},
        {72, 18, 4, -10, -26, -40},
        {110, 100, 93, 86, 80, 75},
        {103, 99, 95, 91, 87, 84},
        {90, 100, 110, 118, 125, 125},
        {110, 100, 80, 60, 45, 30},
        {100, 104, 108, 112, 116, 120}
    };
    unsigned short coefficient;

    /* 2000:e032 copies these tables from DS:1175/117c, then linearly
     * interpolates adjacent entries in quarter steps. */
    for (coefficient = 0; coefficient < 7; ++coefficient) {
        short source = car->drive_setup[
            (unsigned char)source_index[coefficient]];
        short quotient = source / 4;
        short remainder = source % 4;
        car->drive_coefficients[coefficient] = (short)(
            (table[coefficient][quotient] * (4 - remainder) +
             table[coefficient][quotient + 1] * remainder) / 4);
    }
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
                &race->trail_sprites[particle->sprite];
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

static unsigned short next_random(struct SlicksRaceRuntime *race)
{
    /* The DOS helper at 1010:32a7 is Borland's 32-bit LCG. */
    race->random_state = race->random_state * 0x015a4e35UL + 1UL;
    return (unsigned short)((race->random_state >> 16) & 0x7fffUL);
}

static short random_scaled(struct SlicksRaceRuntime *race,
                           unsigned short limit)
{
    return (short)(((unsigned long)next_random(race) * limit) / 0x8000UL);
}

static void add_trail_component(struct SlicksRaceRuntime *race,
                                short x, short y, unsigned char sprite,
                                unsigned char surface, short velocity_x,
                                short velocity_y, unsigned char lifetime)
{
    struct SlicksTrailParticle *particle;
    if (race->trail_particle_count >= SLICKS_TRAIL_PARTICLE_MAX)
        return;
    particle = &race->trail_particles[race->trail_particle_count++];
    particle->x = (long)x * 64L;
    particle->y = (long)y * 64L;
    particle->velocity_x = velocity_x;
    particle->velocity_y = velocity_y;
    particle->lifetime = lifetime;
    particle->sprite = sprite;
    particle->surface = surface;
    particle->saved_valid = 0;
    ++race->skidmark_count;
}

static void emit_wheel_surface(struct SlicksRaceRuntime *race,
                               const struct SlicksRaceCar *car,
                               unsigned short car_index)
{
    long magnitude = (absolute_long(car->velocity_x) +
                      absolute_long(car->velocity_y)) / 2L;
    unsigned short direction = (unsigned short)car->heading /
                               SLICKS_HEADING_STEP;
    unsigned short wheel;
    if (magnitude <= 200L)
        return;
    for (wheel = 0; wheel < 2; ++wheel) {
        short x = (short)(car->x / 100L) - 3 +
                  wheel_x[car_index][wheel][direction];
        short y = (short)(car->y / 100L) - 3 +
                  wheel_y[car_index][wheel][direction];
        unsigned char surface;
        unsigned char sprite;
        short radius;
        short sample_x;
        short sample_y;
        unsigned char sampled_surface;
        if (x < 0 || x >= SLICKS_SCREEN_WIDTH ||
            y < 0 || y >= SLICKS_TRACK_HEIGHT)
            continue;
        surface = race->surface_map[(unsigned long)y * 320UL + x];
        if (surface == 5)
            sprite = (unsigned char)random_scaled(race, 3);
        else if (surface == 3)
            sprite = (unsigned char)random_scaled(race, 3);
        else
            continue;
        radius = (short)(magnitude / 120L);
        sample_x = (short)(x + random_scaled(race, (unsigned short)radius) -
                           radius / 2);
        sample_y = (short)(y + random_scaled(race, (unsigned short)radius) -
                           radius / 2);
        if (sample_x < 0 || sample_x >= SLICKS_SCREEN_WIDTH ||
            sample_y < 0 || sample_y >= SLICKS_TRACK_HEIGHT)
            continue;
        sampled_surface = race->surface_map[
            (unsigned long)sample_y * 320UL + sample_x];
        if (sampled_surface != 2 && sampled_surface != 15 &&
            (sampled_surface < 22 || sampled_surface > 26))
            add_trail_component(race, sample_x, sample_y, sprite, surface,
                                0, 0, 3);
        if (magnitude > 250L) {
            unsigned char lifetime =
                (unsigned char)(random_scaled(race, 10) + 15);
            short velocity_x = (short)(random_scaled(race, 23) - 11);
            short velocity_y = (short)(random_scaled(race, 23) - 11);
            add_trail_component(
                race, x, y, sprite, surface, velocity_x, velocity_y,
                lifetime);
        }
    }
}

static void draw_trail_particles(struct SlicksRaceRuntime *race,
                                 unsigned char *logical)
{
    unsigned short at;
    for (at = 0; at < race->trail_particle_count; ++at) {
        struct SlicksTrailParticle *particle = &race->trail_particles[at];
        unsigned short frame = particle->sprite;
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
    const unsigned short timestep = 2;
    struct SlicksRaceCar *car = &race->cars[car_index];
    unsigned char controls =
        car_index == 0 && race->human_control
            ? race->controls
            : ai_controls(race, car_index);
    unsigned char steering_input =
        (unsigned char)(car_index == 0 && race->human_control ? 100 : 140);
    unsigned char active_drive;
    long steering_step;
    long force_divisor;
    long force_x;
    long force_y;
    long velocity_divisor;
    short velocity_factor;
    unsigned short direction;
    long previous_x = car->x;
    long previous_y = car->y;

    if (car->finished)
        controls = 0;
    /* The DOS car state stores speed as a signed 32-bit fixed quantity.
     * Throttle adds 0xa0 per simulation quantum and .omi byte four supplies
     * the limit in hundreds; neither value is a C-era tuning estimate. */
    active_drive = controls & (SLICKS_CONTROL_ACCELERATE |
                               SLICKS_CONTROL_BRAKE);
    if (controls & SLICKS_CONTROL_ACCELERATE) {
        car->speed_fixed += (long)timestep * 0xa0L;
        if (car->speed_fixed / 100L > car->maximum_speed)
            car->speed_fixed = (long)car->maximum_speed * 100L;
    } else if (!active_drive && car->speed_fixed > 0) {
        /* 2000:0e5e multiplies car-state +10h by the Q15 factor held in
         * DS:53fe.  aeb6 constructs the per-driver table by subtracting the
         * signed setup bias from 7bddh.  This reproduces captured coast
         * sequences such as 10000, 9676, 9363 exactly. */
        long coast_factor = 0x7bddL - car->drive_bias;
        car->speed_fixed = car->speed_fixed * coast_factor / 0x8000L;
    }
    if (controls & SLICKS_CONTROL_BRAKE) {
        /* The ordinary 2000:05b2 brake path first damps both velocity
         * components with Q15 factor 8 in a loop bounded by the elapsed
         * simulation quanta, then clears the longitudinal drive scalar. */
        long brake_factor = 0x7db5L - car->drive_bias;
        unsigned short quantum;
        for (quantum = 0; quantum < timestep; ++quantum) {
            car->velocity_x = car->velocity_x * brake_factor >> 15;
            car->velocity_y = car->velocity_y * brake_factor >> 15;
        }
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
    /* 2000:0ea2..121c is a pair of signed 32-bit force and decay updates.
     * The paired DOS hooks prove every operand and result across 27,030
     * component updates.  The 23 branch is coasting; longitudinal input uses
     * 38.  Integer division truncates toward zero, as the 286 helper does. */
    force_divisor = (long)car->drive_coefficients[3] *
        car->drive_coefficients[0];
    force_divisor *= car->tyre_load / 70 + 10;
    force_divisor *= active_drive ? 38L : 23L;
    force_x = (long)direction_x[direction] * car->speed_fixed * 200L /
        force_divisor;
    force_y = (long)direction_y[direction] * car->speed_fixed * 200L /
        force_divisor;
    velocity_factor = (short)((active_drive ? 0x7dc2L : 0x7bd7L) -
                              car->drive_bias);
    velocity_divisor = 0x8000L + car->drive_coefficients[1];
    car->velocity_x = force_x +
        car->velocity_x * velocity_factor / velocity_divisor;
    car->velocity_y = force_y +
        car->velocity_y * velocity_factor / velocity_divisor;

    /* The original position step multiplies velocity by a normal-game time
     * scale of 100 and divides by 2000. */
    car->x += car->velocity_x / 20L;
    car->y += car->velocity_y / 20L;
    if (move_car_through_track(race, car, previous_x, previous_y)) {
        if (!car->touching_solid)
            ++race->track_collision_count;
        car->touching_solid = 1;
    } else
        car->touching_solid = 0;
    /* 2000:1357..146b applies these fixed-point centre clamps after the
     * material and car-contact walker and deliberately leaves velocity alone. */
    if (car->x < 300L)
        car->x = 300L;
    else if (car->x > 31700L)
        car->x = 31700L;
    if (car->y < 300L)
        car->y = 300L;
    else if (car->y > 17900L)
        car->y = 17900L;
    car->speed = (short)(car->speed_fixed / 100L);
    while (car->heading < 0)
        car->heading += SLICKS_HEADING_FULL;
    while (car->heading >= SLICKS_HEADING_FULL)
        car->heading -= SLICKS_HEADING_FULL;
    slicks_race_resolve_car_collisions(race, car_index);
    emit_wheel_surface(race, car, car_index);
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

void slicks_race_resolve_car_collisions(struct SlicksRaceRuntime *race,
                                        unsigned short current)
{
    struct SlicksRaceCar *a = &race->cars[current];
    const struct SlicksCarProperties *pa = &race->properties[a->vehicle];
    long speed = (absolute_long(a->velocity_x) +
                  absolute_long(a->velocity_y)) / 2L;
    long probe_x = a->x + a->velocity_x * 10L / (speed + 1L);
    long probe_y = a->y + a->velocity_y * 10L / (speed + 1L);
    long extent = (long)pa->collision_radius * 50L;
    unsigned short other;
    unsigned char hit = 0;

    /* 2000:2d27..31bd tests each updated car against all four entrants.  The
     * .omi byte is a full collision-box width: DOS compares a point ten fixed
     * units ahead of the current car with +/- width*50 around the other car.
     * It never separates positions.  A per-car latch, rather than a pair
     * table, suppresses further impulses until that car has no overlap. */
    for (other = 0; other < SLICKS_RACE_CAR_COUNT; ++other) {
        struct SlicksRaceCar *b;
        const struct SlicksCarProperties *pb;
        long delta_vx;
        long delta_vy;
        long ratio;
        if (other == current)
            continue;
        b = &race->cars[other];
        pb = &race->properties[b->vehicle];
        if (a->finished || b->finished)
            continue;
        if (probe_x < b->x - extent || probe_x > b->x + extent ||
            probe_y < b->y - extent || probe_y > b->y + extent)
            continue;
        hit = 1;
        if (!a->touching_car) {
            long magnitude;
            delta_vx = a->velocity_x - b->velocity_x;
            delta_vy = a->velocity_y - b->velocity_y;
            magnitude = absolute_long(delta_vx) + absolute_long(delta_vy);
            ratio = (long)pb->collision_weight * 100L /
                    pa->collision_weight;
            a->velocity_x -= delta_vx * ratio / 100L;
            a->velocity_y -= delta_vy * ratio / 100L;
            ratio = (long)pa->collision_weight * 100L /
                    pb->collision_weight;
            b->velocity_x += delta_vx * ratio / 100L;
            b->velocity_y += delta_vy * ratio / 100L;
            /* 2000:30b0..317f stores a one-update impact magnitude for both
             * cars. Keep the three signed divisions separate: their
             * truncation points are part of the DOS result. */
            a->collision_impact = magnitude * pb->collision_weight / 2L;
            a->collision_impact /= pa->collision_weight;
            a->collision_impact /= 5L;
            b->collision_impact = magnitude * pa->collision_weight / 2L;
            b->collision_impact /= pb->collision_weight;
            b->collision_impact /= 5L;
            if ((unsigned long)a->collision_impact > race->collision_impact)
                race->collision_impact = (unsigned long)a->collision_impact;
            if ((unsigned long)b->collision_impact > race->collision_impact)
                race->collision_impact = (unsigned long)b->collision_impact;
            ++race->collision_count;
        }
        a->touching_car = 1;
        b->touching_car = 1;
    }
    if (!hit)
        a->touching_car = 0;
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
    properties->property_4 = resource[4];
    properties->property_5 = resource[5];
    properties->property_6 = resource[6];
    properties->collision_weight = resource[22];
    for (at = 0; at < SLICKS_SURFACE_GROUP_COUNT; ++at) {
        unsigned short source_at = 7 + at * 3;
        properties->surface[at][0] = (signed char)resource[source_at];
        properties->surface[at][1] = (signed char)resource[source_at + 2];
        properties->surface[at][2] = (signed char)resource[source_at + 1];
    }
    properties->effect_profile = resource[24];
    properties->engine_sound = (signed char)resource[25];
    properties->collision_sound = resource[26];
    properties->surface_sound = resource[27];
    properties->smoke_profile = resource[28];
    properties->engine_volume = resource[29];
    properties->auxiliary_accumulator = (signed char)resource[31];
    /* 1000:ed92..ede9 divides collision impulse by byte 32 before applying
     * the selected driver's impact coefficient to all four damage channels. */
    properties->impact_resistance = resource[32];
    properties->property_33 = resource[33];
    if (!properties->property_4 || !properties->property_6 ||
        !properties->collision_weight || !properties->impact_resistance)
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
    static const short default_drive_bias[SLICKS_RACE_CAR_COUNT] = {
        0, -8, 0, 0
    };
    unsigned short car;
    unsigned short direction;
    if (!race || !logical || !chunky || !race->navigation.zone_count ||
        !race->font.ready)
        return -1;
    race->chunky = chunky;
    race->random_state = 0x1fadec20UL;
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
        state->drive_bias = default_drive_bias[car];
        for (direction = 0; direction < 13; ++direction)
            state->drive_setup[direction] = direction < 5 ? 4 : 0;
        interpolate_drive_coefficients(state);
        state->tyre_load = 0;
        state->maximum_speed = 100;
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
    race->collision_impact = 0;
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
