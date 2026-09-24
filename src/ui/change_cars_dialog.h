#ifndef SLICKS_CHANGE_CARS_DIALOG_H
#define SLICKS_CHANGE_CARS_DIALOG_H
#include "../game/profile_setup.h"

struct SlicksChangeCarsDialog { signed char row; unsigned char done; };

/* 2480d..24859, before allocation: only the signed profile selector exactly
 * vehicle_count+2 is refreshed here. This is NOT the ordinary >=count+1
 * random-per-race rule used by profile selection. Preserve that distinction
 * and perform these RNG draws even if the subsequent dialog allocation fails.
 * The caller supplies validated selected-profile indices. */
static inline signed char slicks_change_cars_prepare(
    struct SlicksProfileSelection *players,const struct SlicksSetupProfile *profiles,
    short vehicle_count,const unsigned char *weights,unsigned long *random_state,
    signed char show)
{
    short special=(short)(unsigned short)((unsigned short)vehicle_count+2);
    for(unsigned slot=0;slot<4;++slot)
        if(players->participation[slot] &&
           (signed char)profiles[players->selected[slot]].vehicle==special) {
            players->vehicle[slot]=(signed char)slicks_choose_profile_vehicle(
                weights,vehicle_count,random_state);
            show=1;
        }
    return show;
}

/* 24997..24a51: visible rows pack participating slots in slot order, not
 * race starting order. Both human and computer roles are editable. */
static inline int slicks_change_cars_driver(const signed char roles[4],int row)
{
    for(int slot=0;slot<4;++slot)
        if(roles[slot] && row--==0) return slot;
    return -1;
}

struct SlicksChangeCarsDrawOps {
    void (*restore)(void *,short,short,short,short,short,short);
    void (*bevel)(void *,short,short,short,short,unsigned char,unsigned char,unsigned char);
    void (*sprite)(void *,signed char,short,short);
    void *context;
};

/* Original 24992..24a51. Restore from the dialog's decorated save-under,
 * then highlight the current row and paint the current vehicle icon. */
static inline int slicks_change_cars_draw(const struct SlicksChangeCarsDialog *d,
    const signed char roles[4],const signed char vehicles[4],short x,short y,
    const struct SlicksChangeCarsDrawOps *ops)
{
    int row=0,selected=-1;
    for(int slot=0;slot<4;++slot) if(roles[slot]) {
        short offset=(short)(row*15);
        ops->restore(ops->context,x,y,0,(short)(offset+6),40,20);
        if(row==d->row) {
            ops->bevel(ops->context,(short)(x+2),(short)(y+offset+6),36,12,50,10,10);
            selected=slot;
        }
        ops->sprite(ops->context,vehicles[slot],(short)(x+15),(short)(y+offset+8));
        ++row;
    }
    return selected;
}

/* 24a69..24af0. Changes affect the session's current vehicles immediately,
 * not the saved profile vehicle selector. Escape accepts the current values;
 * unlike the profile editor this dialog has no rollback operation. */
static inline int slicks_change_cars_key(struct SlicksChangeCarsDialog *d,
    const signed char roles[4],signed char vehicles[4],short vehicle_count,
    unsigned char key)
{
    int driver=slicks_change_cars_driver(roles,d->row),count=0;
    if(driver<0 || vehicle_count<1) return -1;
    for(int i=0;i<4;++i) if(roles[i]) ++count;
    switch(key) {
    case 1: case 28: case 67: case 68: d->done=1; break;
    case 57: case 77:
        vehicles[driver]=(signed char)((unsigned char)vehicles[driver]+1);
        if(vehicles[driver]>=vehicle_count) vehicles[driver]=0;
        break;
    case 75:
        vehicles[driver]=(signed char)((unsigned char)vehicles[driver]-1);
        if(vehicles[driver]<0) vehicles[driver]=(signed char)(vehicle_count-1);
        break;
    case 72: if(d->row>0) --d->row; break;
    case 80: if(d->row<count-1) ++d->row; break;
    }
    return 0;
}
#endif
