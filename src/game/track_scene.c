#include "track_scene.h"

#define SLICKS_SPRITE_COUNT 110
#define SLICKS_PLANE_SIZE 0x10000UL
#define SLICKS_STRIDE 100U

struct TrackSprite {
    const unsigned char *pixels;
    unsigned short width;
    unsigned short height;
};

static unsigned short read_be16(const unsigned char *source)
{
    return ((unsigned short)source[0] << 8) | source[1];
}

static int decode_dat_images(const unsigned char *source,
                             unsigned long source_size,
                             unsigned char *arena,
                             unsigned long arena_size,
                             struct TrackSprite *sprites)
{
    unsigned long source_at = 0;
    unsigned long arena_at = 0;
    unsigned short image;

    for (image = 0; image < SLICKS_SPRITE_COUNT; ++image) {
        unsigned char format;
        unsigned char escape = 0;
        unsigned short width;
        unsigned short height;
        unsigned long pixel_count;
        unsigned long produced = 0;

        if (source_at + 3 > source_size)
            return -1;
        format = source[source_at++];
        width = source[source_at++];
        height = source[source_at++];
        if (format > 3)
            return -1;
        width |= (unsigned short)(format & 1) << 8;
        pixel_count = (unsigned long)width * height;
        if (!width || !height || arena_at + pixel_count > arena_size)
            return -1;

        sprites[image].pixels = arena + arena_at;
        sprites[image].width = width;
        sprites[image].height = height;

        if (format & 2) {
            if (source_at >= source_size)
                return -1;
            escape = source[source_at++];
            while (produced < pixel_count) {
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
                while (count-- && produced < pixel_count)
                    arena[arena_at + produced++] = value;
            }
        } else {
            if (source_at + pixel_count > source_size)
                return -1;
            while (produced < pixel_count) {
                arena[arena_at + produced] = source[source_at + produced];
                ++produced;
            }
            source_at += pixel_count;
        }

        arena_at += pixel_count;
    }
    return 0;
}

static int is_control_object(unsigned char type)
{
    static const unsigned char controls[5] = {79, 80, 81, 82, 89};
    unsigned short i;
    for (i = 0; i < 5; ++i) {
        if (type == controls[i])
            return 1;
    }
    return 0;
}

static void put_pixel(unsigned char *logical, unsigned short x,
                      unsigned short y, unsigned char value)
{
    unsigned long offset;
    if (!value || x >= 320 || y >= 190)
        return;
    offset = (unsigned long)y * SLICKS_STRIDE + (x >> 2);
    offset += (unsigned long)(x & 3) * SLICKS_PLANE_SIZE;
    logical[offset] = value;
}

