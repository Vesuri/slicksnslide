#ifndef SLICKS_SHOP_DRAW_H
#define SLICKS_SHOP_DRAW_H
#include "shop_menu.h"
#include "player_menu_draw.h"
#include "../game/setup_session.h"

struct SlicksShopDrawOps {
    struct SlicksPlayerMenuDrawOps menu;
    void (*number)(void *,unsigned,short,short,short,unsigned char);
    /* Half-open endpoints, as in original 39ed8. */
    void (*rectangle)(void *,short,short,short,short,unsigned char);
};
struct SlicksShopContent {
    struct SlicksSetupSession *session;
    const struct SlicksShopRules *rules;
    const unsigned char (*items)[15];
    const unsigned char *names[4],*footer,*exit_label,*register_label,*separator;
    unsigned char extra;
    short track,total;
};
struct SlicksShopStaticOps {
    struct SlicksShopDrawOps draw;
    void (*colour)(void *,unsigned,unsigned char,unsigned char,unsigned char);
    void (*tint)(void *,short,short,short,short);
};
/* Original static composition 2ca23..2cef4, excluding palette fade/resource
 * ownership. Tint uses the original half-open bounds. Sprite IDs 0..12 are items;
 * 13..22 are vehicles. Save this composition before drawing changing values. */
static inline int slicks_draw_shop_background(const struct SlicksShopContent *c,
    signed char rows,const struct SlicksShopStaticOps *ops)
{
    const struct SlicksPlayerMenuDrawOps *m=&ops->draw.menu;
    void *p=m->context;
    const struct SlicksSetupSession *s=c->session;
    ops->colour(p,0,55,55,30);ops->colour(p,1,60,60,20);
    m->text(p,1,c->footer,3,193,0);
    ops->draw.number(p,0,c->track,284,3,2);
    ops->draw.number(p,0,c->total,293,3,0);
    m->text(p,0,c->separator,286,3,0);
    ops->colour(p,0,59,59,42);
    for(signed char row=0;row<rows;++row) {
        signed char item=slicks_shop_item(c->rules,&s->options,s->inventory[0],
            s->players.participation[0],s->players.vehicle[0],c->extra,row);
        if(item<0) return -1;
        short y=(short)(40+10*row);
        m->text(p,0,c->items[item],97,y,2);
        if(!c->extra && (c->rules->flags[item]&16)) {
            ops->tint(p,62,(short)(y-2),76,(short)(y+3));
            m->text(p,1,c->register_label,75,(short)(y-2),6);
        }
        m->sprite(p,item,98,y);
        for(unsigned column=0;column<s->players.count;++column)
            ops->tint(p,(short)(115+40*column),(short)(y-1),(short)(150+40*column),(short)(y+7));
    }
    ops->colour(p,1,40,40,75);
    unsigned column=0;
    for(unsigned d=0;d<4;++d) if(s->players.selected[d]>0) {
        short x=(short)(40*column),y=(short)(38-7*(s->players.count-column));
        ops->tint(p,(short)(115+x),y,(short)(117+x),40);
        m->sprite(p,(short)(13+s->players.vehicle[d]),(short)(106+x),y);
        m->text(p,0,c->names[d],(short)(115+x),y,0);
        ++column;
    }
    return 0;
}

/* Complete 2c574: columns pack nonzero selected profiles, independently of
 * their human/computer participation. Refresh selectors are one-based actual
 * driver/row numbers, or -1 for all. Unmatched selection returns -1 rather
 * than the original uninitialized local byte. */
static inline signed char slicks_draw_shop_values(const struct SlicksSetupSession *s,
    const struct SlicksShopRules *rules,unsigned char extra,signed char selected_column,
    signed char selected_row,signed char refresh_driver,signed char refresh_row,
    signed char rows,unsigned char selected_colour,unsigned char bar_colour,
    const unsigned char *exit_label,const struct SlicksShopDrawOps *ops)
{
    const struct SlicksPlayerMenuDrawOps *m=&ops->menu;
    void *c=m->context;
    signed char column=0,selected=-1;
    for(unsigned d=0;d<4;++d) if(s->players.selected[d]) {
        short x=(short)(40*column);
        if(refresh_driver<0 || refresh_driver==(int)d+1) {
            short cash_y=(short)(38-7*(s->players.count-column));
            m->restore(c,0,0,(short)(85+x),cash_y,30,7);
            ops->number(c,1,s->cash[d],(short)(106+x),cash_y,2);
            for(signed char row=0;row<rows;++row) if(refresh_row<0 || refresh_row==row+1) {
                signed char item=slicks_shop_item(rules,&s->options,s->inventory[0],
                    s->players.participation[0],s->players.vehicle[0],extra,row);
                if(item<0) return -1;
                short price=slicks_shop_price(rules,&s->options,s->inventory[d],
                    s->players.participation[d],s->players.vehicle[d],item,extra);
                short y=(short)(38+10*row);
                if(row==selected_row && column==selected_column) {
                    m->restore(c,0,0,(short)(115+x),y,35,5);
                    ops->rectangle(c,(short)(115+x),(short)(y+1),(short)(150+x),(short)(y+9),selected_colour);
                } else m->restore(c,0,0,(short)(114+x),y,37,10);
                short count=s->inventory[d][item];
                if(count>0) ops->number(c,0,count,(short)(138+x),(short)(y+2),1);
                signed char height=(signed char)((short)(count*8)/rules->capacity[item]);
                ops->rectangle(c,(short)(115+x),(short)(y+9-height),(short)(118+x),(short)(y+9),bar_colour);
                if(price>0) {
                    if((rules->flags[item]&2) && count>0) price=(short)(price*rules->batch[item]);
                    ops->number(c,1,(short)(price/10),(short)(114+x),y,0);
                }
            }
        }
        if(column==selected_column) selected=(signed char)d;
        ++column;
    }
    short exit_y=(short)(42+10*rows);
    m->restore(c,0,0,115,exit_y,38,11);
    if(selected_row>=rows) m->bevel(c,115,exit_y,35,10,50,10,10);
    m->text(c,0,exit_label,133,(short)(exit_y+2),1);
    return selected;
}
#endif
