#ifndef SLICKS_RACE_RUNTIME_H
#define SLICKS_RACE_RUNTIME_H

#include "track_scene.h"

#define SLICKS_RACE_CAR_COUNT 4
#define SLICKS_VEHICLE_COUNT 10
#define SLICKS_CAR_BASE_DIRECTIONS 4
#define SLICKS_CAR_PIXEL_MAX 100
#define SLICKS_CAR_PROPERTY_SIZE 34
#define SLICKS_FONT_GLYPH_MAX 195
#define SLICKS_FONT_PIXEL_MAX 1600
#define SLICKS_TRACK_MATERIAL_SIZE (320U * 190U)
#define SLICKS_SURFACE_GROUP_COUNT 5
#define SLICKS_START_LIGHT_COUNT 4
#define SLICKS_START_LIGHT_PIXEL_COUNT (23U * 38U)
#define SLICKS_DIRTY_ROW_MAX 16
#define SLICKS_TRAIL_SPRITE_COUNT 3
#define SLICKS_TRAIL_PIXEL_MAX 16
#define SLICKS_TRAIL_PARTICLE_MAX 32

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
    unsigned char model_class;
    unsigned char top_speed;
    unsigned char drive_response;
    unsigned char steering;
    unsigned char collision_weight;
    signed char surface[SLICKS_SURFACE_GROUP_COUNT][3];
    short balance_bias;
    unsigned char effect_profile;
    signed char engine_sound;
    unsigned char collision_sound;
    unsigned char surface_sound;
    unsigned char smoke_profile;
    unsigned char engine_volume;
    signed char auxiliary_accumulator;
    unsigned char ai_speed;
    unsigned char ai_aggression;
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

struct SlicksStartLight {
    unsigned char pixels[SLICKS_START_LIGHT_PIXEL_COUNT];
    unsigned char ready;
};

struct SlicksDirtyRows {
    unsigned short top;
    unsigned short bottom;
};

struct SlicksRaceCar {
    long x;
    long y;
    long speed_fixed;
    long velocity_x;
    long velocity_y;
    long ai_last_x;
    long ai_last_y;
    short heading;
    short speed;
    unsigned short elapsed_centiseconds;
    unsigned short current_lap_centiseconds;
    unsigned short last_lap_centiseconds;
    unsigned short best_lap_centiseconds;
    unsigned short acceleration_remainder;
    unsigned short lap;
    unsigned short finish_time_centiseconds;
    unsigned short ai_stuck_ticks;
    unsigned short ai_recovery_ticks;
    short steering_scale;
    short steering_penalty;
    short steering_property;
    unsigned char waypoint;
    unsigned char vehicle;
    unsigned char style;
    unsigned char steering_amount;
    unsigned char ai_recovery_right;
    unsigned char ai_probe_counter;
    unsigned char finished;
    unsigned char finish_position;
    unsigned char surface_group;
    unsigned char touching_solid;
    unsigned char old_x;
    unsigned char old_y;
    unsigned char old_width;
    unsigned char old_height;
    unsigned char saved_under[SLICKS_CAR_PIXEL_MAX];
    unsigned char saved_valid;
};

struct SlicksTrailParticle {
    long x;
    long y;
    short velocity_x;
    short velocity_y;
    short old_x;
    short old_y;
    unsigned char saved_under[SLICKS_TRAIL_PIXEL_MAX];
    unsigned char lifetime;
    unsigned char saved_valid;
};

struct SlicksRaceRuntime {
    struct SlicksTrackNavigation navigation;
    struct SlicksCarSprite
        sprites[SLICKS_VEHICLE_COUNT][SLICKS_CAR_BASE_DIRECTIONS];
    struct SlicksCarProperties properties[SLICKS_VEHICLE_COUNT];
    struct SlicksRaceFont font;
    struct SlicksStartLight start_lights[SLICKS_START_LIGHT_COUNT];
    struct SlicksCarSprite trail_sprites[SLICKS_TRAIL_SPRITE_COUNT];
    struct SlicksRaceCar cars[SLICKS_RACE_CAR_COUNT];
    struct SlicksTrailParticle trail_particles[SLICKS_TRAIL_PARTICLE_MAX];
    struct SlicksDirtyRows dirty_rows[SLICKS_DIRTY_ROW_MAX];
    unsigned char material_map[SLICKS_TRACK_MATERIAL_SIZE];
    unsigned char start_light_saved_under[SLICKS_START_LIGHT_PIXEL_COUNT];
    unsigned long frame_count;
    unsigned long skidmark_count;
    unsigned long collision_count;
    unsigned long track_collision_count;
    short countdown_ticks;
    unsigned char countdown_stage;
    unsigned char racing;
    unsigned char *chunky;
    unsigned char controls;
    unsigned char human_control;
    unsigned char start_light_visible;
    unsigned char start_light_stage_mask;
    unsigned char dirty_row_count;
    unsigned char trail_particle_count;
    unsigned char laps_to_run;
    unsigned char finished_count;
    unsigned char race_complete;
    unsigned char results_drawn;
    unsigned char active_collision_pairs;
    unsigned char started;
};

void slicks_race_initialize(struct SlicksRaceRuntime *race,
                            const struct SlicksTrackNavigation *navigation);
int slicks_race_add_car_sprite(struct SlicksRaceRuntime *race,
                               unsigned short vehicle,
                               unsigned short base_direction,
                               const unsigned char *resource,
                               unsigned long resource_size);
int slicks_race_add_car_properties(struct SlicksRaceRuntime *race,
                                   unsigned short vehicle,
                                   const unsigned char *resource,
                                   unsigned long resource_size);
int slicks_race_add_font(struct SlicksRaceRuntime *race,
                         const unsigned char *resource,
                         unsigned long resource_size);
int slicks_race_add_start_light(struct SlicksRaceRuntime *race,
                                unsigned short light,
                                const unsigned char *resource,
                                unsigned long resource_size);
int slicks_race_add_trail_sprite(struct SlicksRaceRuntime *race,
                                 unsigned short frame,
                                 const unsigned char *resource,
                                 unsigned long resource_size);
int slicks_race_start(struct SlicksRaceRuntime *race, unsigned char *logical,
                      unsigned char *chunky);
void slicks_race_set_controls(struct SlicksRaceRuntime *race,
                              unsigned char controls,
                              unsigned char human_control);
void slicks_race_set_vehicle(struct SlicksRaceRuntime *race,
                            unsigned short car, unsigned short vehicle);
void slicks_race_set_laps(struct SlicksRaceRuntime *race,
                         unsigned short laps);
void slicks_race_step(struct SlicksRaceRuntime *race, unsigned char *logical);
void slicks_race_clear_dirty_rows(struct SlicksRaceRuntime *race);

#endif
