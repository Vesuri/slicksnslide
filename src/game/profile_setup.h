#ifndef SLICKS_PROFILE_SETUP_H
#define SLICKS_PROFILE_SETUP_H

/* 198b6: only game type 5 overrides the selected profiles. */
static inline unsigned char slicks_profiles_override(short game_type)
{
    return game_type==5;
}

/* 19967..199ce: weighted choice consumes one Borland RNG step even for
 * an empty/all-zero table. Both accumulation loops wrap signed 16-bit sums;
 * replacing this with an unsigned cumulative distribution changes behavior. */
static inline unsigned char slicks_choose_profile_vehicle(
    const unsigned char *weights, short vehicle_count, unsigned long *random_state)
{
    short total=0,cumulative=0;
    for(int i=0;i<vehicle_count;++i)
        total=(short)(unsigned short)((unsigned short)total+weights[i]);
    *random_state=(*random_state*0x015a4e35UL+1UL)&0xffffffffUL;
    short draw=(short)((long)total*(long)((*random_state>>16)&0x7fffUL)/32768L);
    for(int i=0;i<vehicle_count;++i) {
        cumulative=(short)(unsigned short)((unsigned short)cumulative+weights[i]);
        if(draw<cumulative) return (unsigned char)i;
    }
    return 0;
}

struct SlicksSetupProfile {
    unsigned char flags, vehicle, colours[6];
};
struct SlicksProfileSelection {
    short selected[4];
    signed char participation[4], vehicle[4];
    unsigned char colours[4][6], order[4], count;
};

/* Original menu helpers 279b6 and 28344. Callers supply valid profile
 * indices/driver slots; step is -1 or +1. Flag bit 1 permits sharing. */
static inline void slicks_menu_assign_profile(short selected[4],
    const struct SlicksSetupProfile *profiles,unsigned driver,short profile)
{
    if(!(profiles[profile].flags&2))
        for(unsigned i=0;i<4;++i)
            if(i!=driver && selected[i]==profile && selected[i]>0)
                selected[i]=selected[driver];
    selected[driver]=profile;
}

static inline short slicks_menu_step_profile(const short selected[4],
    const struct SlicksSetupProfile *profiles,short count,short current,signed char step)
{
    short candidate=current;
    for(;;) {
        candidate=(short)(unsigned short)((unsigned short)candidate+step);
        if(candidate<0 || candidate>=count) return current;
        if(profiles[candidate].flags&2) return candidate;
        unsigned occupied=0;
        for(unsigned i=0;i<4;++i) occupied|=selected[i]==candidate;
        if(!occupied) return candidate;
    }
}
struct SlicksProfileSetupOps {
    unsigned char (*override_profiles)(void *context);
    unsigned char (*choose_vehicle)(void *context);
    void (*apply_vehicle)(void *context, unsigned driver, signed char vehicle);
    void *context;
};

/* Shared selection/colour stages of 2bb70. Rendering can consume these
 * without rerolling vehicles or invoking property-loading callbacks. */
static inline short slicks_profile_index(short selected,short profile_count,
    unsigned driver,unsigned char override,short override_count)
{
    if(selected>=profile_count) selected=0;
    if(override) selected=(short)driver<override_count?2:1;
    return selected;
}
static inline void slicks_profile_colour_record(unsigned char out[6],
    const struct SlicksSetupProfile *profile,
    const unsigned char fallback_colours[4][6],unsigned *fallback)
{
    unsigned special=profile->flags==6;
    for(unsigned channel=0;channel<6;++channel)
        out[channel]=special?fallback_colours[*fallback][channel]:profile->colours[channel];
    *fallback+=special;
}

static inline void slicks_select_profile_colours(unsigned char colours[4][6],
    const short selected[4],const struct SlicksSetupProfile *profiles,
    short profile_count,const unsigned char fallback_colours[4][6],
    short game_type,short override_count)
{
    unsigned fallback=0;
    for(unsigned driver=0;driver<4;++driver) {
        short index=slicks_profile_index(selected[driver],profile_count,driver,
            slicks_profiles_override(game_type),override_count);
        if(index>0) slicks_profile_colour_record(colours[driver],&profiles[index],
            fallback_colours,&fallback);
    }
}

/* Original 2bb70..2bdd7. Keep the three call boundaries explicit: override
 * mode (198b6), vehicle chooser (19967), and property application (1ccfc).
 * Profiles and the four fallback RGB endpoint pairs come from original data.
 * The caller must provide at least profiles 0..2 even if profile_count < 3,
 * because override mode selects 1/2 after the bounds check. */
static inline void slicks_select_profiles(struct SlicksProfileSelection *state,
    const struct SlicksSetupProfile *profiles, short profile_count,
    const unsigned char fallback_colours[4][6], short override_count,
    short vehicle_count, unsigned char suppress_flagged, signed char choose_mode,
    const struct SlicksProfileSetupOps *ops)
{
    unsigned fallback=0, negative=0;
    state->count=0;
    for(unsigned driver=0;driver<4;++driver) {
        state->participation[driver]=0;
        short selected=slicks_profile_index(state->selected[driver],profile_count,
            driver,ops->override_profiles(ops->context),override_count);
        if(selected>0) {
            const struct SlicksSetupProfile *profile=&profiles[selected];
            state->participation[driver]=(profile->flags&1)
                ? (suppress_flagged ? 0 : 1) : -1;
            slicks_profile_colour_record(state->colours[driver],profile,
                fallback_colours,&fallback);
            if((short)profile->vehicle>=vehicle_count) {
                if(choose_mode>0 || (choose_mode<0 &&
                    (short)profile->vehicle >= (short)(vehicle_count+1)))
                    state->vehicle[driver]=(signed char)ops->choose_vehicle(ops->context);
            } else if(choose_mode) {
                state->vehicle[driver]=(signed char)profile->vehicle;
            } else if(state->vehicle[driver]>=vehicle_count) {
                state->vehicle[driver]=0;
            }
            ops->apply_vehicle(ops->context,driver,state->vehicle[driver]);
            ++state->count;
        }
        state->selected[driver]=selected;
        if(state->participation[driver]<0)
            state->order[negative++]=(unsigned char)driver;
        else
            for(unsigned at=negative;at<4;++at) state->order[at]=(unsigned char)driver;
    }
}
#endif
