#ifndef SLICKS_RACE_TIMING_H
#define SLICKS_RACE_TIMING_H

/* Original 37bc2: the argument is signed, not a frequency in hertz.
 * Values below 100 disable the custom IRQ clock and program PIT count zero
 * (65536). Other values program floor(100*65536/argument). */
static inline unsigned long slicks_timer_divisor(unsigned short argument)
{
    return (short)argument<100?65536UL:6553600UL/argument;
}
static inline unsigned short slicks_speed_timer_argument(short speed)
{ return (unsigned short)((unsigned short)speed*5U); }

/* At nominal PAL 50 Hz, accumulate real PIT input cycles rather than round
 * each update to an integer IRQ count. Division occurs only when configuring
 * speed, never in this per-update path. period=0 retains the historical 100%
 * default for zero-initialized runtime fixtures. */
static inline unsigned short slicks_physics_clock_advance(unsigned long *phase,
    unsigned long period,unsigned char disabled)
{
    if(disabled) return 0;
    if(!period) period=655350UL;
    unsigned short ticks=0; *phase+=1193182UL;
    while(*phase>=period) { *phase-=period; ++ticks; }
    return ticks;
}
/* Real-time variant, counting raster lines of line_rate Hz (15625 on PAL:
 * 227 colour clocks; NTSC 227.5 colour clocks, about 15734). Phase units
 * are 1/line_rate PIT input cycles: a line adds 1193182 and a tick consumes
 * divisor*line_rate, where period=divisor*50. Lines are consumed in chunks
 * so the 32-bit phase cannot overflow at the slowest divisor (65536).
 * line_rate=0 is PAL, for zero-initialized runtime fixtures. */
#define SLICKS_PAL_LINE_RATE 15625UL
static inline unsigned short slicks_physics_clock_lines(unsigned long *phase,
    unsigned long period,unsigned char disabled,unsigned long lines,
    unsigned long line_rate)
{
    if(disabled) return 0;
    if(!period) period=655350UL;
    if(!line_rate) line_rate=SLICKS_PAL_LINE_RATE;
    unsigned long tick=period/50UL*line_rate; unsigned short ticks=0;
    while(lines) {
        unsigned long chunk=lines>2048UL?2048UL:lines; lines-=chunk;
        *phase+=chunk*1193182UL;
        while(*phase>=tick) { *phase-=tick; if(ticks<65535U) ++ticks; }
    }
    return ticks;
}
/* 1000:fe85..fe8b: an update never integrates more than 45 ticks. */
#define SLICKS_PHYSICS_BATCH_MAX 45U
#endif
