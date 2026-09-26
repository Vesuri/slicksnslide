#ifndef SLICKS_TRACK_ACTOR_ASSETS_H
#define SLICKS_TRACK_ACTOR_ASSETS_H
#define SLICKS_TRACK_ACTOR_ASSETS 14
#define SLICKS_TRACK_ACTOR_PIXELS 128
#include "sprite_opacity.h"
struct SlicksTrackActorAsset {
    unsigned char pixels[SLICKS_TRACK_ACTOR_PIXELS];
    unsigned char width,height;
    unsigned char opacity[SLICKS_TRACK_ACTOR_PIXELS],opacity_ready;
};
int slicks_decode_track_actor_assets(const unsigned char *dat,unsigned long size,
    unsigned char *arena,unsigned long arena_size,struct SlicksTrackActorAsset assets[14]);
#endif
