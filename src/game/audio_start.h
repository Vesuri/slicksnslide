#ifndef SLICKS_AUDIO_START_H
#define SLICKS_AUDIO_START_H

/* Original 25358..253f4: publish lights 1/2 before the first two
 * cues; four sample-1 requests, with the last voice
 * immediately retuned to 13000 Hz. Zero frequency means the sample rate.
 * Waits are real milliseconds (19878 -> Borland delay at 14be6). */
static inline void slicks_audio_start_cues(void *context,
    void (*play)(void *,unsigned short,unsigned short),
    void (*wait_ms)(void *,unsigned short),
    void (*show_light)(void *,unsigned short))
{
    if(show_light) show_light(context,1);
    play(context,70,0);
    wait_ms(context,400);
    if(show_light) show_light(context,2);
    play(context,70,0);
    wait_ms(context,500);
    play(context,80,0);
    play(context,70,13000);
}
#endif
