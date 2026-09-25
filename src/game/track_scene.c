#include "track_scene.h"
#include "../graphics/row_offsets.h"
#include "../ui/track_info.h"

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

int slicks_build_track_preview(struct SlicksChunkyUi *ui,
    const unsigned char *dat,unsigned long dat_size,
    const unsigned char *track,unsigned long track_size,
    unsigned char *arena,unsigned long arena_size,short x,short y)
{
    struct SlicksTrackPreview preview;
    struct TrackSprite sprites[SLICKS_SPRITE_COUNT];
    if(!ui || !ui->pixels || !dat || !arena) return -1;
    int result=slicks_track_preview_open(&preview,track,track_size);
    if(result) return result;
    if(decode_dat_images(dat,dat_size,arena,arena_size,sprites)) return -1;
    /* An invalid rotation would select outside the original four resource
     * banks. Reject it before modifying the menu; never silently mask it. */
    for(unsigned i=0;i<preview.count;++i)
        if(preview.objects[5UL*i+3]<SLICKS_SPRITE_COUNT && preview.objects[5UL*i+4]>3) return -1;
    slicks_ui_rectangle(ui,x,y,(short)(unsigned short)(x+65),
        (short)(unsigned short)(y+40),21);
    for(unsigned i=0;i<preview.count;++i) {
        short px,py; unsigned char type,rotation;
        slicks_track_preview_object(&preview,i,x,y,&px,&py,&type,&rotation);
        /* Original 1a1d4 ignores types outside the DAT object table. */
        if(type>=SLICKS_SPRITE_COUNT) continue;
        const struct TrackSprite *sprite=&sprites[type];
        /* 1bf20..1bf96 builds these six resource banks using (rotation&1)*3
         * instead of the requested quarter-turn. Original DS:0777 table. */
        if((type>=31 && type<=34) || type==63 || type==64)
            rotation=(unsigned char)((rotation&1)*3);
        if(slicks_track_preview_sprite(ui,sprite->pixels,sprite->width,
            sprite->height,rotation,px,py)) return -1;
    }
    return 0;
}

int slicks_record_track_actor(struct SlicksTrackNavigation *navigation,
                              unsigned short x, unsigned short y,
                              unsigned char type)
{
    static const unsigned char controls[5] = {79, 80, 81, 82, 89};
    static const signed char offsets[5] = {0, -2, 0, -2, 0};
    unsigned short i;
    for (i = 0; i < 5; ++i) {
        if (type == controls[i]) {
            struct SlicksTrackActor *actor =
                &navigation->actors[navigation->actor_count];
            /* b908..b9ae: signed table byte, SUB/SHL word truncation. */
            actor->x = (short)((unsigned short)(x - offsets[i]) << 4);
            actor->y = (short)((unsigned short)(y - offsets[i]) << 4);
            actor->velocity_x = actor->velocity_y = 0;
            actor->kind = (unsigned char)i;
            actor->layer = 1;
            if (navigation->actor_count < SLICKS_TRACK_ACTOR_MAX - 1)
                ++navigation->actor_count;
            return 1;
        }
    }
    return 0;
}

