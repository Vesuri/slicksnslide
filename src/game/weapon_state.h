#ifndef SLICKS_WEAPON_STATE_H
#define SLICKS_WEAPON_STATE_H

/* 26372..263e6: new-game initialization, not a per-race reset. Saved games
 * restore their own inventories. DS:3022 gates the initial item grants;
 * only bit zero of each DS:106f item flag participates. */
static inline void slicks_new_game_inventory(short inventory[4][13],
    short cash[4], const unsigned char item_flags[13],
    short setup_mode, short starting_cash)
{
    for(unsigned driver=0;driver<4;++driver) {
        cash[driver]=starting_cash;
        for(unsigned slot=0;slot<13;++slot)
            inventory[driver][slot]=!setup_mode && (item_flags[slot]&1) ? 4 : 0;
    }
}

/* 1eb49..1ebba: slots 0..4 are upgrades/fuel, slots 5..12 weapons.
 * An inventory value of one is not selectable. The original cycles forward,
 * then wraps through the current slot inclusively; -1 means none available.
 * Return -2 for negative indices outside the valid -1/no-selection state,
 * rather than reproduce an out-of-array original read. */
static inline signed char slicks_next_weapon(const short inventory[13],
                                             signed char current)
{
    if(current < -1) return -2;
    if(current>=8) current=0;
    for(int next=current+1;next<8;++next)
        if(inventory[next+5]>1) return (signed char)next;
    for(int next=0;next<=current;++next)
        if(inventory[next+5]>1) return (signed char)next;
    return -1;
}
/* Original 1fd72..1fdc6: humans start at the first available weapon;
 * computers consume one shared random draw even when no weapon is owned.
 * The caller's Weapons option does not gate this initialization. */
static inline signed char slicks_initial_weapon(const short inventory[13],signed char role,
    unsigned long *random_state)
{
    signed char first=slicks_next_weapon(inventory,-1);
    if(role<=0) return first;
    *random_state=(*random_state*0x015a4e35UL+1UL)&0xffffffffUL;
    unsigned long random=(*random_state>>16)&0x7fffUL;
    return slicks_next_weapon(inventory,(signed char)((random*6UL)/32768UL));
}
#endif
