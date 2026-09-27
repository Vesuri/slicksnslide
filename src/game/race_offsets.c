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
}
