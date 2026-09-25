#ifndef SLICKS_WEAPON_ACTIONS_H
#define SLICKS_WEAPON_ACTIONS_H
#include "weapon_state.h"

struct SlicksWeaponControl {
    short cooldown, repeat_ticks;
    unsigned char request, cycle_held, probe_counter;
};

/* 20509..20595: the second driving control fires; the fifth control or
 * simultaneous left/right cycles on a new press. Preserve AI requests. */
static inline void slicks_weapon_human_request(struct SlicksWeaponControl *state,
    short enabled,signed char role,signed char selected,unsigned char controls)
{
    if(!enabled || role>=0) return;
    if(controls&2) state->request=selected>=0?1:0;
    if((controls&12)==12 || (controls&16)) {
        if(!state->cycle_held) state->request=2;
        state->cycle_held=1;
    } else state->cycle_held=0;
}

/* 206a5..206e8: signed game-clock comparison; the initial 250 ticks are
 * excluded. The caller has already applied participation/special/finish gates. */
static inline unsigned char slicks_weapon_can_fire(const struct SlicksWeaponControl *state,
    signed char selected,unsigned int clock)
{ return state->request==1 && selected>=0 && state->cooldown<=0 && (int)clock>250; }

/* 20bbf..20c09, only after the original shot loop. The count-one sentinel
 * is retained; allocation failure does not refund ammunition. */
static inline void slicks_weapon_consume(struct SlicksWeaponControl *state,
    short inventory[13],signed char selected,short unlimited)
{
    if(!unlimited) inventory[selected+5]=(short)((unsigned short)inventory[selected+5]-1U);
    if(inventory[selected+5]<=1) state->request=2;
}

/* 20c09..20c6f: cycling releases the fire/brake latch, reloads the new
 * weapon's timer, resets the repeat timer, then clears the action request. */
static inline signed char slicks_weapon_finish_request(struct SlicksWeaponControl *state,
    const short inventory[13],signed char selected,const short delays[8],unsigned char *controls)
{
    if(state->request==2) {
        selected=slicks_next_weapon(inventory,selected);
        *controls&=(unsigned char)~2U;
        if(selected>=0) state->cooldown=delays[(unsigned)selected];
        state->repeat_ticks=0;
    }
    state->request=0;
    return selected;
}

/* 21585..215a2: subtract once per update, only from a positive timer.
 * Do not clamp a negative overshoot to zero. */
static inline void slicks_weapon_cooldown(struct SlicksWeaponControl *state,short ticks)
{
    if(state->cooldown>0)
        state->cooldown=(short)((unsigned short)state->cooldown-(unsigned short)ticks);
}

/* 1ebbb..1ed63. Fixed-point coordinates and long differences wrap at 32
 * bits. The original deliberately tests only the signed low word of the
 * shifted absolute distance. ranges includes the byte preceding weapon 0,
 * because selected==-1 is a valid original input (inventory slot 4). */
static inline unsigned char slicks_weapon_ai_request(unsigned char *counter,
    unsigned driver,const int x[4],const int y[4],short heading,
    const signed char roles[4],signed char selected,const short inventory[13],
    unsigned int clock,const signed char direction_x[16],
    const signed char direction_y[16],const signed char ranges[9])
{
    unsigned char previous=*counter;
    *counter=(unsigned char)(previous+1);
    if(previous<=10) return 0;
    *counter=0;
    int px=x[driver],py=y[driver];
    int dx=direction_x[heading/1200]*3,dy=direction_y[heading/1200]*3;
    if(inventory[selected+5]<=1) return 2;
    int range=ranges[selected+1];
    if(range<0) { range=-range; dx=-dx; dy=-dy; }
    for(int step=0;step<range;++step) {
        px=(int)((unsigned int)px+(unsigned int)dx);
        py=(int)((unsigned int)py+(unsigned int)dy);
        for(unsigned other=0;other<4;++other) if(other!=driver && roles[other]) {
            int ax=(int)((unsigned int)px-(unsigned int)x[other])>>7;
            int ay=(int)((unsigned int)py-(unsigned int)y[other])>>7;
            short sx=(short)(ax<0?-ax:ax),sy=(short)(ay<0?-ay:ay);
            if(sx<7 && sy<7) return (clock&(roles[other]<0?1U:15U))?0:1;
        }
    }
    return 0;
}
#endif
