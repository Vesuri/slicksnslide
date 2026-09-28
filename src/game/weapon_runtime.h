#ifndef SLICKS_WEAPON_RUNTIME_H
#define SLICKS_WEAPON_RUNTIME_H
#include "weapon_fire.h"
#include "actor_slots.h"
#include "track_actor_assets.h"

#define SLICKS_WEAPON_ASSET_COUNT 19
#define SLICKS_WEAPON_ASSET_PIXELS 64
struct SlicksWeaponAsset {
    unsigned char pixels[SLICKS_WEAPON_ASSET_PIXELS];
    unsigned char width,height,ready;
};
struct SlicksWeaponActor {
    struct SlicksActorMotion motion;
    unsigned char asset,kind,priority,occlusion,colour;
    short old_x,old_y;
    /* Three padding bytes before saved pixels give every actor a longword
     * aligned background and a 164-byte stride (400 additional pool bytes). */
    unsigned char old_width,old_height,saved,retain; /* retain: sprite_retention.inc */
    unsigned char saved_under[SLICKS_TRACK_ACTOR_PIXELS] __attribute__((aligned(4)));
};
struct SlicksWeaponRuntime {
    struct SlicksWeaponRules rules;
    struct SlicksWeaponControl controls[4];
    struct SlicksWeaponProjectile projectiles[4][30];
    struct SlicksActorSlots slots;
    /* The dense 24-byte fast trail ABI is unchanged. These maps preserve
     * shared allocation and actor-ID drawing order when holes are reused. */
    short trail_index[SLICKS_ACTOR_CAPACITY];
    unsigned char trail_handle[256];
    /* Exclusive conservative non-point scan bound; zero requests a full
     * shared-pool scan. Uses the native padding byte before the counters. */
    unsigned char ready,bullet_colour,impact_colour,sprite_high_water;
    unsigned long shots,hits,explosions;
    /* Keep slot lookup and counters ahead of the large sprite storage. */
    struct SlicksWeaponActor actors[SLICKS_ACTOR_CAPACITY];
    struct SlicksWeaponAsset assets[SLICKS_WEAPON_ASSET_COUNT];
};
#if defined(__m68k__)
_Static_assert(SLICKS_ACTOR_CAPACITY<=255,"byte sprite scan bound");
_Static_assert(__builtin_offsetof(struct SlicksWeaponRuntime,shots)==
    __builtin_offsetof(struct SlicksWeaponRuntime,sprite_high_water)+1,
    "sprite bound must occupy native counter-alignment padding");
#endif
#endif
