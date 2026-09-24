#ifndef SLICKS_RACE_OPTIONS_H
#define SLICKS_RACE_OPTIONS_H
#include "configuration.h"

struct SlicksRaceOptions {
    short weapons_enabled, inventory_mode, fuel, damage, car_collisions; /* DS:3020/3028 */
    short starting_cash, field_302c, field_302e, field_3030;
};

/* 2bdd8..2bf38. For non-custom modes the caller supplies the original
 * DS:1157[game_type] byte. Preserve the bit masks (not normalized booleans).
 * Names stay address-based where the downstream meaning is not yet proved. */
static inline void slicks_resolve_race_options(struct SlicksRaceOptions *out,
    const struct SlicksConfiguration *config,unsigned char mode_flags)
{
    if(config->options[0]==4) {
        out->weapons_enabled=config->options[7];
        out->inventory_mode=config->options[8];
        out->fuel=config->options[9]>5?config->options[9]:0;
        out->damage=config->options[10];
        out->car_collisions=config->options[11];
        out->starting_cash=config->options[4];
        out->field_302c=config->options[5];
        out->field_302e=config->options[6];
        out->field_3030=config->options[12];
    } else {
        out->weapons_enabled=mode_flags&2;
        out->inventory_mode=mode_flags&1;
        out->fuel=0;
        out->damage=(mode_flags&8)*12;
        out->car_collisions=1;
        out->starting_cash=200;
        out->field_302c=50;
        out->field_302e=20;
        out->field_3030=0;
    }
}
/* Original 22d2c..22d36: a word-zero test, not a signed-positive test. */
static inline unsigned char slicks_car_collisions_disabled(short value)
{ return (unsigned char)(value==0); }
#endif
