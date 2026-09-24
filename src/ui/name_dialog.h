#ifndef SLICKS_NAME_DIALOG_H
#define SLICKS_NAME_DIALOG_H
#include "list_renderer.h"
#include "text_entry.h"

/* Original profile-name caller 28054 and wrapper 307b6. The two saved
 * rectangles are this dialog's own save-under, not a screen shadow. */
struct SlicksNameDialog {
    struct SlicksListRenderer painter;
    struct SlicksSavedRectangle original,field,cursor;
    struct SlicksTextEntry entry;
    unsigned char *name,*cursor_storage;
    unsigned long cursor_capacity;
    short x,y,cursor_x;
    unsigned short limit,flags;
    unsigned char old_colour,active;
};
static inline int slicks_name_dialog_redraw(struct SlicksNameDialog *d)
{
    struct SlicksListRenderer *r=&d->painter;
    if(slicks_restore_rectangle_visible(&r->ui,&d->field,d->x+1,d->y+2)) return -1;
    short width=r->measure(r->context,r->font,d->name);
    if(width<0 || d->x+1+width+r->font[1]>400) return -1;
    r->text(r->context,&r->ui,r->font,d->name,d->x+1,d->y+2,0);
    d->cursor_x=(short)(d->x+1+width);
    return slicks_save_rectangle_visible(&d->cursor,d->cursor_storage,d->cursor_capacity,&r->ui,
        d->cursor_x,(short)(d->y+1+r->font[2]),r->font[1],1);
}
static inline int slicks_name_dialog_open_field(struct SlicksNameDialog *d,unsigned char *name,
    const unsigned char *caption,short x,short y,unsigned char percent,unsigned limit,unsigned flags,
    unsigned char *original,unsigned long original_size,
    unsigned char *field,unsigned long field_size,unsigned char *cursor,unsigned long cursor_size)
{
    if(!d || d->active || !name || !caption || !cursor || !limit || limit>20 || (flags&256)) return -1;
    struct SlicksListRenderer *r=&d->painter;
    if(!r->font || !r->measure || !r->text || !r->ui.pixels || !r->ui.palette) return -1;
    unsigned n=0; while(n<=limit && name[n]) ++n; if(n>limit) return -1;
    short width=(short)(r->font[1]*limit),height=(short)(r->font[2]+3);
    short caption_width=r->measure(r->context,r->font,caption);
    if(caption_width<0) return -1;
    short saved_width=width+4; if(saved_width<caption_width+7) saved_width=caption_width+7;
    short field_width=(short)((r->font[1]+r->font[3])*limit+2);
    unsigned rounded_field=((unsigned)field_width+3)&~3U;
    if(x<0 || y<r->font[2] || x+1+rounded_field>400 || y+height>200 ||
       rounded_field*r->font[2]>field_size || ((unsigned)r->font[1]+3U)/4U*4U>cursor_size) return -1;
    if(slicks_save_rectangle(&d->original,original,original_size,&r->ui,x,
        (short)(y-r->font[2]),(short)(saved_width-4),(short)(2*height-2))) return -1;
    unsigned char table[256]; slicks_ui_tint_table(r->ui.palette,table,15,15,15,percent);
    if(slicks_ui_remap(&r->ui,x,y-1,x+width,y+height,table)) return -1;
    d->old_colour=r->font[6]; r->font[6]=slicks_ui_nearest(&r->ui,60,50,20);
    r->text(r->context,&r->ui,r->font,caption,x+3,(short)(y-r->font[2]+1),0);
    r->font[6]=slicks_ui_nearest(&r->ui,60,40,60);
    if(slicks_save_rectangle_visible(&d->field,field,field_size,&r->ui,x+1,y+2,field_width,r->font[2])) return -1;
    d->x=x; d->y=y; d->name=name; d->cursor_storage=cursor; d->cursor_capacity=cursor_size;
    d->limit=(unsigned short)limit; d->flags=(unsigned short)flags;
    if(slicks_text_entry_begin(&d->entry,name,limit+1,flags)) return -1;
    d->active=1;
    return slicks_name_dialog_redraw(d);
}
/* Existing profile/list title entry keeps its original 20-byte preserved
 * text mode. Saved-game filenames use limit=8, flags=0x1b, at (100,65). */
static inline int slicks_name_dialog_open(struct SlicksNameDialog *d,unsigned char name[21],
    const unsigned char *caption,short x,short y,unsigned char percent,
    unsigned char *original,unsigned long original_size,
    unsigned char *field,unsigned long field_size,unsigned char *cursor,unsigned long cursor_size)
{
    return slicks_name_dialog_open_field(d,name,caption,x,y,percent,20,0x203,
        original,original_size,field,field_size,cursor,cursor_size);
}
static inline int slicks_name_dialog_cursor(struct SlicksNameDialog *d,unsigned visible)
{
    if(!d || !d->active) return -1;
    struct SlicksListRenderer *r=&d->painter;
    short y=(short)(d->y+1+r->font[2]);
    if(slicks_restore_rectangle_visible(&r->ui,&d->cursor,d->cursor_x,y)) return -1;
    if(visible) slicks_ui_rectangle(&r->ui,d->cursor_x,y,(short)(d->cursor_x+r->font[1]),y+1,r->font[6]);
    return 0;
}
/* Return 0 continuing, 1 accepted, 2 cancelled, -1 native bounds failure. */
static inline int slicks_name_dialog_key(struct SlicksNameDialog *d,unsigned char character)
{
    if(!d || !d->active || slicks_name_dialog_cursor(d,0)) return -1;
    unsigned result=slicks_text_entry_key(&d->entry,d->name,d->limit,d->flags,d->painter.font,character);
    if(!result && slicks_name_dialog_redraw(d)) return -1;
    return (int)result;
}
static inline int slicks_name_dialog_close(struct SlicksNameDialog *d)
{
    if(!d || !d->active) return -1;
    d->painter.font[6]=d->old_colour; d->active=0;
    return slicks_restore_rectangle(&d->painter.ui,&d->original,d->x,
        (short)(d->y-d->painter.font[2]),0,0,d->original.width,d->original.height);
}
#endif
