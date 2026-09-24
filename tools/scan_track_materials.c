#include <stdio.h>
#include <stdlib.h>

#include "../src/game/track_scene.h"
#include "host_archive.h"

#define LOGICAL_SIZE 0x40000UL
#define DAT_CAPACITY 65536UL
#define TRACK_CAPACITY 8192UL
#define ARENA_SIZE 65536UL
#define MATERIAL_SIZE (320UL * 190UL)

static long load_file(const char *path, unsigned char *data,
                      unsigned long capacity)
{
    FILE *file = fopen(path, "rb");
    long size;
    if (!file)
        return -1;
    size = (long)fread(data, 1, capacity, file);
    if (fgetc(file) != EOF)
        size = -1;
    fclose(file);
    return size;
}

int main(int argc, char **argv)
{
    unsigned char *logical;
    unsigned char *material;
    unsigned char *surface;
    unsigned char *dat;
    unsigned char *track;
    unsigned char *arena;
    unsigned char masks[65536];
    long masks_size = 0;
    const char *mask_archive = getenv("SLICKS_MASK_ARCHIVE");
    long dat_size;
    int argument;
    int errors = 0;
    if (argc < 3) {
        fprintf(stderr, "usage: %s SLICKS.DAT TRACK.SS...\n", argv[0]);
        return 2;
    }
    if (mask_archive) {
        masks_size = host_archive_load(mask_archive,"masks",masks,sizeof masks);
        if (masks_size <= 0) return 2;
    }
    logical = (unsigned char *)malloc(LOGICAL_SIZE);
    material = (unsigned char *)malloc(MATERIAL_SIZE);
    surface = (unsigned char *)malloc(MATERIAL_SIZE);
    dat = (unsigned char *)malloc(DAT_CAPACITY);
    track = (unsigned char *)malloc(TRACK_CAPACITY);
    arena = (unsigned char *)malloc(ARENA_SIZE);
    if (!logical || !material || !surface || !dat || !track || !arena)
        return 2;
    dat_size = load_file(argv[1], dat, DAT_CAPACITY);
    if (dat_size <= 0)
        return 2;
    for (argument = 2; argument < argc; ++argument) {
        struct SlicksTrackNavigation navigation;
        unsigned long count[32] = {0};
        unsigned long upper_count[32] = {0};
        unsigned long pixel;
        long track_size = load_file(argv[argument], track, TRACK_CAPACITY);
        int objects;
        if (track_size <= 0) {
            printf("ERROR %s read\n", argv[argument]);
            ++errors;
            continue;
        }
        objects = slicks_build_track_scene_options(
            logical, material, surface, dat, (unsigned long)dat_size,
            track, (unsigned long)track_size, arena, ARENA_SIZE,
            &navigation, getenv("SLICKS_SERVICE") != NULL, 0);
        if (objects <= 0) {
            printf("ERROR %s decode\n", argv[argument]);
            ++errors;
            continue;
        }
        if (masks_size && slicks_build_track_masks(material,surface,masks,masks_size,
                track,track_size,arena,ARENA_SIZE,getenv("SLICKS_SERVICE") != NULL,
                track[6+0x165+4],&navigation)<0) {
            printf("ERROR %s masks\n",argv[argument]);
            ++errors;
            continue;
        }
        for (pixel = 0; pixel < MATERIAL_SIZE; ++pixel) {
            ++count[material[pixel] & 31];
            ++upper_count[surface[pixel] & 31];
        }
        printf("%s objects=%d zones=%u actors=%u animated=%lu,%lu,%lu,%lu,%lu jumps=%lu,%lu upper_jumps=%lu,%lu\n",
               argv[argument], objects, navigation.zone_count, navigation.actor_count,
               count[22], count[23], count[24], count[25], count[26],
               count[13], count[14], upper_count[13], upper_count[14]);
        if (getenv("SLICKS_INSPECT_PITS")) {
            unsigned pit;
            printf("  pit pixels lower=%lu upper=%lu routes=%u\n",
                   count[30], upper_count[30], navigation.pit_route_count);
            for (pit = 0; pit < navigation.pit_count; ++pit) {
                int x, y, cx = navigation.pits[pit].x + 3;
                int cy = navigation.pits[pit].y + 3;
                printf("  pit %u target=%u,%u centre=%d,%d (lower/upper)\n",
                       pit, navigation.pits[pit].x, navigation.pits[pit].y, cx, cy);
                for (y = cy - 4; y <= cy + 4; ++y) {
                    printf("    y=%3d", y);
                    for (x = cx - 4; x <= cx + 4; ++x) {
                        if (x < 0 || x >= 320 || y < 0 || y >= 190)
                            printf(" --/--");
                        else
                            printf(" %02u/%02u", material[y * 320 + x],
                                   surface[y * 320 + x]);
                    }
                    putchar('\n');
                }
            }
        }
    }
    free(arena);
    free(track);
    free(dat);
    free(surface);
    free(material);
    free(logical);
    return errors ? 1 : 0;
}
