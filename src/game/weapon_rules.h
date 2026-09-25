#ifndef SLICKS_WEAPON_RULES_H
#define SLICKS_WEAPON_RULES_H
struct SlicksWeaponRules {
    short delay[8],damage[8],lifetime[8],unlimited;
    signed char radius[8],force[8],shots[8],spread[8],ranges[9],speed[8],effect[8],muzzle[8];
    unsigned char fire_sound[8],hit_sound[8];
};
#endif