static void draw_sprite(unsigned char *logical,
                        unsigned char *material_map,
                        unsigned char *surface_map,
                        const struct TrackSprite *sprite,
                        unsigned short origin_x, unsigned short origin_y,
                        unsigned char rotation,
                        unsigned char type)
{
    unsigned short source_y;
    rotation &= 3;
    for (source_y = 0; source_y < sprite->height; ++source_y) {
        unsigned short source_x;
        for (source_x = 0; source_x < sprite->width; ++source_x) {
            unsigned short x;
            unsigned short y;
            unsigned char pixel =
                sprite->pixels[(unsigned long)source_y * sprite->width +
                               source_x];
            if (rotation == 1) {
                x = origin_x + sprite->height - 1 - source_y;
                y = origin_y + source_x;
            } else if (rotation == 2) {
                x = origin_x + sprite->width - 1 - source_x;
                y = origin_y + sprite->height - 1 - source_y;
            } else if (rotation == 3) {
                x = origin_x + source_y;
                y = origin_y + sprite->width - 1 - source_x;
            } else {
                x = origin_x + source_x;
                y = origin_y + source_y;
            }
            put_pixel(logical, x, y, pixel);
            if (x < 320 && y < 190) {
                unsigned char write_material = pixel != 0;
                unsigned char material = 0;

                /* b225 builds the mode-zero b089 buffer independently of
                 * the visible VGA page. Most opaque DAT pixels clear its
                 * five-bit class. The bridge pieces are the exceptions:
                 * type 85 deliberately writes through transparent pixels,
                 * type 87 splits its two source colours, and the two visual
                 * overlays preserve the class already underneath them. */
                if (type == 85) {
                    write_material = 1;
                    material = pixel ? 31 : 1;
                } else if (type == 44 || type == 68) {
                    write_material = 0;
                } else if (type == 49 || type == 90) {
                    material = 2;
                } else if (type == 87) {
                    material = pixel == 217 ? 1 : 0;
                }
                if (write_material)
                    material_map[(unsigned long)y * 320UL + x] = material;

                /* b1a5 stores the independent mode-one b089 value used by
                 * the wheel-effect dispatcher.  The value is a property of
                 * the DAT object class, not of its displayed palette index;
                 * bridge and foreground overlays deliberately preserve the
                 * surface beneath them. */
                {
                    unsigned char surface_pixel = pixel != 0;
                    unsigned long surface_at =
                        (unsigned long)y * 320UL + x;
                    unsigned char previous_surface = surface_map[surface_at];
                    unsigned char write_surface = 1;
                    unsigned char surface = 0;
                    /* The DOS surface pass has a small set of deliberate mask
                     * corrections which differ from the displayed DAT byte. */
                    if (type == 31 && source_x == 12 && source_y == 0)
                        surface_pixel = 0;
                    if ((type == 32 && source_x == 11 && source_y == 2) ||
                        (type == 36 &&
                         ((source_x == 7 && source_y == 13) ||
                          (source_x == 14 && source_y == 6))) ||
                        (type == 28 &&
                         ((source_x == 28 && source_y == 2) ||
                          (source_x == 22 && source_y == 2) ||
                          (source_x == 35 && source_y == 3) ||
                          (source_x == 37 && source_y == 3) ||
                          (source_x == 5 && source_y == 8) ||
                          (source_x == 4 && source_y == 11))) ||
                        (type == 44 &&
                         ((source_x == 4 && source_y == 2) ||
                          (source_x == 3 && source_y == 3) ||
                          (source_x == 6 && source_y == 3) ||
                          (source_x == 6 && source_y == 6))))
                        surface_pixel = 1;
                    if (type == 30 && source_x == 6 && source_y == 9)
                        surface_pixel = 0;
                    if (!surface_pixel || y >= 185)
                        write_surface = 0;
                    /* The mode-one plane has two protected overlay classes.
                     * Object 0 is permanent; object 44 survives later scenery
                     * except for the recovered object-9 eraser. Bits 7 and 6
                     * are build-only priority markers. */
                    if (((previous_surface & 0x80) &&
                         type != 18 && type != 33) ||
                        ((previous_surface & 0x40) && type != 9))
                        write_surface = 0;
                    switch (type) {
                    case 0: surface = 17; break;
                    case 18: case 19: case 20:
                    case 23: case 31: case 32: case 33:
                        surface = 2; break;
                    case 24: case 25: case 27: case 44:
                        surface = 3; break;
                    case 35: case 36: case 37: case 43:
                        surface = 5; break;
                    case 28: case 30: case 50:
                        surface = 11; break;
                    case 39: case 40:
                        surface = pixel == 216 ? 19 :
                                  (pixel == 26 || pixel == 217) ? 2 : 0;
                        break;
                    case 63:
                        surface = pixel == 75 ? 6 :
                                  pixel == 74 ? 5 : 14;
                        break;
                    case 64: surface = 14; break;
                    case 87: surface = pixel == 217 ? 2 : 19; break;
                    case 49:
                        surface = pixel == 26 ? 2 : 0;
                        break;
                    case 68: case 85:
                        write_surface = 0;
                        break;
                    default:
                        surface = 0;
                        break;
                    }
                    if (type == 44 && (previous_surface & 0x1f) == 2)
                        write_surface = 0;
                    if (write_surface) {
                        if (type == 0)
                            surface |= 0x80;
                        else if (type == 44)
                            surface |= 0x40;
                        surface_map[surface_at] = surface;
                    }
                }
            }
        }
    }
}

