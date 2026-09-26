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
}
