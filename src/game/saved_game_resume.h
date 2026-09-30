#ifndef SLICKS_SAVED_GAME_RESUME_H
#define SLICKS_SAVED_GAME_RESUME_H
#include "saved_game.h"
#include "player_profiles.h"

struct SlicksSavedGameResolved {
    short *tracks;
    unsigned track_capacity;
    short profiles[4];
    unsigned char vehicles[4];
};
enum SlicksSavedGameResolveResult { SLICKS_RESUME_READY, SLICKS_RESUME_INVALID,
    SLICKS_RESUME_MISSING_TRACK, SLICKS_RESUME_MISSING_PROFILE };

static inline int slicks_saved_name_equal(const unsigned char *a,const unsigned char *b,unsigned limit)
{
    for(unsigned i=0;i<limit;++i) {
        unsigned char x=a[i],y=b[i];
        if(x>='a' && x<='z') x-=32;
        if(y>='a' && y<='z') y-=32;
        if(x!=y) return 0;
        if(!a[i]) return 1;
    }
    return 1;
}

/* Original 1d2d4..1d33d and 1d48b..1d55b: ASCII-case-insensitive names,
 * LAST matching catalogue/profile entry, skip profile zero. AI maps to
 * built-in profile one, inactive to -2; humans must have an existing name.
 * Resolve everything privately before exposing indices to the live session. */
static inline enum SlicksSavedGameResolveResult slicks_resolve_saved_game(
    struct SlicksSavedGameResolved *out,const struct SlicksSavedGame *game,
    unsigned track_count,const unsigned char *(*track_name)(void *,unsigned),void *context,
    const struct SlicksPlayerProfiles *profiles,unsigned vehicle_count)
{
    if(!out || !game || !profiles || !track_name || !vehicle_count || vehicle_count>128 ||
       track_count>32767 || profiles->count<1 || profiles->count>SLICKS_PROFILE_MAX ||
       slicks_saved_game_size(game)<0 || (unsigned)game->track_count>out->track_capacity ||
       (game->track_count && !out->tracks)) return SLICKS_RESUME_INVALID;
    struct SlicksSavedGameResolved next=*out;
    for(unsigned i=0;i<(unsigned)game->track_count;++i) {
        short found=-1;
        for(unsigned j=0;j<track_count;++j) {
            const unsigned char *name=track_name(context,j);
            if(name && slicks_saved_name_equal(game->tracks[i],name,8)) found=(short)j;
        }
        if(found<0) return SLICKS_RESUME_MISSING_TRACK;
    }
    for(unsigned i=0;i<4;++i) {
        signed char vehicle=game->vehicles[i];
        next.vehicles[i]=(unsigned char)(vehicle<0 || (unsigned)vehicle>=vehicle_count?0:vehicle);
        next.profiles[i]=-2;
        if(game->participation[i]>0) next.profiles[i]=1;
        else if(game->participation[i]<0) {
            unsigned char name[21];
            for(unsigned k=0;k<20;++k) name[k]=game->names[i][k];
            name[20]=0;
            for(unsigned j=1;j<(unsigned)profiles->count;++j)
                if(slicks_saved_name_equal(name,profiles->names[j],21)) next.profiles[i]=(short)j;
            if(next.profiles[i]<0) return SLICKS_RESUME_MISSING_PROFILE;
        }
    }
    /* Catalogue callbacks are stable, read-only views for this transaction.
     * Validate every name/profile before touching caller-owned indices. This
     * second pass avoids a playlist-sized automatic array on the 4 KiB stack. */
    for(unsigned i=0;i<(unsigned)game->track_count;++i)
        for(unsigned j=0;j<track_count;++j) {
            const unsigned char *name=track_name(context,j);
            if(name && slicks_saved_name_equal(game->tracks[i],name,8)) next.tracks[i]=(short)j;
        }
    *out=next;
    return SLICKS_RESUME_READY;
}
#endif
