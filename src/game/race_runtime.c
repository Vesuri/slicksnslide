#include "race_runtime.h"
#include "../ui/race_hud.h"
#include "../graphics/row_offsets.h"
#include "../ui/hud_background.h"
#include "../ui/font_resource.h"
#include "track_records.h"
#include "wheel_geometry.h"
#include "arcade_setup.h"
#include "race_timing.h"
#include "finish_rank.h"
#include "../ui/arcade_hud.h"
#include "../ui/menu_icon.h"
#include "moving_probe.h"
#include "animated_boundary.h"

#if defined(__m68k__)
/* Keep the hard-coded particle_runtime.s ABI checked by the target compiler. */
_Static_assert(sizeof(struct SlicksTrailParticle) == 24, "particle stride");
_Static_assert(__builtin_offsetof(struct SlicksTrailParticle, old_x) == 12 &&
               __builtin_offsetof(struct SlicksTrailParticle, saved_under) == 16 &&
               __builtin_offsetof(struct SlicksTrailParticle, colour) == 18 &&
               __builtin_offsetof(struct SlicksTrailParticle, occlusion_limit) == 22,
               "particle draw offsets");
_Static_assert(sizeof(struct SlicksDirtyPixel) == 4 &&
               __builtin_offsetof(struct SlicksDirtyPixel, y) == 2 &&
               SLICKS_DIRTY_PIXEL_MAX == 512, "particle draw dirty-list ABI");
_Static_assert(__builtin_offsetof(struct SlicksTrailParticle, lifetime) == 17,
               "particle lifetime offset");
_Static_assert(__builtin_offsetof(struct SlicksTrailParticle, saved_valid) == 20,
               "particle saved-under flags offset");
_Static_assert(__builtin_offsetof(struct SlicksTrailParticle, permanent) == 21,
               "particle permanent-mark offset");
_Static_assert(__builtin_offsetof(struct SlicksTrailParticle, state) == 23,
               "particle retirement state offset");
#endif

/* Diagnostic switch, set before starting a race; normal builds leave it zero.
 * Keep surface decisions/RNG/sound events running, but bypass the entire pool. */
unsigned char slicks_race_disable_particles;

#define SLICKS_SCREEN_WIDTH 320
#define SLICKS_TRACK_HEIGHT 190
#define SLICKS_POINT_HEIGHT 184 /* Original race actor pool DS:16c4. */
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
static void weapon_ai_request(struct SlicksRaceRuntime *race,unsigned driver);

/* Original 1991f is stateful: query at its race consumers, not eagerly when
 * advancing the clock. The lap increment preceding a query can affect it. */
static unsigned short race_lap_limit(struct SlicksRaceRuntime *race)
{
    if(race->race_mode==5 && race->laps_to_run>=9999) {
        short completed[4];
        for(unsigned i=0;i<4;++i)
            completed[i]=(short)(unsigned short)(race->cars[i].lap-1U);
        race->laps_to_run=(unsigned short)slicks_arcade_lap_limit(
            race->race_mode,race->arcade_seconds,race->game_clock_ticks,
            (short)race->laps_to_run,completed);
    }
    return race->laps_to_run;
}

void slicks_race_set_mode(struct SlicksRaceRuntime *race,short mode,short seconds)
{
    if(race && !race->started) {
        race->race_mode=mode;
        race->arcade_seconds=seconds;
    }
}

static signed char driver_role(const struct SlicksRaceRuntime *race,unsigned driver)
{
    if(race->participation_ready) return race->participation[driver];
    return driver==0 && race->human_control ? -1 : 1;
}

static long multiply_q15_unsigned(long value, unsigned short factor)
{
    /* The 286 helper clears the factor's high word and retains only the low
     * 32 bits of the signed multiply before its arithmetic 15-bit shift. */
    unsigned int product = (unsigned int)(signed int)value *
        (unsigned int)factor;
    return (long)((signed int)product >> 15);
}
static unsigned short next_random(struct SlicksRaceRuntime *race);
static void activate_track_flags(struct SlicksRaceRuntime *race);
static unsigned char shared_actor_pool(const struct SlicksRaceRuntime *race)
{ return race->weapons.ready || race->track_actors_ready; }
static void draw_trail_particles(struct SlicksRaceRuntime *race,
                                 unsigned short bucket);
static void restore_trail_particles(struct SlicksRaceRuntime *race,
                                    unsigned short bucket);
static void emit_sound_event(struct SlicksRaceRuntime *race,
                             unsigned char sample_block,
                             unsigned char flags,
                             unsigned char priority);

static unsigned char read_pixel(const unsigned char *logical,
                                const unsigned char *chunky,
                                unsigned short x, unsigned short y)
{
    if (chunky)
        return chunky[mult320[y] + x];
    unsigned long offset = (unsigned long)y * SLICKS_STRIDE + (x >> 2);
    offset += (unsigned long)(x & 3) * SLICKS_PLANE_SIZE;
    return logical[offset];
}

static void write_pixel(unsigned char *logical, unsigned char *chunky,
                        unsigned short x,
                        unsigned short y,
                        unsigned char colour)
{
    if (x >= SLICKS_SCREEN_WIDTH || y >= SLICKS_SCREEN_HEIGHT)
        return;
    if (logical) {
        unsigned long offset =
            (unsigned long)y * SLICKS_STRIDE + (x >> 2);
        offset += (unsigned long)(x & 3) * SLICKS_PLANE_SIZE;
        logical[offset] = colour;
    }
    if (chunky)
        chunky[mult320[y] + x] = colour;
}

