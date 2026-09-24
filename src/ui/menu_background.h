#ifndef SLICKS_MENU_BACKGROUND_H
#define SLICKS_MENU_BACKGROUND_H
#include "chunky_ui.h"

/* Visible-page equivalent of 3b9de for a saved 320x200 screen. x/y are the
 * page origin, not the crop destination: DOS adds source x/4 and source y.
 * Source x rounds down to four pixels, width rounds up independently.
 * Rows outside the visible saved screen are discarded, including the player
 * menu's (35,105,100,150) restore. Never read past its 64000-byte snapshot.
 * The saved source must be distinct from the current framebuffer. */
static inline int slicks_restore_menu_background(struct SlicksChunkyUi *ui,
    const unsigned char *saved,short x,short y,short source_x,short source_y,
    short width,short height)
{
    if(x<0 || y<0 || source_x<0 || source_y<0 || width<0 || width>320 || height<0 || height>255)
        return -1;
    int sx=source_x&~3,sy=source_y,left=x+sx,top=y+sy;
    int columns=(width+3)&~3,rows=height;
    if(!width || !height || left>=320 || top>=200 || sx>=320 || sy>=200) return 0;
    if(columns>320-left) columns=320-left;
    if(rows>200-top) rows=200-top;
    unsigned char *destination=ui->pixels+mult320[top]+left;
    const unsigned char *source=saved+mult320[sy]+sx;
    for(int row=0;row<rows;++row,destination+=320,source+=320)
        for(int column=0;column<columns;++column) destination[column]=source[column];
    if(ui->dirty) ui->dirty(ui->dirty_context,(short)left,(short)top,
        (short)(left+columns),(short)(top+rows));
    return 0;
}
#endif
