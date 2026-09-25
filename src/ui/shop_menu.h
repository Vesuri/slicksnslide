#ifndef SLICKS_SHOP_MENU_H
#define SLICKS_SHOP_MENU_H
#include "../game/weapon_shop.h"

/* 2c41c uses driver zero to construct the common row list, even when a
 * different driver is selected. Full (zero-price) items remain visible. */
static inline signed char slicks_shop_item(const struct SlicksShopRules *rules,
    const struct SlicksRaceOptions *options,const short inventory[13],signed char role,
    unsigned vehicle,unsigned char extra,signed char row)
{
    signed char visible=0;
    for(unsigned item=0;item<13;++item)
        if(slicks_shop_price(rules,options,inventory,role,vehicle,item,extra)>=0) {
            if(row<=visible) return (signed char)item;
            ++visible;
        }
    return -1;
}

/* 2c967: no wrap, and computers/inactive slots cannot be selected. */
static inline signed char slicks_shop_driver(const signed char roles[4],signed char driver,signed char direction)
{
    int step=direction<0?-1:1;
    for(int i=driver+step;i>=0 && i<4;i+=step) if(roles[i]<0) return (signed char)i;
    return driver;
}
struct SlicksShopMenu { signed char driver,row,count; unsigned char done,end_game; };
enum SlicksShopAction { SLICKS_SHOP_NONE,SLICKS_SHOP_REDRAW,SLICKS_SHOP_BUY,SLICKS_SHOP_SELL,SLICKS_SHOP_HELP,SLICKS_SHOP_CAPTURE };
/* Original key switch 2cff5..2d229. The caller owns transactions and redraw;
 * original display columns are packed participating drivers. */
static inline enum SlicksShopAction slicks_shop_key(struct SlicksShopMenu *m,
    const signed char roles[4],unsigned char key)
{
    if(m->done) return SLICKS_SHOP_NONE;
    switch(key) {
    case 1: case 67: m->done=1; break;
    case 68: m->done=1; m->end_game=1; break;
    case 59: return SLICKS_SHOP_HELP;
    case 70: return SLICKS_SHOP_CAPTURE;
    case 75: m->driver=slicks_shop_driver(roles,m->driver,-1); return SLICKS_SHOP_REDRAW;
    case 77: m->driver=slicks_shop_driver(roles,m->driver,1); return SLICKS_SHOP_REDRAW;
    case 72: if(m->row>0) --m->row; return SLICKS_SHOP_REDRAW;
    case 80: if(m->row<m->count) ++m->row; return SLICKS_SHOP_REDRAW;
    case 28: case 57:
        if(m->row>=m->count) { m->done=1; break; }
        return SLICKS_SHOP_BUY;
    case 14: case 83: if(m->row<m->count) return SLICKS_SHOP_SELL; break;
    }
    return SLICKS_SHOP_NONE;
}
#endif
