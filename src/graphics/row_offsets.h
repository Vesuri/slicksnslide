#ifndef SLICKS_ROW_OFFSETS_H
#define SLICKS_ROW_OFFSETS_H

/* Rendering coordinates must already be clipped. Target C and assembly share
 * the single read-only table in sgfx_mult320.s; host tests use the same values. */
#if defined(__m68k__)
extern const unsigned int mult320[256];
#else
#define SLICKS_ROWS_4(n) ((n)*320U),(((n)+1)*320U),(((n)+2)*320U),(((n)+3)*320U)
#define SLICKS_ROWS_16(n) SLICKS_ROWS_4(n),SLICKS_ROWS_4((n)+4),SLICKS_ROWS_4((n)+8),SLICKS_ROWS_4((n)+12)
static const unsigned int mult320[256] = {
    SLICKS_ROWS_16(0), SLICKS_ROWS_16(16), SLICKS_ROWS_16(32), SLICKS_ROWS_16(48),
    SLICKS_ROWS_16(64), SLICKS_ROWS_16(80), SLICKS_ROWS_16(96), SLICKS_ROWS_16(112),
    SLICKS_ROWS_16(128), SLICKS_ROWS_16(144), SLICKS_ROWS_16(160), SLICKS_ROWS_16(176),
    SLICKS_ROWS_16(192), SLICKS_ROWS_16(208), SLICKS_ROWS_16(224), SLICKS_ROWS_16(240)
};
#undef SLICKS_ROWS_16
#undef SLICKS_ROWS_4
#endif
#endif
