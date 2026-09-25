#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>
#include "host_archive.h"
#include "../src/game/track_scene.c"
static int old_track_masks(unsigned char *lower, unsigned char *upper,
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
        for (unsigned short sy = 0; sy < sprite->height; ++sy)
        for (unsigned short sx = 0; sx < sprite->width; ++sx) {
            unsigned short x, y;
            if (rotation == 1) { x = ox + sprite->height - 1 - sy; y = oy + sx; }
            else if (rotation == 2) { x = ox + sprite->width - 1 - sx; y = oy + sprite->height - 1 - sy; }
            else if (rotation == 3) { x = ox + sy; y = oy + sprite->width - 1 - sx; }
            else { x = ox + sx; y = oy + sy; }
            /* b283 crops the material pass at y=185, independently of VGA. */
            if (x >= 320 || y >= 185) continue;
            unsigned long dest = mult320[y] + x;
            if (slicks_apply_material_mask(sprite->pixels[(unsigned long)sy * sprite->width + sx],
                                           bridge, lower + dest, upper + dest)) return -1;
        }
    }
    navigation->service_available = service && navigation->pit_count;
    return slicks_build_pit_routes(navigation, lower, 5);
}

static unsigned char masks[65536],track[8192],arena[65536],lower[60800],upper[60800],old_lower[60800],old_upper[60800];
int main(void)
{
    long bytes=host_archive_load("ref/SLICKS.000","masks",masks,sizeof masks);
    if(bytes<=0) return 2;
    glob_t paths; if(glob("ref/TRACKS/*.SS",0,0,&paths)) return 2;
    unsigned cases=0;
    for(unsigned f=0;f<paths.gl_pathc;++f) {
        FILE *in=fopen(paths.gl_pathv[f],"rb"); if(!in) return 2;
        size_t size=fread(track,1,sizeof track,in); fclose(in);
        for(unsigned service=0;service<2;++service) for(unsigned bridge=0;bridge<2;++bridge) {
            struct SlicksTrackNavigation a={0},b={0};
            int x=old_track_masks(old_lower,old_upper,masks,bytes,track,size,arena,sizeof arena,service,bridge,&a);
            int y=slicks_build_track_masks(lower,upper,masks,bytes,track,size,arena,sizeof arena,service,bridge,&b);
            if(x!=y || memcmp(lower,old_lower,sizeof lower) || memcmp(upper,old_upper,sizeof upper) || memcmp(&a,&b,sizeof a)) {
                fprintf(stderr,"Mask regression: %s service=%u bridge=%u\n",paths.gl_pathv[f],service,bridge); return 1;
            }
            ++cases;
        }
    }
    printf("Track mask fast path: %u full-map track/service/bridge comparisons pass\n",cases);
    globfree(&paths); return 0;
}
