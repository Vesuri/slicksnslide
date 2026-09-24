#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/game/track_scene.h"
#include "host_archive.h"

static unsigned char logical[0x40000], material[60800], surface[60800];
static unsigned char dat[65536], track[8192], arena[65536], captured[64000];
static unsigned char captured_surface[64000];

static size_t read_file(const char *path, unsigned char *bytes, size_t capacity)
{
    FILE *file = fopen(path, "rb");
    if (!file) { perror(path); exit(2); }
    size_t length = fread(bytes, 1, capacity, file);
    if (ferror(file) || fgetc(file) != EOF) {
        fprintf(stderr, "Cannot read bounded fixture: %s\n", path);
        exit(2);
    }
    fclose(file);
    return length;
}

int main(int argc, char **argv)
{
    struct SlicksTrackNavigation navigation;
    if (argc < 4 || argc > 7) {
        fprintf(stderr, "usage: %s SLICKS.DAT TRACK.SS DOS-raw-mask.bin|- [DOS-surface.bin [SLICKS.000 [service]]]\n", argv[0]);
        return 2;
    }
    size_t dat_bytes = read_file(argv[1], dat, sizeof dat);
    size_t track_bytes = read_file(argv[2], track, sizeof track);
    int verify_raw = strcmp(argv[3], "-") != 0;
    if (!verify_raw && argc < 5) return 2;
    if (verify_raw && read_file(argv[3], captured, sizeof captured) != sizeof captured)
        return 2;
    if (argc >= 5 && read_file(argv[4], captured_surface, sizeof captured_surface) != sizeof captured_surface)
        return 2;
    if (slicks_build_track_scene(logical, material, surface, dat, dat_bytes,
            track, track_bytes, arena, sizeof arena, &navigation) <= 0)
        return 2;
    if (argc >= 6) {
        long masks_size=host_archive_load(argv[5],"masks",dat,sizeof dat);
        if(masks_size<=0 || slicks_build_track_masks(material,surface,dat,masks_size,
                track,track_bytes,arena,sizeof arena,argc==7 && atoi(argv[6]),
                track[6+0x165+4],&navigation)<0) return 2;
    }
    unsigned mismatches = 0;
    for (unsigned at = 0; at < sizeof material; ++at) {
        /* 1000:b1ab..b1cd packs the decoded mode-zero class in bits 3..7
         * and the low three bits of mode-one surface in bits 0..2.
         * 3000:385c compares this RAW byte, not either decoded class. */
        unsigned char raw = (material[at] << 3) | (surface[at] & 7);
        if (verify_raw && raw != captured[at]) {
            if (!mismatches)
                fprintf(stderr, "First mask mismatch x=%u y=%u native=%u DOS=%u\n",
                        at % 320, at / 320, raw, captured[at]);
            ++mismatches;
        }
    }
    if (verify_raw)
        printf("Native actor mask: %u/60800 mismatches against DOS raw buffer.\n", mismatches);
    else
        puts("Raw actor mask not checked (no fixture supplied).");
    if(argc>=5) {
        unsigned surface_mismatches=0;
        for(unsigned at=0;at<sizeof surface;++at)
            if(surface[at]!=captured_surface[at]) {
                if(!surface_mismatches)
                    fprintf(stderr,"First surface mismatch x=%u y=%u native=%u DOS=%u\n",
                            at%320,at/320,surface[at],captured_surface[at]);
                ++surface_mismatches;
            }
        printf("Native full five-bit surface: %u/60800 mismatches against DOS.\n",surface_mismatches);
        mismatches+=surface_mismatches;
    }
    return mismatches ? 1 : 0;
}
