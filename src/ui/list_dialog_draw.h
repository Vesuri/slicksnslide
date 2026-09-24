#ifndef SLICKS_LIST_DIALOG_DRAW_H
#define SLICKS_LIST_DIALOG_DRAW_H
#include "list_dialog.h"

struct SlicksListDialogDrawOps {
    void (*restore)(void *,short,short,short,short,short,short);
    void (*rectangle)(void *,short,short,short,short,unsigned char);
    void (*text)(void *,short,short,short); /* profile index, x, y */
    void *context;
};

/* Original list-row redraw 3125b..313dc. Restore uses the dialog's saved
 * tinted background, not the full player-menu background. The three palette
 * indices come from the original dialog preparation. Scrollbar/action text
 * and pulse repaint run separately, in their original caller order. */
static inline void slicks_list_dialog_draw_rows(const struct SlicksListDialog *s,
    short left,short top,short right,unsigned char font_height,
    const unsigned char colours[3],const struct SlicksListDialogDrawOps *ops)
{
    if(!s->redraw_list) return;
    for(short row=0;row<s->visible;++row) {
        short index=s->top+row;
        short changed=(signed char)s->redraw_list;
        if(changed>=0 && changed!=index && changed-1!=index) continue;
        short y=(short)(top+4+(font_height+2)*row);
        ops->restore(ops->context,left,top,0,y-top,right-left,font_height+2);
        if(index>=s->count) continue;
        if(index==s->selected) {
            ops->rectangle(ops->context,left+7,y+2,right-6,y+font_height+1,colours[0]);
            ops->rectangle(ops->context,left+7,y,right-6,y+2,colours[1]);
            ops->rectangle(ops->context,left+7,y+font_height,right-6,y+font_height+2,colours[2]);
        }
        ops->text(ops->context,index,left+10,y+1);
    }
}

/* Original 313dc..3152d, after the row pass. Native geometry/counts were
 * validated at initialization; no thumb is drawn when all rows fit. */
static inline void slicks_list_dialog_draw_scrollbar(struct SlicksListDialog *s,
    short left,short top,short right,short bottom,short thumb,
    const unsigned char colours[3],const struct SlicksListDialogDrawOps *ops)
{
    if(!s->redraw_list) return;
    if(s->count>s->visible) {
        ops->restore(ops->context,left,top,right-left-6,5,4,
            (unsigned char)(bottom-top-10));
        short y=(short)((short)((bottom-top-10)*s->top)/s->count+top+4);
        ops->rectangle(ops->context,right-6,y,right-2,y+1,colours[1]);
        ops->rectangle(ops->context,right-6,y,right-5,y+thumb,colours[1]);
        ops->rectangle(ops->context,right-6,y+thumb,right-2,y+thumb+1,colours[2]);
        ops->rectangle(ops->context,right-3,y,right-2,y+thumb,colours[2]);
        ops->rectangle(ops->context,right-5,y+1,right-3,y+thumb,colours[0]);
    }
    s->redraw_list=0;
}

/* 3152f..315a7: only on an update without a list redraw, and only while
 * list focus is active. Do not repaint over the freshly restored row pass.
 * Font colour is restored even though the selected text remains highlighted. */
static inline void slicks_list_dialog_draw_active(const struct SlicksListDialog *s,
    short left,short top,unsigned char font_height,unsigned char highlight,
    unsigned char (*colour)(void *,unsigned char),const struct SlicksListDialogDrawOps *ops)
{
    if(s->redraw_list || s->focus_actions) return;
    unsigned char previous=colour(ops->context,highlight);
    ops->text(ops->context,s->selected,left+10,
        (short)(top+5+(font_height+2)*(s->selected-s->top)));
    colour(ops->context,previous);
}
#endif
