#ifndef SLICKS_AUDIO_HOST_TEST
#include <exec/memory.h>
#include <proto/exec.h>
#endif

#include "amiga_audio.h"
#include "../../game/audio_volume.h"
#include "../../game/audio_pitch.h"

#ifndef SLICKS_AUDIO_HOST_TEST
#define CUSTOM_WORD(offset) (*(volatile unsigned short *)(0xdff000UL + (offset)))
#define CUSTOM_LONG(offset) (*(volatile unsigned long *)(0xdff000UL + (offset)))
#endif
#define REG_DMACON 0x096
#define DMA_SETCLR 0x8000
#define DMA_AUD0 0x0001
#define AUDIO_BASE(channel) (0x0a0 + (channel) * 0x10)
static struct SlicksAmigaAudio *vblank_audio;
volatile unsigned long g_slicks_audio_vbi_spills;
volatile unsigned short g_slicks_audio_vbi_last_line;
#ifdef SLICKS_AUDIO_HOST_TEST
#define AUDIO_LOCK() ((void)0)
#define AUDIO_UNLOCK() ((void)0)
#else
#define AUDIO_LOCK() Disable()
#define AUDIO_UNLOCK() Enable()
#endif

/* Diagnostic only: preserve register programming but suppress DMA enables. */
unsigned char slicks_amiga_audio_disable_dma;

/* DS:05c0: sample.dat block selected for each of the ten vehicles by the
 * original race startup.  Blocks are numbered in archive order. */
static const unsigned char engine_sample_block[SLICKS_AUDIO_ENGINE_COUNT] = {
    17, 17, 21, 22, 19, 18, 20, 18, 23, 24
};

/* DS:05ca and DS:05d4.  The DOS engine update passes this frequency to the
 * sample driver: base * 100 + slope * ((abs(vx) + abs(vy)) / 2). */
static unsigned long absolute_velocity(long value)
{
    return value < 0 ? 0UL-(unsigned long)value : (unsigned long)value;
}

static unsigned short engine_period(struct SlicksAmigaAudio *audio,
                                    unsigned car,unsigned long measured_speed)
{
    unsigned short vehicle = audio->engine_vehicles[car];
    unsigned long frequency = slicks_engine_frequency(vehicle,measured_speed);
    unsigned long period;
    /* A zero driver frequency has no finite Paula equivalent. Keep the
     * hardware boundary safe; normal race speeds never approach this case. */
    period = frequency ? 3546895UL / frequency : 65535UL;
    if(period>65535UL) period=65535UL;
    if (!period)
        period = 1;
    audio->engine_frequencies[car] = (unsigned short)frequency;
    audio->engine_periods[car] = (unsigned short)period;
    if(!car) {
        audio->engine_frequency = (unsigned short)frequency;
        audio->engine_period = (unsigned short)period;
    }
    return (unsigned short)period;
}

static unsigned short read_be16(const unsigned char *source)
{
    return (unsigned short)((source[0] << 8) | source[1]);
}

static unsigned long read_le32(const unsigned char *source)
{
    return (unsigned long)source[0] |
           ((unsigned long)source[1] << 8) |
           ((unsigned long)source[2] << 16) |
           ((unsigned long)source[3] << 24);
}

