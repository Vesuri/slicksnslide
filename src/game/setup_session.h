#ifndef SLICKS_SETUP_SESSION_H
#define SLICKS_SETUP_SESSION_H
#include "profile_setup.h"
#include "race_options.h"
#include "weapon_state.h"
#include "race_rewards.h"

struct SlicksSetupSession {
    struct SlicksProfileSelection players;
    struct SlicksRaceOptions options;
    short inventory[4][13],cash[4],points[4];
    unsigned long random_state;
};
struct SlicksSetupResources {
    const struct SlicksSetupProfile *profiles;
    short profile_count,vehicle_count,override_count;
    const unsigned char (*fallback_colours)[6];
    const unsigned char *vehicle_weights,*item_flags;
    void (*apply_vehicle)(void *,unsigned,signed char);
    void *context;
};
struct SlicksSetupSelectionContext {
    struct SlicksSetupSession *session;
    const struct SlicksSetupResources *resources;
    short game_type;
};
static inline unsigned char slicks_setup_override(void *opaque)
{
    struct SlicksSetupSelectionContext *c=opaque;
    return slicks_profiles_override(c->game_type);
}
static inline unsigned char slicks_setup_choose(void *opaque)
{
    struct SlicksSetupSelectionContext *c=opaque;
    return slicks_choose_profile_vehicle(c->resources->vehicle_weights,
        c->resources->vehicle_count,&c->session->random_state);
}
static inline void slicks_setup_apply(void *opaque,unsigned driver,signed char vehicle)
{
    struct SlicksSetupSelectionContext *c=opaque;
    c->resources->apply_vehicle(c->resources->context,driver,vehicle);
}
static inline void slicks_setup_select(struct SlicksSetupSession *session,
    struct SlicksConfiguration *configuration,const struct SlicksSetupResources *resources,
    signed char choose_mode)
{
    struct SlicksSetupSelectionContext context={session,resources,configuration->options[0]};
    struct SlicksProfileSetupOps ops={slicks_setup_override,slicks_setup_choose,slicks_setup_apply,&context};
    for(unsigned i=0;i<4;++i) session->players.selected[i]=configuration->selected_profile[i];
    slicks_select_profiles(&session->players,resources->profiles,resources->profile_count,
        resources->fallback_colours,resources->override_count,resources->vehicle_count,0,choose_mode,&ops);
    for(unsigned i=0;i<4;++i) configuration->selected_profile[i]=session->players.selected[i];
}

/* 25b25/13392 seed only the low 16 bits; 26154 chooses every random vehicle.
 * Time acquisition is a platform boundary; never reseed on race entry. */
static inline void slicks_setup_session_start(struct SlicksSetupSession *session,
    struct SlicksConfiguration *configuration,const struct SlicksSetupResources *resources,
    unsigned short seed)
{
    *session=(struct SlicksSetupSession){0};
    session->random_state=seed;
    slicks_setup_select(session,configuration,resources,1);
}

/* New-game inventory loop 26372..263e6; the following choose=-1 call is
 * conditional on signed DS:0090 > 0. Saved-game and next-race paths must not
 * call this: their inventories and random stream must survive. */
static inline void slicks_setup_new_game(struct SlicksSetupSession *session,
    struct SlicksConfiguration *configuration,const struct SlicksSetupResources *resources,
    unsigned char mode_flags,short field_0090)
{
    slicks_resolve_race_options(&session->options,configuration,mode_flags);
    /* Original new championship 2a2ce..2a2dd clears DS:6826's four words. */
    for(unsigned i=0;i<4;++i) session->points[i]=0;
    slicks_new_game_inventory(session->inventory,session->cash,resources->item_flags,
        session->options.inventory_mode,session->options.starting_cash);
    if(field_0090>0) slicks_setup_select(session,configuration,resources,-1);
}
/* Original finish award 22cda..22d1b. This is paid at the finish event,
 * not on entry to a results screen or when the next track is selected. */
static inline void slicks_setup_finish_reward(struct SlicksSetupSession *session,
    unsigned driver,signed char rank,const unsigned char points_by_rank[4])
{
    if(driver>=4 || rank<1 || rank>4) return;
    slicks_finish_reward(&session->cash[driver],&session->points[driver],
        points_by_rank[rank-1],(unsigned char)(session->players.count-rank),
        session->options.field_302e);
}
/* Original 2557e..255ff, before standings/selection refresh. All four
 * slots receive the base payment, including inactive ones. */
static inline void slicks_setup_track_reward(struct SlicksSetupSession *session,
    const signed int best_laps[4],unsigned char fastest_points)
{
    slicks_track_rewards(session->cash,session->points,best_laps,
        session->options.field_302c,session->options.field_302e,fastest_points);
}
/* 25937: refresh selections after a race. Random-per-race profiles reroll;
 * fixed and random-once vehicles retain their original selection rules.
 * Inventory/cash and the shared RNG stream are never reinitialized. */
static inline void slicks_setup_after_race(struct SlicksSetupSession *session,
    struct SlicksConfiguration *configuration,const struct SlicksSetupResources *resources)
{ slicks_setup_select(session,configuration,resources,-1); }
#endif
