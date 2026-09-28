#ifndef SLICKS_RETENTION_SNAPSHOT_H
#define SLICKS_RETENTION_SNAPSHOT_H
#include "../../game/race_runtime.h"

/* Diagnostic only. The three terrain arrays are immutable between race
 * starts. Preserve every mutable byte and check the excluded arrays before
 * and after both reference and optimized passes instead of duplicating them. */
#define SLICKS_RETENTION_PREFIX __builtin_offsetof(struct SlicksRaceRuntime,material_map)
struct SlicksRetentionSnapshot {
    unsigned char prefix[SLICKS_RETENTION_PREFIX];
    struct SlicksSteeringCache steering[SLICKS_RACE_CAR_COUNT];
    unsigned int immutable_hash;
};
_Static_assert(sizeof(unsigned int)==4,"32-bit diagnostic hash");
_Static_assert(_Alignof(struct SlicksRaceRuntime)>=4 &&
    SLICKS_RETENTION_PREFIX%4==0 &&
    __builtin_offsetof(struct SlicksRaceRuntime,particle_visibility)%4==0,
    "aligned diagnostic map reads");
_Static_assert(__builtin_offsetof(struct SlicksRaceRuntime,surface_map)==
    SLICKS_RETENTION_PREFIX+SLICKS_TRACK_MATERIAL_SIZE,"snapshot material layout");
_Static_assert(__builtin_offsetof(struct SlicksRaceRuntime,steering_cache)==
    SLICKS_RETENTION_PREFIX+2*SLICKS_TRACK_MATERIAL_SIZE,"snapshot surface layout");
_Static_assert(__builtin_offsetof(struct SlicksRaceRuntime,particle_visibility)==
    __builtin_offsetof(struct SlicksRaceRuntime,steering_cache)+
    sizeof(((struct SlicksRaceRuntime *)0)->steering_cache),"snapshot mutable tail layout");
_Static_assert(sizeof(struct SlicksRaceRuntime)==
    __builtin_offsetof(struct SlicksRaceRuntime,particle_visibility)+
    sizeof(((struct SlicksRaceRuntime *)0)->particle_visibility),"snapshot covers runtime tail");

static unsigned int slicks_retention_map_hash(const struct SlicksRaceRuntime *race)
{
    const unsigned char *p=__builtin_assume_aligned(
        (const unsigned char *)race+SLICKS_RETENTION_PREFIX,4);
    unsigned int hash=0x811c9dc5U;
    for(unsigned i=0;i<2*SLICKS_TRACK_MATERIAL_SIZE/4;++i) {
        unsigned int word;__builtin_memcpy(&word,p+4*i,4);
        hash=((hash<<5)|(hash>>27))^word;
    }
    p=__builtin_assume_aligned((const unsigned char *)race->particle_visibility,4);
    for(unsigned i=0;i<sizeof race->particle_visibility/4;++i) {
        unsigned int word;__builtin_memcpy(&word,p+4*i,4);
        hash=((hash<<5)|(hash>>27))^word;
    }
    return hash;
}
static void slicks_retention_capture(struct SlicksRetentionSnapshot *saved,
                                     const struct SlicksRaceRuntime *race)
{
    __builtin_memcpy(saved->prefix,race,sizeof saved->prefix);
    __builtin_memcpy(saved->steering,race->steering_cache,sizeof saved->steering);
    saved->immutable_hash=slicks_retention_map_hash(race);
}
static int slicks_retention_maps_match(const struct SlicksRetentionSnapshot *saved,
                                      const struct SlicksRaceRuntime *race)
{ return saved->immutable_hash==slicks_retention_map_hash(race); }
static int slicks_retention_restore(struct SlicksRaceRuntime *race,
                                   const struct SlicksRetentionSnapshot *saved)
{
    int unchanged=slicks_retention_maps_match(saved,race);
    __builtin_memcpy(race,saved->prefix,sizeof saved->prefix);
    __builtin_memcpy(race->steering_cache,saved->steering,sizeof saved->steering);
    return unchanged;
}
#endif