static int copy_blocks(struct SlicksAmigaAudio *audio,
                       const unsigned char *source, unsigned long size)
{
    unsigned long at = 0;
    unsigned short block = 0;
    while (block < SLICKS_AUDIO_SAMPLE_COUNT && at + 8 <= size) {
        struct SlicksAmigaSample *sample = &audio->samples[block];
        unsigned short bytes;
        unsigned short header;
        unsigned long next;
        if (source[at] != 't' || source[at + 1] != 'S')
            return -1;
        bytes = read_be16(source + at + 4);
        header = (unsigned short)(
            at + 10 <= size && source[at + 6] == 0 &&
            source[at + 7] == 100 ? 10 : 8);
        if (at + header + bytes > size)
            return -1;
        if(!bytes || bytes==65535U) return -1;
        unsigned short frequency=read_be16(source+at+header-2);
        if(!frequency) return -1;
        {
            unsigned short allocated = (unsigned short)((bytes + 1) & ~1U);
            unsigned short i;
            sample->data = (signed char *)AllocMem(
                allocated, MEMF_CHIP | MEMF_CLEAR);
            if (!sample->data)
                return -1;
            for (i = 0; i < bytes; ++i)
                sample->data[i] =
                    (signed char)(source[at + header + i] ^ 0x80);
            sample->bytes = allocated;
            unsigned long period=(3546895UL+frequency/2U)/frequency;
            sample->period = (unsigned short)(period>65535UL?65535UL:period);
        }
        next = at + header + bytes;
        while (next + 1 < size &&
               (source[next] != 't' || source[next + 1] != 'S'))
            ++next;
        at = next;
        ++block;
    }
    return block == SLICKS_AUDIO_SAMPLE_COUNT ? 0 : -1;
}

int slicks_amiga_audio_add_music(struct SlicksAmigaAudio *audio,
                                 const unsigned char *wave,
                                 unsigned long wave_size)
{
    unsigned long at = 12;
    unsigned long sample_rate = 11025;
    if (!audio || !wave || wave_size < 44 ||
        wave[0] != 'R' || wave[1] != 'I' || wave[2] != 'F' || wave[3] != 'F')
        return -1;
    while (at + 8 <= wave_size) {
        unsigned long size = read_le32(wave + at + 4);
        if (at + 8 + size > wave_size)
            return -1;
        if (wave[at] == 'f' && wave[at + 1] == 'm' &&
            wave[at + 2] == 't' && wave[at + 3] == ' ' && size >= 16)
            sample_rate = read_le32(wave + at + 12);
        if (wave[at] == 'd' && wave[at + 1] == 'a' &&
            wave[at + 2] == 't' && wave[at + 3] == 'a') {
            struct SlicksAmigaSample *sample = &audio->music;
            unsigned short bytes;
            unsigned short i;
            if (size > 65534UL)
                return -1;
            bytes = (unsigned short)((size + 1) & ~1UL);
            sample->data = (signed char *)AllocMem(
                bytes, MEMF_CHIP | MEMF_CLEAR);
            if (!sample->data)
                return -1;
            for (i = 0; i < size; ++i)
                sample->data[i] = (signed char)(wave[at + 8 + i] ^ 0x80);
            sample->bytes = bytes;
            sample->period = (unsigned short)(
                sample_rate ? 3546895UL / sample_rate : 322);
            return 0;
        }
        at += 8 + ((size + 1) & ~1UL);
    }
    return -1;
}

static void start_looping_channel(struct SlicksAmigaAudio *audio,unsigned short channel,
                                  const struct SlicksAmigaSample *sample,
                                  unsigned short period,
                                  unsigned short volume)
{
    audio->pending_sample[channel]=sample;
    audio->pending_period[channel]=period;
    audio->pending_volume[channel]=volume;
    audio->pending_start[channel]=1;
}

static void start_one_shot_channel(struct SlicksAmigaAudio *audio,
                                   unsigned short channel,
                                   const struct SlicksAmigaSample *sample,
                                   unsigned short period,
                                   unsigned short volume)
{
    start_looping_channel(audio,channel, sample, period, volume);
    /* Paula has no non-looping DMA mode.  Do not rewrite AUDxLC/AUDxLEN after
     * a fixed number of raster lines: at period 322 the first audio DMA
     * requests can occur later than that, leaving the original sample as the
     * reload body. Defer the silent reload until the VBI after actual DMA
     * startup (20 ms), irrespective of the game update rate. */
    if (channel < SLICKS_AUDIO_ONE_SHOT_CHANNELS)
        audio->silent_reload_ticks[channel] = 2;
}

