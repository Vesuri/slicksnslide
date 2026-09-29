#ifndef SLICKS_LIST_RENDERER_H
#define SLICKS_LIST_RENDERER_H
#include "list_dialog_draw.h"
#include "list_captions.h"
#include "saved_rectangle.h"
#include "palette_remap.h"

/* Caller owns the buffers/font/resources throughout this modal lifetime. */
struct SlicksListRenderer {
    struct SlicksChunkyUi ui;
    struct SlicksListDialog state;
    struct SlicksListPulse pulse;
    struct SlicksListCaptionLayout captions;
    struct SlicksSavedRectangle original,tinted,caption_saved;
    unsigned char *font;
    const unsigned char *names;
    short left,top,right,bottom,thumb,stride;
    short scrollbar_top,scrollbar_bottom;
    unsigned char colours[3],old_colour,active;
    short (*measure)(void *,const unsigned char *,const unsigned char *);
    void (*text)(void *,struct SlicksChunkyUi *,unsigned char *,const unsigned char *,short,short,unsigned char);
    void *context;
};

/* Optional platform save-under: preserve the original signed scrollbar
 * arithmetic, including wrap above the window, but do not leak its pixels
 * into the underlying menu on close. Only four columns need extra storage.
 * Capture before opening; restore after normal dialog close. */
static inline int slicks_list_save_scrollbar(struct SlicksListRenderer *r,
    struct SlicksSavedRectangle *saved,unsigned char *storage,unsigned long capacity)
{
    r->scrollbar_top=r->top;
    r->scrollbar_bottom=r->bottom+2;
    if(r->scrollbar_bottom>200) r->scrollbar_bottom=200;
    return slicks_save_rectangle(saved,storage,capacity,&r->ui,r->right-6,0,4,200);
}
static inline int slicks_list_restore_scrollbar(struct SlicksListRenderer *r,
    const struct SlicksSavedRectangle *saved)
{
    return slicks_restore_rectangle(&r->ui,saved,r->right-6,0,0,
        r->scrollbar_top,4,r->scrollbar_bottom-r->scrollbar_top);
}
static inline short slicks_list_measure(void *p,const unsigned char *s)
{ struct SlicksListRenderer *r=p; return r->measure(r->context,r->font,s); }
static inline unsigned char slicks_list_colour(void *p,unsigned char colour)
{ struct SlicksListRenderer *r=p; unsigned char old=r->font[6]; r->font[6]=colour; return old; }
static inline unsigned char slicks_list_nearest(void *p,unsigned char red,unsigned char green,unsigned char blue)
{ struct SlicksListRenderer *r=p; return slicks_ui_nearest(&r->ui,red,green,blue); }
static inline void slicks_list_text(void *p,const unsigned char *s,short x,short y,unsigned char flags)
{ struct SlicksListRenderer *r=p; r->text(r->context,&r->ui,r->font,s,x,y,flags); }
static inline void slicks_list_restore(void *p,short x,short y,short sx,short sy,short w,short h)
{ struct SlicksListRenderer *r=p; (void)slicks_restore_rectangle(&r->ui,&r->tinted,x,y,sx,sy,w,h); }
static inline void slicks_list_rectangle(void *p,short l,short t,short right,short b,unsigned char colour)
{
    struct SlicksListRenderer *r=p;
    /* Original signed thumb arithmetic can paint above the dialog. Keep
     * those outlying rows without restoring the entire 200-row strip. */
    if(l<r->right-2 && right>r->right-6 && l<right && t<b) {
        if(t<0) t=0;
        if(b>200) b=200;
        if(t<b) {
            if(t<r->scrollbar_top) r->scrollbar_top=t;
            if(b>r->scrollbar_bottom) r->scrollbar_bottom=b;
        }
    }
    slicks_ui_rectangle(&r->ui,l,t,right,b,colour);
}
static inline void slicks_list_row_text(void *p,short index,short x,short y)
{ struct SlicksListRenderer *r=p; slicks_list_text(r,r->names+(unsigned)index*r->stride,x,y,0); }

/* Preparation order from 30cc4..31155. Allocation is outside hardware
 * takeover; preflight all storage/geometry before changing the screen. */
