#ifndef SLICKS_TRACK_SCENE_H
#define SLICKS_TRACK_SCENE_H

#define SLICKS_TRACK_ZONE_MAX 32

struct SlicksTrackZone {
    unsigned short x[3];
    unsigned char y[3];
    unsigned char speed;
};

struct SlicksTrackNavigation {
    struct SlicksTrackZone zones[SLICKS_TRACK_ZONE_MAX];
    unsigned short zone_count;
    unsigned short start_x;
    unsigned short start_y;
    unsigned char start_heading;
    unsigned char start_style;
};

/*
 * Build a race background and its independent five-bit material map from the
 * original SLICKS.DAT image stream and a version-2 .SS track. The visible
 * destination is the native four-bank VGA store used by the translated
 * graphics primitives.
 */
int slicks_build_track_scene(unsigned char *logical,
                             unsigned char *material_map,
                             unsigned char *surface_map,
                             const unsigned char *dat,
                             unsigned long dat_size,
                             const unsigned char *track,
                             unsigned long track_size,
                             unsigned char *sprite_arena,
                             unsigned long arena_size,
                             struct SlicksTrackNavigation *navigation);

#endif