int slicks_amiga_audio_create(struct SlicksAmigaAudio *audio,
                              const unsigned char *resource,
                              unsigned long resource_size)
{
    unsigned short i;
    if (!audio || !resource)
        return -1;
    for (i = 0; i < sizeof(*audio); ++i)
        ((unsigned char *)audio)[i] = 0;
    if (copy_blocks(audio, resource, resource_size) != 0)
        goto fail;
    audio->silence = (signed char *)AllocMem(2, MEMF_CHIP | MEMF_CLEAR);
    if (!audio->silence)
        goto fail;
    audio->ready = 1;
    vblank_audio = audio;
    slicks_amiga_audio_set_volume(audio,100,50);
    return 0;
fail:
    slicks_amiga_audio_destroy(audio);
    return -1;
}

void slicks_amiga_audio_set_volume(struct SlicksAmigaAudio *audio,short sounds,short background)
{
    if(!audio) return;
    unsigned char master=(unsigned char)sounds;
    if(master>100) master=100;
    unsigned short sound=slicks_paula_volume(master,255);
    unsigned short music=slicks_paula_volume(master,slicks_background_gain(background));
    if(sound!=audio->sound_volume || music!=audio->music_volume) audio->volume_dirty=1;
    audio->sound_volume=sound; audio->music_volume=music;
}

void slicks_amiga_audio_start_engine(struct SlicksAmigaAudio *audio,
                                     unsigned short vehicle,
                                     unsigned short priority)
{
    (void)priority;
    const unsigned short vehicles[4]={vehicle,0,0,0};
    slicks_amiga_audio_start_engines(audio,vehicles,1);
}

void slicks_amiga_audio_start_engines(struct SlicksAmigaAudio *audio,
                                     const unsigned short vehicles[4],unsigned mask)
{
    if(!audio || !audio->ready || !vehicles) return;
    for(unsigned i=0;i<4;++i) if((mask&(1U<<i)) && vehicles[i]>=SLICKS_AUDIO_ENGINE_COUNT) return;
    AUDIO_LOCK();
    slicks_amiga_audio_stop(audio);
    slicks_audio_channels_engines(&audio->channels,mask);
    for(unsigned i=0;i<4;++i) {
        audio->engine_vehicles[i]=(unsigned char)vehicles[i];
        if(!(mask&(1U<<i))) continue;
        unsigned block=engine_sample_block[vehicles[i]];
        start_looping_channel(audio,i,&audio->samples[block],engine_period(audio,i,0),audio->sound_volume);
    }
    audio->engine_vehicle=(unsigned char)vehicles[0];
    audio->engine_sample_block=vehicles[0]<SLICKS_AUDIO_ENGINE_COUNT?engine_sample_block[vehicles[0]]:0;
    audio->engine_started=(unsigned char)((mask&15)!=0);
    AUDIO_UNLOCK();
}

void slicks_amiga_audio_start_music(struct SlicksAmigaAudio *audio)
{
    if (!audio || !audio->ready || !audio->music.data)
        return;
    AUDIO_LOCK();
    /* Results replace the race soundscape; no racing channel is reserved. */
    slicks_amiga_audio_stop(audio);
    start_one_shot_channel(audio, 3, &audio->music,
                           audio->music.period, audio->music_volume);
    audio->channels.owner[3]=SLICKS_AUDIO_MUSIC;
    audio->music_started = 1;
    audio->effect_ticks[3]=(unsigned short)(((unsigned long)audio->music.bytes*
        audio->music.period*50UL+3546894UL)/3546895UL+1);
    AUDIO_UNLOCK();
}

void slicks_amiga_audio_update(struct SlicksAmigaAudio *audio,
                               long velocity_x, long velocity_y)
{
    const long vx[4]={velocity_x,0,0,0},vy[4]={velocity_y,0,0,0};
    slicks_amiga_audio_update_engines(audio,vx,vy);
}

void slicks_amiga_audio_update_engines(struct SlicksAmigaAudio *audio,
                                      const long vx[4],const long vy[4])
{
    unsigned long speeds[4];
    for(unsigned i=0;i<4;++i) speeds[i]=(absolute_velocity(vx[i])+absolute_velocity(vy[i]))/2UL;
    slicks_amiga_audio_update_speeds(audio,speeds);
}

