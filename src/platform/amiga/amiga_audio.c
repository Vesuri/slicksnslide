#include <exec/memory.h>
#include <proto/exec.h>

#include "amiga_audio.h"

#define CUSTOM_WORD(offset) (*(volatile unsigned short *)(0xdff000UL + (offset)))
#define CUSTOM_LONG(offset) (*(volatile unsigned long *)(0xdff000UL + (offset)))
#define REG_DMACON 0x096
#define DMA_SETCLR 0x8000
#define DMA_AUD0 0x0001
#define AUDIO_BASE(channel) (0x0a0 + (channel) * 0x10)

/* DS:05c0: sample.dat block selected for each of the ten vehicles by the
 * original race startup.  Blocks are numbered in archive order. */
static const unsigned char engine_sample_block[SLICKS_AUDIO_ENGINE_COUNT] = {
    17, 17, 21, 22, 19, 18, 20, 18, 23, 24
};

/* DS:05ca and DS:05d4.  The DOS engine update passes this frequency to the
 * sample driver: base * 100 + slope * ((abs(vx) + abs(vy)) / 2). */
static const unsigned char engine_frequency_base[SLICKS_AUDIO_ENGINE_COUNT] = {
    22, 30, 15, 15, 20, 20, 33, 20, 20, 40
};

static const unsigned char engine_frequency_slope[SLICKS_AUDIO_ENGINE_COUNT] = {
    7, 9, 2, 3, 4, 4, 5, 3, 10, 7
};

static unsigned long absolute_velocity(long value)
{
    return value < 0 ? (unsigned long)-value : (unsigned long)value;
}

static unsigned short engine_period(struct SlicksAmigaAudio *audio,
                                    long velocity_x, long velocity_y)
{
    unsigned short vehicle = audio->engine_vehicle;
    unsigned long magnitude =
        (absolute_velocity(velocity_x) + absolute_velocity(velocity_y)) / 2UL;
    unsigned long frequency =
        (unsigned long)engine_frequency_base[vehicle] * 100UL +
        (unsigned long)engine_frequency_slope[vehicle] * magnitude;
    unsigned long period;
    if (frequency > 65535UL)
        frequency = 65535UL;
    period = 3546895UL / frequency;
    if (!period)
        period = 1;
    audio->engine_frequency = (unsigned short)frequency;
    audio->engine_period = (unsigned short)period;
    return (unsigned short)period;
}

