#ifndef SLICKS_CHUNKY_UI_H
#define SLICKS_CHUNKY_UI_H
#include "../graphics/row_offsets.h"
#include "colour_picker.h"

struct SlicksChunkyUi {
    unsigned char *pixels;
    const unsigned char *palette;
    void (*dirty)(void *,short,short,short,short);
    void *dirty_context;
};

/* Original 36fae: palette entry zero is reserved, query bytes are signed,
 * palette bytes unsigned, initial best distance 300, first tie wins. */
static inline unsigned char slicks_ui_nearest(void *context,unsigned char r,
    unsigned char g,unsigned char b)
{
    struct SlicksChunkyUi *ui=context;
    int best=300; unsigned char result=1;
    for(unsigned i=1;i<256;++i) {
        int dr=ui->palette[3*i]-(signed char)r;
        int dg=ui->palette[3*i+1]-(signed char)g;
        int db=ui->palette[3*i+2]-(signed char)b;
        int distance=(dr<0?-dr:dr)+(dg<0?-dg:dg)+(db<0?-db:db);
        if(distance<best) { best=distance; result=(unsigned char)i; }
    }
    return result;
}

/* Clipped original half-open fill for the authoritative 320x200 surface.
 * Painters report actual written bounds; no screen comparison/shadow scan. */
static inline void slicks_ui_rectangle(void *context,short left,short top,
    short right,short bottom,unsigned char colour)
{
    struct SlicksChunkyUi *ui=context;
    if(left<0) left=0;
    if(top<0) top=0;
    if(right>320) right=320;
    if(bottom>200) bottom=200;
    if(left>=right || top>=bottom) return;
    unsigned char *row=ui->pixels+mult320[(unsigned)top]+left;
    for(short y=top;y<bottom;++y,row+=320)
        for(short x=0;x<right-left;++x) row[x]=colour;
    if(ui->dirty) ui->dirty(ui->dirty_context,left,top,right,bottom);
}

static inline void slicks_ui_colour_picker(struct SlicksChunkyUi *ui,
    const struct SlicksColourPicker *picker,struct SlicksColourPickerPulse *pulse,
    unsigned long tick,short x,short y)
{
    unsigned char bars[3]={slicks_ui_nearest(ui,70,20,20),
        slicks_ui_nearest(ui,20,70,20),slicks_ui_nearest(ui,20,20,70)};
    unsigned char unselected=slicks_ui_nearest(ui,10,10,25);
    const struct SlicksColourPickerDrawOps ops={slicks_ui_nearest,slicks_ui_rectangle,ui};
    slicks_colour_picker_draw(picker,pulse,tick,x,y,bars,unselected,&ops);
}

/* Original rounded bevel 309cf..30b5a. The decreasing corner inset and
 * inclusive half-height loop deliberately paint both rows at the centre.
 * This is used by player-menu selection, not a replacement visual design.
 * Native caller supplies positive on-screen-sized dimensions (<=320x200). */
static inline unsigned char slicks_ui_bevel(struct SlicksChunkyUi *ui,
    short x,short y,short width,short height,unsigned char red,
    unsigned char green,unsigned char blue)
{
    unsigned char light=slicks_ui_nearest(ui,(unsigned char)(red+15),
        (unsigned char)(green+15),(unsigned char)(blue+15));
    unsigned char middle=slicks_ui_nearest(ui,red,green,blue);
    unsigned char dark=slicks_ui_nearest(ui,(unsigned char)(red-20),
        (unsigned char)(green-20),(unsigned char)(blue-20));
    short corner=width/5;
    if(corner>25) corner=25;
    short half_height=height/2,half_width=(short)(width/2+1);
    for(short row=0;row<=half_height;++row) {
        for(unsigned pass=0;pass<2;++pass) {
            short py=(short)(unsigned short)(pass?y+height-row:y+row);
            unsigned char edge=pass?dark:light;
            slicks_ui_rectangle(ui,(short)(unsigned short)(x+half_width),py,
                (short)(unsigned short)(x+width-half_width),(short)(unsigned short)(py+1),middle);
            slicks_ui_rectangle(ui,(short)(unsigned short)(x+corner),py,
                (short)(unsigned short)(x+half_width),(short)(unsigned short)(py+1),edge);
            slicks_ui_rectangle(ui,(short)(unsigned short)(x+width-half_width),py,
                (short)(unsigned short)(x+width-corner),(short)(unsigned short)(py+1),edge);
        }
        half_width=corner;
        if(!half_width) half_width=1;
        corner=(short)(corner>>1);
    }
    return middle;
}
#endif