void slicks_amiga_audio_update_speeds(struct SlicksAmigaAudio *audio,
                                     const unsigned long speeds[4])
{
    unsigned short channel;
    if (!audio || !audio->ready)
        return;
    AUDIO_LOCK();
    if(audio->volume_dirty) {
        for(channel=0;channel<4;++channel)
            CUSTOM_WORD(AUDIO_BASE(channel)+8)=audio->channels.owner[channel]==SLICKS_AUDIO_MUSIC?
                audio->music_volume:audio->sound_volume;
        audio->volume_dirty=0;
    }
    for(channel=0;channel<4;++channel) if(audio->channels.engine_mask&(1U<<channel)) {
        unsigned short period=engine_period(audio,channel,speeds[channel]);
        if(audio->channels.owner[channel]==SLICKS_AUDIO_ENGINE && !audio->pending_start[channel])
            CUSTOM_WORD(AUDIO_BASE(channel)+6)=period;
    }
    AUDIO_UNLOCK();
}

void slicks_amiga_audio_vblank(void)
{
    slicks_amiga_audio_tick(vblank_audio);
#ifndef SLICKS_AUDIO_HOST_TEST
    unsigned short line=(unsigned short)(((CUSTOM_WORD(4)&7)<<8)|(CUSTOM_WORD(6)>>8));
    g_slicks_audio_vbi_last_line=line;
    if(line>=56 && line<256) ++g_slicks_audio_vbi_spills;
#endif
}

void slicks_amiga_audio_tick(struct SlicksAmigaAudio *audio)
{
    unsigned short channel,effect;
    if(!audio || !audio->ready) return;
    unsigned stop_mask=0,start_mask=0;
    for(channel=0;channel<4;++channel) {
        unsigned short base=AUDIO_BASE(channel);
        if(audio->pending_start[channel]==1) {
            stop_mask|=1U<<channel;
            CUSTOM_WORD(base+8)=0;
            CUSTOM_WORD(base+6)=124;
            audio->pending_start[channel]=2;
        } else if(audio->pending_start[channel]==2) {
            const struct SlicksAmigaSample *sample=audio->pending_sample[channel];
            CUSTOM_LONG(base)=(unsigned long)sample->data;
            CUSTOM_WORD(base+4)=sample->bytes>>1;
            CUSTOM_WORD(base+6)=audio->channels.owner[channel]==SLICKS_AUDIO_ENGINE?
                audio->engine_periods[channel]:audio->pending_period[channel];
            CUSTOM_WORD(base+8)=audio->channels.owner[channel]==SLICKS_AUDIO_MUSIC?
                audio->music_volume:audio->sound_volume;
            start_mask|=1U<<channel;
            audio->pending_start[channel]=0;
        }
    }
    if(stop_mask) CUSTOM_WORD(REG_DMACON)=(unsigned short)stop_mask;
    if(start_mask && !slicks_amiga_audio_disable_dma)
        CUSTOM_WORD(REG_DMACON)=(unsigned short)(DMA_SETCLR|start_mask);
    for (channel = 0; channel < SLICKS_AUDIO_ONE_SHOT_CHANNELS; ++channel) {
        if(audio->pending_start[channel]) continue;
        unsigned char *ticks = &audio->silent_reload_ticks[channel];
        if (*ticks && !--*ticks) {
            unsigned short base = AUDIO_BASE(channel);
            CUSTOM_LONG(base) = (unsigned long)audio->silence;
            CUSTOM_WORD(base + 4) = 1;
        }
    }
    for (effect = 0; effect < SLICKS_AUDIO_EFFECT_CHANNELS; ++effect) {
        if(audio->pending_start[effect]) continue;
        if (audio->effect_ticks[effect] &&
            !--audio->effect_ticks[effect]) {
            CUSTOM_WORD(REG_DMACON) = (unsigned short)(DMA_AUD0 << effect);
            audio->effect_priority[effect] = 0;
            audio->silent_reload_ticks[effect]=0;
            if(audio->channels.owner[effect]==SLICKS_AUDIO_MUSIC) audio->music_started=0;
            slicks_audio_channels_finish(&audio->channels,effect);
            if(audio->channels.owner[effect]==SLICKS_AUDIO_ENGINE)
                start_looping_channel(audio,effect,&audio->samples[engine_sample_block[audio->engine_vehicles[effect]]],
                    audio->engine_periods[effect],audio->sound_volume);
        }
    }
}

