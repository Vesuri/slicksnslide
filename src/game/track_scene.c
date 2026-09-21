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
                        const struct TrackSprite *sprite,
                        unsigned short origin_x, unsigned short origin_y,
                        unsigned char rotation)
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
        }
    }
}

int slicks_build_track_scene(unsigned char *logical,
                             const unsigned char *dat,
                             unsigned long dat_size,
                             const unsigned char *track,
                             unsigned long track_size,
                             unsigned char *sprite_arena,
                             unsigned long arena_size)
{
    struct TrackSprite sprites[SLICKS_SPRITE_COUNT];
    unsigned long at;
    unsigned short count;
    unsigned short object;

    if (!logical || !dat || !track || !sprite_arena || track_size < 6 ||
        track[4] != 0x7e || track[5] != 2 ||
        decode_dat_images(dat, dat_size, sprite_arena, arena_size,
                          sprites) != 0)
        return -1;

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
            draw_sprite(logical, &sprites[type], x, y, rotation);
    }
    return (int)count;
}
