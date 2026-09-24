#ifndef SLICKS_PLAYER_MENU_PREPARE_H
#define SLICKS_PLAYER_MENU_PREPARE_H
#include "palette_remap.h"

/* Static painting in 283c3..285ec, after players.bmp has been loaded. Fonts
 * are the original runtime objects: kirj, pieni, iso, each with a palette.
 * The callback is the original string renderer (font slots 0,1,2 here).
 * Asset loading, snapshot allocation and fade/display ownership are separate.
 * Title is the resolved original resource string (36227), not uppercased. */
struct SlicksPlayerMenuPrepareOps {
    void (*text)(void *,unsigned,const unsigned char *,short,short,unsigned char);
    void *context;
};
static inline void slicks_prepare_player_menu(struct SlicksChunkyUi *ui,
    unsigned char *fonts[3],const unsigned char *title,const unsigned char *footer,
    unsigned char footer_percent,const struct SlicksPlayerMenuPrepareOps *ops)
{
    unsigned char table[256],footer_table[256];
    slicks_ui_tint_table(ui->palette,table,15,15,45,50);
    slicks_ui_remap(ui,0,0,80,11,table);
    slicks_ui_remap(ui,0,11,80,12,table);
    fonts[2][6]=slicks_ui_nearest(ui,60,60,70);
    fonts[0][6]=slicks_ui_nearest(ui,50,50,70);
    ops->text(ops->context,2,title,77,1,2);
    slicks_ui_tint_table(ui->palette,footer_table,20,20,30,footer_percent);
    slicks_ui_remap(ui,0,191,320,200,footer_table);
    fonts[1][6]=slicks_ui_nearest(ui,45,55,45);
    ops->text(ops->context,1,footer,1,193,0);
    for(short i=0;i<4;++i) slicks_ui_remap(ui,48,(short)(29+16*i),225,(short)(40+16*i),table);
    slicks_ui_remap(ui,50,105,120,157,table);
}
#endif
