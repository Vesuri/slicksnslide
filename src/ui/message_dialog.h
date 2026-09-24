#ifndef SLICKS_MESSAGE_DIALOG_H
#define SLICKS_MESSAGE_DIALOG_H
#include "list_renderer.h"

/* Original 347da..3498a, blocking/save-under form (flags=0) used by
 * Clear Top 10s. Allocation and the blocking key read belong to the caller. */
struct SlicksMessageDialog {
    struct SlicksListRenderer painter;
    struct SlicksSavedRectangle original;
    short left,top;
    unsigned char active;
};
static inline int slicks_message_dialog_open(struct SlicksMessageDialog *d,
    const unsigned char *message,short x,short y,unsigned char percent,
    unsigned char *storage,unsigned long capacity)
{
    if(!d || d->active || !message) return -1;
    struct SlicksListRenderer *r=&d->painter;
    if(!r->font || !r->measure || !r->text || !r->ui.pixels || !r->ui.palette) return -1;
    short measured=r->measure(r->context,r->font,message);
    if(measured<0 || measured>306) return -1;
    short half=(short)(measured/2+7),height=(short)(r->font[2]+8);
    short left=(short)(x-half),top=(short)(y-4);
    if(slicks_save_rectangle(&d->original,storage,capacity,&r->ui,left,top,2*half,height)) return -1;
    unsigned char table[256]; slicks_ui_tint_table(r->ui.palette,table,15,15,15,percent);
    if(slicks_ui_remap(&r->ui,left,top,x+half,top+height,table)) return -1;
    unsigned char old=r->font[6]; r->font[6]=slicks_ui_nearest(&r->ui,60,60,30);
    r->text(r->context,&r->ui,r->font,message,x,y,1);
    r->font[6]=old;
    d->left=left; d->top=top; d->active=1;
    return 0;
}
static inline int slicks_message_dialog_close(struct SlicksMessageDialog *d)
{
    if(!d || !d->active) return -1;
    int result=slicks_restore_rectangle(&d->painter.ui,&d->original,d->left,d->top,
        0,0,d->original.width,d->original.height);
    if(!result) d->active=0;
    return result;
}
#endif
