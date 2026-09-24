#ifndef SLICKS_PLAYER_MENU_DRAW_H
#define SLICKS_PLAYER_MENU_DRAW_H
#include "../game/player_profiles.h"

struct SlicksPlayerMenuDrawOps {
    void (*restore)(void *,short,short,short,short,short,short);
    void (*bevel)(void *,short,short,short,short,unsigned char,unsigned char,unsigned char);
    void (*sprite)(void *,short,short,short); /* -1 computer, otherwise vehicle */
    void (*text)(void *,unsigned,const unsigned char *,short,short,unsigned char); /* 0 DS:680 font, 1 DS:688 */
    void *context;
};
struct SlicksPlayerMenuLabels {
    const unsigned char *random_vehicle,*random_each_race,*actions[4];
};

/* Redraw stage 286ab..28981; profile selection/property refresh is separate.
 * Preserve original subrect arguments (the renderer handles VGA byte rounding)
 * and exact ordering. Reject invalid profile/sprite indices before drawing. */
static inline int slicks_draw_player_menu(unsigned row,const short selected[4],
    const signed char participation[4],const struct SlicksPlayerProfiles *profiles,
    short vehicle_count,const struct SlicksPlayerMenuLabels *labels,
    const struct SlicksPlayerMenuDrawOps *ops)
{
    if(row>7 || profiles->count<1 || profiles->count>100 || vehicle_count<0) return -1;
    for(unsigned i=0;i<4;++i) if(selected[i]>0) {
        /* Original override setup may select slots 1/2 after its count check,
         * even following deletion below three records. Storage remains 100
         * slots; drawing must not invent a second logical-count clamp. */
        if(selected[i]>=SLICKS_PROFILE_MAX || (signed char)profiles->setup[selected[i]].vehicle<0) return -1;
        unsigned n=0; while(n<21 && profiles->names[selected[i]][n]) ++n;
        if(n==21) return -1;
    }
    ops->restore(ops->context,0,0,40,28,190,72);
    ops->restore(ops->context,0,0,35,105,100,150);
    if(row<4) ops->bevel(ops->context,43,(short)(28+16*row),167,11,50,10,10);
    else ops->bevel(ops->context,52,(short)(107+12*(row-4)),65,11,50,10,10);
    for(unsigned i=0;i<4;++i) if(selected[i]>0) {
        unsigned index=(unsigned)selected[i]; short x=50,y=(short)(30+16*i);
        if(participation[i]>0) { ops->sprite(ops->context,-1,49,(short)(29+16*i)); x=65; }
        ops->text(ops->context,1,profiles->names[index],x,y,0);
        short vehicle=(signed char)profiles->setup[index].vehicle;
        if(vehicle<vehicle_count) ops->sprite(ops->context,vehicle,215,(short)(31+16*i));
        else if(vehicle==vehicle_count) ops->text(ops->context,1,labels->random_vehicle,215,y,0);
        else if(vehicle==(short)(unsigned short)(vehicle_count+1)) ops->text(ops->context,1,labels->random_each_race,215,y,0);
    }
    for(unsigned i=0;i<4;++i) ops->text(ops->context,0,labels->actions[i],85,(short)(110+12*i),1);
    return 0;
}
#endif
