#ifndef SLICKS_AUDIO_VOLUME_H
#define SLICKS_AUDIO_VOLUME_H
#include "configuration.h"

/* 39bf0..39bfd: only the argument's low byte participates; the resulting
 * byte scales the background WAV at 389e2..389fc with divisor 255. */
static inline unsigned char slicks_background_gain(short option)
{ return (unsigned char)(((unsigned char)option*5U)/2U); }

/* Platform boundary: map the original master and sample gains to Paula's
 * linear 0..64 range. This quantizes amplitude, not sample timing or priority. */
static inline unsigned short slicks_paula_volume(unsigned char master,unsigned char sample_gain)
{ return (unsigned short)((64UL*master*sample_gain)/(100UL*255UL)); }
#endif
