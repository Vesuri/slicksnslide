#ifndef SLICKS_RACE_RUNTIME_H
#define SLICKS_RACE_RUNTIME_H

#include "track_scene.h"
#include "weapon_runtime.h"

#define SLICKS_RACE_CAR_COUNT 4
#define SLICKS_VEHICLE_COUNT 10
#define SLICKS_CAR_BASE_DIRECTIONS 4
#define SLICKS_CAR_PIXEL_MAX 100
#define SLICKS_CAR_PROPERTY_SIZE 34
#define SLICKS_FONT_GLYPH_MAX 195
#define SLICKS_FONT_PIXEL_MAX 4096
#define SLICKS_RACE_FONT_NAME "kirj.@f"
#define SLICKS_TRACK_MATERIAL_SIZE (320U * 190U)
#define SLICKS_SURFACE_GROUP_COUNT 5
#define SLICKS_START_LIGHT_COUNT 4
#define SLICKS_START_LIGHT_PIXEL_COUNT (23U * 38U)
#define SLICKS_DIRTY_ROW_MAX 16
#define SLICKS_TRAIL_PARTICLE_MAX 256
#define SLICKS_DIRTY_PIXEL_MAX (SLICKS_TRAIL_PARTICLE_MAX * 2)
#define SLICKS_TRAIL_PRIORITY_COUNT 4
#define SLICKS_SOUND_EVENT_MAX 8
#define SLICKS_SOUND_SAMPLE_COUNT 26

#define SLICKS_CONTROL_ACCELERATE 1
#define SLICKS_CONTROL_BRAKE 2
#define SLICKS_CONTROL_LEFT 4
#define SLICKS_CONTROL_RIGHT 8

struct SlicksCarSprite {
    unsigned char pixels[SLICKS_CAR_PIXEL_MAX];
    signed char wheel_x[4][2], wheel_y[4][2];
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
    unsigned char property_4;
    unsigned char property_5;
    unsigned char property_6;
    unsigned char collision_weight;
    short drive_bias;
    signed char surface[SLICKS_SURFACE_GROUP_COUNT][3];
    unsigned char effect_profile;
    signed char engine_sound;
    unsigned char collision_sound;
    unsigned char surface_sound;
    unsigned char smoke_profile;
    unsigned char property_29;
    signed char auxiliary_accumulator;
    unsigned char impact_resistance;
    unsigned char property_33;
    unsigned char ready;
};

struct SlicksRaceFont {
    unsigned char codes[SLICKS_FONT_GLYPH_MAX];
    unsigned char widths[SLICKS_FONT_GLYPH_MAX];
    unsigned short offsets[SLICKS_FONT_GLYPH_MAX];
    unsigned char lookup[256];
    unsigned char pixels[SLICKS_FONT_PIXEL_MAX];
    unsigned char runtime[6144]; /* Original kirj padded font/palette layout. */
    unsigned short pixel_count;
    unsigned char glyph_count;
    unsigned char height;
    unsigned char advance_extra;
    unsigned char ready;
};

struct SlicksHudRun {
    short x, y;
    unsigned short width;
    char text[9];
};

struct SlicksHudIcon {
    unsigned char pixels[16*8];
    unsigned char width, height, ready;
};

struct SlicksStartLight {
    unsigned char pixels[SLICKS_START_LIGHT_PIXEL_COUNT];
    unsigned char ready;
};

struct SlicksDirtyRows {
    unsigned short left;
    unsigned short top;
    unsigned short right;
    unsigned short bottom;
};

struct SlicksDirtyPixel {
    unsigned short x;
    unsigned char y;
    unsigned char unused;
};

struct SlicksSoundEvent {
    unsigned char sample_block;
    unsigned char flags;
    unsigned char priority;
};

