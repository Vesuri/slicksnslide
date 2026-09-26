#ifndef SLICKS_TRACK_MATERIAL_SAMPLE_H
#define SLICKS_TRACK_MATERIAL_SAMPLE_H
#include "../graphics/row_offsets.h"

/* Shared exact arithmetic for hot callers and the out-of-line public API.
 * Preserve the original 16-bit linear-address and packed-plane wrapping. */
static inline int slicks_track_material_sample_inline(const unsigned char *lower,
    const unsigned char *upper,short x,short y,signed char layer)
{
    /* In the visible positive-coordinate domain, the original packed
     * address equals raw exactly: 4*(y*80+x/4)+(x&3) == y*320+x.
     * Keep signed/truncated/wrapped addressing in the general path. */
    if(layer && (unsigned short)x<320 && (unsigned short)y<190) {
        if(!lower || !upper)return -1;
        return upper[mult320[(unsigned short)y]+(unsigned short)x]&31U;
    }
    unsigned row=(unsigned short)y<256?mult320[(unsigned short)y]:(unsigned short)y*320U;
    unsigned short raw=(unsigned short)(row+(unsigned short)x);
    /* Original 1bd30 clears FA00 raw bytes and FE80 packed bytes before
     * b283's material compositor, whose producers crop at row 185. */
    if(raw>=64000 || !lower || (layer && !upper)) return -1;
    unsigned material=raw<60800?lower[raw]:0;
    if(layer) {
        unsigned short packed=(unsigned short)((row>>2)+x/4);
        if(packed>=65152) return -1;
        unsigned at=(unsigned)packed*4U+((unsigned short)x&3U);
        material=(raw<60800?(upper[raw]&7U):0U)+(at<60800?(upper[at]&24U):0U);
    }
    return (int)material;
}
#endif
