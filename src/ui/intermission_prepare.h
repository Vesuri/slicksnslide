#ifndef SLICKS_INTERMISSION_PREPARE_H
#define SLICKS_INTERMISSION_PREPARE_H
#include "palette_remap.h"

/* Original 241f7..2423f builds one table from DS:68ae (the retained race
 * palette), not necessarily the currently displayed DS:71b8 palette.
 * Keep the table for the later preview surround at 243c9..243e9. */
static inline int slicks_intermission_prepare_panel(struct SlicksChunkyUi *ui,
    const unsigned char source_palette[768],unsigned char table[256],unsigned count)
{
    if(!ui || !ui->pixels || !source_palette || !table || count>4) return -1;
    slicks_ui_tint_table(source_palette,table,30,30,55,82);
    return slicks_ui_remap(ui,80,60,275,(short)(120+10*count),table);
}
static inline int slicks_intermission_prepare_preview(struct SlicksChunkyUi *ui,
    const unsigned char table[256])
{
    if(!ui || !ui->pixels || !table) return -1;
    return slicks_ui_remap(ui,20,30,95,85,table);
}
#endif
