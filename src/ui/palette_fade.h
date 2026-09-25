#ifndef SLICKS_PALETTE_FADE_H
#define SLICKS_PALETTE_FADE_H
/* 378a3: signed word arithmetic, a 0..64 palette weight and the configured
 * PIT argument (not the resulting IRQ frequency) determine the duration. */
static inline short slicks_fade_duration(unsigned short timer,short duration)
{ return (short)(unsigned short)(timer*duration)/100; }
static inline short slicks_fade_weight(short start,short end,short duration,short step)
{
    start=(short)(unsigned short)((unsigned short)start<<6)/100;
    end=(short)(unsigned short)((unsigned short)end<<6)/100;
    return duration>0?(short)(unsigned short)((duration-step)*start+end*step)/duration:end;
}
static inline void slicks_fade_palette(unsigned char out[768],const unsigned char in[768],short weight)
{
    for(unsigned i=0;i<768;++i)
        out[i]=(unsigned char)((short)(unsigned short)(in[i]*weight)/64);
}
/* 37a14..37a37: skip elapsed ticks, but never skip the final palette. */
static inline short slicks_fade_advance(short step,short duration,unsigned short ticks)
{
    short extra=(short)(unsigned short)(ticks-1);
    if(extra>0) {
        short next=(short)(unsigned short)(step+extra);
        if(next>=duration && step<duration) next=(short)(duration-1);
        step=next;
    }
    return (short)(unsigned short)(step+1);
}
#endif
