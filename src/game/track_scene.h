#ifndef SLICKS_TRACK_SCENE_H
#define SLICKS_TRACK_SCENE_H

/* The supplied version-2 tracks use as many as 46 navigation regions.
 * Keep a power-of-two ceiling above the observed files while retaining the
 * on-disk word count validation in the decoder. */
#define SLICKS_TRACK_ZONE_MAX 64
#define SLICKS_TRACK_CHECKPOINT_MAX 100
#define SLICKS_TRACK_ALTERNATE_MAX 101 /* CURVE6 has 100 records plus slot 0. */
#define SLICKS_TRACK_PIT_MAX 10
#define SLICKS_TRACK_PIT_ROUTE_MAX 30
#define SLICKS_TRACK_ACTOR_MAX 100 /* DOS slot 99 is overflow scratch. */

struct SlicksChunkyUi;
/* Original records-panel preview, directly from DAT sprites and SS objects.
 * Return 1 for the old-format notice, -1 for rejected input, 0 drawn. */
int slicks_build_track_preview(struct SlicksChunkyUi *ui,
    const unsigned char *dat,unsigned long dat_size,
    const unsigned char *track,unsigned long track_size,
    unsigned char *arena,unsigned long arena_size,short x,short y);

struct SlicksTrackActor {
    short x, y; /* Signed, wrapping Q4 coordinates. */
    short velocity_x, velocity_y;
    unsigned char kind, layer;
};

struct SlicksTrackPoint { short x, y; };

struct SlicksTrackCheckpoint {
    unsigned short x[2];
    unsigned char y[2];
};

struct SlicksTrackZone {
    unsigned short x[3];
    unsigned short y[3]; /* DOS widens file bytes before expanding bounds. */
    unsigned char speed;
};

void slicks_expand_track_zone(struct SlicksTrackZone *zone);

struct SlicksTrackNavigation {
    struct SlicksTrackActor actors[SLICKS_TRACK_ACTOR_MAX];
    unsigned short actor_count; /* Original count clamps to 99, not 100. */
    struct SlicksTrackCheckpoint checkpoints[SLICKS_TRACK_CHECKPOINT_MAX];
    unsigned short checkpoint_count;
    struct SlicksTrackZone zones[SLICKS_TRACK_ZONE_MAX];
    unsigned short zone_count;
    struct SlicksTrackPoint alternate[SLICKS_TRACK_ALTERNATE_MAX];
    unsigned short alternate_count; /* Includes reserved fallback slot zero. */
    struct SlicksTrackPoint pits[SLICKS_TRACK_PIT_MAX];
    unsigned char pit_count;
    unsigned char service_available; /* DOS DS:36a6, set by objects 68/69. */
    unsigned char pit_route_count;
    unsigned char pit_route_zone[SLICKS_TRACK_PIT_ROUTE_MAX];
    unsigned char pit_route_destination[SLICKS_TRACK_PIT_ROUTE_MAX];
    unsigned short start_x;
    unsigned short start_y;
    unsigned char start_heading;
    unsigned char start_style;
};

int slicks_record_track_actor(struct SlicksTrackNavigation *navigation,
                              unsigned short x, unsigned short y,
                              unsigned char type);

void slicks_record_track_pit(struct SlicksTrackNavigation *navigation,
                             unsigned short x, unsigned short y,
                             unsigned char type);
int slicks_build_pit_routes(struct SlicksTrackNavigation *navigation,
                            const unsigned char *material_map, short boundary_level);

/* Original c5a0 for an actual car. Returns -1 outside retained map coverage
 * (upper layer requires visible coordinates); suppression accesses no map. */
int slicks_track_car_sample(const unsigned char *lower, const unsigned char *upper,
                            short x, short y, unsigned char layer,
                            short special_state, unsigned char sampling_enabled,
                            short boundary_level);

/* Original b225 pixel compositor and /masks archive-resource pass. */
int slicks_apply_material_mask(unsigned char pixel, unsigned char bridge,
                               unsigned char *lower, unsigned char *upper);
int slicks_build_track_masks(unsigned char *lower, unsigned char *upper,
                             const unsigned char *masks, unsigned long masks_size,
                             const unsigned char *track, unsigned long track_size,
                             unsigned char *arena, unsigned long arena_size,
                             unsigned char service, unsigned char bridge,
                             struct SlicksTrackNavigation *navigation);

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

/* Setup-aware loader: objects 68/69 are visible only with fuel or damage. */
int slicks_track_object_enabled(unsigned char type, short fuel, short damage);
int slicks_build_track_scene_options(unsigned char *logical,
                             unsigned char *material_map,
                             unsigned char *surface_map,
                             const unsigned char *dat, unsigned long dat_size,
                             const unsigned char *track, unsigned long track_size,
                             unsigned char *sprite_arena, unsigned long arena_size,
                             struct SlicksTrackNavigation *navigation,
                             short fuel, short damage);

#endif