static inline __attribute__((always_inline)) int apply_material_mask(unsigned char pixel, unsigned char bridge,
                               unsigned char *lower, unsigned char *upper)
{
    unsigned char category = 1, value = pixel, previous = *lower;
    if (pixel > 37) return -1;
    if (!pixel) return 0;
    switch (pixel) {
    case 1: value = 31; break;
    case 2: case 16: case 30: category = 2; break;
    case 15: category = 3; break;
    case 18: category = 8; value = 31; break;
    case 19: category = 5; break;
    case 20: category = 8; value = 1; break;
    case 27: case 28: case 29: case 35: category = 0; break;
    case 31: category = 9; value = 0; break;
    case 32: category = 8; value = 2; break;
    case 33: category = 7; value = 1; break;
    case 34: category = 6; value = 1; break;
    case 36: value = 18; break;
    case 37: category = 2; value = 27; break;
    }
    /* b2f8..b426: categories 5..8 override existing lower occupancy. */
    if (category >= 5 && category <= 8) previous = 0;
    if (previous) {
        if (category == 1 && (previous == 17 || previous == 2)) return 0;
        if ((category == 8 || category == 6) && (*upper == 1 || *upper == 19))
            *upper = 0;
        if (category == 9) *upper = 0;
        *lower = value & 31;
    } else {
        switch (category) {
        case 1: case 5: case 9:
            if (value == 31) value = 0;
            if (category == 1 && (*upper == 17 || *upper == 2)) return 0;
            *lower = 0; *upper = value; break;
        case 2: *lower = 0; *upper = value; break;
        case 3: *lower = value; *upper = value; break;
        case 6: *lower = 2; *upper = 2; break;
        case 7: *lower = 1; *upper = 2; break;
        case 8:
            if (*upper == 1 || *upper == 19 || !bridge) *upper = 0;
            *lower = value; break;
        default: break;
        }
    }
    return 0;
}

int slicks_apply_material_mask(unsigned char pixel,unsigned char bridge,
    unsigned char *lower,unsigned char *upper)
{ return apply_material_mask(pixel,bridge,lower,upper); }

int slicks_build_track_masks(unsigned char *lower, unsigned char *upper,
                             const unsigned char *masks, unsigned long masks_size,
                             const unsigned char *track, unsigned long track_size,
                             unsigned char *arena, unsigned long arena_size,
                             unsigned char service, unsigned char bridge,
                             struct SlicksTrackNavigation *navigation)
{
    struct TrackSprite sprites[SLICKS_SPRITE_COUNT];
    unsigned long at = 6 + 0x165UL + 15;
    unsigned short count, object;
    if (!lower || !upper || !masks || !track || !arena || !navigation ||
        decode_dat_images(masks, masks_size, arena, arena_size, sprites)) return -1;
    for (object = 0; object < 2; ++object) {
        while (at < track_size && track[at]) ++at;
        if (at == track_size) return -1;
        ++at;
    }
    if (at + 10 > track_size) return -1;
    at += 8;
    count = read_be16(track + at); at += 2;
    if (at + (unsigned long)count * 5 > track_size) return -1;
    for (unsigned long i = 0; i < 320UL * 190UL; ++i) lower[i] = upper[i] = 0;
    for (object = 0; object < count; ++object, at += 5) {
        unsigned short ox = read_be16(track + at), oy = track[at + 2];
        unsigned char type = track[at + 3], rotation = track[at + 4] & 3;
        const struct TrackSprite *sprite;
        if (type >= SLICKS_SPRITE_COUNT) return -1;
        if (!service && (type == 68 || type == 69)) continue;
        sprite = &sprites[type];
        unsigned width=(rotation&1)?sprite->height:sprite->width;
        unsigned height=(rotation&1)?sprite->width:sprite->height;
        unsigned first=ox>=320?65536U-ox:0;
        if(first>=width) continue;
        unsigned x=(unsigned short)(ox+first),length=width-first;
        if(length>320-x) length=320-x;
        for(unsigned row=0;row<height;++row) {
            unsigned short y=(unsigned short)(oy+row);
            /* Original mask crop differs from the visible scene crop. */
            if(y>=185) continue;
            long base,step;
            if(rotation==1) { base=(long)(sprite->height-1)*sprite->width+row; step=-(long)sprite->width; }
            else if(rotation==2) { base=(long)(sprite->height-1-row)*sprite->width+sprite->width-1; step=-1; }
            else if(rotation==3) { base=sprite->width-1-row; step=sprite->width; }
            else { base=(long)row*sprite->width; step=1; }
            const unsigned char *src=sprite->pixels+base+(long)first*step;
            unsigned char *lo=lower+mult320[y]+x,*hi=upper+mult320[y]+x;
            for(unsigned col=0;col<length;++col,src+=step,++lo,++hi) {
                unsigned char pixel=*src;
                if(pixel && apply_material_mask(pixel,bridge,lo,hi)) return -1;
            }
        }
    }
    navigation->service_available = service && navigation->pit_count;
    return slicks_build_pit_routes(navigation, lower, 5);
}