struct SlicksRaceCar {
    long x;
    long y;
    long speed_fixed;
    long measured_speed; /* Previous DS:684e = (abs(vx)+abs(vy))/2. */
    unsigned char position_scale; /* Selected profile's unsigned DS:3f42. */
    long velocity_x;
    long velocity_y;
    long collision_impact;
    long pending_damage_impact;
    long ai_last_x;
    long ai_last_y;
    short heading;
    short speed;
    unsigned short elapsed_centiseconds;
    unsigned int elapsed_time_units; /* DOS duration units: two per tick. */
    unsigned int current_lap_time_units;
    unsigned int last_lap_time_units;
    unsigned int best_lap_time_units;
    unsigned short current_lap_centiseconds;
    unsigned short last_lap_centiseconds;
    unsigned short best_lap_centiseconds;
    unsigned short acceleration_remainder;
    unsigned short lap;
    unsigned short checkpoint; /* DS:305c, independent of AI DS:6902. */
    unsigned short finish_time_centiseconds;
    short ai_stuck_ticks; /* DS:68fa signed stationary watchdog. */
    short ai_recovery_ticks; /* DS:691a state timer. */
    short ai_turn_ticks; /* DS:6922 recovery direction timer. */
    short ai_target_x, ai_target_y; /* DS:690a/6912 alternate destination. */
    unsigned short ai_contact_ticks; /* DS:53c6 time since boundary contact. */
    unsigned short ai_contact_threshold; /* DS:2fa4. */
    unsigned char ai_route_seen; /* DS:6932. */
    signed char ai_state; /* DS:692a: route 0, alternate target 1, escape 2. */
    signed char ai_service_state; /* DS:6936. */
    unsigned char ai_control_latch; /* Persistent DS:5344..5347 inputs. */
    short steering_scale;
    short damage[4]; /* DOS DS:304f..3055, including steering penalty. */
    unsigned int fuel; /* Signed DOS long DS:305f, stored as raw 32-bit bits. */
    unsigned int fuel_capacity; /* DS:3063. */
    short fuel_upgrade; /* ES:6a7e, copied from setup inventory slot 2. */
    unsigned char service_flags; /* DS:305e; bit zero means fuel exhausted. */
    signed char damage_turn_sign;
    short steering_property;
    short drive_bias;
    short special_drive_state;
    short special_drive_target; /* DOS DS:305a. */
    short drive_setup[13];
    short drive_coefficients[7];
    unsigned short maximum_speed;
    unsigned char waypoint;
    unsigned char vehicle;
    unsigned char style;
    short steering_amount; /* Original per-driver BP-48 word, may exceed 255. */
    unsigned char forward_drive_latch; /* DOS DS:5374. */
    signed char ai_recovery_right; /* DS:692e; positive selects right. */
    unsigned char finished;
    unsigned char finish_position;
    unsigned char touching_solid;
    unsigned char touching_car;
    unsigned char collision_partner; /* Original per-driver BP-4c scratch byte. */
    unsigned char actor_layer;
    unsigned char selected_surface; /* DS:537c, before special suppression. */
    unsigned char effective_surface; /* DOS DS:5378, after layer selection. */
    unsigned char oil_active; /* DS:4c70, reset by selected (not effective) surface. */
    signed char oil_turn_sign; /* DS:4c78, chosen once on oil entry. */
    unsigned char actor_contact;
    unsigned char previous_actor_contact; /* DS:5370, not touching_solid. */
    unsigned char collision_sampling; /* DS:4daa, rearmed on clear ground. */
    short collision_safe_x; /* DS:53ce. */
    unsigned char collision_safe_y; /* DS:6818, deliberately byte-sized. */
    /* The playfield is 320 pixels wide.  This must not be narrowed: the
     * BASIC grid starts at x=263, and an 8-bit saved coordinate restores its
     * background at x=7, leaving a car ghost at both edges. */
    unsigned short old_x;
    unsigned char old_y;
    unsigned char old_width;
    unsigned char old_height;
    unsigned char saved_under[SLICKS_CAR_PIXEL_MAX];
    unsigned char saved_valid;
};

/* Half-open original d9b6 status rectangles; colour is a semantic slot:
 * 0 background, 1 fuel, 2 damage. Palette resolution is separate. */
struct SlicksStatusRect {
    short left, top, right, bottom;
    unsigned char colour;
};

struct SlicksStatusClock {
    unsigned long remainder;
    unsigned short ticks;
};

/* Reserved priority-zero actor, distinct from the main car sprite. */
struct SlicksCarShadow {
    short x, y;
    unsigned short old_x;
    unsigned char old_y, old_width, old_height;
    unsigned char saved_under[256];
    unsigned char saved_valid, lifetime;
    signed char state;
};

struct SlicksTrailParticle {
    long x;
    long y;
    short velocity_x;
    short velocity_y;
    short old_x;
    short old_y;
    unsigned char saved_under;
    unsigned char lifetime;
    unsigned char colour;
    unsigned char priority;
    /* Bit 0: saved-under is live. Bit 1: the old display pixel existed at
     * the start of this update and may need direct planar synchronization. */
    unsigned char saved_valid;
    /* A 24-byte stride replaces the awkward 22-byte index multiply with
     * (index * 3) << 3 in the hot restore/advance/draw loops. */
    unsigned char permanent;
    unsigned char occlusion_limit;
    signed char state; /* DOS point states 1/5, retained retirement -2/-6. */
};

