#ifndef SLICKS_SAVED_RECTANGLE_H
#define SLICKS_SAVED_RECTANGLE_H
#include "chunky_ui.h"

struct SlicksSavedRectangle {
    unsigned char *pixels;
    unsigned short width,height;
};

/* Save-under for dialogs that extend into VGA's invisible right margin.
 * Keep the original rounded stride, but never address pixels outside the
 * native surface. Invisible bytes are padding, not another screen row. */
static inline int slicks_save_rectangle_visible(struct SlicksSavedRectangle *saved,
    unsigned char *storage,unsigned long capacity,const struct SlicksChunkyUi *ui,
    short x,short y,short width,short height)
{
    if(!saved || !storage || !ui || !ui->pixels || x<0 || x>=400 || y<0 ||
       width<=0 || width>320 || height<=0 || y+height>200) return -1;
    unsigned columns=((unsigned)width+3)&~3U;
    if(x+columns>400 || columns*(unsigned)height>capacity) return -1;
    for(unsigned row=0;row<(unsigned)height;++row)
        for(unsigned col=0;col<columns;++col)
            storage[row*columns+col]=x+col<320?ui->pixels[mult320[y+row]+x+col]:0;
    *saved=(struct SlicksSavedRectangle){storage,(unsigned short)columns,(unsigned short)height};
    return 0;
}
static inline int slicks_restore_rectangle_visible(struct SlicksChunkyUi *ui,
    const struct SlicksSavedRectangle *saved,short x,short y)
{
    if(!ui || !ui->pixels || !saved || !saved->pixels || x<0 || x>=400 || y<0 ||
       x+saved->width>400 || y+saved->height>200) return -1;
    unsigned columns=x<320?320-x:0;
    if(columns>saved->width) columns=saved->width;
    for(unsigned row=0;row<saved->height;++row)
        for(unsigned col=0;col<columns;++col)
            ui->pixels[mult320[y+row]+x+col]=saved->pixels[row*saved->width+col];
    if(columns && saved->height && ui->dirty)
        ui->dirty(ui->dirty_context,x,y,(short)(x+columns),(short)(y+saved->height));
    return 0;
}

/* Visible native equivalent of 3abe5. DOS rounds width up to four pixels
 * and rotates the four plane copies to preserve an unaligned source x.
 * Store those same pixels compactly in chunky order. Storage is caller-owned,
 * distinct from the screen; no allocation or full-screen shadow is required. */
static inline int slicks_save_rectangle(struct SlicksSavedRectangle *saved,
    unsigned char *storage,unsigned long capacity,const struct SlicksChunkyUi *ui,
    short x,short y,short width,short height)
{
    if(!saved || !storage || !ui || !ui->pixels || x<0 || y<0 ||
       width<=0 || width>320 || height<=0 || height>200) return -1;
    unsigned columns=((unsigned)width+3)&~3U;
    if(x+columns>320 || y+height>200 || columns*(unsigned)height>capacity) return -1;
    const unsigned char *source=ui->pixels+mult320[(unsigned)y]+x;
    unsigned char *destination=storage;
    for(short row=0;row<height;++row,source+=320,destination+=columns)
        for(unsigned col=0;col<columns;++col) destination[col]=source[col];
    *saved=(struct SlicksSavedRectangle){storage,(unsigned short)columns,(unsigned short)height};
    return 0;
}

/* 3b9de subrectangle semantics; full crop also supplies 3aaf2 restoration.
 * x/y are the saved bitmap's destination origin; source x rounds down to
 * four and crop width rounds up independently, just as in the DOS routine.
 * Reject reads outside the saved rectangle rather than emulating DOS spills. */
static inline int slicks_restore_rectangle(struct SlicksChunkyUi *ui,
    const struct SlicksSavedRectangle *saved,short x,short y,short sx,short sy,
    short width,short height)
{
    if(!ui || !ui->pixels || !saved || !saved->pixels || x<0 || y<0 || sx<0 || sy<0 ||
       width<0 || width>320 || height<0 || height>200) return -1;
    unsigned source_x=(unsigned)sx&~3U,columns=((unsigned)width+3)&~3U;
    unsigned left=(unsigned)x+source_x,top=(unsigned)y+(unsigned)sy;
    if(source_x+columns>saved->width || sy+height>saved->height ||
       left+columns>320 || top+height>200) return -1;
    if(!width || !height) return 0;
    const unsigned char *source=saved->pixels+(unsigned)sy*saved->width+source_x;
    unsigned char *destination=ui->pixels+mult320[top]+left;
    for(short row=0;row<height;++row,source+=saved->width,destination+=320)
        for(unsigned col=0;col<columns;++col) destination[col]=source[col];
    if(ui->dirty) ui->dirty(ui->dirty_context,(short)left,(short)top,
        (short)(left+columns),(short)(top+height));
    return 0;
}
#endif
