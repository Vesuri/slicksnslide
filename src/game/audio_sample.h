#ifndef SLICKS_AUDIO_SAMPLE_H
#define SLICKS_AUDIO_SAMPLE_H

/* Original 3000:89e2..89ff: unsigned PCM, signed division toward zero. */
static inline signed char slicks_sample_pcm(unsigned char value,unsigned char gain)
{
    return (signed char)(((int)value-128)*(int)gain/255);
}

#endif