struct SlicksRaceRuntime {
    struct SlicksTrackNavigation navigation;
    struct SlicksCarSprite
        sprites[SLICKS_VEHICLE_COUNT][SLICKS_CAR_BASE_DIRECTIONS];
    struct SlicksCarProperties properties[SLICKS_VEHICLE_COUNT];
    struct SlicksRaceFont font;
    unsigned char hud_background[320*16];
    unsigned char hud_background_ready;
    unsigned char hud_colours[3]; /* Racing, finished, track information. */
    char hud_track_name[9];
    unsigned short hud_record_time;
    unsigned char hud_track_ready;
    struct SlicksHudIcon hud_weapon_icons[8];
    short weapon_inventory[4][13];
    signed char weapon_capacity[13], selected_weapon[4];
    unsigned char weapons_enabled, weapon_hud_colour;
    struct SlicksWeaponRuntime weapons;
    unsigned char setup_inventory_ready;
    unsigned char hud_status_options[4];
    signed char hud_weapon_selection[4];
    struct SlicksStartLight start_lights[SLICKS_START_LIGHT_COUNT];
    struct SlicksRaceCar cars[SLICKS_RACE_CAR_COUNT];
    struct SlicksCarShadow shadows[SLICKS_RACE_CAR_COUNT];
    unsigned char actor_page; /* DOS expiry phase before display-page toggle. */
    struct SlicksTrailParticle trail_particles[SLICKS_TRAIL_PARTICLE_MAX];
    unsigned char trail_priority_indices
        [SLICKS_TRAIL_PRIORITY_COUNT][SLICKS_TRAIL_PARTICLE_MAX];
    unsigned short trail_priority_counts[SLICKS_TRAIL_PRIORITY_COUNT];
    struct SlicksDirtyRows dirty_rows[SLICKS_DIRTY_ROW_MAX];
    struct SlicksDirtyPixel dirty_pixels[SLICKS_DIRTY_PIXEL_MAX];
    struct SlicksSoundEvent sound_events[SLICKS_SOUND_EVENT_MAX];
    unsigned char material_map[SLICKS_TRACK_MATERIAL_SIZE];
    unsigned char surface_map[SLICKS_TRACK_MATERIAL_SIZE];
    unsigned char start_light_saved_under[SLICKS_START_LIGHT_PIXEL_COUNT];
    unsigned long frame_count;
    unsigned long physics_tick_phase;
    unsigned long physics_tick_period;
    unsigned char physics_timer_disabled;
    unsigned long skidmark_count;
    unsigned long collision_count;
    unsigned char car_collisions_disabled; /* Inverse of DS:3028's word-zero gate. */
    unsigned long collision_impact;
    unsigned long track_collision_count;
    unsigned long sound_event_totals[SLICKS_SOUND_SAMPLE_COUNT];
    unsigned long random_state;
    short damage_scale; /* DS:3026; captured setup is zero (disabled). */
    short fuel_option; /* DS:3024; zero disables fuel restrictions. */
    short pit_repair_ticks; /* Race-local BP-64, shared by all four cars. */
    short boundary_level; /* DS:4c6c, initial animated-boundary level 5. */
    unsigned char collision_error; /* Unsupported retained-map sample. */
    unsigned char damage_enabled; /* DS:36a6, independent master gate. */
    short countdown_ticks;
    unsigned char countdown_stage;
    unsigned char racing;
    unsigned char *chunky;
    unsigned char controls;
    unsigned char human_control;
    unsigned char participation_ready;
    signed char participation[4]; /* Original DS:4bc6: 0 off, negative human, positive AI. */
    unsigned char driver_controls[4];
    void (*poll_driver_devices)(struct SlicksRaceRuntime *,unsigned short);
    unsigned char start_light_visible;
    unsigned char start_light_stage_mask;
    unsigned char dirty_row_count;
    unsigned short dirty_pixel_count;
    unsigned char sound_event_count;
    unsigned short trail_particle_count;
    unsigned short laps_to_run; /* Original word DS:4c18; Arcade uses 9999. */
    unsigned char finished_count;
    unsigned char race_complete;
    unsigned char results_drawn; /* Legacy name: final race-frame handoff ready, not UI proof. */
    unsigned char chunky_authoritative;
    unsigned long profile_frame;
    void (*profile_marker)(unsigned char phase);
    struct SlicksHudRun hud_runs[SLICKS_RACE_CAR_COUNT][3];
    unsigned char hud_run_count[SLICKS_RACE_CAR_COUNT];
    unsigned char hud_valid[SLICKS_RACE_CAR_COUNT];
    unsigned char status_colours[3];
    unsigned char collision_colour; /* Nearest palette match to 55,55,10. */
    unsigned char started;
    short race_mode, arcade_seconds;
    unsigned int game_clock_ticks; /* Original DS:74bc, wrapping 32-bit. */
    unsigned int finish_deadline; /* Original DS:6862. */
    short arcade_bar_right,arcade_bar_top;
    unsigned char arcade_colours[3],arcade_hud_valid,arcade_hud_count,arcade_hud_text;
    signed char finish_ranks[4]; /* Original signed bytes DS:4bce. */
    unsigned char finish_ranks_ready;
    void (*finish_reward)(struct SlicksRaceRuntime *,unsigned,signed char);
    void (*track_reward)(struct SlicksRaceRuntime *);
    unsigned char track_rewarded;
};

