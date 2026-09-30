#ifndef SLICKS_CHAMPIONSHIP_H
#define SLICKS_CHAMPIONSHIP_H
#include "saved_game_resume.h"
#include "setup_session.h"
#include "arcade_setup.h"

/* The .SSS file is a between-races checkpoint, not a mid-race snapshot.
 * Options, input bindings, colours and RNG are deliberately not serialized
 * by the original. Keep the current configuration and RNG on resume. */
static inline int slicks_championship_export(struct SlicksSavedGame *out,
    unsigned char tracks[][8],unsigned capacity,const short *selection,unsigned count,short next_track,
    unsigned catalogue_count,const unsigned char *(*name)(void *,unsigned),void *context,
    const struct SlicksSetupSession *session,const struct SlicksPlayerProfiles *profiles,
    const unsigned char scales[4])
{
    if(!out || !tracks || !selection || !name || !session || !profiles || !scales ||
       !count || count>SLICKS_SAVED_GAME_TRACK_MAX || count>capacity ||
       next_track<0 || (unsigned)next_track>=count) return -1;
    for(unsigned i=0;i<count;++i)
        if(selection[i]<0 || (unsigned)selection[i]>=catalogue_count || !name(context,selection[i])) return -1;
    for(unsigned i=0;i<4;++i)
        if(session->players.participation[i] &&
           (session->players.selected[i]<1 || session->players.selected[i]>=profiles->count)) return -1;
    struct SlicksSavedGame game={.track_count=(short)count,.next_track=next_track,
        .tracks=(const unsigned char (*)[8])tracks};
    for(unsigned i=0;i<count;++i) {
        const unsigned char *s=name(context,selection[i]); unsigned j=0;
        for(;j<8 && s[j];++j) tracks[i][j]=s[j];
        for(;j<8;++j) tracks[i][j]=0;
    }
    for(unsigned i=0;i<4;++i) {
        if(session->players.participation[i])
            for(unsigned j=0;j<20;++j) game.names[i][j]=profiles->names[session->players.selected[i]][j];
        game.vehicles[i]=session->players.vehicle[i];
        game.participation[i]=session->players.participation[i];
        game.position_scale[i]=scales[i]; game.points[i]=session->points[i]; game.cash[i]=session->cash[i];
        for(unsigned j=0;j<13;++j) game.inventory[i][j]=session->inventory[i][j];
    }
    *out=game; return 0;
}

/* Validate and stage every field before the caller publishes the session or
 * playlist. Race asset preparation can still fail: keep the old live session
 * until that has succeeded as well. No new-game reset or random-car draw. */
static inline enum SlicksSavedGameResolveResult slicks_championship_stage(
    struct SlicksSetupSession *out,struct SlicksSavedGameResolved *resolved,
    const struct SlicksSavedGame *game,const struct SlicksSetupSession *current,
    const struct SlicksConfiguration *config,const struct SlicksPlayerProfiles *profiles,
    const unsigned char fallback[4][6],unsigned vehicle_count,unsigned track_count,
    const unsigned char *(*name)(void *,unsigned),void *context)
{
    if(!out || !resolved || !game || !current || !config || !profiles || !fallback ||
       game->track_count<=0 || game->next_track<0 ||
       game->next_track>=slicks_arcade_track_count(config->options[0],config->options[14],game->track_count))
        return SLICKS_RESUME_INVALID;
    unsigned active=0;
    for(unsigned i=0;i<4;++i) {
        if(game->participation[i]>0 && profiles->count<2) return SLICKS_RESUME_MISSING_PROFILE;
        if(game->participation[i]) ++active;
    }
    if(!active) return SLICKS_RESUME_INVALID;
    struct SlicksSavedGameResolved r=*resolved;
    enum SlicksSavedGameResolveResult status=slicks_resolve_saved_game(&r,game,track_count,name,context,profiles,vehicle_count);
    if(status!=SLICKS_RESUME_READY) return status;
    struct SlicksSetupSession next=*current;
    unsigned fallback_index=0,negative=0;
    next.players.count=0;
    for(unsigned i=0;i<4;++i) {
        next.players.selected[i]=r.profiles[i]; next.players.vehicle[i]=(signed char)r.vehicles[i];
        next.players.participation[i]=game->participation[i];
        if(game->participation[i]) {
            ++next.players.count;
            slicks_profile_colour_record(next.players.colours[i],&profiles->setup[r.profiles[i]],fallback,&fallback_index);
        }
        if(game->participation[i]<0) next.players.order[negative++]=(unsigned char)i;
        else for(unsigned j=negative;j<4;++j) next.players.order[j]=(unsigned char)i;
        next.points[i]=game->points[i]; next.cash[i]=game->cash[i];
        next.saved_position_scale[i]=game->position_scale[i];
        for(unsigned j=0;j<13;++j) next.inventory[i][j]=game->inventory[i][j];
    }
    next.saved_position_scale_valid=1;
    *out=next; *resolved=r; return SLICKS_RESUME_READY;
}
#endif
