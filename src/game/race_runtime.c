#include "race_runtime.h"

#define SLICKS_SCREEN_WIDTH 320
#define SLICKS_TRACK_HEIGHT 190
#define SLICKS_SCREEN_HEIGHT 200
#define SLICKS_STRIDE 100U
#define SLICKS_PLANE_SIZE 0x10000UL
#define SLICKS_HEADING_STEP 0x4b0
#define SLICKS_HEADING_FULL 0x4b00
#define SLICKS_SKID_COLOUR 128
#define SLICKS_TIMER_COLOUR 254

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

static unsigned char read_pixel(const unsigned char *logical,
                                unsigned short x, unsigned short y)
{
    unsigned long offset = (unsigned long)y * SLICKS_STRIDE + (x >> 2);
    offset += (unsigned long)(x & 3) * SLICKS_PLANE_SIZE;
    return logical[offset];
}

static void write_pixel(unsigned char *logical, unsigned short x,
                        unsigned short y, unsigned char colour)
{
    unsigned long offset;
    if (x >= SLICKS_SCREEN_WIDTH || y >= SLICKS_SCREEN_HEIGHT)
        return;
    offset = (unsigned long)y * SLICKS_STRIDE + (x >> 2);
    offset += (unsigned long)(x & 3) * SLICKS_PLANE_SIZE;
    logical[offset] = colour;
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

static void restore_car(unsigned char *logical, struct SlicksRaceCar *car)
{
    unsigned short x;
    unsigned short y;
    unsigned short at = 0;
    if (!car->saved_valid)
        return;
    for (y = 0; y < car->old_height; ++y) {
        for (x = 0; x < car->old_width; ++x)
            write_pixel(logical, car->old_x + x, car->old_y + y,
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
        &race->sprites[car_index][base_direction];
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
            if (pixel)
                write_pixel(logical, origin_x + x, origin_y + y, pixel);
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

static unsigned char ai_controls(const struct SlicksRaceRuntime *race,
                                 const struct SlicksRaceCar *car)
{
    const struct SlicksTrackZone *zone =
        &race->navigation.zones[car->waypoint];
    long dx = (long)zone->x[2] * 100 - car->x;
    long dy = (long)zone->y[2] * 100 - car->y;
    short target_heading =
        (short)nearest_direction(dx, dy) * SLICKS_HEADING_STEP;
    short difference = heading_difference(target_heading, car->heading);
    short target_speed = (short)zone->speed * 2;
    unsigned char controls = 0;

    if (difference < -SLICKS_HEADING_STEP / 3)
        controls |= SLICKS_CONTROL_LEFT;
    if (difference > SLICKS_HEADING_STEP / 3)
        controls |= SLICKS_CONTROL_RIGHT;
    if (car->speed < target_speed)
        controls |= SLICKS_CONTROL_ACCELERATE;
    if (car->speed > target_speed + 12)
        controls |= SLICKS_CONTROL_BRAKE;
    return controls;
}

static void advance_waypoint(struct SlicksRaceRuntime *race,
                             struct SlicksRaceCar *car)
{
    const struct SlicksTrackZone *zone =
        &race->navigation.zones[car->waypoint];
    short x = (short)(car->x / 100);
    short y = (short)(car->y / 100);

    /* Each original ten-byte navigation record contains a rectangular
     * trigger followed by the point and speed to use while approaching it.
     * Entering that region selects the following record. */
    if (x >= (short)zone->x[0] && x <= (short)zone->x[1] &&
        y >= (short)zone->y[0] && y <= (short)zone->y[1]) {
        ++car->waypoint;
        if (car->waypoint >= race->navigation.zone_count) {
            car->waypoint = 0;
            ++car->lap;
        }
    }
}

static void leave_skidmark(struct SlicksRaceRuntime *race,
                           unsigned char *logical,
                           const struct SlicksRaceCar *car)
{
    unsigned short direction = (unsigned short)car->heading / SLICKS_HEADING_STEP;
    short x = (short)(car->x / 100);
    short y = (short)(car->y / 100);
    short across_x = direction_y[direction] / 25;
    short across_y = -direction_x[direction] / 25;
    write_pixel(logical, x + across_x, y + across_y, SLICKS_SKID_COLOUR);
    write_pixel(logical, x - across_x, y - across_y, SLICKS_SKID_COLOUR);
    race->skidmark_count += 2;
}

static void update_car(struct SlicksRaceRuntime *race, unsigned char *logical,
                       unsigned short car_index)
{
    struct SlicksRaceCar *car = &race->cars[car_index];
    const struct SlicksCarProperties *properties =
        &race->properties[car_index];
    unsigned char controls =
        car_index == 0 && race->human_control
            ? race->controls
            : ai_controls(race, car);
    unsigned char turning = controls & (SLICKS_CONTROL_LEFT |
                                        SLICKS_CONTROL_RIGHT);
    unsigned short direction;

    if ((controls & SLICKS_CONTROL_ACCELERATE) && car->speed < 150) {
        unsigned short acceleration =
            car->acceleration_remainder + properties->acceleration * 2U;
        car->speed += acceleration / 100U;
        car->acceleration_remainder = acceleration % 100U;
        if (car->speed > 150)
            car->speed = 150;
    }
    else if (!(controls & SLICKS_CONTROL_ACCELERATE) && car->speed > 0)
        --car->speed;
    if ((controls & SLICKS_CONTROL_BRAKE) && car->speed > 0)
        car->speed -= car->speed > 3 ? 3 : car->speed;

    if ((controls & SLICKS_CONTROL_LEFT) && car->speed > 5)
        car->heading -=
            (90 + car->speed / 3) * properties->steering / 100;
    if ((controls & SLICKS_CONTROL_RIGHT) && car->speed > 5)
        car->heading +=
            (90 + car->speed / 3) * properties->steering / 100;
    while (car->heading < 0)
        car->heading += SLICKS_HEADING_FULL;
    while (car->heading >= SLICKS_HEADING_FULL)
        car->heading -= SLICKS_HEADING_FULL;

    direction = (unsigned short)car->heading / SLICKS_HEADING_STEP;
    car->x += (long)direction_x[direction] * car->speed / 100;
    car->y += (long)direction_y[direction] * car->speed / 100;
    if (turning && car->speed > 65 && !(race->frame_count & 1))
        leave_skidmark(race, logical, car);
    advance_waypoint(race, car);
    car->elapsed_centiseconds += 2;
}

static void clear_timer_strip(unsigned char *logical)
{
    unsigned short x;
    unsigned short y;
    for (y = SLICKS_TRACK_HEIGHT; y < SLICKS_SCREEN_HEIGHT; ++y)
        for (x = 0; x < SLICKS_SCREEN_WIDTH; ++x)
            write_pixel(logical, x, y, 0);
}

static void draw_digit(unsigned char *logical, unsigned short x,
                       unsigned short y, unsigned char digit)
{
    static const unsigned short glyphs[10] = {
        0x7b6f, 0x2492, 0x73e7, 0x73cf, 0x5bc9,
        0x79cf, 0x79ef, 0x7249, 0x7bef, 0x7bcf
    };
    unsigned short bits = glyphs[digit % 10];
    unsigned short row;
    unsigned short column;
    for (row = 0; row < 5; ++row)
        for (column = 0; column < 3; ++column)
            if (bits & (1U << (14 - row * 3 - column)))
                write_pixel(logical, x + column, y + row,
                            SLICKS_TIMER_COLOUR);
}

static void draw_timers(struct SlicksRaceRuntime *race, unsigned char *logical)
{
    unsigned short car;
    clear_timer_strip(logical);
    for (car = 0; car < SLICKS_RACE_CAR_COUNT; ++car) {
        unsigned short x = 4 + car * 80;
        unsigned short time = race->cars[car].elapsed_centiseconds;
        unsigned short seconds = time / 100;
        unsigned short hundredths = time % 100;
        draw_digit(logical, x, 192, car + 1);
        draw_digit(logical, x + 6, 192, (seconds / 10) % 10);
        draw_digit(logical, x + 10, 192, seconds % 10);
        write_pixel(logical, x + 14, 193, SLICKS_TIMER_COLOUR);
        write_pixel(logical, x + 14, 195, SLICKS_TIMER_COLOUR);
        draw_digit(logical, x + 17, 192, hundredths / 10);
        draw_digit(logical, x + 21, 192, hundredths % 10);
    }
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
                               unsigned short car,
                               unsigned short base_direction,
                               const unsigned char *resource,
                               unsigned long resource_size)
{
    if (!race || car >= SLICKS_RACE_CAR_COUNT ||
        base_direction >= SLICKS_CAR_BASE_DIRECTIONS)
        return -1;
    return decode_sprite(&race->sprites[car][base_direction], resource,
                         resource_size);
}

int slicks_race_add_car_properties(struct SlicksRaceRuntime *race,
                                   unsigned short car,
                                   const unsigned char *resource,
                                   unsigned long resource_size)
{
    struct SlicksCarProperties *properties;
    unsigned short at;
    if (!race || !resource || car >= SLICKS_RACE_CAR_COUNT ||
        resource_size != SLICKS_CAR_PROPERTY_SIZE)
        return -1;
    properties = &race->properties[car];
    for (at = 0; at < SLICKS_CAR_PROPERTY_SIZE; ++at)
        properties->raw[at] = resource[at];

    /* The original loader transposes the 34-byte .omi record into its
     * per-car property tables.  These fields have been traced through the
     * original collision and driving routines; the remaining bytes stay in
     * raw[] until their behaviour is recovered. */
    properties->body_radius_x = resource[0];
    properties->body_radius_y = resource[1];
    properties->collision_radius = resource[2];
    properties->acceleration = resource[4];
    properties->steering = resource[6];
    properties->collision_weight = resource[22];
    if (!properties->acceleration || !properties->steering ||
        !properties->collision_weight)
        return -1;
    properties->ready = 1;
    return 0;
}

int slicks_race_start(struct SlicksRaceRuntime *race, unsigned char *logical)
{
    unsigned short car;
    unsigned short direction;
    if (!race || !logical || !race->navigation.zone_count)
        return -1;
    for (car = 0; car < SLICKS_RACE_CAR_COUNT; ++car)
        if (!race->properties[car].ready)
            return -1;
    for (car = 0; car < SLICKS_RACE_CAR_COUNT; ++car)
        for (direction = 0; direction < SLICKS_CAR_BASE_DIRECTIONS;
             ++direction)
            if (!race->sprites[car][direction].ready)
                return -1;

    for (car = 0; car < SLICKS_RACE_CAR_COUNT; ++car) {
        struct SlicksRaceCar *state = &race->cars[car];
        short lane = (car & 1) ? 4 : -4;
        short row = (short)(car / 2) * 8;
        state->x = ((long)race->navigation.start_x + lane) * 100;
        state->y = ((long)race->navigation.start_y - row) * 100;
        state->heading =
            (short)race->navigation.start_heading * 0x78;
        state->style = (unsigned char)car;
        state->waypoint = 0;
        state->speed = 0;
        draw_car(race, logical, car);
    }
    draw_timers(race, logical);
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

void slicks_race_step(struct SlicksRaceRuntime *race, unsigned char *logical)
{
    unsigned short car;
    if (!race || !logical || !race->started)
        return;
    /* Saved-under images contain any cars drawn before them.  Restore in the
     * opposite order so the final restore exposes the real track surface. */
    for (car = SLICKS_RACE_CAR_COUNT; car > 0; --car)
        restore_car(logical, &race->cars[car - 1]);
    for (car = 0; car < SLICKS_RACE_CAR_COUNT; ++car)
        update_car(race, logical, car);
    draw_timers(race, logical);
    for (car = 0; car < SLICKS_RACE_CAR_COUNT; ++car)
        draw_car(race, logical, car);
    ++race->frame_count;
}
