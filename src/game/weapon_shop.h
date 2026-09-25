#ifndef SLICKS_WEAPON_SHOP_H
#define SLICKS_WEAPON_SHOP_H
#include "race_options.h"

struct SlicksShopRules {
    unsigned char flags[13];
    signed char capacity[13],batch[13],vehicle_capacity[10],weapon_weight[8];
    short base_price[13],ammunition_price[13];
};

/* 26703..26784: weapon slots consume vehicle carrying capacity even with
 * sentinel ammunition count one; also include the prospective purchase. */
static inline short slicks_shop_capacity(const struct SlicksShopRules *r,
    const short inventory[13],unsigned vehicle,unsigned item)
{
    short available=r->vehicle_capacity[vehicle]; unsigned weapon=0;
    for(unsigned i=0;i<13;++i) if(r->flags[i]&2) {
        if(inventory[i]>0 || i==item) available=(short)(available-r->weapon_weight[weapon]);
        ++weapon;
    }
    return available<0?0:available;
}

/* 2aad6..2ac52: zero means unavailable/full, -1 hidden by setup options.
 * Prices remain in the original tenths until the transaction rounds them. */
static inline short slicks_shop_price(const struct SlicksShopRules *r,
    const struct SlicksRaceOptions *options,const short inventory[13],
    signed char role,unsigned vehicle,unsigned item,unsigned char extra_enabled)
{
    if(!role) return -1;
    if(inventory[item]>=r->capacity[item]) return 0;
    unsigned flags=r->flags[item];
    if(((flags&1) && !options->inventory_mode) ||
       ((flags&2) && !options->weapons_enabled) ||
       ((flags&4) && !options->damage) || ((flags&8) && !options->fuel)) return -1;
    if((flags&16) && !extra_enabled) return 0;
    if(flags&1) {
        int price=r->base_price[item];
        for(int i=0;i<inventory[item];++i) {
            price=(int)((unsigned int)price*108U)/100;
            if(price>32000) price=32000;
        }
        return (short)price;
    }
    if(flags&2) {
        signed char available=(signed char)slicks_shop_capacity(r,inventory,vehicle,item);
        if(!inventory[item]) return available>0?r->base_price[item]:0;
        return r->ammunition_price[item];
    }
    return -1;
}

/* 2d0b1..2d181. A first purchase buys the weapon (count one), not its
 * ammunition batch. Each subsequent unit recomputes price and affordability;
 * insufficient cash/capacity can therefore buy only part of a batch. */
static inline unsigned slicks_shop_buy(const struct SlicksShopRules *r,
    const struct SlicksRaceOptions *options,short inventory[13],short *cash,
    signed char role,unsigned vehicle,unsigned item,unsigned char extra_enabled)
{
    unsigned bought=0;
    for(int i=0;i<r->batch[item];++i) {
        short cost=slicks_shop_price(r,options,inventory,role,vehicle,item,extra_enabled)/10;
        if(cost>0 && *cash>=cost) {
            *cash=(short)(*cash-cost);
            inventory[item]=(short)((unsigned short)inventory[item]+1U);
            ++bought;
            if((r->flags[item]&2) && inventory[item]==1) break;
        }
    }
    return bought;
}

/* 2d184..2d21a: decrement first, then price the remaining level and return
 * half its buying price. Selling is one unit even for ammunition batches. */
static inline unsigned slicks_shop_sell(const struct SlicksShopRules *r,
    const struct SlicksRaceOptions *options,short inventory[13],short *cash,
    signed char role,unsigned vehicle,unsigned item,unsigned char extra_enabled)
{
    if(inventory[item]<=0) return 0;
    --inventory[item];
    short refund=slicks_shop_price(r,options,inventory,role,vehicle,item,extra_enabled)/20;
    *cash=(short)((unsigned short)*cash+(unsigned short)refund);
    return 1;
}

/* 2c45c..2c573. The executable initializes the signed minimum price to -1,
 * then updates it only if greater than a positive price. Consequently it
 * stays -1, is changed to zero, and prevents another shopping iteration.
 * Preserve the observed single random attempt per computer, not an imagined
 * spend-all-cash loop. Failed attempts still consume the shared RNG draw. */
static inline void slicks_shop_computers(const struct SlicksShopRules *r,
    const struct SlicksRaceOptions *options,short inventory[4][13],short cash[4],
    const signed char roles[4],const signed char vehicles[4],unsigned char extra_enabled,
    unsigned long *random_state)
{
    for(unsigned driver=0;driver<4;++driver) if(roles[driver]>0) {
        *random_state=(*random_state*0x015a4e35UL+1UL)&0xffffffffUL;
        unsigned item=(unsigned)(((*random_state>>16)&0x7fffUL)*15UL/32768UL);
        if(item>=13) item=1;
        short price=slicks_shop_price(r,options,inventory[driver],roles[driver],
            (unsigned)vehicles[driver],item,extra_enabled)/10;
        if(price && cash[driver]>=price) {
            inventory[driver][item]=(short)((unsigned short)inventory[driver][item]+1U);
            cash[driver]=(short)(cash[driver]-price);
        }
    }
}
#endif