static inline int slicks_list_renderer_open(struct SlicksListRenderer *r,
    short selected,short count,const unsigned char *captions,unsigned char percent,
    unsigned char flags,unsigned char *original,unsigned long original_size,
    unsigned char *tinted,unsigned long tinted_size,unsigned char *caption,unsigned long caption_size)
{
    if(!r || r->active || !r->font || !r->names || !r->measure || !r->text ||
       !r->ui.pixels || !r->ui.palette || r->stride<=0 || r->stride>127 ||
       count<0 || count>4095 || r->left<0 || r->top<0 || r->right>320 || r->bottom>200 ||
       r->right-r->left<16 || r->bottom-r->top<r->font[2]+12) return -1;
    for(short i=0;i<count;++i) {
        unsigned j=0; while(j<(unsigned)r->stride && r->names[(unsigned)i*r->stride+j]) ++j;
        if(j==(unsigned)r->stride) return -1;
    }
    struct SlicksListCaptionOps ops={slicks_list_measure,slicks_list_colour,slicks_list_nearest,slicks_list_text,r};
    unsigned char action=selected<0?(unsigned char)(-1-selected):0;
    if(slicks_list_caption_layout(&r->captions,captions,r->font[2],action,&ops) ||
       slicks_list_dialog_init(&r->state,selected,count,r->bottom-r->top,r->font[2],
           r->captions.count,r->captions.action,flags,&r->thumb)) return -1;
    unsigned width=((unsigned)(r->right-r->left)+3)&~3U;
    unsigned height=(unsigned)(r->bottom-r->top);
    short cx=(short)(r->left-r->captions.width-4),cy=(short)(r->top+1);
    unsigned cw=((unsigned)r->captions.width+10)&~3U,ch=(unsigned)r->captions.height+3;
    if(!original || !tinted || original_size<width*height || tinted_size<width*(height-2) ||
       r->left+width>320 || (captions[0] && (!caption || cx<0 || cx+cw>320 || cy+ch>200 || caption_size<cw*ch))) return -1;
    unsigned char table[256];
    if(captions[0]) {
        if(slicks_save_rectangle(&r->caption_saved,caption,caption_size,&r->ui,cx,cy,
            r->captions.width+7,r->captions.height+3)) return -1;
        slicks_ui_tint_table(r->ui.palette,table,7,7,7,percent);
        slicks_ui_remap(&r->ui,cx,cy,r->left+2,r->top+r->captions.height+4,table);
        unsigned char edge=slicks_ui_nearest(&r->ui,40,40,30);
        slicks_ui_rectangle(&r->ui,cx,cy,r->left,r->top+2,edge);
        slicks_ui_rectangle(&r->ui,cx,r->top+r->captions.height+3,r->left,r->top+r->captions.height+4,edge);
        slicks_list_captions(r->captions.labels,r->left,r->top,r->font[2],r->captions.action,40,&ops);
    } else r->caption_saved=(struct SlicksSavedRectangle){0,0,0};
    r->old_colour=slicks_list_colour(r,slicks_ui_nearest(&r->ui,50,50,60));
    r->colours[0]=slicks_ui_nearest(&r->ui,50,20,20);
    r->colours[1]=slicks_ui_nearest(&r->ui,60,30,30);
    r->colours[2]=slicks_ui_nearest(&r->ui,45,10,10);
    if(slicks_save_rectangle(&r->original,original,original_size,&r->ui,r->left,r->top,r->right-r->left,(short)height)) return -1;
    slicks_ui_tint_table(r->ui.palette,table,10,10,10,percent);
    slicks_ui_remap(&r->ui,r->left,r->top,r->right,r->bottom,table);
    if(count>r->state.visible) {
        slicks_ui_tint_table(r->ui.palette,table,35,35,35,percent);
        slicks_ui_remap(&r->ui,r->right-6,r->top+4,r->right-2,r->bottom-7,table);
    }
    if(slicks_save_rectangle(&r->tinted,tinted,tinted_size,&r->ui,r->left,r->top,r->right-r->left,(short)(height-2))) return -1;
    r->pulse=(struct SlicksListPulse){0,7}; r->active=1;
    return 0;
}

static inline void slicks_list_renderer_draw(struct SlicksListRenderer *r,unsigned long tick)
{
    if(!r->active || r->state.done) return;
    slicks_list_dialog_normalize(&r->state);
    unsigned char colour=slicks_list_dialog_pulse(&r->state,&r->pulse,tick,slicks_list_nearest,r);
    if(r->state.redraw_actions || r->state.focus_actions) {
        struct SlicksListCaptionOps captions={slicks_list_measure,slicks_list_colour,slicks_list_nearest,slicks_list_text,r};
        slicks_list_captions(r->captions.labels,r->left,r->top,r->font[2],r->state.action,colour,&captions);
        r->state.redraw_actions=0;
    }
    struct SlicksListDialogDrawOps ops={slicks_list_restore,slicks_list_rectangle,slicks_list_row_text,r};
    if(r->state.redraw_list) {
        slicks_list_dialog_draw_rows(&r->state,r->left,r->top,r->right,r->font[2],r->colours,&ops);
        slicks_list_dialog_draw_scrollbar(&r->state,r->left,r->top,r->right,r->bottom,r->thumb,r->colours,&ops);
    } else slicks_list_dialog_draw_active(&r->state,r->left,r->top,r->font[2],colour,slicks_list_colour,&ops);
}
static inline short slicks_list_renderer_close(struct SlicksListRenderer *r)
{
    if(!r->active) return -1;
    if(!(r->state.flags&64)) slicks_restore_rectangle(&r->ui,&r->original,r->left,r->top,0,0,r->original.width,r->original.height);
    if(r->caption_saved.pixels) slicks_restore_rectangle(&r->ui,&r->caption_saved,
        r->left-r->captions.width-4,r->top+1,0,0,r->caption_saved.width,r->caption_saved.height);
    r->font[6]=r->old_colour; r->active=0;
    return slicks_list_dialog_result(&r->state);
}
#endif