static void mark_dirty_rect(struct SlicksRaceRuntime *race,
                            short left, short top, short right, short bottom)
{
    struct SlicksDirtyRows rows;
    unsigned short at;
    if (left < 0)
        left = 0;
    if (top < 0)
        top = 0;
    if (right > SLICKS_SCREEN_WIDTH)
        right = SLICKS_SCREEN_WIDTH;
    if (bottom > SLICKS_SCREEN_HEIGHT)
        bottom = SLICKS_SCREEN_HEIGHT;
    if (left >= right || top >= bottom)
        return;
    left &= (short)~31;
    right = (short)((right + 31) & (short)~31);
    if (right > SLICKS_SCREEN_WIDTH)
        right = SLICKS_SCREEN_WIDTH;
    rows.left = (unsigned short)left;
    rows.top = (unsigned short)top;
    rows.right = (unsigned short)right;
    rows.bottom = (unsigned short)bottom;

    /* Repeatedly fold overlapping or touching half-open intervals.  Removing
     * a match can expose another overlap, so restart after every union. */
    for (;;) {
        unsigned char merged = 0;
        for (at = 0; at < race->dirty_row_count; ++at) {
            struct SlicksDirtyRows *existing = &race->dirty_rows[at];
            if (rows.left >= existing->left &&
                rows.top >= existing->top &&
                rows.right <= existing->right &&
                rows.bottom <= existing->bottom)
                return;
            if (rows.right < existing->left || rows.left > existing->right ||
                rows.bottom < existing->top || rows.top > existing->bottom)
                continue;
            if (existing->left < rows.left)
                rows.left = existing->left;
            if (existing->top < rows.top)
                rows.top = existing->top;
            if (existing->right > rows.right)
                rows.right = existing->right;
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
        if (race->dirty_rows[at].left < rows.left)
            rows.left = race->dirty_rows[at].left;
        if (race->dirty_rows[at].top < rows.top)
            rows.top = race->dirty_rows[at].top;
        if (race->dirty_rows[at].right > rows.right)
            rows.right = race->dirty_rows[at].right;
        if (race->dirty_rows[at].bottom > rows.bottom)
            rows.bottom = race->dirty_rows[at].bottom;
    }
    race->dirty_rows[0] = rows;
    race->dirty_row_count = 1;
}

static void mark_dirty_rows(struct SlicksRaceRuntime *race,
                            short top, short bottom)
{
    mark_dirty_rect(race, 0, top, SLICKS_SCREEN_WIDTH, bottom);
}

static void mark_dirty_pixel(struct SlicksRaceRuntime *race,
                             short x, short y)
{
    struct SlicksDirtyPixel *pixel;
    if (x < 0 || x >= SLICKS_SCREEN_WIDTH ||
        y < 0 || y >= SLICKS_SCREEN_HEIGHT)
        return;
    if (race->dirty_pixel_count >= SLICKS_DIRTY_PIXEL_MAX) {
        /* Moving particles and timer glyphs share this bounded list. Keep
         * overflow updates through the existing rectangle merge/fallback. */
        mark_dirty_rect(race, x, y, x + 1, y + 1);
        return;
    }
    pixel = &race->dirty_pixels[race->dirty_pixel_count++];
    pixel->x = (unsigned short)x;
    pixel->y = (unsigned char)y;
    pixel->unused = 0;
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
    mark_dirty_rect(race, SLICKS_START_LIGHT_X, SLICKS_START_LIGHT_Y,
                    SLICKS_START_LIGHT_X + SLICKS_START_LIGHT_WIDTH,
                    SLICKS_START_LIGHT_Y + SLICKS_START_LIGHT_HEIGHT);
    if (!race->start_light_visible) {
        for (y = 0; y < SLICKS_START_LIGHT_HEIGHT; ++y)
            for (x = 0; x < SLICKS_START_LIGHT_WIDTH; ++x)
                race->start_light_saved_under[at++] =
                    read_pixel(logical, race->chunky_authoritative
                                    ? race->chunky : 0,
                               SLICKS_START_LIGHT_X + x,
                               SLICKS_START_LIGHT_Y + y);
        race->start_light_visible = 1;
    }
    at = 0;
    for (y = 0; y < SLICKS_START_LIGHT_HEIGHT; ++y)
        for (x = 0; x < SLICKS_START_LIGHT_WIDTH; ++x)
            write_pixel(logical, race->chunky, SLICKS_START_LIGHT_X + x,
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
    mark_dirty_rect(race, SLICKS_START_LIGHT_X, SLICKS_START_LIGHT_Y,
                    SLICKS_START_LIGHT_X + SLICKS_START_LIGHT_WIDTH,
                    SLICKS_START_LIGHT_Y + SLICKS_START_LIGHT_HEIGHT);
    for (y = 0; y < SLICKS_START_LIGHT_HEIGHT; ++y)
        for (x = 0; x < SLICKS_START_LIGHT_WIDTH; ++x)
            write_pixel(logical, race->chunky, SLICKS_START_LIGHT_X + x,
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
    mark_dirty_rect(race, car->old_x, car->old_y,
                    car->old_x + car->old_width,
                    car->old_y + car->old_height);
    if (!logical) {
        unsigned char *destination = race->chunky +
            mult320[car->old_y] + car->old_x;
#if defined(__m68k__)
        extern void slicks_restore_car_chunky(unsigned char *,const unsigned char *,unsigned,unsigned);
        slicks_restore_car_chunky(destination,car->saved_under,car->old_width,car->old_height);
#else
        for (y = 0; y < car->old_height; ++y) {
            for (x = 0; x < car->old_width; ++x)
                destination[x] = car->saved_under[at++];
            destination += SLICKS_SCREEN_WIDTH;
        }
#endif
        car->saved_valid = 0;
        return;
    }
    for (y = 0; y < car->old_height; ++y) {
        for (x = 0; x < car->old_width; ++x)
            write_pixel(logical, race->chunky,
                        car->old_x + x, car->old_y + y,
                        car->saved_under[at++]);
    }
    car->saved_valid = 0;
}

static void restore_shadows(struct SlicksRaceRuntime *race,
                             unsigned char *logical)
{
    short i;
    for (i = SLICKS_RACE_CAR_COUNT - 1; i >= 0; --i) {
        struct SlicksCarShadow *shadow = &race->shadows[i];
        unsigned short x, y, at = 0;
        if (!shadow->saved_valid)
            continue;
        for (y = 0; y < shadow->old_height; ++y)
            for (x = 0; x < shadow->old_width; ++x)
                write_pixel(logical, race->chunky, shadow->old_x + x,
                            shadow->old_y + y, shadow->saved_under[at++]);
        mark_dirty_rect(race, shadow->old_x, shadow->old_y,
                        shadow->old_x + shadow->old_width,
                        shadow->old_y + shadow->old_height);
        shadow->saved_valid = 0;
    }
}

static void advance_shadow(struct SlicksCarShadow *shadow, unsigned char page)
{
    /* 3000:3918: expiry BEFORE drawing; -3 reserves the slot indefinitely. */
    if (shadow->state > 0 && shadow->lifetime && !--shadow->lifetime) {
        if (!page)
            shadow->lifetime = 1;
        else
            shadow->state = -3;
    }
}

static void draw_shadows(struct SlicksRaceRuntime *race, unsigned char *logical)
{
    unsigned short i;
    for (i = 0; i < SLICKS_RACE_CAR_COUNT; ++i) {
        if(!driver_role(race,i)) continue;
        struct SlicksRaceCar *car = &race->cars[i];
        struct SlicksCarShadow *shadow = &race->shadows[i];
        const struct SlicksCarProperties *properties = &race->properties[car->vehicle];
        short inset = properties->body_radius_y;
        short side = properties->body_radius_x;
        short left, top, right, bottom, x, y;
        unsigned short at = 0;
        if (car->special_drive_state > 500) {
            /* 2000:3ebf: no displacement of the main car; only this actor. */
            shadow->x = (short)(car->x / 100) - 3;
            shadow->y = (short)(car->y / 100) - 3 + car->special_drive_state / 500;
            shadow->state = 3;
            shadow->lifetime = 3;
        }
        advance_shadow(shadow, race->actor_page);
        /* Shipped OMI records have side 5 or 7 and inset 1. The bound
         * protects saved-under storage from invalid external resources. */
        if (shadow->state <= 0 || !side || side > 16 || !inset)
            continue;
        left = shadow->x + inset;
        top = shadow->y + inset;
        right = left + side;
        bottom = top + side;
        if (left < 0) left = 0;
        if (top < 0) top = 0;
        if (right > SLICKS_SCREEN_WIDTH) right = SLICKS_SCREEN_WIDTH;
        if (bottom > SLICKS_TRACK_HEIGHT) bottom = SLICKS_TRACK_HEIGHT;
        if (left >= right || top >= bottom)
            continue;
        shadow->old_x = left;
        shadow->old_y = top;
        shadow->old_width = right - left;
        shadow->old_height = bottom - top;
        for (y = top; y < bottom; ++y)
            for (x = left; x < right; ++x) {
                shadow->saved_under[at++] = read_pixel(logical,
                    race->chunky_authoritative ? race->chunky : 0, x, y);
                /* Generated DOS source 1000:c3fc, not a recoloured car. */
                if (!(((x - shadow->x) ^ (y - shadow->y) ^ inset) & 1))
                    write_pixel(logical, race->chunky, x, y, 37);
            }
        shadow->saved_valid = 1;
        mark_dirty_rect(race, left, top, right, bottom);
    }
}

static int actor_pixel_visible(const struct SlicksRaceRuntime *race,
                               unsigned long at, unsigned char limit)
{
    /* 3000:3694 bypasses masking at zero; 4b21..4b24 rejects raw > limit. */
    return !limit ||
        ((race->material_map[at] << 3) | (race->surface_map[at] & 7)) <= limit;
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
    unsigned char occlusion_limit = (unsigned char)(car->actor_layer * 15);

    rotated_size(sprite, rotation, &width, &height);
    origin_x = (short)(car->x / 100) - width / 2;
    origin_y = (short)(car->y / 100) - height / 2;
    if (origin_x < 0 || origin_y < 0 ||
        origin_x + width > SLICKS_SCREEN_WIDTH ||
        origin_y + height > SLICKS_TRACK_HEIGHT)
        return;
    mark_dirty_rect(race, origin_x, origin_y,
                    origin_x + width, origin_y + height);

    car->old_x = origin_x;
    car->old_y = (unsigned char)origin_y;
    car->old_width = width;
    car->old_height = height;
    if (!logical) {
        const unsigned char *source_row;
        short source_dx;
        short source_dy;
        unsigned char *destination = race->chunky +
            mult320[(unsigned short)origin_y] +
            (unsigned short)origin_x;
#if !defined(__m68k__)
        for (y = 0; y < height; ++y) {
            for (x = 0; x < width; ++x)
                car->saved_under[at++] = destination[x];
            destination += SLICKS_SCREEN_WIDTH;
        }
        destination = race->chunky +
            mult320[(unsigned short)origin_y] +
            (unsigned short)origin_x;
#endif
        if (rotation == 1) {
            source_row = sprite->pixels +
                (unsigned short)(sprite->height - 1) * sprite->width;
            source_dx = (short)-sprite->width;
            source_dy = 1;
        } else if (rotation == 2) {
            source_row = sprite->pixels +
                (unsigned short)sprite->width * sprite->height - 1;
            source_dx = -1;
            source_dy = (short)-sprite->width;
        } else if (rotation == 3) {
            source_row = sprite->pixels + sprite->width - 1;
            source_dx = sprite->width;
            source_dy = -1;
        } else {
            source_row = sprite->pixels;
            source_dx = 1;
            source_dy = sprite->width;
        }
#if defined(__m68k__)
        extern void slicks_draw_car_chunky(unsigned char *,const unsigned char *,unsigned char *,
            const unsigned char *,const unsigned char *,unsigned,unsigned,int,int,unsigned,unsigned);
        unsigned long row=mult320[(unsigned short)origin_y]+(unsigned short)origin_x;
        slicks_draw_car_chunky(destination,source_row,car->saved_under,
            race->material_map+row,race->surface_map+row,width,height,
            source_dx,source_dy,car->style*5,occlusion_limit);
#else
        for (y = 0; y < height; ++y) {
            const unsigned char *source = source_row;
            for (x = 0; x < width; ++x) {
                unsigned char pixel = *source;
                if (pixel && actor_pixel_visible(race,
                        mult320[origin_y + y] + origin_x + x,
                        occlusion_limit)) {
                    if (pixel >= 1 && pixel <= 5)
                        pixel += car->style * 5;
                    destination[x] = pixel;
                }
                source += source_dx;
            }
            source_row += source_dy;
            destination += SLICKS_SCREEN_WIDTH;
        }
#endif
        car->saved_valid = 1;
        return;
    }
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x)
            car->saved_under[at++] =
                read_pixel(logical, race->chunky_authoritative
                                        ? race->chunky : 0,
                           origin_x + x, origin_y + y);
    }
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            unsigned char pixel = sprite_pixel(sprite, rotation, x, y);
            if (pixel && actor_pixel_visible(race,
                    mult320[origin_y + y] + origin_x + x,
                    occlusion_limit)) {
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

static void draw_layered_cars(struct SlicksRaceRuntime *race,
                              unsigned char *logical)
{
    unsigned short pass, car;
    /* 2000:3e7d..3eba: nonzero layer has priority 3, zero has 4.
     * Car slots 1..4 precede particle slots at equal priority (3000:3c12).
     * Priority-3 particles therefore sit between the two car groups. */
    for (pass = 0; pass < 2; ++pass) {
        for (car = 0; car < SLICKS_RACE_CAR_COUNT; ++car)
            if (driver_role(race,car) && (!race->cars[car].actor_layer) == pass)
                draw_car(race, logical, car);
        if (!pass)
            draw_trail_particles(race, 1);
    }
}

static void restore_layered_cars(struct SlicksRaceRuntime *race,
                                 unsigned char *logical)
{
    unsigned short pass, car;
    /* Called before simulation changes actor_layer: these are still the
     * layers used by the previous draw. Unwind the saved-under stack. */
    for (pass = 0; pass < 2; ++pass) {
        for (car = SLICKS_RACE_CAR_COUNT; car > 0; --car)
            if ((!!race->cars[car - 1].actor_layer) == pass)
                restore_car(race, logical, &race->cars[car - 1]);
        if (!pass)
            restore_trail_particles(race, 1);
    }
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

static void ai_watchdog(struct SlicksRaceCar *car, unsigned short ticks)
{
    /* f0d8..f162. Sprite-origin comparison is equivalent after subtracting
     * the common three-pixel origin offset. All timers are signed words. */
    if (car->x / 100 != car->ai_last_x / 100 ||
        car->y / 100 != car->ai_last_y / 100) {
        car->ai_last_x = car->x;
        car->ai_last_y = car->y;
        car->ai_stuck_ticks = 150;
    } else if (car->ai_service_state < 2) {
        car->ai_stuck_ticks = (short)(car->ai_stuck_ticks -
                                      (signed char)ticks);
        if (car->ai_stuck_ticks < 0) {
            car->ai_state = 2;
            car->ai_recovery_ticks = 40;
            car->ai_stuck_ticks = 50;
            if (car->ai_service_state)
                car->ai_service_state = -1;
        }
    } else {
        car->ai_stuck_ticks = 100;
    }
}

static unsigned char ai_escape_controls(struct SlicksRaceRuntime *race,
                                        struct SlicksRaceCar *car,
                                        unsigned short ticks)
{
    /* f71c..f7dd uses the OLD direction this update, then possibly chooses
     * the direction for the NEXT update. Expiration still emits controls. */
    unsigned char controls = SLICKS_CONTROL_ACCELERATE |
        (car->ai_recovery_right > 0 ? SLICKS_CONTROL_RIGHT :
                                     SLICKS_CONTROL_LEFT);
    if (car->ai_turn_ticks <= 0) {
        car->ai_recovery_right = (signed char)(
            ((unsigned long)next_random(race) * 2UL) / 32768UL);
        car->ai_turn_ticks = 50;
    }
    car->ai_turn_ticks = (short)(car->ai_turn_ticks - (signed char)ticks);
    car->ai_recovery_ticks = (short)(car->ai_recovery_ticks -
                                    (signed char)ticks);
    if (car->ai_recovery_ticks < 0) {
        car->ai_recovery_ticks = 300;
        car->ai_state = 0;
        car->ai_turn_ticks = 0;
    }
    return controls;
}

static int advance_waypoint(struct SlicksRaceRuntime *race,
                             struct SlicksRaceCar *car);

static void select_ai_alternate(struct SlicksRaceRuntime *race,
                                 struct SlicksRaceCar *car)
{
    unsigned short best = 0, index;
    short distance = 30000;
    if (!race->navigation.alternate_count) {
        car->ai_state = 2;
        car->ai_recovery_ticks = 400;
        return;
    }
    for (index = 1; index < race->navigation.alternate_count; ++index) {
        const struct SlicksTrackPoint *point = &race->navigation.alternate[index];
        short candidate = (short)(
            absolute_long((long)point->x - car->x / 100L - 4L) +
            absolute_long((long)point->y - car->y / 100L - 4L));
        if (candidate < distance) {
            distance = candidate;
            best = index;
        }
    }
    car->ai_target_x = race->navigation.alternate[best].x;
    car->ai_target_y = race->navigation.alternate[best].y;
}

static void ai_contact_transition(struct SlicksRaceCar *car)
{
    /* f162..f1de: timers are signed; contact age/threshold are unsigned. */
    if (car->ai_state == 0 && car->ai_recovery_ticks < 0 &&
        car->ai_contact_ticks < car->ai_contact_threshold) {
        car->ai_contact_ticks = 32000;
        car->ai_state = car->ai_route_seen ? 1 : 2;
        car->ai_recovery_ticks = car->ai_route_seen ? 250 : 350;
    }
    if (car->ai_state == 1 && car->ai_recovery_ticks < 0 &&
        car->ai_contact_ticks < car->ai_contact_threshold) {
        car->ai_state = 2;
        car->ai_recovery_ticks = 400;
    }
}

static void ai_alternate_progress(struct SlicksRaceCar *car, unsigned short ticks)
{
    /* f679..f706 ordinary (non-service) alternate destination. */
    unsigned short x = (unsigned short)(car->x / 100L - 3L);
    unsigned short y = (unsigned short)(car->y / 100L - 3L);
    if (x >= (unsigned short)(car->ai_target_x - 4) &&
        x <= (unsigned short)(car->ai_target_x + 4) &&
        y >= (unsigned short)(car->ai_target_y - 4) &&
        y <= (unsigned short)(car->ai_target_y + 4)) {
        car->ai_recovery_ticks = 400;
        car->ai_state = 0;
        car->ai_route_seen = 0;
        car->ai_target_x = -1;
    }
    if (car->measured_speed < 7000)
        car->ai_recovery_ticks = (short)(car->ai_recovery_ticks - (signed char)ticks);
    else
        car->ai_recovery_ticks = 0;
}

static unsigned char ai_service_approach(struct SlicksRaceCar *car,
                                          unsigned char controls)
{
    /* f504..f5e5 passes each wrapped word subtraction to a long abs
     * helper with a ZERO high word, then compares the signed low-word sum.
     * Do not replace this with Manhattan distance between signed points. */
    unsigned short dx = (unsigned short)(car->x / 100L - 3L - car->ai_target_x);
    unsigned short dy = (unsigned short)(car->y / 100L - 3L - car->ai_target_y);
    short distance = (short)(dx + dy);
    if (distance < 10)
        car->ai_service_state = 2;
    if (distance < 80) {
        controls &= (unsigned char)~(SLICKS_CONTROL_ACCELERATE | SLICKS_CONTROL_BRAKE);
        if (!(car->measured_speed > 700 ||
              (distance < 20 && car->measured_speed > 200) ||
              (distance < 9 && car->measured_speed > 80)))
            controls |= SLICKS_CONTROL_ACCELERATE;
    }
    return controls;
}

static void ai_update_contact_age(struct SlicksRaceCar *car, unsigned short ticks)
{
    /* 3d4a..3d7b tests DS:536c, latched for the whole update, not the
     * last movement substep's wall result. Test before adding; 60000
     * is not a saturating clamp. */
    if (!car->actor_contact) {
        if (car->ai_contact_ticks < 60000)
            car->ai_contact_ticks = (unsigned short)(car->ai_contact_ticks + ticks);
    } else if (!car->touching_car) {
        /* DS:4dae is the existing car-contact latch, set for both cars at
         * 3183..318e and cleared for the current car at 31af..31b8. */
        car->ai_contact_ticks = 0;
    }
}

static unsigned char ai_steering(struct SlicksRaceRuntime *race,
                                 struct SlicksRaceCar *car)
{
    const struct SlicksTrackZone *zone =
        &race->navigation.zones[car->waypoint];
    long dx = (car->ai_state == 1 ? (long)car->ai_target_x : zone->x[2]) -
              (car->x / 100L - 3L);
    long dy = (car->ai_state == 1 ? (long)car->ai_target_y : zone->y[2]) -
              (car->y / 100L - 3L);
    unsigned char target_direction = dos_vector_direction(dx, dy);
    /* e31b..e34a quantizes heading BEFORE subtracting the target sector.
     * Preserve the original +4 convention until wrapping: normalizing both
     * operands first reverses some exactly-opposite (+/-8) turn choices. */
    short difference = car->heading / SLICKS_HEADING_STEP + 4 -
                       ((target_direction + 4) & 15);
    unsigned char controls = car->ai_control_latch &
        (SLICKS_CONTROL_LEFT | SLICKS_CONTROL_RIGHT);
    if (difference > 8) difference -= 16;
    if (difference < -8) difference += 16;


    /* e3ae..e3de changes both steering bytes only for a nonzero sector
     * error. Exactly aligned headings retain last update's steering. The
     * AI branch of the preceding 99cf input routine does not clear it. */
    if (difference != 0)
        controls &= (unsigned char)~(SLICKS_CONTROL_LEFT | SLICKS_CONTROL_RIGHT);
    if (difference > 0)
        controls |= SLICKS_CONTROL_LEFT;
    if (difference < 0)
        controls |= SLICKS_CONTROL_RIGHT;
    {
        long speed = car->measured_speed;
        unsigned char velocity_direction = target_direction;
        /* e2a2..e302 divides measured speed by ten, then divides each
         * component by that rounded result. Neither vx*10/(speed+1) nor
         * recomputing speed after pair collisions is equivalent. */
        if (speed > 700L)
            velocity_direction = dos_vector_direction(
                car->velocity_x / (speed / 10L),
                car->velocity_y / (speed / 10L));

        /* e204 coasts through ordinary corrections, brakes only beyond five
         * direction sectors, and restores throttle whenever the velocity is
         * already aligned with the route target. Recovery and avoidance
         * are separate caller-level states, not part of e204. */
        if (speed <= 700L || velocity_direction == target_direction ||
            (difference >= -1 && difference <= 1)) {
            controls |= SLICKS_CONTROL_ACCELERATE;
            controls &= (unsigned char)~SLICKS_CONTROL_BRAKE;
        } else if (difference < -5 || difference > 5) {
            controls |= SLICKS_CONTROL_BRAKE;
            controls &= (unsigned char)~SLICKS_CONTROL_ACCELERATE;
        }
    }
    return controls;
}

static void ai_request_service(struct SlicksRaceRuntime *race,
                                struct SlicksRaceCar *car)
{
    /* f7dd..f849: use the original 991f limit, including the timed race's
     * lazy final-lap selection after its clock expires. */
    if (!car->ai_service_state && (car->damage[0] > 500 ||
        (race->fuel_option && (signed int)car->fuel_capacity / 4 > (signed int)car->fuel &&
         (short)(race_lap_limit(race) - 1) > (short)(car->lap - 1))))
        car->ai_service_state = -1;
}

static void ai_complete_service(const struct SlicksRaceRuntime *race,
                                 struct SlicksRaceCar *car)
{
    /* f5e5..f676: multiply as wrapped 32-bit quantities, then signed
     * compare. Dividing capacity to a percentage loses DOS rounding. */
    if (car->damage[0] >= 10) return;
    if (race->fuel_option && (signed int)(car->fuel * 10U) <
                             (signed int)(car->fuel_capacity * 9U)) return;
    car->ai_recovery_ticks = 400;
    car->ai_state = 0;
    car->ai_route_seen = 0;
    car->ai_target_x = -1;
    car->ai_service_state = 0;
}

static unsigned char ai_controls(struct SlicksRaceRuntime *race,
                                 unsigned short car_index,
                                 unsigned short ticks)
{
    struct SlicksRaceCar *car = &race->cars[car_index];
    weapon_ai_request(race,car_index);
    ai_watchdog(car, ticks);
    ai_contact_transition(car);
    /* f1de..f7dd uses sequential state checks, not mutually exclusive
     * branches. A route hit may enter state one and steer again now. */
    if (car->ai_state == 0) {
        car->ai_control_latch = ai_steering(race, car);
        if (advance_waypoint(race, car)) {
            car->ai_route_seen = 1;
            car->ai_recovery_ticks = car->measured_speed < 7000 ?
                (short)(400 - (signed char)ticks) : 0;
        }
    }
    if (car->ai_state == 1 && car->ai_target_x < 0)
        select_ai_alternate(race, car);
    if (car->ai_state == 1) {
        car->ai_control_latch = ai_steering(race, car);
        if (car->ai_service_state > 0) {
            car->ai_control_latch = ai_service_approach(car, car->ai_control_latch);
            ai_complete_service(race, car);
        } else
            ai_alternate_progress(car, ticks);
    }
    if (car->ai_state == 2) {
        car->ai_control_latch = ai_escape_controls(race, car, ticks);
        ai_request_service(race, car);
        return car->ai_control_latch;
    }
    ai_request_service(race, car);
    return car->ai_control_latch;
}

static int car_track_sample(const struct SlicksRaceRuntime *race,
                             const struct SlicksRaceCar *car, short x, short y)
{
    return slicks_track_car_sample(race->material_map, race->surface_map,
        x, y, car->actor_layer, car->special_drive_state,
        car->collision_sampling, race->boundary_level);
}

static int update_track_sampling(struct SlicksRaceRuntime *race,
                                  struct SlicksRaceCar *car)
{
    /* 238eb..2399f, before damage consumption and special-state advance. */
    if (car->actor_contact || car->previous_actor_contact || car->special_drive_state)
        return 0;
    if (!car->collision_sampling) {
        int blocked;
        car->collision_sampling = 1;
        blocked = car_track_sample(race, car, (short)(car->x / 100L),
                                    (short)(car->y / 100L));
        if (blocked < 0) return -1;
        if (blocked) car->collision_sampling = 0;
    }
    car->collision_safe_x = (short)(car->x / 100L);
    car->collision_safe_y = (unsigned char)(car->y / 100L);
    return 0;
}

static void apply_surface_contact(struct SlicksRaceCar *car)
{
    /* Real surface-switch case 27: 232f6..23357. */
    if (car->effective_surface != 27) return;
    if (car->collision_sampling) {
        car->velocity_x = (signed int)car->velocity_x / 2;
        car->velocity_y = (signed int)car->velocity_y / 2;
        car->collision_sampling = 0;
    }
    car->previous_actor_contact = 1;
}

static void apply_oil_spin(struct SlicksRaceRuntime *race,
    struct SlicksRaceCar *car,unsigned short ticks)
{
    /* Original 23613..236b4 and 2372c. Keep its two separate long
     * operations, signed tick word and single heading correction. */
    if(car->effective_surface!=18 ||
       !race->properties[car->vehicle].collision_sound) return;
    if(!car->oil_active)
        car->oil_turn_sign=(signed char)((next_random(race)*2UL/32768UL)*2-1);
    unsigned int product=(unsigned int)(signed int)car->measured_speed *
        (unsigned int)(signed int)car->oil_turn_sign;
    signed int turn=(signed int)product/10;
    product=(unsigned int)turn*(unsigned int)(signed int)(short)ticks;
    car->heading=(short)((unsigned short)car->heading+(unsigned short)product);
    if(car->heading<0) car->heading=(short)(car->heading+SLICKS_HEADING_FULL);
    if(car->heading>SLICKS_HEADING_FULL)
        car->heading=(short)(car->heading-SLICKS_HEADING_FULL);
    car->oil_active=1;
}

static void apply_surface_velocity(struct SlicksRaceCar *car,
    const struct SlicksCarProperties *properties,unsigned short ticks)
{
    /* Original 2335a..238eb repeats the low-dword Q15 multiply once per
     * signed tick. Factors are initialized by 2aeb6 from the driver bias;
     * surface 7/8 can amplify velocity, so this is not a generic clamp. */
    unsigned short base;
    switch(car->effective_surface) {
    case 3: case 4: base=0x7dd4; break;
    case 5: case 6: base=0x7ee9; break;
    case 7: case 8: base=0x8118; break;
    case 11: case 12: base=0x7c31; break;
    case 18:
        if(!properties->collision_sound) return;
        base=0x8000; break;
    default: return;
    }
    unsigned short factor=(unsigned short)(base-car->drive_bias);
    for(short tick=0;tick<(short)ticks;++tick) {
        car->velocity_x=multiply_q15_unsigned(car->velocity_x,factor);
        car->velocity_y=multiply_q15_unsigned(car->velocity_y,factor);
    }
}

static short surface_limit(unsigned char value,unsigned short factor,short divisor)
{
    /* Original IMUL retains AX, then CWD/IDIV sign-extends that word. */
    return (short)((short)((unsigned short)value*factor)/divisor);
}

static void update_surface_limits(struct SlicksRaceCar *car,
    const struct SlicksCarProperties *properties,unsigned short ticks)
{
    car->steering_scale=(short)(properties->property_6*10);
    car->maximum_speed=properties->property_4;
    unsigned surface=car->effective_surface,group;
    unsigned steer_factor,speed_factor;
    switch(surface) {
    case 3: group=0; steer_factor=80; speed_factor=80; break;
    case 4: group=0; steer_factor=100; speed_factor=120; break;
    case 5: group=1; steer_factor=70; speed_factor=75; break;
    case 6:
        car->steering_scale=surface_limit(properties->property_6,95,10);
        car->maximum_speed=(unsigned short)surface_limit(properties->property_4,120,100);
        return;
    case 7: case 8:
        if((short)ticks<=0) return;
        group=2; steer_factor=140; speed_factor=30; break;
    case 11: case 12:
        if((short)ticks<=0) return;
        group=3; steer_factor=60; speed_factor=65; break;
    case 18:
        if(!properties->collision_sound) return;
        group=4; steer_factor=200; speed_factor=100; break;
    default: return;
    }
    car->steering_scale=surface_limit((unsigned char)properties->surface[group][1],steer_factor,10);
    car->maximum_speed=(unsigned short)surface_limit((unsigned char)properties->surface[group][0],speed_factor,100);
}

static void update_actor_layer(struct SlicksRaceRuntime *race,
                               struct SlicksRaceCar *car)
{
    short x = (short)(car->x / 100L), y = (short)(car->y / 100L);
    unsigned long at;
    unsigned char material, selected;
    if (x < 0 || x >= 320 || y < 0 || y >= 190)
        return;
    at = mult320[(unsigned short)y] + (unsigned short)x;
    material = race->material_map[at];
    /* 2000:29d2..2a61: choose the surface after entry, before exit.
     * A negative special state also suppresses the layer here. */
    if (!car->actor_layer && !material && !car->special_drive_state &&
        !car->actor_contact)
        car->actor_layer = 1;
    selected = car->actor_layer ? race->surface_map[at] : material;
    car->selected_surface = selected;
    if(selected!=18) car->oil_active=0;
    if (selected == 19 || car->special_drive_state)
        car->actor_layer = 0;
    car->effective_surface = car->special_drive_state ? 0 : selected;
    if (car->special_drive_state)
        car->collision_sampling = 0;
}

static void initialize_car_fuel(struct SlicksRaceRuntime *race,
                                 struct SlicksRaceCar *car)
{
    /* 24c49..24cca: preserve both signed divisions and intervening wrap. */
    signed int base = (signed int)race->fuel_option *
        race->properties[car->vehicle].property_33 / 10;
    unsigned int product = (unsigned int)base *
        (unsigned int)((signed int)car->fuel_upgrade + 46);
    product *= 72U;
    car->fuel_capacity = (unsigned int)((signed int)product / 100);
    car->fuel = car->fuel_capacity - 1U;
}

static void consume_idle_fuel(struct SlicksRaceRuntime *race,
                               struct SlicksRaceCar *car, unsigned short ticks)
{
    car->fuel -= (unsigned int)(signed int)(short)(ticks * 2U);
    if ((signed int)car->fuel < 0) {
        car->fuel = 0;
        if (race->fuel_option && race->damage_enabled)
            car->service_flags |= 1;
    }
}

static void refuel_car_at_pit(struct SlicksRaceCar *car, unsigned short ticks)
{
    if (car->effective_surface != 30 || car->measured_speed >= 300) return;
    car->service_flags &= (unsigned char)~1U;
    car->fuel += (unsigned int)(signed int)(short)((unsigned long)ticks * 60U);
    if ((signed int)car->fuel >= (signed int)car->fuel_capacity)
        car->fuel = car->fuel_capacity - 1U;
}

static int repair_car_at_pit(struct SlicksRaceRuntime *race,
                              struct SlicksRaceCar *car, unsigned short ticks)
{
    unsigned short channel;
    if (car->effective_surface != 30 || car->measured_speed >= 300)
        return 0;
    race->pit_repair_ticks = (short)(race->pit_repair_ticks + ticks);
    if (race->pit_repair_ticks < 5) return 0;
    race->pit_repair_ticks = (short)(race->pit_repair_ticks - 5);
    for (channel = 0; channel < 4; ++channel) {
        car->damage[channel] = (short)(car->damage[channel] - 8);
        if (car->damage[channel] < 0 || !race->damage_scale)
            car->damage[channel] = 0;
    }
    return 1; /* DOS refreshes the status HUD here; full HUD remains open. */
}

static long collision_decay(long velocity)
{
    return (signed int)((unsigned int)velocity * 10U) / 16;
}

static long collision_reflect(long velocity)
{
    return (signed int)(0U - (unsigned int)velocity * 10U) / 16;
}

static void resolve_track_velocity(struct SlicksRaceRuntime *race,
                                   struct SlicksRaceCar *car,
                                   short x, short y)
{
    int left = car_track_sample(race, car, (short)(x - 1), y);
    int up = car_track_sample(race, car, x, (short)(y - 1));
    int right = car_track_sample(race, car, (short)(x + 1), y);
    int down = car_track_sample(race, car, x, (short)(y + 1));
    long old_x = car->velocity_x;
    long old_y = car->velocity_y;
    if (left < 0 || up < 0 || right < 0 || down < 0) {
        race->collision_error = 1;
        return;
    }

    /* 1000:c63e classifies the four neighbouring collision samples.  Its
     * multiply/divide sequences are signed component * 10 / 16. */
    if (left && right) {
        car->velocity_x = collision_decay(old_x);
        car->velocity_y = collision_reflect(old_y);
    } else if (up && down) {
        car->velocity_x = collision_reflect(old_x);
        car->velocity_y = collision_decay(old_y);
    } else if ((left && up) || (right && down)) {
        car->velocity_x = collision_reflect(old_y);
        car->velocity_y = collision_reflect(old_x);
    } else if ((up && right) || (left && down)) {
        car->velocity_x = collision_decay(old_y);
        car->velocity_y = collision_decay(old_x);
    } else if (up || down) {
        car->velocity_x = collision_decay(old_x);
        car->velocity_y = collision_reflect(old_y);
    } else if (left || right) {
        car->velocity_x = collision_reflect(old_x);
        car->velocity_y = collision_decay(old_y);
    } else {
        car->velocity_x = collision_reflect(old_x);
        car->velocity_y = collision_reflect(old_y);
    }
    /* Single-axis contacts use the reflected component, not total speed.
     * c574 takes its signed absolute value before the division by two. */
    if (!(left && right) && !(up && down) &&
        !((left && up) || (right && down)) &&
        !((up && right) || (left && down)) && (left || up || right || down)) {
        long component = (up || down) ? car->velocity_y : car->velocity_x;
        car->pending_damage_impact = (component < 0 ? -component : component) / 2;
    } else
        car->pending_damage_impact = (signed int)car->measured_speed / 3;
    car->x = (short)(x * 100 + 50);
    car->y = (short)(y * 100 + 50);
}

static int move_car_through_track(struct SlicksRaceRuntime *race,
                                  struct SlicksRaceCar *car,
                                  long previous_x, long previous_y,
                                  short *hit_y)
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
    /* cb02's ordinary car caller has no actor probe. A zero-length ray
     * therefore returns without sampling, including a blocked centre. */
    if (!count) return 0;
    if (abs_x < 0 || abs_y < 0) { race->collision_error = 1; return 0; }

    for (step = 0; step <= count; ++step) {
        short x;
        short y;
        if (abs_y > abs_x) {
            x = (short)(old_x -
                (short)((short)((long)step * delta_x) / abs_y));
            y = (short)(old_y + sign_y * (short)step);
        } else if (abs_x) {
            x = (short)(old_x + sign_x * (short)step);
            y = (short)(old_y - (short)((short)((long)step * delta_y) / abs_x));
        } else {
            x = old_x;
            y = old_y;
        }
        int blocked = car_track_sample(race, car, x, y);
        if (blocked < 0) { race->collision_error = 1; return 0; }
        if (blocked) {
            /* Only step zero is ignored, not an arbitrary blocked prefix. */
            if (!step)
                continue;
            resolve_track_velocity(race, car, clear_x, clear_y);
            if (hit_y) *hit_y = clear_y;
            return 1;
        }
        clear_x = x;
        clear_y = y;
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
        if (!remainder) {
            car->drive_coefficients[coefficient] = table[coefficient][quotient];
            continue;
        }
        car->drive_coefficients[coefficient] = (short)(
            (table[coefficient][quotient] * (4 - remainder) +
             table[coefficient][quotient + 1] * remainder) / 4);
    }
}

static int ai_inside_zone(const struct SlicksRaceCar *car,
                           const struct SlicksTrackZone *zone)
{
    /* f1fb..f277 compares DS:53b6/53be (sprite origin), not the centre.
     * The four conditional jumps are unsigned and include both edges. */
    unsigned short x = (unsigned short)(car->x / 100L - 3L);
    unsigned short y = (unsigned short)(car->y / 100L - 3L);
    return x >= zone->x[0] && x <= zone->x[1] &&
           y >= zone->y[0] && y <= zone->y[1];
}

static void ai_route_service_entry(struct SlicksRaceRuntime *race,
                                    struct SlicksRaceCar *car)
{
    unsigned short index, pit = 0;
    if (car->ai_service_state > 0) {
        car->ai_service_state = 0;
        car->ai_contact_threshold = 350;
    }
    if (car->ai_service_state >= 0) return;
    car->ai_contact_threshold = 50;
    if (!race->navigation.pit_route_count) {
        if (!race->damage_enabled) return;
    } else {
        for (index = 0; index < race->navigation.pit_route_count; ++index)
            if (car->waypoint == race->navigation.pit_route_zone[index]) break;
        if (index == race->navigation.pit_route_count) return;
        pit = race->navigation.pit_route_destination[index];
    }
    car->ai_service_state = 1;
    car->ai_state = 1;
    car->ai_target_x = race->navigation.pits[pit].x;
    car->ai_target_y = race->navigation.pits[pit].y;
}

static int advance_waypoint(struct SlicksRaceRuntime *race,
                             struct SlicksRaceCar *car)
{
    const struct SlicksTrackZone *zone =
        &race->navigation.zones[car->waypoint];
    if (ai_inside_zone(car, zone)) {
        ai_route_service_entry(race, car);
        ++car->waypoint;
        if (car->waypoint >= race->navigation.zone_count) {
            car->waypoint = 0;
        }
        return 1;
    }
    return 0;
}

static unsigned short display_time_centiseconds(unsigned int raw)
{
    /* aceb..ad27 receives only the low word, clamps BEFORE scaling, and
     * truncates integer division. Keep raw units outside this formatter. */
    unsigned short value = (unsigned short)raw;
    if (value > 17999U) value = 17999U;
    return (unsigned short)((unsigned long)value * 5UL / 9UL);
}

static void advance_car_clock(struct SlicksRaceCar *car, unsigned short ticks)
{
    car->elapsed_time_units += (unsigned int)ticks * 2U;
    car->current_lap_time_units += (unsigned int)ticks * 2U;
    car->elapsed_centiseconds = display_time_centiseconds(car->elapsed_time_units);
    car->current_lap_centiseconds = display_time_centiseconds(car->current_lap_time_units);
}

static void advance_checkpoint(struct SlicksRaceRuntime *race,
                                struct SlicksRaceCar *car)
{
    /* 27b6..2840: sprite origin +4, unsigned inclusive bounds, at most one
     * checkpoint per update. Reaching the last one does not itself finish. */
    if (car->checkpoint < race->navigation.checkpoint_count) {
        const struct SlicksTrackCheckpoint *point =
            &race->navigation.checkpoints[car->checkpoint];
        unsigned short x = (unsigned short)(car->x / 100L + 1L);
        unsigned short y = (unsigned short)(car->y / 100L + 1L);
        if (x >= point->x[0] && x <= point->x[1] &&
            y >= point->y[0] && y <= point->y[1])
        {
            ++car->checkpoint;
            if(race_lap_limit(race)==car->lap && car->checkpoint==race->navigation.checkpoint_count) {
                race->track_flag_activations=(signed char)(race->track_flag_activations+1);
                if(race->track_flag_activations==1) activate_track_flags(race);
            }
        }
    }
}

static void record_lap_clock(struct SlicksRaceCar *car)
{
    car->last_lap_time_units = car->current_lap_time_units;
    if ((signed int)car->last_lap_time_units < (signed int)car->best_lap_time_units)
        car->best_lap_time_units = car->last_lap_time_units;
    car->last_lap_centiseconds = display_time_centiseconds(car->last_lap_time_units);
    car->best_lap_centiseconds = display_time_centiseconds(car->best_lap_time_units);
    car->current_lap_time_units = 0;
    car->current_lap_centiseconds = 0;
    ++car->lap;
}

static signed char assign_race_finish(struct SlicksRaceRuntime *race,unsigned driver)
{
    if(!race->finish_ranks_ready) {
        /* Until the first finish, original ranks remain -1 for entrants and
         * zero for absent drivers. Keep the byte state after that point. */
        for(unsigned i=0;i<4;++i)
            race->finish_ranks[i]=driver_role(race,i)?
                (race->cars[i].finished?(signed char)race->cars[i].finish_position:-1):0;
        race->finish_ranks_ready=1;
    }
    signed char assigned=slicks_assign_finish_rank(race->finish_ranks,driver);
    if(assigned==1) {
        short laps[4];
        for(unsigned i=0;i<4;++i) laps[i]=driver_role(race,i)?
            (short)(unsigned short)(race->cars[i].lap-1):0;
        slicks_adjust_finish_laps(race->finish_ranks,laps);
    }
    race->finished_count=0;
    for(unsigned i=0;i<4;++i) if(driver_role(race,i)) {
        race->cars[i].finished=race->finish_ranks[i]>=0;
        race->cars[i].finish_position=race->cars[i].finished?(unsigned char)race->finish_ranks[i]:0;
        race->finished_count+=race->cars[i].finished;
    }
    return assigned;
}

static void advance_lap_after_checkpoint(struct SlicksRaceRuntime *race,
                                    struct SlicksRaceCar *car)
{
    /* 2a61..2a8e requires selected DS:537c OR mode-zero DS:5384 ==17.
     * 2981 reads mode one into 5380; 29ab reads mode zero into 5384. */
    if (car->checkpoint >= race->navigation.checkpoint_count &&
        (car->selected_surface == 17 ||
         (car->x / 100L >= 0 && car->x / 100L < 320 &&
          car->y / 100L >= 0 && car->y / 100L < 190 &&
          race->material_map[mult320[car->y / 100L] + car->x / 100L] == 17))) {
            car->checkpoint = 0;
            record_lap_clock(car);
            unsigned short lap_limit=race_lap_limit(race);
            /* 2000:2b17..2b4d announces an ordinary new lap with block 25
             * at priority 18, and the final lap with block 8 at priority 19.
             * The DOS lap counter starts one lower than this public HUD
             * value, hence these comparisons follow the increment above. */
            if (car->lap == lap_limit)
                emit_sound_event(race, 8, 0, 19);
            else if (car->lap < lap_limit)
                emit_sound_event(race, 25, 0, 18);
            if (!car->finished && car->lap > lap_limit) {
                race->laps_to_run=0; /* Original 22b71: remaining cars finish their current lap. */
                signed char assigned=assign_race_finish(race,(unsigned)(car-race->cars));
                unsigned unfinished=0;
                for(unsigned i=0;i<4;++i)
                    unfinished+=driver_role(race,i)!=0 && !race->cars[i].finished;
                race->finish_deadline=slicks_finish_deadline(race->race_mode,
                    race->game_clock_ticks,race->finish_deadline,unfinished);
                car->finish_time_centiseconds = car->elapsed_centiseconds;
                /* 2000:2bdf..2bfa plays this only for finishing position 1. */
                if (assigned == 1)
                    emit_sound_event(race, 9, 2, 30);
                if(race->finish_reward)
                    race->finish_reward(race,(unsigned)(car-race->cars),
                        race->finish_ranks[(unsigned)(car-race->cars)]);
            }
    }
}

/* Combined entry retained for isolated checkpoint/finish oracle fixtures. */
static inline __attribute__((unused)) void advance_lap_checkpoints(struct SlicksRaceRuntime *race,
                                    struct SlicksRaceCar *car)
{
    advance_checkpoint(race,car);
    advance_lap_after_checkpoint(race,car);
}

#include "weapon_actors.inc"

static void build_actor_order(struct SlicksRaceRuntime *race,int reverse)
{
    for(unsigned p=0;p<128;++p) race->actor_order_head[p]=0;
    race->actor_order_max=0;
    unsigned count=race->weapons.slots.high_water;
    for(unsigned i=1;i<count;++i) {
        unsigned h=reverse?i:count-i;
        int t=race->weapons.trail_index[h];
        if(t>=0) {
            if(reverse && !(race->trail_particles[t].saved_valid&1)) continue;
        } else {
            const struct SlicksWeaponActor *a=&race->weapons.actors[h];
            if(reverse?!a->saved:(!a->kind || race->weapons.slots.state[h]<=0)) continue;
        }
        unsigned p=t>=0?race->trail_particles[t].priority:race->weapons.actors[h].priority;
        if(p>=128) continue;
        if(p>race->actor_order_max) race->actor_order_max=(unsigned char)p;
        race->actor_order_next[h]=race->actor_order_head[p];
        race->actor_order_head[p]=(unsigned char)h;
    }
    race->actor_order_ready=1;
}

static void restore_trail_priority(struct SlicksRaceRuntime *race,
                                    unsigned short bucket,int priority)
{
    unsigned short at;
    if (slicks_race_disable_particles)
        return;
    int ordered=race->actor_order_ready && (priority>=0 || bucket!=3);
    unsigned p=priority>=0?(unsigned)priority:bucket==0?0:bucket==1?3:5;
    if(ordered) {
        const unsigned char *next=race->actor_order_next;
        const short *trail_index=race->weapons.trail_index;
        struct SlicksTrailParticle *particles=race->trail_particles;
        unsigned char *chunky=race->chunky;
        for(unsigned h=race->actor_order_head[p];h;h=next[h]) {
            int t=trail_index[h];
            if(t<0) { restore_weapon_actor(race,h);continue; }
            struct SlicksTrailParticle *particle=particles+t;
            if(particle->saved_valid&1) {
                chunky[mult320[(unsigned short)particle->old_y]+(unsigned short)particle->old_x]=particle->saved_under;
                particle->saved_valid=2;
            }
        }
        return;
    }
    at = ordered?race->actor_order_head[p]:shared_actor_pool(race)?race->weapons.slots.high_water:race->trail_priority_counts[bucket];
    while (at) {
        unsigned index;
        unsigned handle;
        if(ordered) { handle=at;at=race->actor_order_next[at]; }
        else handle=--at;
        if(shared_actor_pool(race)) {
            int trail=race->weapons.trail_index[handle];
            if(trail<0) {
                if(priority>=0 && race->weapons.actors[handle].priority!=priority) continue;
                if(weapon_bucket(race->weapons.actors[handle].priority)==bucket)
                    restore_weapon_actor(race,handle);
                continue;
            }
            index=(unsigned)trail;
            if(!ordered && weapon_bucket(race->trail_particles[index].priority)!=bucket) continue;
        } else index=race->trail_priority_indices[bucket][handle];
        struct SlicksTrailParticle *particle =
            &race->trail_particles[index];
        if(priority>=0 && particle->priority!=priority) continue;
        if (particle->saved_valid & 1) {
            unsigned long pixel_at =
                mult320[(unsigned short)particle->old_y] +
                (unsigned short)particle->old_x;
            race->chunky[pixel_at] = particle->saved_under;
            particle->saved_valid = 2;
        }
    }
}

static void restore_trail_particles(struct SlicksRaceRuntime *race,unsigned short bucket)
{ restore_trail_priority(race,bucket,-1); }

static void commit_expiring_trails(struct SlicksRaceRuntime *race);

static void advance_trail_particles(struct SlicksRaceRuntime *race)
{
    extern unsigned short slicks_advance_particles(
        struct SlicksTrailParticle *particles, unsigned long count,
        unsigned char indices[SLICKS_TRAIL_PRIORITY_COUNT]
                             [SLICKS_TRAIL_PARTICLE_MAX],
        unsigned short counts[SLICKS_TRAIL_PRIORITY_COUNT],
        struct SlicksDirtyPixel *dirty_pixels,
        unsigned short *dirty_pixel_count, unsigned char *chunky,
        unsigned long actor_page);
    if (slicks_race_disable_particles)
        return;
    /* Only the capacity fallback needs a separate bounds-list pass. */
    if (race->dirty_pixel_count + race->trail_particle_count >=
        SLICKS_DIRTY_PIXEL_MAX)
        commit_expiring_trails(race);
    int shared=shared_actor_pool(race);
    unsigned char *handles=race->weapons.trail_handle;
    short *indices=race->weapons.trail_index;
    signed char *states=race->weapons.slots.state;
    if(shared) {
        unsigned char *src=handles,*dst=handles;
        const struct SlicksTrailParticle *particle=race->trail_particles;
        for(unsigned left=race->trail_particle_count;left;--left,++particle) {
            unsigned h=*src++;
            indices[h]=-1;
            if(particle->state<0) states[h]=0;
            else *dst++=(unsigned char)h;
        }
    }
    race->trail_particle_count = slicks_advance_particles(
        race->trail_particles, race->trail_particle_count,
        race->trail_priority_indices, race->trail_priority_counts,
        race->dirty_pixels, &race->dirty_pixel_count, race->chunky,
        race->actor_page);
    if(shared) {
        const struct SlicksTrailParticle *particle=race->trail_particles;
        unsigned count=race->trail_particle_count;
        for(unsigned i=0;i<count;++i,++particle) {
            unsigned h=*handles++;
            indices[h]=(short)i;
            states[h]=particle->state;
        }
    }
}

/* DOS state +1a=5 expires to -5: 3000:3e36 skips saved-under restoration,
 * and 3fc5..4000 draws into both pages before freeing the actor. Its three
 * ticks are an actor lifetime, not a lifetime for the mark on the road.
 * Bake only after ALL saved-under layers have been unwound, so an older
 * overlapping actor/car cannot subsequently erase the permanent pixel. */
static void commit_expiring_trails(struct SlicksRaceRuntime *race)
{
    unsigned short at;
    if (slicks_race_disable_particles)
        return;
    for (at = 0; at < race->trail_priority_counts[0]; ++at) {
        struct SlicksTrailParticle *particle = &race->trail_particles[
            race->trail_priority_indices[0][at]];
        if (race->actor_page && particle->permanent && particle->lifetime == 1 &&
            (particle->saved_valid & 2)) {
            unsigned long pixel_at =
                mult320[(unsigned short)particle->old_y] + (unsigned short)particle->old_x;
            race->chunky[pixel_at] = particle->colour;
            /* The existing expiry pass already queues this old pixel.
             * Only invoke the rectangle-overflow fallback here when there
             * might not be room for every expiring actor in that pass. */
            if (race->dirty_pixel_count + race->trail_particle_count >=
                SLICKS_DIRTY_PIXEL_MAX)
                mark_dirty_pixel(race, particle->old_x, particle->old_y);
        }
    }
}

static unsigned short next_random(struct SlicksRaceRuntime *race)
{
    /* The DOS helper at 1010:32a7 is Borland's 32-bit LCG. */
    race->random_state =
        (race->random_state * 0x015a4e35UL + 1UL) & 0xffffffffUL;
    return (unsigned short)((race->random_state >> 16) & 0x7fffUL);
}

static short random_scaled(struct SlicksRaceRuntime *race,
                           unsigned short limit)
{
    return (short)(((unsigned long)next_random(race) * limit) / 0x8000UL);
}

static void apply_damage(struct SlicksRaceRuntime *race, unsigned short index,
                         long impulse)
{
    struct SlicksRaceCar *car = &race->cars[index];
    long amount;
    if (impulse <= 0 || !race->damage_scale || !race->damage_enabled)
        return;
    /* 1000:ed64..ee7f: keep low product dwords and each signed division.
     * Properties are validated by the loader before enabling a race. */
    amount = (signed int)((unsigned int)impulse *
                         (unsigned int)(signed int)race->damage_scale);
    amount /= race->properties[car->vehicle].impact_resistance;
    amount = (signed int)((unsigned int)amount *
                         (unsigned int)(signed int)car->drive_coefficients[5]);
    amount /= 100;
    amount /= 2;
    if (!car->damage_turn_sign)
        car->damage_turn_sign = (signed char)(random_scaled(race, 2) * 2 - 1);
    for (unsigned short channel = 0; channel < 4; ++channel) {
        short value = (short)(car->damage[channel] + (short)amount);
        car->damage[channel] = value > 999 ? 999 : value;
    }
}

static void consume_car_damage(struct SlicksRaceRuntime *race,
                               unsigned short index)
{
    struct SlicksRaceCar *car = &race->cars[index];
    /* 2000:399f..39d6 consumes this driver's pending impact at the tail,
     * not immediately for both participants inside the pair collision. */
    apply_damage(race, index, (car->pending_damage_impact - 300L) / 4L);
    car->pending_damage_impact = 0;
}

static void add_trail_component(struct SlicksRaceRuntime *race,
                                short x, short y, unsigned char colour,
                                unsigned char priority, short velocity_x,
                                short velocity_y, unsigned char lifetime)
{
    struct SlicksTrailParticle *particle;
    unsigned short bucket;
    unsigned short particle_index;
    if (slicks_race_disable_particles)
        return;
    if (race->trail_particle_count >= SLICKS_TRAIL_PARTICLE_MAX)
        return;
    if(shared_actor_pool(race)) {
        short h=slicks_actor_allocate(&race->weapons.slots,1);
        if(!h) return;
        race->weapons.trail_handle[race->trail_particle_count]=(unsigned char)h;
        race->weapons.trail_index[h]=(short)race->trail_particle_count;
        reset_weapon_actor(&race->weapons.actors[h]);
    }
    particle_index = race->trail_particle_count++;
    particle = &race->trail_particles[particle_index];
    particle->x = (long)x * 64L;
    particle->y = (long)y * 64L;
    particle->velocity_x = velocity_x;
    particle->velocity_y = velocity_y;
    particle->lifetime = lifetime;
    particle->colour = colour;
    particle->priority = priority;
    particle->saved_valid = 0;
    particle->occlusion_limit = 0;
    /* Both road marks (2000:26d6) and the short-lived stationary surface
     * marks (1000:e934) use DOS state 5. The 30..49-tick variant does not. */
    particle->permanent = (priority == 0 && lifetime == 3);
    particle->state = particle->permanent ? 5 : 1;
    if (priority == 0)
        bucket = 0;
    else if (priority == 3)
        bucket = 1;
    else if (priority == 5)
        bucket = 2;
    else
        bucket = 3;
    race->trail_priority_indices[bucket]
        [race->trail_priority_counts[bucket]++] =
            (unsigned char)particle_index;
    ++race->skidmark_count;
}

/* Original 1e7b1..1ea80, including allocation-dependent lifetime RNG. */
static void emit_offroad_wheel(struct SlicksRaceRuntime *race,short x,short y,
    unsigned char colour,unsigned char layer,long magnitude,unsigned char long_lived)
{
    if(magnitude<=200) return;
    short radius=(short)(magnitude/120L);
    short sx=(short)(x+random_scaled(race,(unsigned short)radius)-radius/2);
    short sy=(short)(y+random_scaled(race,(unsigned short)radius)-radius/2);
    if(sx<0 || sx>=320 || sy<0 || sy>=200) return;
    int material=slicks_track_material_sample(race->material_map,race->surface_map,sx,sy,layer);
    if(material!=2 && material!=15 && (material<22 || material>26)) {
        unsigned before=race->trail_particle_count;
        add_trail_component(race,sx,sy,colour,0,0,0,long_lived?30:3);
        if(long_lived && (race->trail_particle_count>before || slicks_race_disable_particles)) {
            unsigned char life=(unsigned char)(random_scaled(race,20)+30);
            if(race->trail_particle_count>before) race->trail_particles[before].lifetime=life;
        }
    }
    if(magnitude>250) {
        unsigned char life=(unsigned char)(random_scaled(race,10)+15);
        short vy=(short)(random_scaled(race,23)-11);
        short vx=(short)(random_scaled(race,23)-11);
        add_trail_component(race,x,y,colour,5,vx,vy,life);
    }
}

static void emit_wheel_surface(struct SlicksRaceRuntime *race,
                               const struct SlicksRaceCar *car,
                               unsigned short car_index,
                               unsigned char controls)
{
    unsigned char road_threshold = race->properties[car->vehicle].effect_profile;
    long magnitude = (absolute_long(car->velocity_x) +
                      absolute_long(car->velocity_y)) / 2L;
    unsigned short direction = (unsigned short)car->heading /
                               SLICKS_HEADING_STEP;
    const struct SlicksCarSprite *wheel_sprite = &race->sprites[car->vehicle][direction & 3];
    unsigned short rotation = direction >> 2;
    (void)car_index; /* Geometry follows the vehicle, not the driver slot. */
    unsigned short wheel;
    unsigned short first_particle = race->trail_particle_count;
    /* 2000:233b skips wheel effects for any nonzero special-drive state. */
    if (!car->forward_drive_latch || car->special_drive_state)
        return;
    for (wheel = 0; wheel < 2; ++wheel) {
        short x = (short)(car->x / 100L) - 3 +
                  wheel_sprite->wheel_x[rotation][wheel];
        short y = (short)(car->y / 100L) - 3 +
                  wheel_sprite->wheel_y[rotation][wheel];
        unsigned char surface;
        unsigned char colour;
        if (wheel_sprite->wheel_x[rotation][wheel] < 0)
            continue;
        /* Wheels may extend beyond x=319 even when the car centre is legal.
         * Original b089 wraps the linear address, not the visible rectangle. */
        int material=slicks_track_material_sample(race->material_map,race->surface_map,
            x,y,car->actor_layer);
        if(material<0) { race->collision_error=1; continue; }
        surface=(unsigned char)material;

        if (surface == 0 || surface == 1 || surface == 17 ||
            surface == 19 || surface == 31) {
            unsigned char emits = 0;
            /* 2000:2270..2319 gates the low-speed road cloud from the
             * current accelerator/brake state and the four driver thresholds
             * at DS:4ee0. Braking emits ABOVE twice the threshold; throttle
             * uses the signed, word-wrapped damage-adjusted threshold. */
            if ((controls & SLICKS_CONTROL_BRAKE) &&
                !(controls & SLICKS_CONTROL_ACCELERATE) &&
                magnitude > (long)road_threshold * 2L)
                emits = 1;
            else if ((controls & SLICKS_CONTROL_ACCELERATE) &&
                     !(controls & SLICKS_CONTROL_BRAKE) &&
                     !(car->service_flags & 1) &&
                     magnitude < (short)((short)(road_threshold *
                         (10-car->damage[0]/100))*10)/10)
                emits = 1;
            if (emits) {
                unsigned char lifetime;
                short velocity_x;
                short velocity_y;
                colour = (unsigned char)(70 + random_scaled(race, 3));
                lifetime =
                    (unsigned char)(random_scaled(race, 15) + 5);
                velocity_x = (short)(random_scaled(race, 22) - 11);
                velocity_y = (short)(random_scaled(race, 22) - 11);
                add_trail_component(
                    race, x, y, 218, 6, velocity_x, velocity_y,
                    lifetime);
                if (magnitude > 100L)
                    emit_sound_event(
                        race, (unsigned char)(2 + random_scaled(race, 3)),
                        2, 10);
                add_trail_component(race, x, y, colour, 0, 0, 0, 3);
            }
            continue;
        }

        if (surface == 7 || surface == 8) {
            if (magnitude > 300L) {
                short velocity_x =
                    (short)(random_scaled(race, 20) - 10);
                short velocity_y =
                    (short)(random_scaled(race, 20) - 10);
                add_trail_component(
                    race, x, y, 55, 3, velocity_x, velocity_y, 20);
            }
            continue;
        }
        if (surface == 3 || surface == 4)
            colour = (unsigned char)(67 + random_scaled(race, 3));
        else if (surface == 5 || surface == 6 || surface == 9 ||
                 surface == 10 || surface == 13 || surface == 14)
            colour = (unsigned char)(61 + random_scaled(race, 3));
        else if (surface == 11 || surface == 12)
            colour = (unsigned char)(64 + random_scaled(race, 3));
        else
            continue;
        emit_offroad_wheel(race,x,y,colour,car->actor_layer,magnitude,surface==11 || surface==12);
    }
    for (; first_particle < race->trail_particle_count; ++first_particle)
        race->trail_particles[first_particle].occlusion_limit =
            (unsigned char)(car->actor_layer * 15);
}

static void draw_trail_point(struct SlicksRaceRuntime *race,
                             struct SlicksTrailParticle *particle)
{
#if defined(__m68k__)
    extern int slicks_draw_particle(struct SlicksTrailParticle *,unsigned char *,
        const unsigned char *,const unsigned char *,struct SlicksDirtyPixel *,
        unsigned short *,const unsigned int *);
    if(!slicks_draw_particle(particle,race->chunky,race->material_map,
        race->surface_map,race->dirty_pixels,&race->dirty_pixel_count,mult320))
        return;
#endif
        /* 3000:39af/39ca use SAR on signed 16-bit coordinates. Negative
         * fractions round down, not toward zero into the visible border. */
        short x = particle->x < 0
            ? (short)-((63 - particle->x) >> 6)
            : (short)(particle->x >> 6);
        short y = particle->y < 0
            ? (short)-((63 - particle->y) >> 6)
            : (short)(particle->y >> 6);
        if (x < 0 || x >= SLICKS_SCREEN_WIDTH ||
            y < 0 || y >= SLICKS_POINT_HEIGHT) {
            if (particle->saved_valid & 2)
                mark_dirty_pixel(race, particle->old_x, particle->old_y);
            particle->saved_valid = 0;
            return;
        }
        if (particle->occlusion_limit) {
            unsigned long at = mult320[(unsigned short)y] +
                               (unsigned short)x;
            if (!actor_pixel_visible(race, at, particle->occlusion_limit)) {
                if (particle->saved_valid & 2)
                    mark_dirty_pixel(race, particle->old_x, particle->old_y);
                particle->saved_valid = 0;
                return;
            }
        }
        {
            unsigned long pixel_at =
                mult320[(unsigned short)y] +
                (unsigned short)x;
            if (!(particle->saved_valid & 2)) {
                mark_dirty_pixel(race, x, y);
            } else if (particle->old_x != x || particle->old_y != y) {
                mark_dirty_pixel(race, particle->old_x, particle->old_y);
                mark_dirty_pixel(race, x, y);
            }
            particle->old_x = x;
            particle->old_y = y;
            particle->saved_under = race->chunky[pixel_at];
            race->chunky[pixel_at] = particle->colour;
        }
        particle->saved_valid = 1;
}

static void draw_trail_priority(struct SlicksRaceRuntime *race,
                                 unsigned short bucket,int priority)
{
    if(slicks_race_disable_particles) return;
    if(race->actor_order_ready && (priority>=0 || bucket!=3)) {
        unsigned p=priority>=0?(unsigned)priority:bucket==0?0:bucket==1?3:5;
        const unsigned char *next=race->actor_order_next;
        const short *trail_index=race->weapons.trail_index;
        struct SlicksTrailParticle *particles=race->trail_particles;
#if defined(__m68k__)
        extern unsigned slicks_draw_particle_batch(struct SlicksTrailParticle *,
            unsigned char *,const unsigned char *,const unsigned char *,
            struct SlicksDirtyPixel *,unsigned short *,const unsigned int *,
            const unsigned short *,unsigned);
        unsigned short indices[SLICKS_ACTOR_CAPACITY];
        for(unsigned h=race->actor_order_head[p];h;) {
            int t=trail_index[h];
            if(t<0) { draw_weapon_actor(race,h);h=next[h];continue; }
            unsigned n=0;
            do {
                indices[n++]=(unsigned short)t;
                h=next[h];
                if(!h) break;
                t=trail_index[h];
            } while(t>=0);
            unsigned done=slicks_draw_particle_batch(particles,race->chunky,
                race->material_map,race->surface_map,race->dirty_pixels,
                &race->dirty_pixel_count,mult320,indices,n);
            for(;done<n;++done) draw_trail_point(race,particles+indices[done]);
        }
#else
        for(unsigned h=race->actor_order_head[p];h;h=next[h]) {
            int t=trail_index[h];
            if(t<0) draw_weapon_actor(race,h);
            else draw_trail_point(race,particles+t);
        }
#endif
        return;
    }
    unsigned count=shared_actor_pool(race)?race->weapons.slots.high_water:race->trail_priority_counts[bucket];
    for(unsigned at=0;at<count;++at) {
        unsigned index;
        if(shared_actor_pool(race)) {
            int trail=race->weapons.trail_index[at];
            if(trail<0) {
                if(priority>=0 && race->weapons.actors[at].priority!=priority) continue;
                if(weapon_bucket(race->weapons.actors[at].priority)==bucket)
                    draw_weapon_actor(race,at);
                continue;
            }
            index=(unsigned)trail;
            if(weapon_bucket(race->trail_particles[index].priority)!=bucket) continue;
        } else index=race->trail_priority_indices[bucket][at];
        struct SlicksTrailParticle *particle=&race->trail_particles[index];
        if(priority>=0 && particle->priority!=priority) continue;
        draw_trail_point(race,particle);
    }
}

static void draw_trail_particles(struct SlicksRaceRuntime *race,unsigned short bucket)
{ draw_trail_priority(race,bucket,-1); }

static void restore_race_actors(struct SlicksRaceRuntime *race,unsigned char *logical)
{
    if(!race->track_actors_ready) {
        restore_trail_particles(race,3);restore_trail_particles(race,2);
        restore_layered_cars(race,logical);restore_trail_particles(race,0);
        restore_shadows(race,logical);return;
    }
    int profile=race->profile_marker && race->frame_count+1==race->profile_frame;
    if(profile) race->profile_marker(10);
    build_actor_order(race,1);
    if(profile) race->profile_marker(11);
    for(int p=race->actor_order_max;p>=6;--p)
        if(race->actor_order_head[p]) restore_trail_priority(race,3,p);
    restore_trail_particles(race,2);
    restore_trail_priority(race,3,4);
    if(profile) race->profile_marker(12);
    restore_layered_cars(race,logical);
    if(profile) race->profile_marker(13);
    restore_trail_priority(race,3,2);
    restore_shadows(race,logical);
    restore_trail_priority(race,3,1);
    restore_trail_particles(race,0);
    if(profile) race->profile_marker(14);
    race->actor_order_ready=0;
}

static void draw_race_actors(struct SlicksRaceRuntime *race,unsigned char *logical)
{
    if(!race->track_actors_ready) {
        draw_shadows(race,logical);draw_trail_particles(race,0);
        draw_layered_cars(race,logical);draw_trail_particles(race,2);
        draw_trail_particles(race,3);return;
    }
    int profile=race->profile_marker && race->frame_count+1==race->profile_frame;
    if(profile) race->profile_marker(20);
    build_actor_order(race,0);
    if(profile) race->profile_marker(21);
    draw_trail_particles(race,0);
    draw_trail_priority(race,3,1);
    draw_shadows(race,logical);
    draw_trail_priority(race,3,2);
    if(profile) race->profile_marker(22);
    draw_layered_cars(race,logical);
    if(profile) race->profile_marker(23);
    draw_trail_priority(race,3,4);
    draw_trail_particles(race,2);
    for(unsigned p=6;p<=race->actor_order_max;++p)
        if(race->actor_order_head[p]) draw_trail_priority(race,3,p);
    if(profile) race->profile_marker(24);
    race->actor_order_ready=0;
}

static void record_track_contact(struct SlicksRaceRuntime *race,
                                  struct SlicksRaceCar *car,
                                  int hit)
{
    if (hit) {
        /* The response has already selected the branch-specific impact. */
        if (!car->touching_solid) {
            ++race->track_collision_count;
        }
        /* DOS 2000:1352 sets DS:536c on contact; a later clear substep
         * does not reset it. The outer-update tail clears it at 3d86. */
        car->actor_contact = 1;
    }
    car->touching_solid = hit != 0;
}

static void emit_contact_sound(struct SlicksRaceRuntime *race,
                                const struct SlicksRaceCar *car)
{
    /* 23ada..23b07: update-wide rising contact, then DS:4c4f + DS:4dae.
     * DS:0196 belongs to weapon sounds, not heading-selected impacts. */
    if (car->actor_contact && !car->previous_actor_contact)
        emit_sound_event(race, (unsigned char)(5 + car->touching_car), 2, 14);
}

static void emit_damage_smoke(struct SlicksRaceRuntime *race,
                               struct SlicksRaceCar *car)
{
    /* 239dc..23ada counts updates, not elapsed ticks. Keep the counter when
     * repairs lower damage below the gate, and consume RNG with a full pool. */
    if(car->damage[0]<=400) return;
    race->track_actor_scratch=(short)(car->damage[0]-400);
    car->damage_smoke_ticks=(short)(car->damage_smoke_ticks+1);
    if(car->damage_smoke_ticks<=30/(car->damage[0]-400)) return;
    car->damage_smoke_ticks=0;
    short vy=(short)(random_scaled(race,15)-7);
    short vx=(short)(random_scaled(race,15)-7);
    unsigned before=race->trail_particle_count;
    add_trail_component(race,(short)(car->x/100),(short)(car->y/100),
        race->damage_smoke_colour,7,vx,vy,30);
    if(shared_actor_pool(race)) race->track_actor_scratch=
        race->trail_particle_count>before?race->weapons.trail_handle[before]:0;
    if(race->trail_particle_count>before)
        race->trail_particles[before].occlusion_limit=car->actor_layer*15;
}

static void emit_contact_particles(struct SlicksRaceRuntime *race,
                                    struct SlicksRaceCar *car,
                                    unsigned short index)
{
    short divisor = car->collision_partner ? 60 : -126;
    unsigned short radius = car->collision_partner ? 12 : 7;
    unsigned short count = car->collision_partner ? 3 : 7;
    unsigned char lifetime = car->collision_partner ? 35 : 50;
    if (!car->actor_contact || car->previous_actor_contact) return;
    car->collision_partner = 0;
    /* 23b0f..23c94: RNG order is colour, Y jitter, then X jitter.
     * Consume all three draws even if the native particle pool is full. */
    for (unsigned short i=0;i<count;++i) {
        unsigned char colour = random_scaled(race,3) == 0 ?
            (unsigned char)(index*5+3) : race->collision_colour;
        short vy = (short)((short)((signed int)car->velocity_y / divisor) +
                            random_scaled(race,radius*2) - radius);
        short vx = (short)((short)((signed int)car->velocity_x / divisor) +
                            random_scaled(race,radius*2) - radius);
        unsigned short first = race->trail_particle_count;
        add_trail_component(race,(short)(car->x/100L),(short)(car->y/100L),
                            colour,3,vx,vy,lifetime);
        if(shared_actor_pool(race)) race->track_actor_scratch=
            race->trail_particle_count>first?race->weapons.trail_handle[first]:0;
        if (race->trail_particle_count > first)
            race->trail_particles[first].occlusion_limit = car->actor_layer*15;
    }
}

#include "weapon_simulation.inc"
#include "track_actor_motion.inc"

void slicks_race_set_timer(struct SlicksRaceRuntime *race,unsigned short argument)
{
    if(!race) return;
    race->physics_tick_phase=0;
    race->physics_tick_period=slicks_timer_divisor(argument)*50UL;
    race->physics_timer_disabled=(unsigned char)((short)argument<100);
}
static unsigned short next_physics_ticks(struct SlicksRaceRuntime *race)
{
    return slicks_physics_clock_advance(&race->physics_tick_phase,
        race->physics_tick_period,race->physics_timer_disabled);
}

static short profile_steering_input(unsigned char scale,signed char participation)
{
    /* 1fcf5..1fd3d: only positive (AI) participation scales by 7/5. */
    return participation>0 ? (short)((unsigned short)scale*7/5) : scale;
}

static short steering_delta(const struct SlicksRaceCar *car,
                            short input, unsigned short ticks)
{
    /* Each IMUL before IDIV is followed by CWD, discarding the product's
     * high word. Preserve this even on hosts where long is 64 bits. */
    short value = (short)((long)input * (car->steering_scale / 10));
    value /= 155;
    value = (short)((long)value * (80 - car->damage[3] / 25));
    value /= 100;
    value = (short)((long)value * car->steering_property);
    value /= 50;
    return (short)((long)value * ticks);
}

static short damage_heading_delta(const struct SlicksRaceCar *car,
                                   unsigned short ticks)
{
    /* 2000:0d58..0da2 uses the preceding update's measured speed, not
     * the throttle scalar or the velocity after this update's braking. */
    if (!car->damage_turn_sign || car->measured_speed <= 30)
        return 0;
    short value = (short)((long)car->damage_turn_sign * ticks);
    value = (short)((long)value * car->damage[2]);
    return value / 70;
}

static void apply_brake(struct SlicksRaceRuntime *race,
                        struct SlicksRaceCar *car, unsigned short ticks)
{
    /* Ordinary no-contact branch 2000:05bc..06a5. Throttle sets this
     * latch; braking at low measured speed arms reverse for the NEXT
     * update, provided the vehicle's DS:4ee4 property is nonzero. */
    if (car->forward_drive_latch == 1) {
        unsigned short factor = (unsigned short)(0x7db5L - car->drive_bias);
        for (unsigned short tick = 0; tick < ticks; ++tick) {
            car->velocity_x = multiply_q15_unsigned(car->velocity_x, factor);
            car->velocity_y = multiply_q15_unsigned(car->velocity_y, factor);
        }
        car->speed_fixed = 0;
        if (car->measured_speed < 100 &&
            race->properties[car->vehicle].engine_sound != 0)
            car->forward_drive_latch = 0;
    } else if (car->damage[0] < 999) {
        car->speed_fixed = (short)(-(long)car->maximum_speed * 33L);
    }
}

static unsigned char apply_surface_jump(struct SlicksRaceRuntime *race,
                                        struct SlicksRaceCar *car)
{
    /* Recovered CS:surface switch cases 13/14 at physical 23737/23793.
     * The switch table was not followed by the original static listing. */
    unsigned char surface = car->effective_surface;
    if ((surface != 13 && surface != 14) || car->measured_speed <= 350)
        return 0;
    car->special_drive_target = (short)(
        (car->measured_speed / (surface == 13 ? 20L : 30L)) *
        race->properties[car->vehicle].model_class);
    if (surface == 14) {
        car->velocity_x /= 2;
        car->velocity_y /= 2;
    }
    return 1; /* DOS requests DS:4c51 sound at the later per-driver tail. */
}

static void advance_special_state(struct SlicksRaceCar *car,
                                  unsigned short ticks)
{
    /* 2000:3cb1..3d3e: signed word operations, with no upward clamp. */
    short step = (short)((long)ticks * 266L);
    if (!car->special_drive_target && car->special_drive_state > 0) {
        car->special_drive_state = (short)(car->special_drive_state - step);
        if (car->special_drive_state < 0)
            car->special_drive_state = 0;
    }
    if (car->special_drive_state < car->special_drive_target)
        car->special_drive_state = (short)(car->special_drive_state + step);
    if (car->special_drive_state >= car->special_drive_target)
        car->special_drive_target = 0;
}

static void clamp_car_to_track(struct SlicksRaceCar *car)
{
    /* 2000:1357..14d1: each clamped axis damps BOTH velocity components.
     * A corner therefore applies the factor twice. aeb6 initializes the
     * unsigned Q15 factor at DS:53e6 from 7dd4h minus the driver bias. */
    unsigned short factor = (unsigned short)(0x7dd4L - car->drive_bias);
    if (car->x < 300L || car->x > 31700L) {
        car->x = car->x < 300L ? 300L : 31700L;
        car->velocity_x = multiply_q15_unsigned(car->velocity_x, factor);
        car->velocity_y = multiply_q15_unsigned(car->velocity_y, factor);
    }
    if (car->y < 300L || car->y > 17900L) {
        car->y = car->y < 300L ? 300L : 17900L;
        car->velocity_x = multiply_q15_unsigned(car->velocity_x, factor);
        car->velocity_y = multiply_q15_unsigned(car->velocity_y, factor);
    }
}

static long scaled_position(long position,long velocity,unsigned char scale)
{
    /* 21230..212bc: retain the low 32-bit multiply before signed /2000,
     * then add the displacement modulo 32 bits. */
    unsigned int product=(unsigned int)(signed int)velocity*scale;
    signed int delta=(signed int)product/2000;
    return (signed int)((unsigned int)(signed int)position+(unsigned int)delta);
}

static void integrate_car_motion(struct SlicksRaceRuntime *race,
                                 struct SlicksRaceCar *car,
                                 unsigned short timestep, unsigned char active_drive)
{
    long force_divisor;
    long force_x;
    long force_y;
    long velocity_divisor;
    unsigned short velocity_factor;
    unsigned short direction;
    /* DOS 0dd1..14dc repeats force, position and track contact for every
     * elapsed tick. Controls/steering and actor emission remain outside. */
    for (short quantum = 0; quantum < (short)timestep; ++quantum) {
        long previous_x = car->x;
        long previous_y = car->y;
        direction = (unsigned short)car->heading / SLICKS_HEADING_STEP;
        if (car->special_drive_state > 0) {
            /* 2000:0dd9..0e4c bypasses both normal force branches. Factor seven
             * is unsigned after the helper clears CX, which matters when the
             * per-driver bias raises it beyond 7fffh. */
            unsigned short special_factor = (unsigned short)(
                0x7ffc - car->drive_bias);
            car->velocity_x = multiply_q15_unsigned(
                car->velocity_x, special_factor);
            car->velocity_y = multiply_q15_unsigned(
                car->velocity_y, special_factor);
        } else if (car->special_drive_state < 0) {
            /* 0e5e..0e9e: negative states coast the drive scalar but skip
             * both normal force/velocity branches. Position still advances. */
            car->speed_fixed = multiply_q15_unsigned(car->speed_fixed,
                (unsigned short)(0x7bddL - car->drive_bias));
        } else {
            /* 0e5e..0e91 decays the signed scalar unconditionally when
             * coasting, including reverse. The low-dword product uses
             * arithmetic shift, not a division rounded toward zero. */
            if (!active_drive)
                car->speed_fixed = multiply_q15_unsigned(car->speed_fixed,
                    (unsigned short)(0x7bddL - car->drive_bias));
            /* 2000:0ea2..121c is a pair of signed 32-bit force and decay updates.
             * The paired DOS hooks prove every operand and result across 27,030
             * component updates. The 23 branch is coasting; longitudinal input
             * uses 38. Integer division truncates toward zero, as on the 286. */
            force_divisor = (long)car->drive_coefficients[3] *
                car->drive_coefficients[0];
            /* DS:304f is damage channel zero, not an independent tyre
             * load: all four force branches divide it by 70 here. */
            force_divisor *= car->damage[0] / 70 + 10;
            force_divisor *= active_drive ? 38L : 23L;
            force_x = (long)direction_x[direction] *
                car->speed_fixed * 200L / force_divisor;
            force_y = (long)direction_y[direction] *
                car->speed_fixed * 200L / force_divisor;
            velocity_factor = (unsigned short)(
                (active_drive ? 0x7dc2L : 0x7bd7L) - car->drive_bias);
            velocity_divisor = 0x8000L + car->drive_coefficients[1];
            car->velocity_x = force_x +
                car->velocity_x * velocity_factor / velocity_divisor;
            car->velocity_y = force_y +
                car->velocity_y * velocity_factor / velocity_divisor;
        }

        car->x=scaled_position(car->x,car->velocity_x,car->position_scale);
        car->y=scaled_position(car->y,car->velocity_y,car->position_scale);
        record_track_contact(race, car,
            move_car_through_track(race, car, previous_x, previous_y, &quantum));
        /* Both cb02 output pointers alias BP-3a, the loop counter.
         * A hit stores clear X then clear Y there; INC/CMP subsequently
         * uses that Y rather than the incoming quantum. */
        clamp_car_to_track(car);
    }
}

static void apply_throttle(struct SlicksRaceCar *car, unsigned short ticks)
{
    /* 041b..0509: both tick products are signed low words; the scalar
     * addition wraps at 32 bits. Finished entrants retain drive, capped
     * at 7000 while the fixed-lap race is still in progress. */
    car->fuel -= (unsigned int)(signed int)(short)(ticks * 2U);
    car->speed_fixed = (signed int)((unsigned int)car->speed_fixed +
        (unsigned int)(signed int)(short)(ticks * 160U));
    if (car->speed_fixed / 100L > car->maximum_speed)
        car->speed_fixed = (short)((long)car->maximum_speed * 100L);
    if ((car->service_flags & 1) && car->speed_fixed > 3000)
        car->speed_fixed = 3000;
    if (car->finished && car->speed_fixed > 7000)
        car->speed_fixed = 7000;
    car->forward_drive_latch = 1;
}

static unsigned char apply_finish_gate(struct SlicksRaceRuntime *race,unsigned car,
                                       unsigned char controls)
{
    signed char rank=race->cars[car].finished?1:-1;
    if(slicks_finish_controls_suppressed(race->game_clock_ticks,race->finish_deadline,rank)) {
        /* Original clears the two drive bytes and skips input steering;
         * momentum, damage yaw and the rest of physics continue normally. */
        race->driver_controls[car]&=~(SLICKS_CONTROL_ACCELERATE|SLICKS_CONTROL_BRAKE);
        race->cars[car].ai_control_latch&=~(SLICKS_CONTROL_ACCELERATE|SLICKS_CONTROL_BRAKE);
        if(car==0) race->controls&=~(SLICKS_CONTROL_ACCELERATE|SLICKS_CONTROL_BRAKE);
        controls=0;
    }
    if(slicks_finish_expired(race->game_clock_ticks,race->finish_deadline))
        race->race_complete=1;
    return controls;
}

static unsigned char prepare_car_motion(struct SlicksRaceRuntime *race,
                       unsigned short car_index, unsigned short timestep)
{
    struct SlicksRaceCar *car = &race->cars[car_index];
    consume_idle_fuel(race, car, timestep);
    unsigned char controls =
        driver_role(race,car_index)<0
            ? (race->participation_ready ? race->driver_controls[car_index] : race->controls)
            : ai_controls(race, car_index, timestep);
    controls=apply_finish_gate(race,car_index,controls);
    short steering_input = profile_steering_input(car->position_scale,
        driver_role(race,car_index));
    unsigned char active_drive;
    long steering_step;

    /* 2000:03f2..03ff jumps directly to damage yaw for ANY nonzero
     * special state, skipping throttle, brake and input steering. */
    if (car->special_drive_state != 0)
        controls = 0;
    /* The DOS car state stores speed as a signed 32-bit fixed quantity.
     * Throttle adds 0xa0 per simulation quantum and .omi byte four supplies
     * the limit in hundreds; neither value is a C-era tuning estimate. */
    active_drive = controls & (SLICKS_CONTROL_ACCELERATE |
                               SLICKS_CONTROL_BRAKE);
    if (controls & SLICKS_CONTROL_ACCELERATE) {
        apply_throttle(car, timestep);
    }
    unsigned char weapon_gate=(unsigned char)(!car->special_drive_state &&
        !slicks_finish_controls_suppressed(race->game_clock_ticks,race->finish_deadline,car->finished?1:-1));
    if(weapon_gate && race->weapons.ready)
        slicks_weapon_human_request(&race->weapons.controls[car_index],race->weapons_enabled,
            driver_role(race,car_index),race->selected_weapon[car_index],controls);
    if ((controls & SLICKS_CONTROL_BRAKE) &&
        (!race->weapons.ready || !race->weapons.controls[car_index].request)) {
        apply_brake(race, car, timestep);
    }
    if(weapon_gate && race->weapons.ready) {
        weapon_fire_driver(race,car_index,&controls);
        if(!(controls&SLICKS_CONTROL_BRAKE)) {
            race->driver_controls[car_index]&=~SLICKS_CONTROL_BRAKE;
            car->ai_control_latch&=~SLICKS_CONTROL_BRAKE;
            if(!car_index) race->controls&=~SLICKS_CONTROL_BRAKE;
        }
    }
    active_drive=controls&(SLICKS_CONTROL_ACCELERATE|SLICKS_CONTROL_BRAKE);
    car->speed = (short)(car->speed_fixed / 100L);

    /* 2000:0c79..0d54 performs these divisions separately with signed IDIV;
     * preserving their order is observable. The final IMUL at 0cd6/0d4c
     * scales the rounded result by elapsed ticks, not the input. The +26h
     * penalty is zero in the captured normal race but remains explicit
     * for the recovered state. */
    car->steering_amount = steering_input;
    steering_step = steering_delta(car, steering_input, timestep);
    if (controls & SLICKS_CONTROL_LEFT)
        car->heading -= (short)steering_step;
    if (controls & SLICKS_CONTROL_RIGHT)
        car->heading += (short)steering_step;
    car->heading += damage_heading_delta(car, timestep);
    while (car->heading < 0)
        car->heading += SLICKS_HEADING_FULL;
    while (car->heading >= SLICKS_HEADING_FULL)
        car->heading -= SLICKS_HEADING_FULL;

    integrate_car_motion(race, car, timestep, active_drive);
    car->speed = (short)(car->speed_fixed / 100L);
    while (car->heading < 0)
        car->heading += SLICKS_HEADING_FULL;
    while (car->heading >= SLICKS_HEADING_FULL)
        car->heading -= SLICKS_HEADING_FULL;
    return controls;
}

static void finish_car_update(struct SlicksRaceRuntime *race,
                              unsigned short car_index, unsigned short timestep,
                              unsigned char controls)
{
    struct SlicksRaceCar *car = &race->cars[car_index];
    unsigned char jump_sound;
    /* 2000:21fc..2243 refreshes measured speed before the later car-pair
     * collision pass. Keep it distinct from longitudinal drive speed. */
    car->measured_speed = (absolute_long(car->velocity_x) +
                           absolute_long(car->velocity_y)) / 2L;
    emit_wheel_surface(race, car, car_index, controls);
    if (!car->finished) advance_car_clock(car, timestep);
    /* 227b6 precedes layer sampling, and 22a61 precedes pair collisions.
     * Finished entrants still traverse checkpoints and cross the line; only
     * their one-shot finishing award is gated by the assigned rank. */
    advance_checkpoint(race,car);
    update_actor_layer(race, car);
    advance_lap_after_checkpoint(race,car);
    slicks_race_resolve_car_collisions(race, car_index);
    update_surface_limits(car,&race->properties[car->vehicle],timestep);
    apply_oil_spin(race,car,timestep);
    apply_surface_velocity(car,&race->properties[car->vehicle],timestep);
    (void)repair_car_at_pit(race, car, timestep);
    refuel_car_at_pit(car, timestep);
    jump_sound = apply_surface_jump(race, car);
    apply_surface_contact(car);
    if (update_track_sampling(race, car) < 0)
        race->collision_error = 1;
    consume_car_damage(race, car_index);
    emit_damage_smoke(race,car);
    emit_contact_sound(race, car);
    emit_contact_particles(race, car, car_index);
    /* 2000:3c94..3cae consumes the jump request before state advancement.
     * DS:4c51 is slot 7 in the sample-handle table starting at DS:4c4a. */
    if (jump_sound)
        emit_sound_event(race, 7, 2, 12);
    advance_special_state(car, timestep);
    if (driver_role(race,car_index)>0)
        ai_update_contact_age(car, timestep);
    car->previous_actor_contact = car->actor_contact;
    car->actor_contact = 0;
}

static void update_cars(struct SlicksRaceRuntime *race, unsigned short ticks)
{
    int profile=race->profile_marker && race->frame_count+1==race->profile_frame;
    if(profile) race->profile_marker(30);
    unsigned char controls[SLICKS_RACE_CAR_COUNT];
    unsigned short car;
    /* 02eb..14e8 completes motion for every driver before the separate
     * 21a7..3d94 tail loop. Pair collisions must see all new positions. */
    for (car = 0; car < SLICKS_RACE_CAR_COUNT; ++car)
        if(driver_role(race,car)) controls[car] = prepare_car_motion(race, car, ticks);
    if(profile) race->profile_marker(31);
    update_weapon_projectiles(race,ticks);
    if(profile) race->profile_marker(32);
    for (car = 0; car < SLICKS_RACE_CAR_COUNT; ++car)
        if(driver_role(race,car)) finish_car_update(race, car, ticks, controls[car]);
    if(profile) race->profile_marker(33);
}

static long absolute_long(long value)
{
    return value < 0 ? -value : value;
}

static void emit_sound_event(struct SlicksRaceRuntime *race,
                             unsigned char sample_block,
                             unsigned char flags,
                             unsigned char priority)
{
    struct SlicksSoundEvent *event;
    unsigned short at;
    unsigned short selected = 0;
    if (sample_block < SLICKS_SOUND_SAMPLE_COUNT)
        ++race->sound_event_totals[sample_block];
    if (race->sound_event_count < SLICKS_SOUND_EVENT_MAX) {
        event = &race->sound_events[race->sound_event_count++];
    } else {
        /* DOS submits immediately to the mixer, where a higher-priority
         * request can displace a lower-priority channel.  Preserve that
         * property across this one-frame bridge instead of allowing eight
         * tyre requests to hide a lap, finish, or collision event. */
        for (at = 1; at < SLICKS_SOUND_EVENT_MAX; ++at)
            if (race->sound_events[at].priority <
                race->sound_events[selected].priority)
                selected = at;
        if (priority <= race->sound_events[selected].priority)
            return;
        event = &race->sound_events[selected];
    }
    event->sample_block = sample_block;
    event->flags = flags;
    event->priority = priority;
}

void slicks_race_resolve_car_collisions(struct SlicksRaceRuntime *race,
                                        unsigned short current)
{
    if(race->car_collisions_disabled || !driver_role(race,current)) return;
    struct SlicksRaceCar *a = &race->cars[current];
    const struct SlicksCarProperties *pa = &race->properties[a->vehicle];
    long speed = (absolute_long(a->velocity_x) +
                  absolute_long(a->velocity_y)) / 2L;
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
        if (other == current || !driver_role(race,other))
            continue;
        b = &race->cars[other];
        /* 2000:2d53..2d60 excludes cars on the other bridge layer. */
        if (a->actor_layer != b->actor_layer)
            continue;
        pb = &race->properties[b->vehicle];
        /* Every candidate recomputes this from the current velocity. An
         * earlier pair in the same scan may already have changed it; the
         * measured-speed denominator remains the pre-collision value. */
        long probe_x = a->x + a->velocity_x * 10L / (speed + 1L);
        long probe_y = a->y + a->velocity_y * 10L / (speed + 1L);
        if (probe_x < b->x - extent || probe_x > b->x + extent ||
            probe_y < b->y - extent || probe_y > b->y + extent)
            continue;
        hit = 1;
        if (!a->touching_car) {
            long magnitude;
            /* 2000:2f50 skips 30a3/30ab on a latched overlap. */
            a->actor_contact = 1;
            b->actor_contact = 1;
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
            a->pending_damage_impact = a->collision_impact;
            b->pending_damage_impact = b->collision_impact;
            if ((unsigned long)a->collision_impact > race->collision_impact)
                race->collision_impact = (unsigned long)a->collision_impact;
            if ((unsigned long)b->collision_impact > race->collision_impact)
                race->collision_impact = (unsigned long)b->collision_impact;
            ++race->collision_count;
        }
        a->touching_car = 1;
        b->touching_car = 1;
        a->collision_partner = (unsigned char)other;
    }
    if (!hit)
        a->touching_car = 0;
}

static void draw_hud_background(struct SlicksRaceRuntime *race,
                                 unsigned char *logical)
{
    mark_dirty_rows(race, 184, 200);
    for (unsigned y=184;y<200;++y)
        for (unsigned x=0;x<320;++x)
            write_pixel(logical,race->chunky,x,y,
                        race->hud_background[mult320[y-184]+x]);
}

#if defined(__m68k__)
extern void slicks_draw_original_text(unsigned char *chunky,
    unsigned char *font, const char *text, unsigned short x, unsigned short y);
#endif

/* HUD font commands use flags 0/2 and validated on-screen coordinates.
 * Target drawing executes the translated original string/glyph routines.
 * The scalar host adapter lets game-state/dirty tests run without Amiga ABI. */
static void draw_hud_run(struct SlicksRaceRuntime *race,
                         const struct SlicksHudRun *run)
{
#if defined(__m68k__)
    slicks_draw_original_text(race->chunky,race->font.runtime,run->text,run->x,run->y);
#else
    unsigned x=run->x;
    for(unsigned i=0;run->text[i];++i) {
        unsigned glyph=race->font.lookup[(unsigned char)run->text[i]];
        if(glyph>=race->font.glyph_count) continue;
        unsigned width=race->font.widths[glyph];
        for(unsigned y=0;y<race->font.height;++y)
            for(unsigned column=0;column<width;++column) {
                unsigned pixel=race->font.pixels[race->font.offsets[glyph]+y*width+column];
                if(pixel) race->chunky[mult320[run->y+y]+x+column]=race->font.runtime[5+pixel];
            }
        x+=width+race->font.advance_extra;
    }
#endif
}

int slicks_race_status_rects(const struct SlicksRaceRuntime *race,
                             unsigned short index, unsigned short timer,
                             struct SlicksStatusRect rectangles[3])
{
    const struct SlicksRaceCar *car;
    short left;
    unsigned count = 0;
    if (!race || !rectangles || index >= SLICKS_RACE_CAR_COUNT) return -1;
    if (!driver_role(race,index)) return 0;
    if (!race->fuel_option && !race->damage_scale) return 0;
    car = &race->cars[index];
    left = (short)(106 + index * 60);
    rectangles[count++] = (struct SlicksStatusRect){left,187,(short)(left+20),190,0};
    if (race->fuel_option) {
        signed int width;
        if (car->service_flags & 1) width = 20 * (timer & 1);
        else {
            /* dc9d..dcd1: low-32-bit product, signed division, wrapped
             * increment, then a second signed division. Not fuel*20/cap. */
            signed int numerator = (signed int)(car->fuel * 40U);
            signed int denominator = (signed int)car->fuel_capacity;
            if (!denominator || (numerator == (-2147483647 - 1) && denominator == -1))
                return -1;
            width = (signed int)((unsigned int)(numerator / denominator) + 1U) / 2;
        }
        rectangles[count++] = (struct SlicksStatusRect){left,188,
            (short)((unsigned short)left + (unsigned short)width),189,1};
    }
    if (race->damage_scale && car->damage[0] > 0) {
        short width = car->damage[0] / 40;
        if (width > 20) width = 20;
        rectangles[count++] = (struct SlicksStatusRect){left,189,(short)(left+width),190,2};
    }
    return (int)count;
}

void slicks_race_set_status_palette(struct SlicksRaceRuntime *race,
                                    const unsigned char palette[768])
{
    static const unsigned char rgb[14][3]={{15,15,25},{50,50,15},{60,20,5},{55,55,10},
        {50,50,70},{60,60,35},{55,55,65},{33,33,70},{4,4,4},{40,40,40},{60,60,60},
        {64,64,50},{45,45,45},{30,30,30}};
    for (unsigned slot=0;slot<14;++slot) {
        unsigned best=300, selected=1;
        for (unsigned entry=1;entry<256;++entry) {
            unsigned distance=0;
            for (unsigned component=0;component<3;++component) {
                int delta=(int)palette[entry*3+component]-rgb[slot][component];
                distance+=(unsigned)(delta<0?-delta:delta);
            }
            if (distance<best) { best=distance; selected=entry; }
        }
        if (slot<3) race->status_colours[slot]=(unsigned char)selected;
        else if (slot==3) race->collision_colour=(unsigned char)selected;
        else if(slot<7) race->hud_colours[slot-4]=(unsigned char)selected;
        else if(slot==7) race->weapon_hud_colour=(unsigned char)selected;
        else if(slot<11) race->arcade_colours[slot-8]=(unsigned char)selected;
        else if(slot==11) race->weapons.bullet_colour=(unsigned char)selected;
        else if(slot==12) race->weapons.impact_colour=(unsigned char)selected;
        else race->damage_smoke_colour=(unsigned char)selected;
    }
    race->arcade_hud_valid=0;
}

void slicks_status_clock_advance(struct SlicksStatusClock *clock,
                                 unsigned long pal_vblanks)
{
    /* BIOS 0040:006c cadence: PIT input / 65536, independent of the
     * game's /13107 simulation clock and of the number of updates. */
    while (pal_vblanks--) {
        clock->remainder+=1193182UL;
        if (clock->remainder>=65536UL*50UL) {
            clock->remainder-=65536UL*50UL;
            ++clock->ticks;
        }
    }
}

#if defined(__m68k__)
extern void slicks_draw_chunky_icon(unsigned char *, const unsigned char *,
    unsigned short, unsigned short, unsigned short, unsigned short);
#endif

static void draw_weapon_icon(struct SlicksRaceRuntime *race, unsigned char *logical,
                              const struct SlicksHudWeapon *command)
{
    const struct SlicksHudIcon *icon=&race->hud_weapon_icons[command->icon-5];
    unsigned left=(unsigned short)command->restore_x,changed=0;
    for(unsigned y=0;y<8;++y) for(unsigned x=0;x<16;++x) {
        unsigned char expected=race->hud_background[mult320[y+8]+left+x];
        if(x && x<=icon->width && y<icon->height) {
            unsigned char pixel=icon->pixels[y*icon->width+x-1];
            if(pixel) expected=pixel;
        }
        if(race->chunky[mult320[y+192]+left+x]!=expected) changed=1;
    }
    if(!changed) return;
    for(unsigned y=0;y<8;++y) for(unsigned x=0;x<16;++x)
        race->chunky[mult320[y+192]+left+x]=race->hud_background[mult320[y+8]+left+x];
#if defined(__m68k__)
    slicks_draw_chunky_icon(race->chunky,icon->pixels,command->icon_x,192,icon->width,icon->height);
#else
    for(unsigned y=0;y<icon->height;++y) for(unsigned x=0;x<icon->width;++x)
        if(icon->pixels[y*icon->width+x])
            race->chunky[mult320[y+192]+command->icon_x+x]=icon->pixels[y*icon->width+x];
#endif
    if(logical) for(unsigned y=192;y<200;++y) for(unsigned x=left;x<left+16;++x)
        write_pixel(logical,0,x,y,race->chunky[mult320[y]+x]);
    mark_dirty_rect(race,left,192,left+16,200);
}

int slicks_race_draw_status(struct SlicksRaceRuntime *race,
                            unsigned char *logical, unsigned short timer)
{
    struct SlicksStatusRect rectangles[4][3];
    int counts[4];
    struct SlicksHudWeapon weapons[4];
    int weapon_draw[4];
    if (!race) return -1;
    if (!race->fuel_option && !race->damage_scale && !race->weapons_enabled) return 0;
    if (!race->chunky || !race->status_colours[0]) return -1;
    if (race->chunky_authoritative) logical=0;
    /* Validate all four divisions before changing either representation. */
    for (unsigned car=0;car<4;++car) {
        if(!driver_role(race,car)) { counts[car]=weapon_draw[car]=0; continue; }
        counts[car]=slicks_race_status_rects(race,car,timer,rectangles[car]);
        if (counts[car]<0) return -1;
        weapon_draw[car]=slicks_hud_weapon(car,1,race->weapons_enabled,
            race->selected_weapon[car],race->weapon_inventory[car],race->weapon_capacity,&weapons[car]);
        if(weapon_draw[car]<0) return -1;
        if(weapon_draw[car] && (!race->hud_background_ready ||
           !race->hud_weapon_icons[weapons[car].icon-5].ready)) return -1;
        if(!counts[car] && race->weapons_enabled) {
            short left=(short)(106+car*60);
            rectangles[car][0]=(struct SlicksStatusRect){left,187,(short)(left+20),190,0};
            counts[car]=1;
        }
    }
    for (unsigned car=0;car<4;++car)
    for (int i=0;i<counts[car];++i) {
        const struct SlicksStatusRect *rect=&rectangles[car][i];
        int left=rect->left<0?0:rect->left;
        int right=rect->right>320?320:rect->right;
        unsigned char colour=race->status_colours[rect->colour];
        if (right<=left) continue;
        for (int y=rect->top;y<rect->bottom;++y)
        for (int x=left;x<right;++x) {
            if (race->chunky[mult320[y]+x]!=colour) {
                write_pixel(logical,race->chunky,(unsigned short)x,(unsigned short)y,colour);
                mark_dirty_pixel(race,(unsigned short)x,(unsigned short)y);
            }
        }
    }
    for(unsigned car=0;car<4;++car) if(weapon_draw[car]) {
        int left=weapons[car].bar_left,right=weapons[car].bar_right;
        if(right>320) right=320;
        for(int x=left;x<right;++x) {
            if(race->chunky[mult320[187]+x]!=race->weapon_hud_colour) {
                write_pixel(logical,race->chunky,x,187,race->weapon_hud_colour);
                mark_dirty_pixel(race,x,187);
            }
        }
        draw_weapon_icon(race,logical,&weapons[car]);
    }
    return 0;
}

/* Text runs carry layout, not a framebuffer shadow. The command generator
 * retains ddc0's alignment and last-lap/best-lap selection. */
static struct SlicksHudRun hud_text_run(const struct SlicksRaceFont *font,
                                        const struct SlicksHudText *command)
{
    struct SlicksHudRun run = {0};
    run.x = command->x; run.y = command->y;
    if (command->numeric) {
        unsigned value = command->number < 0
            ? -(int)command->number : command->number;
        char reverse[6];
        unsigned n = 0, at = 0;
        do { reverse[n++] = (char)('0' + value % 10); value /= 10; } while (value);
        if (command->number < 0) run.text[at++] = '-';
        while (n) run.text[at++] = reverse[--n];
    } else {
        for (unsigned i = 0; i < sizeof command->text; ++i)
            run.text[i] = command->text[i];
    }
    for (unsigned i = 0; run.text[i]; ++i) {
        unsigned glyph = font->lookup[(unsigned char)run.text[i]];
        if (glyph < font->glyph_count)
            run.width += font->widths[glyph] + font->advance_extra;
    }
    if ((command->flags & 3) == 2) run.x -= run.width;
    else if ((command->flags & 3) == 1) run.x -= run.width / 2;
    return run;
}

int slicks_race_set_track_info(struct SlicksRaceRuntime *race,
    const char *path, const unsigned char *data, unsigned long size)
{
    struct SlicksTrackRecords records={0};
    race->hud_track_ready=0;
    if(slicks_track_records(data,size,&records)<0) return -1;
    const char *stem=path;
    for(const char *p=path;*p;++p) if(*p=='/' || *p==':') stem=p+1;
    unsigned n=0;
    while(n<8 && stem[n] && stem[n]!='.') {
        race->hud_track_name[n]=stem[n]; ++n;
    }
    for(;n<9;++n) race->hud_track_name[n]=0;
    race->hud_record_time=(unsigned short)(records.entries[1][20]|
                                          records.entries[1][21]<<8);
    race->hud_track_ready=1;
    for(unsigned i=0;i<4;++i) race->hud_valid[i]=0;
    return 0;
}

static void draw_arcade_timer(struct SlicksRaceRuntime *race,unsigned char *logical)
{
    if(race->race_mode!=5 || !race->chunky || !race->font.ready) return;
    struct SlicksArcadeHud hud;
    if(slicks_arcade_hud(race->race_mode,race->arcade_seconds,race->game_clock_ticks,
        race->game_clock_ticks,race->finish_deadline,&hud)<0) return;
    short right=hud.count>1?hud.rects[1].right:0;
    short top=hud.count>1?hud.rects[1].top:0;
    if(race->arcade_hud_valid && race->arcade_hud_count==hud.count &&
        race->arcade_hud_text==hud.text && race->arcade_bar_right==right &&
        race->arcade_bar_top==top) return;
    for(unsigned i=0;i<hud.count;++i) {
        const struct SlicksArcadeRect *r=&hud.rects[i];
        int left=r->left<0?0:r->left,end=r->right>320?320:r->right;
        for(int y=r->top;y<r->bottom;++y)
            for(int x=left;x<end;++x)
                write_pixel(logical,race->chunky,x,y,race->arcade_colours[i]);
        if(end>left) mark_dirty_rect(race,left,r->top,end,r->bottom);
    }
    if(hud.text) {
        struct SlicksHudText command={0};
        command.x=70; command.y=189; command.flags=1;
        const char *text=hud.text==1?"LAST":"LAP";
        for(unsigned i=0;text[i];++i) command.text[i]=text[i];
        struct SlicksHudRun run=hud_text_run(&race->font,&command);
        race->font.runtime[6]=race->arcade_colours[2];
        draw_hud_run(race,&run);
        if(logical) for(int y=189;y<189+race->font.height;++y)
            for(int x=run.x;x<run.x+run.width;++x)
                write_pixel(logical,0,x,y,race->chunky[mult320[y]+x]);
        mark_dirty_rect(race,run.x,189,run.x+run.width,189+race->font.height);
    }
    race->arcade_hud_count=hud.count; race->arcade_hud_text=hud.text;
    race->arcade_bar_right=right; race->arcade_bar_top=top; race->arcade_hud_valid=1;
}

static void draw_timers(struct SlicksRaceRuntime *race, unsigned char *logical)
{
    if (!race->font.ready || !race->hud_background_ready || !race->chunky) return;
    if (!race->hud_valid[0] && !race->hud_valid[1] &&
        !race->hud_valid[2] && !race->hud_valid[3]) {
        draw_hud_background(race, logical);
        race->arcade_hud_valid=0;
        if(race->hud_track_ready) {
            struct SlicksHudText commands[2];
            unsigned count=slicks_hud_track_text(race->hud_track_name,
                                                race->hud_record_time,commands);
            race->font.runtime[6]=race->hud_colours[2];
            for(unsigned i=0;i<count;++i) {
                struct SlicksHudRun run=hud_text_run(&race->font,&commands[i]);
                draw_hud_run(race,&run);
            }
            if(logical)
                for(unsigned y=184;y<200;++y)
                    for(unsigned x=0;x<90;++x)
                        write_pixel(logical,0,x,y,race->chunky[mult320[y]+x]);
        }
    }
    for (unsigned car = 0; car < SLICKS_RACE_CAR_COUNT; ++car) {
        if(!driver_role(race,car)) continue;
        struct SlicksHudText commands[3];
        struct SlicksHudRun runs[3];
        const struct SlicksRaceCar *state = &race->cars[car];
        unsigned char options=(race->weapons_enabled?1:0)|
            (race->fuel_option?2:0)|(race->damage_scale?4:0);
        unsigned char place=state->finished?state->finish_position:0;
        unsigned short last=(unsigned short)state->last_lap_time_units;
        unsigned short best=(unsigned short)state->best_lap_time_units;
        if(race->hud_valid[car] && race->hud_input[car].lap==state->lap &&
           race->hud_input[car].place==place && race->hud_input[car].last==last &&
           race->hud_input[car].best==best && race->hud_input[car].options==options)
            continue;
        race->hud_input[car].lap=state->lap;race->hud_input[car].place=place;
        race->hud_input[car].last=last;race->hud_input[car].best=best;
        race->hud_input[car].options=options;
        unsigned count = slicks_hud_driver_text(car, state->lap,
            place,last,best,commands);
        unsigned changed = !race->hud_valid[car] ||
            count != race->hud_run_count[car];
        signed char selected=race->weapons_enabled?race->selected_weapon[car]:-1;
        /* Selection changes call the original status painter, not 1ddc0's
         * full text/background redraw. In particular, empty selection keeps
         * the previous icon until a lap/finish redraw removes it. */
        if(race->hud_status_options[car]!=options) changed=1;
        for (unsigned i = 0; i < count; ++i) {
            runs[i] = hud_text_run(&race->font, &commands[i]);
            const struct SlicksHudRun *old = &race->hud_runs[car][i];
            if (runs[i].x != old->x || runs[i].y != old->y ||
                runs[i].width != old->width) changed = 1;
            for (unsigned j = 0; j < sizeof runs[i].text; ++j)
                if (runs[i].text[j] != old->text[j]) changed = 1;
        }
        if (!changed) continue;
        /* 1ddd8..1de31 restores the original saved 50x22 HUD rectangle,
         * rounded to 52 pixels by 3abe5. Rows 200..207 are off-display VGA
         * storage, so never write them into the native 320x200 surface. */
        unsigned left=90+car*60;
        unsigned right=left+52<320?left+52:320;
        for(unsigned y=186;y<200;++y)
            for(unsigned x=left;x<right;++x)
                race->chunky[mult320[y]+x]=race->hud_background[mult320[y-184]+x];
        race->font.runtime[6]=race->hud_colours[state->finished?1:0];
        for(unsigned i=0;i<count;++i) draw_hud_run(race,&runs[i]);
        if(logical)
            for(unsigned y=186;y<200;++y)
                for(unsigned x=left;x<right;++x)
                    write_pixel(logical,0,x,y,race->chunky[mult320[y]+x]);
        mark_dirty_rect(race,left,186,right,200);
        for (unsigned i = 0; i < count; ++i) race->hud_runs[car][i] = runs[i];
        race->hud_run_count[car] = count;
        race->hud_status_options[car]=options;
        race->hud_weapon_selection[car]=selected;
        race->hud_valid[car] = 1;
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
    race->cars[0].vehicle = 5;
    race->cars[1].vehicle = 2;
    race->boundary_level = 5;
    race->random_state = 0x1fadec20UL; /* Legacy diagnostics; menus supply their seed. */
    for(unsigned car=0;car<4;++car) {
        race->selected_weapon[car]=-1;
        race->cars[car].position_scale=100; /* Explicit legacy diagnostic setup. */
    }
}

void slicks_race_set_service_options(struct SlicksRaceRuntime *race,
                                     short fuel_setting, short damage_setting)
{
    if (!race || race->started)
        return;
    race->fuel_option = fuel_setting > 5 ? fuel_setting : 0;
    race->damage_scale = damage_setting;
}

int slicks_race_set_participation(struct SlicksRaceRuntime *race,
                                 const signed char participation[4])
{
    if(!race || !participation || race->started) return -1;
    unsigned count=0;
    for(unsigned car=0;car<4;++car) count+=participation[car]!=0;
    if(!count) return -1; /* Native GO guard: no entrants, no race. */
    for(unsigned car=0;car<4;++car) race->participation[car]=participation[car];
    race->participation_ready=1;
    return 0;
}

int slicks_race_set_inventory(struct SlicksRaceRuntime *race,
                              const short inventory[4][13])
{
    if (!race || !inventory || race->started) return -1;
    /* Six interpolation knots cover upgrade levels 0..20. Do not silently
     * clamp corrupt/unsupported saved state or index beyond those tables. */
    for (unsigned car=0;car<4;++car)
        if (inventory[car][0]<0 || inventory[car][0]>20 ||
            inventory[car][1]<0 || inventory[car][1]>20 ||
            inventory[car][3]<0 || inventory[car][3]>20) return -1;
    for (unsigned car=0;car<4;++car)
        for (unsigned slot=0;slot<13;++slot)
            race->weapon_inventory[car][slot]=inventory[car][slot];
    race->setup_inventory_ready=1;
    return 0;
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
    struct SlicksCarSprite *sprite=&race->sprites[vehicle][base_direction];
    if(decode_sprite(sprite,resource,resource_size)) return -1;
    sprite->ready=0;
    unsigned markers=0;
    for(unsigned i=0;i<(unsigned)sprite->width*sprite->height;++i)
        markers+=sprite->pixels[i]>=254;
    /* All supplied models have at most two markers. More would require
     * per-orientation pixel storage because FE selection is scan ordered. */
    if(markers>2) return -1;
    for(unsigned rotation=0;rotation<4;++rotation) {
        unsigned char pixels[SLICKS_CAR_PIXEL_MAX],width,height;
        rotated_size(sprite,rotation,&width,&height);
        for(unsigned y=0;y<height;++y) for(unsigned x=0;x<width;++x)
            pixels[y*width+x]=sprite_pixel(sprite,rotation,x,y);
        if(slicks_extract_wheels(pixels,width,height,sprite->wheel_x[rotation],
                                sprite->wheel_y[rotation])) return -1;
    }
    for(unsigned i=0;i<(unsigned)sprite->width*sprite->height;++i) {
        if(sprite->pixels[i]==255) sprite->pixels[i]=25;
        else if(sprite->pixels[i]==254) sprite->pixels[i]=184;
    }
    sprite->ready=1;
    return 0;
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
    /* 1d01a..1d029: unsigned file byte, subtract 100, then double. */
    properties->drive_bias = ((short)resource[23] - 100) * 2;
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
    properties->property_29 = resource[29];
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
    if (!race || !resource || resource_size < 12 || resource[9]!=2)
        return -1;
    font = &race->font;
    for (glyph = 0; glyph < 256; ++glyph)
        font->lookup[glyph] = 0xff;
    font->glyph_count = resource[4];
    font->height = resource[6];
    /* Runtime font header starts at archive byte 4. 300e5 measures each
     * glyph as width + header[3] + DS:1604 - 1; HUD DS:1604 is 1. */
    font->advance_extra = resource[7];
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
        font->offsets[glyph] = pixel_count;
        font->lookup[font->codes[glyph]] = (unsigned char)glyph;
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
    if (slicks_decode_font_resource(resource, resource_size, font->runtime,
                                   sizeof font->runtime) < 0)
        return -1;
    font->pixel_count = pixel_count;
    font->ready = 1;
    return 0;
}

int slicks_race_add_hud_background(struct SlicksRaceRuntime *race,
                                  const unsigned char *resource,
                                  unsigned long resource_size)
{
    if(!race) return -1;
    race->hud_background_ready=0;
    if(slicks_decode_hud_background(resource,resource_size,race->hud_background)) return -1;
    race->hud_background_ready=1;
    return 0;
}

int slicks_race_add_weapon_icon(struct SlicksRaceRuntime *race,
    unsigned short weapon, const unsigned char *resource, unsigned long size)
{
    if(!race || weapon>=8) return -1;
    struct SlicksHudIcon *icon=&race->hud_weapon_icons[weapon];
    unsigned short width,height;
    icon->ready=0;
    if(slicks_decode_hud_image(resource,size,icon->pixels,sizeof icon->pixels,
                               &width,&height) || width>15 || height>8) return -1;
    icon->width=(unsigned char)width; icon->height=(unsigned char)height;
    icon->ready=1;
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

int slicks_race_start(struct SlicksRaceRuntime *race, unsigned char *logical,
                      unsigned char *chunky)
{
    unsigned short car;
    unsigned short direction;
    if (!race || !logical || !chunky || !race->navigation.zone_count ||
        !race->font.ready)
        return -1;
    for (car = 0; car < SLICKS_RACE_CAR_COUNT; ++car)
        if (race->cars[car].vehicle >= SLICKS_VEHICLE_COUNT)
            return -1;
    race->chunky = chunky;
    if(race->track_actors_ready) {
        /* Seed saved-under from actual scenery before the very first actor
         * draw. Later frames never deinterleave the VGA surface again. */
        const unsigned char *s=logical;
        unsigned char *d=chunky;
        for(unsigned y=0;y<200;++y,s+=20)
            for(unsigned x=0;x<80;++x,++s) {
                *d++=s[0]; *d++=s[0x10000]; *d++=s[0x20000]; *d++=s[0x30000];
            }
        race->chunky_authoritative=1;
        logical=0;
    }
    race->finish_ranks_ready=0;
    race->track_rewarded=0;
    race->race_complete=0;
    race->results_drawn=0;
    race->finished_count=0;
    race->actor_page = 0; /* First gameplay PRE after the DOS setup toggle. */
    race->boundary_level=5;
    race->boundary_timer=120;
    race->boundary_direction=1;
    race->boundary_palette_pending=0;
    race->track_flag_activations=0;
    race->track_actor_scratch=0;
    if(shared_actor_pool(race)) initialize_weapon_actors(race);
    race->pit_repair_ticks = 0;
    for (car = 0; car < SLICKS_RACE_CAR_COUNT; ++car) {
        race->shadows[car].state = -3;
        race->shadows[car].lifetime = 0;
        race->shadows[car].saved_valid = 0;
    }
    for (car = 0; car < SLICKS_VEHICLE_COUNT; ++car)
        if (!race->properties[car].ready)
            return -1;
    for (car = 0; car < SLICKS_START_LIGHT_COUNT; ++car)
        if (!race->start_lights[car].ready)
            return -1;
    for (car = 0; car < SLICKS_VEHICLE_COUNT; ++car)
        for (direction = 0; direction < SLICKS_CAR_BASE_DIRECTIONS;
             ++direction)
            if (!race->sprites[car][direction].ready)
                return -1;

    for (car = 0; car < SLICKS_RACE_CAR_COUNT; ++car) {
        struct SlicksRaceCar *state = &race->cars[car];
        /* Original 1c111..1c241 initializes all four slots, even absent
         * drivers. Zero here would give an inactive slot the fastest lap. */
        state->best_lap_time_units=30000U;
        if(!driver_role(race,car)) continue;
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
        state->actor_layer = race->navigation.start_style;
        state->actor_contact = 0;
        state->oil_active = 0;
        state->oil_turn_sign = 0;
        state->waypoint = 0;
        state->lap = 1;
        state->speed = 0;
        state->speed_fixed = 0;
        state->measured_speed = 0;
        state->forward_drive_latch = 0;
        state->steering_amount = profile_steering_input(state->position_scale,
            driver_role(race,car));
        state->steering_scale = 1000; /* Original race initializer 1c20b. */
        for (direction = 0; direction < 4; ++direction)
            state->damage[direction] = 0;
        state->damage_turn_sign = 0;
        state->damage_smoke_ticks = 0;
        state->pending_damage_impact = 0;
        state->special_drive_state = state->special_drive_target = 0;
        state->drive_bias = race->properties[state->vehicle].drive_bias;
        for (direction = 0; direction < 13; ++direction)
            state->drive_setup[direction] = race->setup_inventory_ready
                ? race->weapon_inventory[car][direction] : (direction < 5 ? 4 : 0);
        if (race->setup_inventory_ready)
            state->fuel_upgrade=state->drive_setup[2];
        initialize_car_fuel(race, state);
        interpolate_drive_coefficients(state);
        /* Steering reads DS:6aee, the seventh interpolated upgrade
         * coefficient, not a fixed normal-loadout value of 104. */
        state->steering_property = state->drive_coefficients[6];
        state->maximum_speed = 100;
        state->ai_last_x = state->x;
        state->ai_last_y = state->y;
        state->ai_stuck_ticks = 700;
        state->ai_target_x = -1; /* fdfd alternate target not selected yet. */
        state->ai_contact_ticks = 60000;
        state->ai_contact_threshold = 350;
        state->ai_route_seen = 1;
        state->ai_recovery_ticks = 400;
        if(!race->track_actors_ready) draw_car(race, logical, car);
    }
    race->game_clock_ticks = 0;
    race->finish_deadline = 0;
    race->arcade_hud_valid = 0;
    draw_timers(race, logical);
    draw_arcade_timer(race, logical);
    if(race->track_actors_ready) draw_race_actors(race,logical);
    draw_start_light(race, logical, 0);
    race->countdown_ticks = 0x78;
    race->countdown_stage = 0;
    race->racing = 0;
    race->laps_to_run = race->race_mode==5 ? 9999 : 4;
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
    if (race && !race->started && car < SLICKS_RACE_CAR_COUNT &&
        vehicle < SLICKS_VEHICLE_COUNT)
        race->cars[car].vehicle = (unsigned char)vehicle;
}

void slicks_race_set_laps(struct SlicksRaceRuntime *race,
                          unsigned short laps)
{
    if (race)
        race->laps_to_run = race->race_mode==5 ? 9999 : (laps ? laps : 1);
}

void slicks_race_use_chunky_surface(struct SlicksRaceRuntime *race)
{
    if (race && race->chunky)
        race->chunky_authoritative = 1;
}

void slicks_race_step(struct SlicksRaceRuntime *race, unsigned char *logical)
{
    unsigned char profile;
    unsigned short car;
    unsigned short ticks;
    if (!race || !logical || !race->started)
        return;
    race->sound_event_count=0;
    /* The original race loop returns at completion. Do not keep advancing
     * clocks/physics or emitting late finish awards under the results UI. */
    if(race->race_complete) return;
    if (race->chunky_authoritative)
        logical = 0;
    profile = (unsigned char)(race->profile_marker &&
        race->frame_count + 1 == race->profile_frame);
    if (profile)
        race->profile_marker(0);
    race->collision_impact = 0;
    ticks = next_physics_ticks(race);
    race->game_clock_ticks += ticks;
    if(slicks_advance_boundary(&race->boundary_level,&race->boundary_timer,
            &race->boundary_direction,ticks,&race->random_state,
            race->boundary_colours))
        race->boundary_palette_pending=1;
    if (!race->racing) {
        if(race->track_actors_ready) {
            restore_start_light(race,logical);
            restore_race_actors(race,logical);
        }
        update_track_actor_motion(race);
        /* fe3c resets the DOS clock before the lights; fe9f advances it
         * before countdown handling. Lap timestamps start at zero (fdd1).
         * Stationary grid time therefore belongs to the first lap. */
        for (car = 0; car < SLICKS_RACE_CAR_COUNT; ++car)
            if(driver_role(race,car)) advance_car_clock(&race->cars[car], ticks);
        race->countdown_ticks -= ticks;
        if (race->countdown_ticks < 0) {
            ++race->countdown_stage;
            race->countdown_ticks = 10;
            if (!race->track_actors_ready && race->countdown_stage < SLICKS_START_LIGHT_COUNT)
                draw_start_light(race, logical, race->countdown_stage);
            else if (!race->track_actors_ready && race->countdown_stage == SLICKS_START_LIGHT_COUNT)
                restore_start_light(race, logical);
            if (race->countdown_stage > 5)
                race->racing = 1;
        }
        advance_weapon_actors(race);
        if(race->track_actors_ready) {
            draw_race_actors(race,logical);
            if(race->countdown_stage<SLICKS_START_LIGHT_COUNT)
                draw_start_light(race,logical,race->countdown_stage);
        }
        race->actor_page ^= 1;
        ++race->frame_count;
        return;
    }
    /* Priority 0, layer-1 cars, priority 3, layer-0 cars, priorities 5/6.
     * Restore in reverse layer order, then redraw forward. */
    restore_race_actors(race,logical);
    update_track_actor_motion(race);
    if (profile)
        race->profile_marker(1);
    if(race->poll_driver_devices) race->poll_driver_devices(race,ticks);
    update_cars(race, ticks);
    if(race->race_complete) slicks_race_award_track(race);
    if (profile)
        race->profile_marker(3);
    /* DOS 2000:3f51 calls the actor update after all four car tails have
     * emitted their effects. New points move/decrement on this same pass. */
    advance_trail_particles(race);
    advance_weapon_actors(race);
    if (profile)
        race->profile_marker(2);
    draw_timers(race, logical);
    draw_arcade_timer(race, logical);
    if (profile)
        race->profile_marker(4);
    draw_race_actors(race,logical);
    if (profile)
        race->profile_marker(5);
    /* Resource-backed post-race screens belong to the platform caller.
     * Retain the diagnostic handoff flag, but do not paint a fabricated
     * RESULTS panel over the last native race frame. */
    if (race->race_complete)
        race->results_drawn=1;
    race->actor_page ^= 1;
    ++race->frame_count;
}

void slicks_race_award_track(struct SlicksRaceRuntime *race)
{
    if(!race || race->track_rewarded) return;
    race->track_rewarded=1;
    if(race->track_reward) race->track_reward(race);
}

void slicks_race_prune_dirty_pixels(struct SlicksRaceRuntime *race)
{
    unsigned out=0;
    for(unsigned i=0;i<race->dirty_pixel_count;++i) {
        struct SlicksDirtyPixel pixel=race->dirty_pixels[i];
        unsigned r;
        for(r=0;r<race->dirty_row_count;++r) {
            const struct SlicksDirtyRows *rect=&race->dirty_rows[r];
            if(pixel.x>=rect->left && pixel.x<rect->right &&
               pixel.y>=rect->top && pixel.y<rect->bottom) break;
        }
        if(r==race->dirty_row_count) race->dirty_pixels[out++]=pixel;
    }
    race->dirty_pixel_count=(unsigned short)out;
}

void slicks_race_clear_dirty_rows(struct SlicksRaceRuntime *race)
{
    if (race) {
        race->dirty_row_count = 0;
        race->dirty_pixel_count = 0;
    }
}