void slicks_race_initialize(struct SlicksRaceRuntime *race,
                            const struct SlicksTrackNavigation *navigation);
/* Once on race-loop return, whether deadline completion or user exit. */
void slicks_race_award_track(struct SlicksRaceRuntime *race);
/* Configure before slicks_race_start; mode 5 uses the original Arcade clock. */
void slicks_race_set_mode(struct SlicksRaceRuntime *race, short mode,
                          short arcade_seconds);
/* Original custom-race setup conversion; call before track loading/start. */
void slicks_race_set_service_options(struct SlicksRaceRuntime *race,
                                     short fuel_setting, short damage_setting);
int slicks_race_set_participation(struct SlicksRaceRuntime *race,
                                 const signed char participation[4]);
int slicks_race_set_inventory(struct SlicksRaceRuntime *race,
                              const short inventory[4][13]);
int slicks_race_add_car_sprite(struct SlicksRaceRuntime *race,
                               unsigned short vehicle,
                               unsigned short base_direction,
                               const unsigned char *resource,
                               unsigned long resource_size);
int slicks_race_add_car_properties(struct SlicksRaceRuntime *race,
                                   unsigned short vehicle,
                                   const unsigned char *resource,
                                   unsigned long resource_size);
/* Returns -1 for an original signed fuel-division fault. No invented bar. */
int slicks_race_status_rects(const struct SlicksRaceRuntime *race,
                             unsigned short car, unsigned short timer,
                             struct SlicksStatusRect rectangles[3]);
void slicks_race_set_status_palette(struct SlicksRaceRuntime *race,
                                    const unsigned char palette[768]);
int slicks_race_set_track_info(struct SlicksRaceRuntime *race,
    const char *path, const unsigned char *data, unsigned long size);
void slicks_status_clock_advance(struct SlicksStatusClock *clock,
                                 unsigned long pal_vblanks);
int slicks_race_draw_status(struct SlicksRaceRuntime *race,
                            unsigned char *logical, unsigned short timer);

int slicks_race_add_font(struct SlicksRaceRuntime *race,
                         const unsigned char *resource,
                         unsigned long resource_size);
int slicks_race_add_hud_background(struct SlicksRaceRuntime *race,
                                  const unsigned char *resource,
                                  unsigned long resource_size);
int slicks_race_add_weapon_icon(struct SlicksRaceRuntime *race,
    unsigned short weapon, const unsigned char *resource, unsigned long size);
int slicks_race_add_weapon_asset(struct SlicksRaceRuntime *race,
    unsigned asset,const unsigned char *resource,unsigned long size);
int slicks_race_add_start_light(struct SlicksRaceRuntime *race,
                                unsigned short light,
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
void slicks_race_use_chunky_surface(struct SlicksRaceRuntime *race);
void slicks_race_step(struct SlicksRaceRuntime *race, unsigned char *logical);
void slicks_race_set_timer(struct SlicksRaceRuntime *race,unsigned short argument);
void slicks_race_resolve_car_collisions(struct SlicksRaceRuntime *race,
                                        unsigned short current);
void slicks_race_clear_dirty_rows(struct SlicksRaceRuntime *race);

#endif
