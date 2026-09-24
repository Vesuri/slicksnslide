#ifndef SLICKS_AUDIO_PITCH_H
#define SLICKS_AUDIO_PITCH_H
/* Original DS:05ca/05d4 and 2000:2776..27a5. The driver argument is a
 * word: multiplication/addition wrap, rather than saturating at 65535. */
static const unsigned char engine_frequency_base[10]={22,30,15,15,20,20,33,20,20,40};
static const unsigned char engine_frequency_slope[10]={7,9,2,3,4,4,5,3,10,7};
static unsigned short slicks_engine_frequency(unsigned vehicle,unsigned long measured_speed)
{
    return (unsigned short)(engine_frequency_base[vehicle]*100U+
        engine_frequency_slope[vehicle]*(unsigned short)measured_speed);
}
#endif
