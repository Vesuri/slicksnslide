#ifndef SLICKS_RACE_RUNTIME_H
#define SLICKS_RACE_RUNTIME_H

#include "track_scene.h"

#define SLICKS_RACE_CAR_COUNT 4
#define SLICKS_CAR_BASE_DIRECTIONS 4
#define SLICKS_CAR_PIXEL_MAX 100
#define SLICKS_CAR_PROPERTY_SIZE 34
#define SLICKS_FONT_GLYPH_MAX 195
#define SLICKS_FONT_PIXEL_MAX 1600

#define SLICKS_CONTROL_ACCELERATE 1
#define SLICKS_CONTROL_BRAKE 2
#define SLICKS_CONTROL_LEFT 4
#define SLICKS_CONTROL_RIGHT 8

struct SlicksCarSprite {
    unsigned char pixels[SLICKS_CAR_PIXEL_MAX];
    unsigned char width;
    unsigned char height;
    unsigned char ready;
};

struct SlicksCarProperties {
    unsigned char raw[SLICKS_CAR_PROPERTY_SIZE];
    unsigned char body_radius_x;
    unsigned char body_radius_y;
    unsigned char collision_radius;
    unsigned char acceleration;
    unsigned char steering;
    unsigned char collision_weight;
    unsigned char ready;
};

struct SlicksRaceFont {
    unsigned char codes[SLICKS_FONT_GLYPH_MAX];
    unsigned char widths[SLICKS_FONT_GLYPH_MAX];
    unsigned char pixels[SLICKS_FONT_PIXEL_MAX];
    unsigned short pixel_count;
    unsigned char glyph_count;
    unsigned char height;
    unsigned char ready;
};

struct SlicksRaceCar {
    long x;
    long y;
    short heading;
    short speed;
    unsigned short elapsed_centiseconds;
    unsigned short current_lap_centiseconds;
    unsigned short last_lap_centiseconds;
    unsigned short best_lap_centiseconds;
    unsigned short acceleration_remainder;
    unsigned short lap;
    unsigned char waypoint;
    unsigned char style;
    unsigned char old_x;
    unsigned char old_y;
    unsigned char old_width;
    unsigned char old_height;
    unsigned char saved_under[SLICKS_CAR_PIXEL_MAX];
    unsigned char saved_valid;
};

struct SlicksRaceRuntime {
    struct SlicksTrackNavigation navigation;
    struct SlicksCarSprite
        sprites[SLICKS_RACE_CAR_COUNT][SLICKS_CAR_BASE_DIRECTIONS];
    struct SlicksCarProperties properties[SLICKS_RACE_CAR_COUNT];
    struct SlicksRaceFont font;
    struct SlicksRaceCar cars[SLICKS_RACE_CAR_COUNT];
    unsigned long frame_count;
    unsigned long skidmark_count;
    unsigned long collision_count;
    short countdown_ticks;
    unsigned char countdown_stage;
    unsigned char racing;
    unsigned char controls;
    unsigned char human_control;
    unsigned char started;
};

void slicks_race_initialize(struct SlicksRaceRuntime *race,
                            const struct SlicksTrackNavigation *navigation);
int slicks_race_add_car_sprite(struct SlicksRaceRuntime *race,
                               unsigned short car,
                               unsigned short base_direction,
                               const unsigned char *resource,
                               unsigned long resource_size);
int slicks_race_add_car_properties(struct SlicksRaceRuntime *race,
                                   unsigned short car,
                                   const unsigned char *resource,
                                   unsigned long resource_size);
int slicks_race_add_font(struct SlicksRaceRuntime *race,
                         const unsigned char *resource,
                         unsigned long resource_size);
int slicks_race_start(struct SlicksRaceRuntime *race, unsigned char *logical);
void slicks_race_set_controls(struct SlicksRaceRuntime *race,
                              unsigned char controls,
                              unsigned char human_control);
void slicks_race_step(struct SlicksRaceRuntime *race, unsigned char *logical);

#endif
