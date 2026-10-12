#ifndef SLICKS_AUDIO_START_H
#define SLICKS_AUDIO_START_H

/* Original 25376..253f4: four sample-1 requests, with the last voice
 * immediately retuned to 13000 Hz. Zero frequency means the sample rate.
 * Waits are real milliseconds (19878 -> Borland delay at 14be6). */
static inline void slicks_audio_start_cues(void *context,
    void (*play)(void *,unsigned short,unsigned short),
    void (*wait_ms)(void *,unsigned short))
{
    play(context,70,0);
    wait_ms(context,400);
    play(context,70,0);
    wait_ms(context,500);
    play(context,80,0);
    play(context,70,13000);
}
#endif