static int is_category_one_surface(unsigned char type)
{
    /* b34e..b35f: category 1 preserves an existing surface 17 or 2.
     * These categories are observed in original track-construction probes;
     * mixed-category bridge images are handled pixel-by-pixel separately. */
    switch (type) {
    case 0: case 2: case 4: case 9: case 11: case 12: case 13: case 15:
    case 24: case 25: case 26: case 27: case 35: case 44: case 63: case 64:
        return 1;
    default:
        return 0;
    }
}

static int is_category_two_surface(unsigned char type)
{
    return type == 18 || type == 19 || type == 20 || type == 21 ||
           type == 23 || type == 31 || type == 32 || type == 33;
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
                } else if (type == 13 || type == 44 || type == 68) {
                    write_material = 0;
                } else if (type == 49 || type == 90) {
                    material = 2;
                } else if (type == 39 || type == 40 || type == 65) {
                    /* Original b225 writes on the BUMPS bridge pieces:
                     * 216 -> 0, 217 -> 1, 218 -> 31, 26/219 -> 2.
                     * The surface class is separate (19/2/0 below). */
                    material = (pixel == 26 || pixel == 219) ? 2 :
                               pixel == 217 ? 1 : pixel == 218 ? 31 : 0;
                } else if (type == 87) {
                    material = pixel == 217 ? 1 : 0;
                }
                if (write_material)
                    material_map[mult320[y] + x] = material;

                /* b1a5 stores the independent mode-one b089 value used by
                 * the wheel-effect dispatcher.  The value is a property of
                 * the DAT object class, not of its displayed palette index;
                 * bridge and foreground overlays deliberately preserve the
                 * surface beneath them. */
                {
                    unsigned char surface_pixel = pixel != 0;
                    unsigned long surface_at =
                        mult320[y] + x;
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
                         !is_category_two_surface(type)) ||
                        ((previous_surface & 0x40) && type != 9))
                        write_surface = 0;
                    switch (type) {
                    case 0: surface = 17; break;
                    case 18: case 19: case 20: case 21:
                    case 23: case 31: case 32: case 33:
                        surface = 2; break;
                    case 24: case 25: case 26: case 27: case 44:
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
                    if (is_category_one_surface(type) &&
                        ((previous_surface & 0x1f) == 2 ||
                         (previous_surface & 0x1f) == 17))
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

int slicks_track_object_enabled(unsigned char type, short fuel, short damage)
{
    /* b9b0..b9f9: test full signed option words for nonzero, not > 0. */
    return (type != 68 && type != 69) || fuel != 0 || damage != 0;
}

/* Paint in destination scanline order, one VGA bank at a time. Rotation,
 * clipping and address multiplication stay outside the pixel loop. */
static void draw_visual(unsigned char *logical,const struct TrackSprite *sprite,
    unsigned short ox,unsigned short oy,unsigned char rotation)
{
    rotation&=3;
    unsigned width=(rotation&1)?sprite->height:sprite->width;
    unsigned height=(rotation&1)?sprite->width:sprite->height;
    for(unsigned row=0;row<height;++row) {
        unsigned short y=(unsigned short)(oy+row);
        if(y>=190) continue;
        long base,step;
        if(rotation==1) { base=(long)(sprite->height-1)*sprite->width+row; step=-(long)sprite->width; }
        else if(rotation==2) { base=(long)(sprite->height-1-row)*sprite->width+sprite->width-1; step=-1; }
        else if(rotation==3) { base=sprite->width-1-row; step=sprite->width; }
        else { base=(long)row*sprite->width; step=1; }
        for(unsigned bank=0;bank<4;++bank) {
            unsigned col=(bank-(ox&3))&3;
            for(;col<width && (unsigned short)(ox+col)>=320;col+=4) {}
            if(col>=width) continue;
            unsigned short x=(unsigned short)(ox+col);
            unsigned char *dst=logical+((unsigned long)bank<<16)+(unsigned long)y*100+(x>>2);
            const unsigned char *src=sprite->pixels+base+(long)col*step;
            for(;col<width && x<320;col+=4,x+=4,++dst,src+=step*4)
                if(*src) *dst=*src;
        }
    }
}

static int build_track_scene(unsigned char *logical,
                             unsigned char *material_map,
                             unsigned char *surface_map,
                             const unsigned char *dat,
                             unsigned long dat_size,
                             const unsigned char *track,
                             unsigned long track_size,
                             unsigned char *sprite_arena,
                             unsigned long arena_size,
                             struct SlicksTrackNavigation *navigation,
                             short fuel, short damage,unsigned char visual_only)
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

    for (at = 0; !visual_only && at < 320UL * 190UL; ++at) {
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

    navigation->pit_count = 0;
    navigation->service_available = 0;
    navigation->actor_count = 0;
    for (object = 0; object < count; ++object) {
        unsigned short x = read_be16(track + at);
        unsigned short y = track[at + 2];
        unsigned char type = track[at + 3];
        unsigned char rotation = track[at + 4];
        at += 5;
        if (type >= SLICKS_SPRITE_COUNT)
            return -1;
        slicks_record_track_pit(navigation, x, y, type);
        if (!slicks_track_object_enabled(type, fuel, damage)) {
            navigation->service_available = 0;
            continue;
        }
        if (!slicks_record_track_actor(navigation, x, y, type)) {
            if(visual_only) draw_visual(logical,&sprites[type],x,y,rotation);
            else draw_sprite(logical, material_map, surface_map, &sprites[type],
                        x, y, rotation, type);
        }
    }

    /* Strip the construction priorities before gameplay samples b089. */
    {
        unsigned long surface_at;
        for (surface_at = 0; !visual_only && surface_at < 320UL * 190UL; ++surface_at)
            surface_map[surface_at] &= 0x1f;
    }

    /* ba19..ba89: independent lap checkpoint rectangles, not AI regions. */
    if (at + 2 > track_size)
        return -1;
    count = read_be16(track + at);
    at += 2;
    if (count > SLICKS_TRACK_CHECKPOINT_MAX ||
        at + (unsigned long)count * 6 > track_size)
        return -1;
    navigation->checkpoint_count = count;
    for (object = 0; object < count; ++object) {
        navigation->checkpoints[object].x[0] = read_be16(track + at);
        navigation->checkpoints[object].y[0] = track[at + 2];
        navigation->checkpoints[object].x[1] = read_be16(track + at + 3);
        navigation->checkpoints[object].y[1] = track[at + 5];
        at += 6;
    }

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
        slicks_expand_track_zone(&navigation->zones[object]);
    }

    /* bba5..bc04 stores alternate AI targets at indices 1..file count. */
    if (at + 2 > track_size)
        return -1;
    count = read_be16(track + at);
    at += 2;
    if (count >= SLICKS_TRACK_ALTERNATE_MAX ||
        at + (unsigned long)count * 4 + 7 > track_size)
        return -1;
    navigation->alternate_count = count + 1;
    navigation->alternate[0].x = navigation->alternate[0].y = 0;
    for (object = 1; object <= count; ++object) {
        navigation->alternate[object].x = (short)read_be16(track + at);
        navigation->alternate[object].y = track[at + 2];
        at += 4; /* Fourth byte is consumed and discarded by DOS. */
    }
    /* DOS x[100] at 6688+200 aliases y[0] at 6750. Preserve that
     * observable fallback value without writing outside native arrays. */
    if (count == 100)
        navigation->alternate[0].y = navigation->alternate[100].x;
    navigation->start_x = read_be16(track + at);
    navigation->start_y = read_be16(track + at + 2);
    navigation->start_heading = track[at + 4];
    navigation->start_style = track[at + 5];
    if (!visual_only && slicks_build_pit_routes(navigation, material_map, 5) < 0)
        return -1;
    return (int)object_count;
}

