/* Compiled only with -S for the 68020 target: each line becomes an assembler
 * equate, so native routines use the compiler's own structure layout. */
#include "race_runtime.h"

#define OFFSET(name, type, member) __asm__ volatile( \
    "\n@@" #name " equ %c0" :: "i"(__builtin_offsetof(type, member)))
#define VALUE(name, value) __asm__ volatile( \
    "\n@@" #name " equ %c0" :: "i"(value))

void slicks_race_offsets(void);
void slicks_race_offsets(void)
{
    OFFSET(RACE_CARS, struct SlicksRaceRuntime, cars);
    OFFSET(RACE_MATERIAL_MAP, struct SlicksRaceRuntime, material_map);
    OFFSET(RACE_SURFACE_MAP, struct SlicksRaceRuntime, surface_map);
    OFFSET(RACE_BOUNDARY_LEVEL, struct SlicksRaceRuntime, boundary_level);
    OFFSET(RACE_TRACK_COLLISION_COUNT, struct SlicksRaceRuntime, track_collision_count);
    OFFSET(RACE_COLLISION_ERROR, struct SlicksRaceRuntime, collision_error);

    OFFSET(RACE_TRAIL_PARTICLES, struct SlicksRaceRuntime, trail_particles);
    OFFSET(RACE_TRAIL_PARTICLE_COUNT, struct SlicksRaceRuntime, trail_particle_count);
    OFFSET(RACE_DIRTY_PIXELS, struct SlicksRaceRuntime, dirty_pixels);
    OFFSET(RACE_DIRTY_PIXEL_COUNT, struct SlicksRaceRuntime, dirty_pixel_count);
    OFFSET(RACE_ACTOR_ORDER_HEAD, struct SlicksRaceRuntime, actor_order_head);
    OFFSET(RACE_ACTOR_ORDER_NEXT, struct SlicksRaceRuntime, actor_order_next);
    OFFSET(RACE_ACTOR_ORDER_MAX, struct SlicksRaceRuntime, actor_order_max);
    OFFSET(RACE_ACTOR_ORDER_READY, struct SlicksRaceRuntime, actor_order_ready);
    OFFSET(RACE_ACTOR_ORDER_DRAWN, struct SlicksRaceRuntime, actor_order_drawn);
    OFFSET(RACE_SKIDMARK_COUNT, struct SlicksRaceRuntime, skidmark_count);
    OFFSET(RACE_RANDOM_STATE, struct SlicksRaceRuntime, random_state);
    OFFSET(RACE_EMISSION_SLOT_CURSOR, struct SlicksRaceRuntime, emission_slot_cursor);
    OFFSET(RACE_SOUND_EVENTS, struct SlicksRaceRuntime, sound_events);
    OFFSET(RACE_SOUND_EVENT_COUNT, struct SlicksRaceRuntime, sound_event_count);
    OFFSET(RACE_SOUND_EVENT_TOTALS, struct SlicksRaceRuntime, sound_event_totals);
    OFFSET(RACE_WEAPON_SLOTS, struct SlicksRaceRuntime, weapons.slots);
    OFFSET(RACE_TRAIL_HANDLE, struct SlicksRaceRuntime, weapons.trail_handle);
    OFFSET(RACE_TRAIL_INDEX, struct SlicksRaceRuntime, weapons.trail_index);
    OFFSET(RACE_ACTORS, struct SlicksRaceRuntime, weapons.actors);
    OFFSET(RACE_SPRITES, struct SlicksRaceRuntime, sprites);
    OFFSET(RACE_PROPERTIES, struct SlicksRaceRuntime, properties);
    OFFSET(SLOTS_HIGH_WATER, struct SlicksActorSlots, high_water);
    OFFSET(SLOTS_CAPACITY, struct SlicksActorSlots, capacity);
    VALUE(ACTOR_SIZE, sizeof(struct SlicksWeaponActor));
    OFFSET(ACTOR_KIND, struct SlicksWeaponActor, kind);
    OFFSET(ACTOR_SAVED, struct SlicksWeaponActor, saved);
    VALUE(SPRITE_SIZE, sizeof(struct SlicksCarSprite));
    OFFSET(SPRITE_WHEEL_X, struct SlicksCarSprite, wheel_x);
    OFFSET(SPRITE_WHEEL_Y, struct SlicksCarSprite, wheel_y);
    VALUE(PROPERTY_SIZE, sizeof(struct SlicksCarProperties));
    OFFSET(PROPERTY_EFFECT_PROFILE, struct SlicksCarProperties, effect_profile);
    VALUE(PARTICLE_SIZE, sizeof(struct SlicksTrailParticle));
    VALUE(PARTICLE_MAX, SLICKS_TRAIL_PARTICLE_MAX);
    OFFSET(PARTICLE_X, struct SlicksTrailParticle, x);
    OFFSET(PARTICLE_Y, struct SlicksTrailParticle, y);
    OFFSET(PARTICLE_VELOCITY_X, struct SlicksTrailParticle, velocity_x);
    OFFSET(PARTICLE_VELOCITY_Y, struct SlicksTrailParticle, velocity_y);
    OFFSET(PARTICLE_LIFETIME, struct SlicksTrailParticle, lifetime);
    OFFSET(PARTICLE_COLOUR, struct SlicksTrailParticle, colour);
    OFFSET(PARTICLE_PRIORITY, struct SlicksTrailParticle, priority);
    OFFSET(PARTICLE_SAVED_VALID, struct SlicksTrailParticle, saved_valid);
    OFFSET(PARTICLE_PERMANENT, struct SlicksTrailParticle, permanent);
    OFFSET(PARTICLE_OCCLUSION_LIMIT, struct SlicksTrailParticle, occlusion_limit);
    OFFSET(PARTICLE_STATE, struct SlicksTrailParticle, state);
    OFFSET(RACE_NAVIGATION_ACTORS, struct SlicksRaceRuntime, navigation.actors);
    OFFSET(RACE_NAVIGATION_ACTOR_COUNT, struct SlicksRaceRuntime, navigation.actor_count);
    OFFSET(RACE_TRACK_ACTOR_HANDLES, struct SlicksRaceRuntime, track_actor_handles);
    OFFSET(RACE_TRACK_STATIONARY_READY, struct SlicksRaceRuntime, track_stationary_ready);
    OFFSET(RACE_TRACK_ACTORS_READY, struct SlicksRaceRuntime, track_actors_ready);
    OFFSET(RACE_TRACK_ACTOR_SCRATCH, struct SlicksRaceRuntime, track_actor_scratch);
    VALUE(TRACK_ACTOR_SIZE, sizeof(struct SlicksTrackActor));
    OFFSET(TRACK_ACTOR_X, struct SlicksTrackActor, x);
    OFFSET(TRACK_ACTOR_Y, struct SlicksTrackActor, y);
    OFFSET(TRACK_ACTOR_VELOCITY_X, struct SlicksTrackActor, velocity_x);
    OFFSET(TRACK_ACTOR_VELOCITY_Y, struct SlicksTrackActor, velocity_y);
    OFFSET(TRACK_ACTOR_KIND, struct SlicksTrackActor, kind);
    OFFSET(TRACK_ACTOR_LAYER, struct SlicksTrackActor, layer);
    OFFSET(ACTOR_MOTION_X, struct SlicksWeaponActor, motion.x);
    OFFSET(ACTOR_MOTION_Y, struct SlicksWeaponActor, motion.y);
    OFFSET(ACTOR_MOTION_VX, struct SlicksWeaponActor, motion.vx);
    OFFSET(ACTOR_MOTION_AX, struct SlicksWeaponActor, motion.ax);
    OFFSET(ACTOR_MOTION_LIFETIME, struct SlicksWeaponActor, motion.lifetime);
    OFFSET(ACTOR_MOTION_AGE, struct SlicksWeaponActor, motion.age);
    OFFSET(ACTOR_MOTION_FRAME, struct SlicksWeaponActor, motion.frame);
    OFFSET(ACTOR_MOTION_PERIOD, struct SlicksWeaponActor, motion.period);
    OFFSET(ACTOR_MOTION_VY, struct SlicksWeaponActor, motion.vy);
    OFFSET(ACTOR_MOTION_AY, struct SlicksWeaponActor, motion.ay);
    OFFSET(ACTOR_MOTION_FRAMES, struct SlicksWeaponActor, motion.frames);
    OFFSET(RACE_ACTOR_PAGE, struct SlicksRaceRuntime, actor_page);
    OFFSET(ACTOR_PRIORITY, struct SlicksWeaponActor, priority);
    OFFSET(ACTOR_OCCLUSION, struct SlicksWeaponActor, occlusion);
    VALUE(ACTOR_CAPACITY, SLICKS_ACTOR_CAPACITY);
    OFFSET(CAR_MEASURED_SPEED, struct SlicksRaceCar, measured_speed);
    OFFSET(RACE_DIRTY_ROWS, struct SlicksRaceRuntime, dirty_rows);
    OFFSET(RACE_DIRTY_ROW_COUNT, struct SlicksRaceRuntime, dirty_row_count);
    VALUE(SLICKS_DIRTY_ROW_MAX_VALUE, SLICKS_DIRTY_ROW_MAX);
    OFFSET(RACE_CHUNKY, struct SlicksRaceRuntime, chunky);
    OFFSET(RACE_CACHE_READY, struct SlicksRaceRuntime, car_render_cache.ready);
    OFFSET(RACE_CACHE_TILE_MAX, struct SlicksRaceRuntime, car_render_cache.tile_max);
    OFFSET(RACE_CACHE_CARS, struct SlicksRaceRuntime, car_render_cache.cars);
    VALUE(CACHE_CAR_SIZE, sizeof(((struct SlicksRaceRuntime *)0)->car_render_cache.cars[0]));
    VALUE(CACHE_CAR_READY, __builtin_offsetof(struct SlicksRaceRuntime, car_render_cache.cars[0].ready)-
        __builtin_offsetof(struct SlicksRaceRuntime, car_render_cache.cars[0]));
    VALUE(CACHE_CAR_VEHICLE, __builtin_offsetof(struct SlicksRaceRuntime, car_render_cache.cars[0].vehicle)-
        __builtin_offsetof(struct SlicksRaceRuntime, car_render_cache.cars[0]));
    VALUE(CACHE_CAR_STYLE, __builtin_offsetof(struct SlicksRaceRuntime, car_render_cache.cars[0].style)-
        __builtin_offsetof(struct SlicksRaceRuntime, car_render_cache.cars[0]));
    VALUE(CACHE_CAR_FRAMES, __builtin_offsetof(struct SlicksRaceRuntime, car_render_cache.cars[0].frames)-
        __builtin_offsetof(struct SlicksRaceRuntime, car_render_cache.cars[0]));
    VALUE(CACHE_FRAME_SIZE, sizeof(((struct SlicksRaceRuntime *)0)->car_render_cache.cars[0].frames[0]));
    VALUE(CACHE_FRAME_PIXELS, __builtin_offsetof(struct SlicksRaceRuntime, car_render_cache.cars[0].frames[0].pixels)-
        __builtin_offsetof(struct SlicksRaceRuntime, car_render_cache.cars[0].frames[0]));
    VALUE(CACHE_FRAME_OPACITY, __builtin_offsetof(struct SlicksRaceRuntime, car_render_cache.cars[0].frames[0].opacity)-
        __builtin_offsetof(struct SlicksRaceRuntime, car_render_cache.cars[0].frames[0]));
    VALUE(CACHE_FRAME_WIDTH, __builtin_offsetof(struct SlicksRaceRuntime, car_render_cache.cars[0].frames[0].width)-
        __builtin_offsetof(struct SlicksRaceRuntime, car_render_cache.cars[0].frames[0]));
    VALUE(CACHE_FRAME_HEIGHT, __builtin_offsetof(struct SlicksRaceRuntime, car_render_cache.cars[0].frames[0].height)-
        __builtin_offsetof(struct SlicksRaceRuntime, car_render_cache.cars[0].frames[0]));
    OFFSET(SPRITE_WIDTH, struct SlicksCarSprite, width);
    OFFSET(SPRITE_HEIGHT, struct SlicksCarSprite, height);
    OFFSET(RACE_RACING, struct SlicksRaceRuntime, racing);
    OFFSET(RACE_SPRITE_DIRTY_DEFERRED, struct SlicksRaceRuntime, sprite_dirty_deferred);
    OFFSET(RACE_RACE_MODE, struct SlicksRaceRuntime, race_mode);
    OFFSET(RACE_SHADOWS, struct SlicksRaceRuntime, shadows);
    VALUE(SHADOW_SIZE, sizeof(struct SlicksCarShadow));
    OFFSET(SHADOW_SAVED_VALID, struct SlicksCarShadow, saved_valid);
    OFFSET(SHADOW_STATE, struct SlicksCarShadow, state);
    OFFSET(RACE_WEAPONS_READY, struct SlicksRaceRuntime, weapons.ready);
    OFFSET(RACE_SPRITE_DIRTY_PREVIOUS, struct SlicksRaceRuntime, sprite_dirty_previous);
    VALUE(PREV_SIZE, sizeof(((struct SlicksRaceRuntime *)0)->sprite_dirty_previous[0]));
    VALUE(PREV_X, __builtin_offsetof(struct SlicksRaceRuntime, sprite_dirty_previous[0].x)-
        __builtin_offsetof(struct SlicksRaceRuntime, sprite_dirty_previous[0]));
    VALUE(PREV_Y, __builtin_offsetof(struct SlicksRaceRuntime, sprite_dirty_previous[0].y)-
        __builtin_offsetof(struct SlicksRaceRuntime, sprite_dirty_previous[0]));
    VALUE(PREV_KIND, __builtin_offsetof(struct SlicksRaceRuntime, sprite_dirty_previous[0].kind)-
        __builtin_offsetof(struct SlicksRaceRuntime, sprite_dirty_previous[0]));
    VALUE(PREV_ASSET, __builtin_offsetof(struct SlicksRaceRuntime, sprite_dirty_previous[0].asset)-
        __builtin_offsetof(struct SlicksRaceRuntime, sprite_dirty_previous[0]));
    VALUE(PREV_FRAME, __builtin_offsetof(struct SlicksRaceRuntime, sprite_dirty_previous[0].frame)-
        __builtin_offsetof(struct SlicksRaceRuntime, sprite_dirty_previous[0]));
    VALUE(PREV_COLOUR, __builtin_offsetof(struct SlicksRaceRuntime, sprite_dirty_previous[0].colour)-
        __builtin_offsetof(struct SlicksRaceRuntime, sprite_dirty_previous[0]));
    VALUE(PREV_PRIORITY, __builtin_offsetof(struct SlicksRaceRuntime, sprite_dirty_previous[0].priority)-
        __builtin_offsetof(struct SlicksRaceRuntime, sprite_dirty_previous[0]));
    VALUE(PREV_OCCLUSION, __builtin_offsetof(struct SlicksRaceRuntime, sprite_dirty_previous[0].occlusion)-
        __builtin_offsetof(struct SlicksRaceRuntime, sprite_dirty_previous[0]));
    OFFSET(ACTOR_ASSET, struct SlicksWeaponActor, asset);
    OFFSET(ACTOR_COLOUR, struct SlicksWeaponActor, colour);
    OFFSET(ACTOR_RETAIN, struct SlicksWeaponActor, retain);
    OFFSET(RACE_TRACK_ACTOR_ASSETS, struct SlicksRaceRuntime, track_actor_assets);
    VALUE(TRACK_ASSET_SIZE, sizeof(struct SlicksTrackActorAsset));
    OFFSET(TRACK_ASSET_WIDTH, struct SlicksTrackActorAsset, width);
    OFFSET(TRACK_ASSET_HEIGHT, struct SlicksTrackActorAsset, height);
    OFFSET(ENTRY_RIGHT, struct SlicksRetentionEntry, right);
    OFFSET(ENTRY_BOTTOM, struct SlicksRetentionEntry, bottom);
    OFFSET(RET_COUNT, struct SlicksRetentionState, count);
    OFFSET(RET_VALID, struct SlicksRetentionState, valid);
    OFFSET(RET_CANDIDATES, struct SlicksRetentionState, candidates);
    OFFSET(RET_ENTRIES, struct SlicksRetentionState, entries);
    OFFSET(RET_ROWS, struct SlicksRetentionState, rows);
    OFFSET(RET_CELLS, struct SlicksRetentionState, cells);
    VALUE(ENTRY_SIZE, sizeof(struct SlicksRetentionEntry));
    OFFSET(ENTRY_LEFT, struct SlicksRetentionEntry, left);
    OFFSET(ENTRY_TOP, struct SlicksRetentionEntry, top);
    OFFSET(ENTRY_KEY, struct SlicksRetentionEntry, key);
    OFFSET(ENTRY_HANDLE, struct SlicksRetentionEntry, handle);
    OFFSET(ENTRY_FLAGS, struct SlicksRetentionEntry, flags);
    OFFSET(ENTRY_ASSET, struct SlicksRetentionEntry, asset);
    OFFSET(ENTRY_FRAME, struct SlicksRetentionEntry, frame);
    OFFSET(ENTRY_PRIORITY, struct SlicksRetentionEntry, priority);
    OFFSET(RET_MAX_PRIORITY, struct SlicksRetentionState, max_priority);
    OFFSET(RET_KIND_WIDTH, struct SlicksRetentionState, kind_width);
    OFFSET(RET_KIND_HEIGHT, struct SlicksRetentionState, kind_height);
    OFFSET(RET_REBUILD_PENDING, struct SlicksRetentionState, rebuild_pending);
    OFFSET(PARTICLE_PRIORITY_B, struct SlicksTrailParticle, priority);
    VALUE(CONTROL_ACCELERATE, SLICKS_CONTROL_ACCELERATE);
    VALUE(CONTROL_BRAKE, SLICKS_CONTROL_BRAKE);

    VALUE(CAR_SIZE, sizeof(struct SlicksRaceCar));
    OFFSET(CAR_X, struct SlicksRaceCar, x);
    OFFSET(CAR_Y, struct SlicksRaceCar, y);
    OFFSET(CAR_SPEED_FIXED, struct SlicksRaceCar, speed_fixed);
    OFFSET(CAR_VELOCITY_X, struct SlicksRaceCar, velocity_x);
    OFFSET(CAR_VELOCITY_Y, struct SlicksRaceCar, velocity_y);
    OFFSET(CAR_HEADING, struct SlicksRaceCar, heading);
    OFFSET(CAR_DAMAGE, struct SlicksRaceCar, damage);
    OFFSET(CAR_DRIVE_BIAS, struct SlicksRaceCar, drive_bias);
    OFFSET(CAR_SPECIAL_DRIVE_STATE, struct SlicksRaceCar, special_drive_state);
    OFFSET(CAR_DRIVE_COEFFICIENTS, struct SlicksRaceCar, drive_coefficients);
    OFFSET(CAR_POSITION_SCALE, struct SlicksRaceCar, position_scale);
    OFFSET(CAR_TOUCHING_SOLID, struct SlicksRaceCar, touching_solid);
    OFFSET(CAR_ACTOR_LAYER, struct SlicksRaceCar, actor_layer);
    OFFSET(CAR_ACTOR_CONTACT, struct SlicksRaceCar, actor_contact);
    OFFSET(CAR_COLLISION_SAMPLING, struct SlicksRaceCar, collision_sampling);
    OFFSET(CAR_SERVICE_FLAGS, struct SlicksRaceCar, service_flags);
    OFFSET(CAR_VEHICLE, struct SlicksRaceCar, vehicle);
    OFFSET(CAR_FORWARD_DRIVE_LATCH, struct SlicksRaceCar, forward_drive_latch);
    OFFSET(CAR_STYLE, struct SlicksRaceCar, style);
    OFFSET(CAR_OLD_X, struct SlicksRaceCar, old_x);
    OFFSET(CAR_OLD_Y, struct SlicksRaceCar, old_y);
    OFFSET(CAR_OLD_WIDTH, struct SlicksRaceCar, old_width);
    OFFSET(CAR_OLD_HEIGHT, struct SlicksRaceCar, old_height);
    OFFSET(CAR_SAVED_VALID, struct SlicksRaceCar, saved_valid);
    OFFSET(CAR_SAVED_UNDER, struct SlicksRaceCar, saved_under);
}
