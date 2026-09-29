#ifndef SLICKS_TITLE_BACKGROUND_H
#define SLICKS_TITLE_BACKGROUND_H
#include "palette_remap.h"

/* Original startup 261e8..2623d, before the DS:4c1c background capture.
 * Apply once to decoded artwork, never again on menu/demo returns. */
static inline void slicks_title_prepare_background(unsigned char *pixels,
    const unsigned char *palette)
{
    struct SlicksChunkyUi ui={pixels,palette,0,0};
    unsigned char table[256];
    slicks_ui_tint_table(palette,table,24,24,34,20);
    (void)slicks_ui_remap(&ui,105,72,215,174,table);
    slicks_ui_tint_table(palette,table,24,24,34,40);
    (void)slicks_ui_remap(&ui,107,74,213,172,table);
}
#endif
