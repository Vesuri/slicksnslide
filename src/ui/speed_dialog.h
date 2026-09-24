#ifndef SLICKS_SPEED_DIALOG_H
#define SLICKS_SPEED_DIALOG_H
#include "track_records_renderer.h"
#include "palette_remap.h"
#include "../game/race_timing.h"

/* Original 1e00a: changes are immediate, including on Escape/F1. The
 * caller sets the configuration dirty byte on entry, even without edits. */
struct SlicksSpeedDialog {
    short displayed;
    unsigned char old_colour,background,done;
};
static inline short slicks_speed_dialog_key(struct SlicksSpeedDialog *d,
    short speed,unsigned char key)
{
    int delta=0;
    if(key==1 || key==0x1c || key==0x39 || key==0x3b) d->done=1;
    else if(key==0x4b) delta=-1;
    else if(key==0x4d) delta=1;
    else if(key==0x48) delta=5;
    else if(key==0x50) delta=-5;
    else if(key==0x47 || key==0x4f) speed=100;
    speed=(short)(unsigned short)((unsigned short)speed+delta);
    if(speed<50) speed=50;
    if(speed>200) speed=200;
    return speed;
}
static inline int slicks_speed_dialog_open(struct SlicksSpeedDialog *d,
    struct SlicksRecordsRenderer *r)
{
    if(!d || !r || !r->fonts[0] || !r->text || !r->ui.pixels || !r->ui.palette) return -1;
    *d=(struct SlicksSpeedDialog){0,r->fonts[0][6],0,0};
    r->fonts[0][6]=slicks_ui_nearest(&r->ui,50,50,20);
    d->background=slicks_ui_nearest(&r->ui,20,10,40);
    unsigned char tint[256]; slicks_ui_tint_table(r->ui.palette,tint,20,10,40,80);
    if(slicks_ui_remap(&r->ui,80,45,130,80,tint)) return -1;
    return r->text(r->context,&r->ui,r->fonts[0],(const unsigned char *)"%",114,60,0,0);
}
static inline int slicks_speed_dialog_draw(struct SlicksSpeedDialog *d,
    struct SlicksRecordsRenderer *r,short speed)
{
    if(d->displayed==speed) return 0;
    slicks_ui_rectangle(&r->ui,93,60,113,68,d->background);
    unsigned char number[7]; slicks_records_decimal(speed,number);
    if(r->text(r->context,&r->ui,r->fonts[0],number,113,60,2,0)) return -1;
    d->displayed=speed; return 0;
}
static inline unsigned short slicks_speed_dialog_close(struct SlicksSpeedDialog *d,
    struct SlicksRecordsRenderer *r,short speed)
{
    r->fonts[0][6]=d->old_colour;
    /* Original close calls 37bc2 with a wrapped 16-bit speed*5 value.
     * Interpreting that timer call belongs to the platform integration. */
    return slicks_speed_timer_argument(speed);
}
#endif