int slicks_build_track_scene_options(unsigned char *logical,
    unsigned char *lower,unsigned char *upper,const unsigned char *dat,unsigned long dat_size,
    const unsigned char *track,unsigned long track_size,unsigned char *arena,unsigned long arena_size,
    struct SlicksTrackNavigation *navigation,short fuel,short damage)
{ return build_track_scene(logical,lower,upper,dat,dat_size,track,track_size,arena,arena_size,navigation,fuel,damage,0); }

int slicks_build_track_visuals(unsigned char *logical,
    unsigned char *lower,unsigned char *upper,const unsigned char *dat,unsigned long dat_size,
    const unsigned char *track,unsigned long track_size,unsigned char *arena,unsigned long arena_size,
    struct SlicksTrackNavigation *navigation,short fuel,short damage)
{ return build_track_scene(logical,lower,upper,dat,dat_size,track,track_size,arena,arena_size,navigation,fuel,damage,1); }

/* Legacy DAT-only callers retain their historical visible-pit behavior.
 * Production uses the explicit setup-aware entry point and archive masks. */
int slicks_build_track_scene(unsigned char *logical,
                             unsigned char *material_map, unsigned char *surface_map,
                             const unsigned char *dat, unsigned long dat_size,
                             const unsigned char *track, unsigned long track_size,
                             unsigned char *arena, unsigned long arena_size,
                             struct SlicksTrackNavigation *navigation)
{
    return slicks_build_track_scene_options(logical,material_map,surface_map,
        dat,dat_size,track,track_size,arena,arena_size,navigation,1,0);
}

