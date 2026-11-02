#ifndef SLICKS_VIDEO_TIMING_H
#define SLICKS_VIDEO_TIMING_H

/* PAL and NTSC display timing, from the measured beam and the crystal.
 *
 * The beam standard (BEAMCON0, which WHDLoad's NTSC option or a boot menu
 * may set) decides the line length and frame height. The crystal (Alice's
 * PAL/NTSC identity, VPOSR bit 12) decides the colour clock that drives the
 * beam, CIAs and Paula. Every combination runs the game at PAL speed: game
 * time advances in PAL fields of real time, races in real raster lines. */

#define SLICKS_PAL_COLOUR_CLOCK 3546895UL
#define SLICKS_NTSC_COLOUR_CLOCK 3579545UL
/* A PAL field is 313 lines of 227 colour clocks: 142102 half clocks. The
 * crystal ratio 709379/715909 is 869/877 within 1e-7, which keeps the
 * per-field accumulator in 32 bits. */
#define SLICKS_PAL_FIELD_HALF_CLOCKS 142102UL
#define SLICKS_VIDEO_TICK_UNIT (SLICKS_PAL_FIELD_HALF_CLOCKS * 877UL)

struct SlicksVideoTiming {
    unsigned short frame_lines;   /* Lines per frame, as measured. */
    unsigned short line_half_clocks; /* 454 on a PAL beam, 455 on NTSC. */
    unsigned short display_start; /* First of the 200 playfield lines. */
    unsigned short display_end;   /* First line after the playfield. */
    unsigned long colour_clock;   /* Hz. */
    unsigned long line_rate;      /* Raster lines per second, rounded. */
    unsigned long tick_step;      /* Per frame, in SLICKS_VIDEO_TICK_UNIT parts. */
    unsigned char ntsc_beam;
    unsigned char ntsc_crystal;
};

/* A 200-line window centred like the port's PAL $38..$ff, and on NTSC the
 * standard $2c..$f3 screen, which leaves no line below the visible area. */
static inline void slicks_video_timing_init(struct SlicksVideoTiming *t,
    unsigned short frame_lines, unsigned char ntsc_crystal)
{
    t->frame_lines = frame_lines;
    t->ntsc_beam = (unsigned char)(frame_lines < 288);
    t->ntsc_crystal = ntsc_crystal;
    t->line_half_clocks = t->ntsc_beam ? 455 : 454;
    t->display_start = t->ntsc_beam ? 0x2c : 0x38;
    t->display_end = (unsigned short)(t->display_start + 200);
    t->colour_clock = ntsc_crystal ? SLICKS_NTSC_COLOUR_CLOCK : SLICKS_PAL_COLOUR_CLOCK;
    t->line_rate = (2UL * t->colour_clock + t->line_half_clocks / 2U) /
                   t->line_half_clocks;
    t->tick_step = (unsigned long)frame_lines * t->line_half_clocks *
                   (ntsc_crystal ? 869UL : 877UL);
}

/* The first display-end edge strictly after raster time `at`. */
static inline unsigned long slicks_video_next_display_end(
    const struct SlicksVideoTiming *t, unsigned long at)
{
    unsigned long lines = t->frame_lines;
    return (at + lines - t->display_end) / lines * lines + t->display_end;
}

/* One display frame of game time; returns 1 when a PAL field completes. */
static inline unsigned char slicks_video_tick(const struct SlicksVideoTiming *t,
    unsigned long *phase)
{
    *phase += t->tick_step;
    if (*phase < SLICKS_VIDEO_TICK_UNIT) return 0;
    *phase -= SLICKS_VIDEO_TICK_UNIT;
    return 1;
}

#endif
