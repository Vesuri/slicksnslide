#ifndef SLICKS_COLOUR_DIALOG_H
#define SLICKS_COLOUR_DIALOG_H
#include "list_renderer.h"
#include "colour_picker.h"

/* Original 2f290..2f6d9, composed with the verified RGB controls/painter.
 * The profile owns destination; only an accepted close writes its bytes. */
struct SlicksColourDialog {
    struct SlicksListRenderer painter;
    struct SlicksSavedRectangle saved;
    struct SlicksColourPicker state;
    struct SlicksColourPickerPulse pulse;
    unsigned char *destination;
    unsigned char bars[3],unselected,active;
    short x,y;
};
static inline int slicks_colour_dialog_open(struct SlicksColourDialog *d,unsigned char rgb[3],
    const unsigned char *caption,short x,short y,unsigned char *storage,unsigned long capacity)
{
    if(!d || d->active || !rgb || !caption) return -1;
    struct SlicksListRenderer *r=&d->painter;
    if(!r->font || !r->text || !r->ui.pixels || !r->ui.palette) return -1;
    if(slicks_save_rectangle(&d->saved,storage,capacity,&r->ui,x,y,80,30)) return -1;
    unsigned char table[256]; slicks_ui_tint_table(r->ui.palette,table,15,15,40,75);
    if(slicks_ui_remap(&r->ui,x,y+2,x+77,y+30,table)) return -1;
    unsigned char previous=r->font[6]; r->font[6]=slicks_ui_nearest(&r->ui,60,50,20);
    r->text(r->context,&r->ui,r->font,caption,x+3,y,0);
    r->font[6]=previous;
    slicks_colour_picker_begin(&d->state,rgb);
    d->bars[0]=slicks_ui_nearest(&r->ui,70,20,20);
    d->bars[1]=slicks_ui_nearest(&r->ui,20,70,20);
    d->bars[2]=slicks_ui_nearest(&r->ui,20,20,70);
    d->unselected=slicks_ui_nearest(&r->ui,10,10,25);
    d->pulse=(struct SlicksColourPickerPulse){0,0};
    d->destination=rgb; d->x=x; d->y=y; d->active=1;
    return 0;
}
static inline int slicks_colour_dialog_draw(struct SlicksColourDialog *d,unsigned long tick)
{
    if(!d || !d->active) return -1;
    struct SlicksColourPickerDrawOps ops={slicks_ui_nearest,slicks_ui_rectangle,&d->painter.ui};
    slicks_colour_picker_draw(&d->state,&d->pulse,tick,d->x,d->y,d->bars,d->unselected,&ops);
    return 0;
}
static inline int slicks_colour_dialog_close(struct SlicksColourDialog *d)
{
    if(!d || !d->active || !d->state.result) return -1;
    if(slicks_restore_rectangle(&d->painter.ui,&d->saved,d->x,d->y,0,0,80,30)) return -1;
    d->active=0;
    return (int)slicks_colour_picker_finish(&d->state,d->destination);
}
#endif
