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
#endif
