#ifndef SLICKS_TRACK_DATA_VIEW_H
#define SLICKS_TRACK_DATA_VIEW_H
#include "chunky_ui.h"

/* Original 23f87..23fd8. The runtime maps already contain the decoded
 * five-bit values returned by 1b089: lower = raw>>3, upper = packed layer
 * bits plus raw&7. Preserve column-major plot order and leave the HUD alone.
 * The caller must enforce the nonzero demo/debug flag and scan 57/58. */
static inline int slicks_track_data_view_draw(struct SlicksChunkyUi *ui,
    const unsigned char *lower,const unsigned char *upper,unsigned char layer)
{
    if(!ui || !ui->pixels || !lower || (layer && !upper)) return -1;
    const unsigned char *source=layer?upper:lower;
    for(unsigned x=0;x<320;++x) {
        unsigned at=x;
        for(unsigned y=0;y<190;++y,at+=320)
            ui->pixels[at]=source[at]&31;
    }
    if(ui->dirty) ui->dirty(ui->dirty_context,0,0,320,190);
    return 0;
}
#endif