void slicks_amiga_audio_play_effect(struct SlicksAmigaAudio *audio,
                                    unsigned short sample_block,
                                    unsigned short flags,
                                    unsigned short priority)
{
    const struct SlicksAmigaSample *sample;
    int selected;
    unsigned long duration;
    if (!audio || !audio->ready ||
        sample_block >= SLICKS_AUDIO_SAMPLE_COUNT)
        return;
    AUDIO_LOCK();
    selected=slicks_audio_channels_request(&audio->channels,flags,priority);
    if(selected<0) { AUDIO_UNLOCK(); return; }
    sample = &audio->samples[sample_block];
    audio->silent_reload_ticks[selected]=0;
    if(flags&1) start_looping_channel(audio,(unsigned short)selected,sample,sample->period,audio->sound_volume);
    else start_one_shot_channel(audio,(unsigned short)selected,sample,sample->period,audio->sound_volume);
    duration = (unsigned long)sample->bytes * sample->period * 50UL;
    audio->effect_ticks[selected] = (unsigned short)(
        (duration + 3546894UL) / 3546895UL + 1);
    if (!audio->effect_ticks[selected])
        audio->effect_ticks[selected] = 1;
    if(flags&1) audio->effect_ticks[selected]=0;
    audio->effect_priority[selected] = (unsigned char)audio->channels.priority[selected];
    audio->last_effect_sample_block = (unsigned char)sample_block;
    audio->last_effect_priority = (unsigned char)priority;
    AUDIO_UNLOCK();
}

void slicks_amiga_audio_stop(struct SlicksAmigaAudio *audio)
{
    AUDIO_LOCK();
    CUSTOM_WORD(REG_DMACON) = DMA_AUD0 | (DMA_AUD0 << 1) |
                              (DMA_AUD0 << 2) | (DMA_AUD0 << 3);
    for(unsigned channel=0;channel<4;++channel) CUSTOM_WORD(AUDIO_BASE(channel)+8)=0;
    if (audio) {
        audio->engine_started = 0;
        audio->music_started = 0;
        audio->channels=(struct SlicksAudioChannels){0};
        unsigned short effect;
        for (effect = 0; effect < SLICKS_AUDIO_EFFECT_CHANNELS; ++effect) {
            audio->effect_ticks[effect] = 0;
            audio->effect_priority[effect] = 0;
            audio->pending_start[effect] = 0;
        }
        for (effect = 0; effect < SLICKS_AUDIO_ONE_SHOT_CHANNELS; ++effect)
            audio->silent_reload_ticks[effect] = 0;
    }
    AUDIO_UNLOCK();
}

void slicks_amiga_audio_destroy(struct SlicksAmigaAudio *audio)
{
    unsigned short i;
    if (!audio)
        return;
    AUDIO_LOCK();
    if(vblank_audio==audio) vblank_audio=0;
    AUDIO_UNLOCK();
    for (i = 0; i < SLICKS_AUDIO_SAMPLE_COUNT; ++i) {
        if (audio->samples[i].data)
            FreeMem(audio->samples[i].data, audio->samples[i].bytes);
        audio->samples[i].data = 0;
        audio->samples[i].bytes = 0;
    }
    if (audio->music.data)
        FreeMem(audio->music.data, audio->music.bytes);
    audio->music.data = 0;
    audio->music.bytes = 0;
    if (audio->silence)
        FreeMem(audio->silence, 2);
    audio->silence = 0;
    audio->ready = 0;
}