static void wait_audio_dma(void)
{
    unsigned short line = CUSTOM_WORD(0x006) & 0xff00;
    unsigned short changes = 0;
    while (changes < 2) {
        unsigned short next = CUSTOM_WORD(0x006) & 0xff00;
        if (next != line) {
            line = next;
            ++changes;
        }
    }
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
            sample->period = 322;
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

static void start_looping_channel(unsigned short channel,
                                  const struct SlicksAmigaSample *sample,
                                  unsigned short period,
                                  unsigned short volume)
{
    unsigned short base = AUDIO_BASE(channel);
    CUSTOM_WORD(REG_DMACON) = (unsigned short)(DMA_AUD0 << channel);
    wait_audio_dma();
    CUSTOM_LONG(base) = (unsigned long)sample->data;
    CUSTOM_WORD(base + 4) = sample->bytes >> 1;
    CUSTOM_WORD(base + 6) = period;
    CUSTOM_WORD(base + 8) = volume;
    CUSTOM_WORD(REG_DMACON) =
        (unsigned short)(DMA_SETCLR | (DMA_AUD0 << channel));
}

static void start_one_shot_channel(struct SlicksAmigaAudio *audio,
                                   unsigned short channel,
                                   const struct SlicksAmigaSample *sample,
                                   unsigned short period,
                                   unsigned short volume)
{
    start_looping_channel(channel, sample, period, volume);
    /* Paula has no non-looping DMA mode.  Do not rewrite AUDxLC/AUDxLEN after
     * a fixed number of raster lines: at period 322 the first audio DMA
     * requests can occur later than that, leaving the original sample as the
     * reload body.  Defer the silent reload until the frame updates below.
     * Two updates are still shorter than the smallest samples.dat effect. */
    if (channel >= 1 && channel <= SLICKS_AUDIO_ONE_SHOT_CHANNELS)
        audio->silent_reload_ticks[channel - 1] = 2;
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
    return 0;
fail:
    slicks_amiga_audio_destroy(audio);
    return -1;
}

void slicks_amiga_audio_start_engine(struct SlicksAmigaAudio *audio,
                                     unsigned short vehicle,
                                     unsigned short priority)
{
    unsigned short block;
    if (!audio || !audio->ready || vehicle >= SLICKS_AUDIO_ENGINE_COUNT)
        return;
    (void)priority;
    block = engine_sample_block[vehicle];
    audio->engine_vehicle = (unsigned char)vehicle;
    audio->engine_sample_block = (unsigned char)block;
    start_looping_channel(0, &audio->samples[block],
                          engine_period(audio, 0, 0), 64);
    audio->engine_started = 1;
}

void slicks_amiga_audio_start_music(struct SlicksAmigaAudio *audio)
{
    if (!audio || !audio->ready || !audio->music.data)
        return;
    start_one_shot_channel(audio, 3, &audio->music,
                           audio->music.period, 32);
    audio->music_started = 1;
}

void slicks_amiga_audio_update(struct SlicksAmigaAudio *audio,
                               long velocity_x, long velocity_y)
{
    unsigned short channel;
    unsigned short effect;
    if (!audio || !audio->ready)
        return;
    if (audio->engine_started)
        CUSTOM_WORD(AUDIO_BASE(0) + 6) =
            engine_period(audio, velocity_x, velocity_y);
    for (channel = 1; channel <= SLICKS_AUDIO_ONE_SHOT_CHANNELS; ++channel) {
        unsigned char *ticks = &audio->silent_reload_ticks[channel - 1];
        if (*ticks && !--*ticks) {
            unsigned short base = AUDIO_BASE(channel);
            CUSTOM_LONG(base) = (unsigned long)audio->silence;
            CUSTOM_WORD(base + 4) = 1;
        }
    }
    for (effect = 0; effect < SLICKS_AUDIO_EFFECT_CHANNELS; ++effect) {
        if (audio->effect_ticks[effect] &&
            !--audio->effect_ticks[effect]) {
            CUSTOM_WORD(REG_DMACON) =
                (unsigned short)(DMA_AUD0 << (effect + 1));
            audio->effect_priority[effect] = 0;
        }
    }
}

void slicks_amiga_audio_play_effect(struct SlicksAmigaAudio *audio,
                                    unsigned short sample_block,
                                    unsigned short flags,
                                    unsigned short priority)
{
    const struct SlicksAmigaSample *sample;
    unsigned short effect;
    unsigned short selected = SLICKS_AUDIO_EFFECT_CHANNELS;
    unsigned long duration;
    if (!audio || !audio->ready ||
        sample_block >= SLICKS_AUDIO_SAMPLE_COUNT)
        return;
    if (!priority)
        priority = 1;
    if (flags & 2) {
        for (effect = 0; effect < SLICKS_AUDIO_EFFECT_CHANNELS; ++effect)
            if (audio->effect_ticks[effect] &&
                audio->effect_priority[effect] == priority)
                return;
    }
    for (effect = 0; effect < SLICKS_AUDIO_EFFECT_CHANNELS; ++effect) {
        if (!audio->effect_ticks[effect]) {
            selected = effect;
            break;
        }
        if (selected == SLICKS_AUDIO_EFFECT_CHANNELS &&
            audio->effect_priority[effect] <= priority)
            selected = effect;
    }
    if (selected == SLICKS_AUDIO_EFFECT_CHANNELS)
        return;
    sample = &audio->samples[sample_block];
    start_one_shot_channel(audio, (unsigned short)(selected + 1), sample,
                           sample->period, 64);
    duration = (unsigned long)sample->bytes * sample->period * 50UL;
    audio->effect_ticks[selected] = (unsigned short)(
        (duration + 3546894UL) / 3546895UL);
    if (!audio->effect_ticks[selected])
        audio->effect_ticks[selected] = 1;
    audio->effect_priority[selected] = (unsigned char)priority;
    audio->last_effect_sample_block = (unsigned char)sample_block;
    audio->last_effect_priority = (unsigned char)priority;
}

void slicks_amiga_audio_stop(struct SlicksAmigaAudio *audio)
{
    CUSTOM_WORD(REG_DMACON) = DMA_AUD0 | (DMA_AUD0 << 1) |
                              (DMA_AUD0 << 2) | (DMA_AUD0 << 3);
    if (audio) {
        audio->engine_started = 0;
        audio->music_started = 0;
        unsigned short effect;
        for (effect = 0; effect < SLICKS_AUDIO_EFFECT_CHANNELS; ++effect) {
            audio->effect_ticks[effect] = 0;
            audio->effect_priority[effect] = 0;
        }
        for (effect = 0; effect < SLICKS_AUDIO_ONE_SHOT_CHANNELS; ++effect)
            audio->silent_reload_ticks[effect] = 0;
    }
}

void slicks_amiga_audio_destroy(struct SlicksAmigaAudio *audio)
{
    unsigned short i;
    if (!audio)
        return;
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
