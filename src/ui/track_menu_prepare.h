#ifndef SLICKS_TRACK_MENU_PREPARE_H
#define SLICKS_TRACK_MENU_PREPARE_H
#include "player_menu_prepare.h"

/* Original 26e62..2702e, after loading trckmenu.@I and trckmenu.@p.
 * Font slots follow the original pointers: kirj, pieni, iso. Resource IO,
 * fades, selected-row tint and the saved background belong to the caller. */
static inline void slicks_prepare_track_menu(struct SlicksChunkyUi *ui,
    unsigned char *fonts[3],const unsigned char *title,const unsigned char *footer,
    short total,unsigned char percent,const struct SlicksPlayerMenuPrepareOps *ops)
{
    unsigned char table[256];
    slicks_ui_tint_table(ui->palette,table,30,30,40,percent);
    slicks_ui_remap(ui,0,0,320,11,table);
    slicks_ui_tint_table(ui->palette,table,20,20,30,percent);
    slicks_ui_remap(ui,0,191,320,200,table);
    fonts[2][6]=slicks_ui_nearest(ui,65,45,60);
    ops->text(ops->context,2,title,160,1,1);
    fonts[1][6]=slicks_ui_nearest(ui,45,55,45);
    ops->text(ops->context,1,footer,1,193,0);
    /* 34654 treats blue=0xacb7 as a greyscale sentinel: both green and
     * blue become red before calling the tint-table builder. */
    slicks_ui_tint_table(ui->palette,table,20,20,20,percent);
    slicks_ui_remap(ui,105,26,220,112,table);
    if(total>22) {
        slicks_ui_tint_table(ui->palette,table,10,10,20,percent);
        slicks_ui_remap(ui,5,15,8,188,table);
    }
}
#endif