int slicks_track_material_sample(const unsigned char *lower,const unsigned char *upper,
    short x,short y,signed char layer)
{
    unsigned row=(unsigned short)y<256?mult320[(unsigned short)y]:(unsigned short)y*320U;
    unsigned short raw=(unsigned short)(row+(unsigned short)x);
    /* Original 1bd30 clears FA00 raw bytes and FE80 packed bytes before
     * b283's material compositor, whose producers crop at row 185. */
    if(raw>=64000 || !lower || (layer && !upper)) return -1;
    unsigned material=raw<60800?lower[raw]:0;
    if(layer) {
        unsigned short packed=(unsigned short)((row>>2)+x/4);
        if(packed>=65152) return -1;
        unsigned at=(unsigned)packed*4U+((unsigned short)x&3U);
        material=(raw<60800?(upper[raw]&7U):0U)+(at<60800?(upper[at]&24U):0U);
    }
    return (int)material;
}

int slicks_track_projectile_sample(const unsigned char *lower,const unsigned char *upper,
    short x,short y,signed char layer,short boundary_level)
{
    int material=slicks_track_material_sample(lower,upper,x,y,layer);
    if(material<0) return -1;
    return material==2 || (material>=22 && material<=26 &&
        (short)(material-22)<=(short)(boundary_level-1));
}