int slicks_build_track_scene(unsigned char *logical,
                             unsigned char *material_map,
                             unsigned char *surface_map,
                             const unsigned char *dat,
                             unsigned long dat_size,
                             const unsigned char *track,
                             unsigned long track_size,
                             unsigned char *sprite_arena,
                             unsigned long arena_size,
                             struct SlicksTrackNavigation *navigation)
{
    struct TrackSprite sprites[SLICKS_SPRITE_COUNT];
    unsigned long at;
    unsigned short count;
    unsigned short object;
    unsigned short object_count;

    if (!logical || !material_map || !surface_map || !dat || !track || !sprite_arena ||
        !navigation ||
        track_size < 6 ||
        track[4] != 0x7e || track[5] != 2 ||
        decode_dat_images(dat, dat_size, sprite_arena, arena_size,
                          sprites) != 0)
        return -1;

    for (at = 0; at < 320UL * 190UL; ++at) {
        material_map[at] = 0;
        surface_map[at] = 0;
    }

    /* The original loader seeks forward 165h after consuming the two-byte
     * file signature, reads its two compatibility words and flags byte, then
     * advances ten bytes to the two checksummed track strings. */
    at = 6 + 0x165UL;
    if (at + 5 + 10 > track_size)
        return -1;
    at += 5 + 10;
    for (object = 0; object < 2; ++object) {
        while (at < track_size && track[at++])
            ;
        if (at > track_size)
            return -1;
    }
    if (at + 10 > track_size)
        return -1;
    at += 8; /* Four original string checksum words. */
    count = read_be16(track + at);
    object_count = count;
    at += 2;
    if (at + (unsigned long)count * 5 > track_size)
        return -1;

    for (object = 0; object < count; ++object) {
        unsigned short x = read_be16(track + at);
        unsigned short y = track[at + 2];
        unsigned char type = track[at + 3];
        unsigned char rotation = track[at + 4];
        at += 5;
        if (type >= SLICKS_SPRITE_COUNT)
            return -1;
        if (!is_control_object(type))
            draw_sprite(logical, material_map, surface_map, &sprites[type],
                        x, y, rotation, type);
    }

    /* Strip the construction priorities before gameplay samples b089. */
    {
        unsigned long surface_at;
        for (surface_at = 0; surface_at < 320UL * 190UL; ++surface_at)
            surface_map[surface_at] &= 0x1f;
    }

    /* Six-byte auxiliary line records follow the placed objects. */
    if (at + 2 > track_size)
        return -1;
    count = read_be16(track + at);
    at += 2;
    if (at + (unsigned long)count * 6 > track_size)
        return -1;
    at += (unsigned long)count * 6;

    /* The next records are the original AI/navigation regions: three points
     * and one speed byte each.  The third point is the driving target. */
    if (at + 2 > track_size)
        return -1;
    count = read_be16(track + at);
    at += 2;
    if (count > SLICKS_TRACK_ZONE_MAX ||
        at + (unsigned long)count * 10 > track_size)
        return -1;
    navigation->zone_count = count;
    for (object = 0; object < count; ++object) {
        unsigned short point;
        for (point = 0; point < 3; ++point) {
            navigation->zones[object].x[point] = read_be16(track + at);
            navigation->zones[object].y[point] = track[at + 2];
            at += 3;
        }
        navigation->zones[object].speed = track[at++];
    }

    /* Optional auxiliary path points are not used by BASIC.SS, but consume
     * them exactly so the common start-pose trailer is parsed correctly. */
    if (at + 2 > track_size)
        return -1;
    count = read_be16(track + at);
    at += 2;
    if (at + (unsigned long)count * 4 + 7 > track_size)
        return -1;
    at += (unsigned long)count * 4;
    navigation->start_x = read_be16(track + at);
    navigation->start_y = read_be16(track + at + 2);
    navigation->start_heading = track[at + 4];
    navigation->start_style = track[at + 5];
    return (int)object_count;
}
