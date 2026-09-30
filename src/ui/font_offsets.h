#ifndef SLICKS_FONT_OFFSETS_H
#define SLICKS_FONT_OFFSETS_H
/* Match the original glyph decoder's 16-bit accumulated byte offset.
 * Geometry must remain immutable while the caller advertises this table. */
static inline void slicks_font_offsets(const unsigned char *font,unsigned short offsets[256])
{
    unsigned count=font[0],at=0;
    const unsigned char *widths=font+6+font[5]+count;
    for(unsigned i=0;i<count;++i) {
        offsets[i]=(unsigned short)at;
        at=(at+((widths[i]+3U)&~3U)*font[2])&65535U;
    }
}
#endif
