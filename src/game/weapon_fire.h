#ifndef SLICKS_WEAPON_FIRE_H
#define SLICKS_WEAPON_FIRE_H
#include "weapon_actions.h"
#include "weapon_projectile.h"
#include "weapon_rules.h"

/* Resource allocation and sound playback are explicit platform boundaries.
 * Allocation returns the original shared actor handle, including zero on
 * exhaustion. The shot state and ammunition still advance in that case. */
struct SlicksWeaponFireOps {
    void (*flash)(void *,unsigned,signed char);
    void (*sound)(void *,unsigned char,unsigned char,unsigned char);
    short (*allocate)(void *,signed char);
    void *context;
};

/* 206a5..20c6f, in original call/RNG order. The sound and optional flash
 * precede the ammunition check. A full local projectile array suppresses
 * allocation, RNG and timer reload, but not ammunition consumption. */
static inline signed char slicks_weapon_fire(
    struct SlicksWeaponControl *control,short inventory[13],signed char selected,
    unsigned char *controls,unsigned driver,unsigned int clock,
    struct SlicksWeaponProjectile projectiles[30],const int x[4],const int y[4],
    const signed char roles[4],short heading,signed char layer,
    const signed char dirx[16],const signed char diry[16],
    unsigned long *random_state,const struct SlicksWeaponRules *rules,
    const struct SlicksWeaponFireOps *ops,short *last_slot)
{
    if(slicks_weapon_can_fire(control,selected,clock)) {
        unsigned type=(unsigned)selected;
        if(rules->muzzle[type]) ops->flash(ops->context,driver,rules->muzzle[type]);
        ops->sound(ops->context,rules->fire_sound[type],2,14);
        if(inventory[type+5]>1) {
            for(int shot=0;shot<rules->shots[type];++shot) {
                unsigned slot=slicks_weapon_free_slot(projectiles);
                if(last_slot) *last_slot=(short)slot;
                if(!slot) continue;
                struct SlicksWeaponProjectile *p=&projectiles[slot];
                p->handle=ops->allocate(ops->context,selected);
                slicks_weapon_projectile_init(p,driver,x,y,roles,heading,selected,
                    layer,rules->lifetime[type],rules->speed[type],rules->spread[type],
                    dirx,diry,random_state);
                control->cooldown=rules->delay[type];
            }
            slicks_weapon_consume(control,inventory,selected,rules->unlimited);
        }
    }
    return slicks_weapon_finish_request(control,inventory,selected,rules->delay,controls);
}
#endif