int slicks_track_car_sample(const unsigned char *lower, const unsigned char *upper,
                            short x, short y, unsigned char layer,
                            short special_state, unsigned char sampling_enabled,
                            short boundary_level)
{
    unsigned short offset;
    unsigned char material;
    if (special_state || !sampling_enabled)
        return 0;
    /* Upper-plane signed x/4 has different aliasing outside the visible
     * rectangle. Do not pretend the decoded map retains those DOS bytes. */
    if (layer && (x < 0 || x >= 320 || y < 0 || y >= 190))
        return -1;
    /* Use the shared row table for visible samples, but preserve the DOS
     * 16-bit address wrapping for out-of-range lower-layer coordinates. */
    offset = (unsigned short)((y >= 0 && y < 256 ? mult320[y] :
        (unsigned short)y * 320U) + (unsigned short)x);
    if (offset >= 320U * 190U || !lower || (layer && !upper))
        return -1;
    material = layer ? upper[offset] : lower[offset];
    return material == 2 || (material >= 22 && material <= 26 &&
        (short)(material - 22) <= (short)(boundary_level - 1));
}

static int pit_path_blocked(const unsigned char *map, short x, short y,
                             short end_x, short end_y, short level)
{
    short dx = (short)(x - end_x), dy = (short)(y - end_y);
    short width = dx < 0 ? (short)-dx : dx;
    short height = dy < 0 ? (short)-dy : dy;
    short extent = height > width ? height : width;
    long step;
    /* cb02 with car=-1, layer=0 and no actor test. CWD after IMUL
     * intentionally discards the product's high word before division. */
    if (width < 0 || height < 0) return -1;
    if (!extent) return 0;
    for (step = 0; step <= extent; ++step) {
        short px, py;
        int blocked;
        if (height > width) {
            px = (short)(x - (short)(step * dx) / height);
            py = (short)(y + (dy < 0 ? step : -step));
        } else {
            px = (short)(x + (dx < 0 ? step : -step));
            py = (short)(y - (short)(step * dy) / width);
        }
        /* Negative-car visibility always samples the lower map, with no
         * special-state/latch suppression. Share the c5a0 material rules. */
        blocked = slicks_track_car_sample(map, 0, px, py, 0, 0, 1, level);
        if (blocked < 0) return -1;
        if (step && blocked)
            return 1;
    }
    return 0;
}

int slicks_build_pit_routes(struct SlicksTrackNavigation *navigation,
                            const unsigned char *material_map, short boundary_level)
{
    unsigned short zone, pit;
    navigation->pit_route_count = 0;
    for (zone = 0; zone < navigation->zone_count; ++zone)
        for (pit = 0; pit < navigation->pit_count; ++pit) {
            int blocked;
            unsigned char index = navigation->pit_route_count;
            if (index >= SLICKS_TRACK_PIT_ROUTE_MAX) continue;
            blocked = pit_path_blocked(material_map,
                (short)navigation->zones[zone].x[0], (short)navigation->zones[zone].y[0],
                navigation->pits[pit].x, navigation->pits[pit].y, boundary_level);
            if (blocked < 0) return -1;
            if (!blocked) {
                navigation->pit_route_zone[index] = (unsigned char)zone;
                navigation->pit_route_destination[index] = (unsigned char)pit;
                ++navigation->pit_route_count;
            }
        }
    return 0;
}

void slicks_record_track_pit(struct SlicksTrackNavigation *navigation,
                             unsigned short x, unsigned short y,
                             unsigned char type)
{
    /* b8cb..b908: rotation does not change the service destination. */
    if (type != 68 && type != 69)
        return;
    navigation->service_available = 1;
    if (navigation->pit_count < SLICKS_TRACK_PIT_MAX) {
        struct SlicksTrackPoint *point = &navigation->pits[navigation->pit_count++];
        point->x = (short)x;
        point->y = (short)(y + 5);
    }
}

void slicks_expand_track_zone(struct SlicksTrackZone *zone)
{
    /* bb56..bb96: signed lower-bound checks, word-sized additions.
     * Target coordinates and checkpoint rectangles are not expanded. */
    if ((short)zone->x[0] > 1) zone->x[0] -= 2;
    zone->x[1] = (unsigned short)(zone->x[1] + 2);
    if ((short)zone->y[0] > 1) zone->y[0] -= 2;
    zone->y[1] = (unsigned short)(zone->y[1] + 2);
}
