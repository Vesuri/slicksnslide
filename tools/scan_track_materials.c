#include <stdio.h>
#include <stdlib.h>

#include "../src/game/track_scene.h"

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
    long dat_size;
    int argument;
    int errors = 0;
    if (argc < 3) {
        fprintf(stderr, "usage: %s SLICKS.DAT TRACK.SS...\n", argv[0]);
        return 2;
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
        unsigned long pixel;
        long track_size = load_file(argv[argument], track, TRACK_CAPACITY);
        int objects;
        if (track_size <= 0) {
            printf("ERROR %s read\n", argv[argument]);
            ++errors;
            continue;
        }
        objects = slicks_build_track_scene(
            logical, material, surface, dat, (unsigned long)dat_size,
            track, (unsigned long)track_size, arena, ARENA_SIZE,
            &navigation);
        if (objects <= 0) {
            printf("ERROR %s decode\n", argv[argument]);
            ++errors;
            continue;
        }
        for (pixel = 0; pixel < MATERIAL_SIZE; ++pixel)
            ++count[material[pixel] & 31];
        printf("%s objects=%d zones=%u animated=%lu,%lu,%lu,%lu,%lu\n",
               argv[argument], objects, navigation.zone_count,
               count[22], count[23], count[24], count[25], count[26]);
    }
    free(arena);
    free(track);
    free(dat);
    free(surface);
    free(material);
    free(logical);
    return errors ? 1 : 0;
}
