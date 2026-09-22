#include <exec/memory.h>
#include <proto/exec.h>

#include "amiga_audio.h"

#define CUSTOM_WORD(offset) (*(volatile unsigned short *)(0xdff000UL + (offset)))
#define CUSTOM_LONG(offset) (*(volatile unsigned long *)(0xdff000UL + (offset)))
#define REG_DMACON 0x096
#define DMA_SETCLR 0x8000
#define DMA_AUD0 0x0001
#define AUDIO_BASE(channel) (0x0a0 + (channel) * 0x10)

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

static int copy_block(struct SlicksAmigaSample *sample,
                      const unsigned char *source, unsigned long size,
                      unsigned short wanted)
{
    unsigned long at = 0;
    unsigned short block = 0;
    while (at + 8 <= size) {
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
        if (block == wanted) {
            unsigned short allocated = (unsigned short)((bytes + 1) & ~1U);
            unsigned short i;
            sample->data = (signed char *)AllocMem(
                allocated, MEMF_CHIP | MEMF_CLEAR);
            if (!sample->data)
                return -1;
            for (i = 0; i < bytes; ++i)
                sample->data[i] = (signed char)(source[at + header + i] ^ 0x80);
            sample->bytes = allocated;
            sample->period = 322;
            return 0;
        }
        next = at + header + bytes;
        while (next + 1 < size &&
               (source[next] != 't' || source[next + 1] != 'S'))
            ++next;
        at = next;
        ++block;
    }
    return -1;
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
            struct SlicksAmigaSample *sample = &audio->samples[12];
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

static void start_channel(unsigned short channel,
                          const struct SlicksAmigaSample *sample,
                          unsigned short period, unsigned short volume)
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

int slicks_amiga_audio_create(struct SlicksAmigaAudio *audio,
                              const unsigned char *resource,
                              unsigned long resource_size)
{
    unsigned short i;
    if (!audio || !resource)
        return -1;
    for (i = 0; i < sizeof(*audio); ++i)
        ((unsigned char *)audio)[i] = 0;
    for (i = 0; i < SLICKS_AUDIO_ENGINE_COUNT; ++i) {
        unsigned short block = (unsigned short)(i < 9 ? 16 + i : 25);
        if (copy_block(&audio->samples[i], resource, resource_size, block) != 0)
            goto fail;
    }
    if (copy_block(&audio->samples[10], resource, resource_size, 13) != 0 ||
        copy_block(&audio->samples[11], resource, resource_size, 12) != 0)
        goto fail;
    audio->ready = 1;
    return 0;
fail:
    slicks_amiga_audio_destroy(audio);
    return -1;
}

void slicks_amiga_audio_start_engine(struct SlicksAmigaAudio *audio,
                                     unsigned short vehicle,
                                     unsigned short volume)
{
    if (!audio || !audio->ready || vehicle >= SLICKS_AUDIO_ENGINE_COUNT)
        return;
    volume /= 2;
    if (volume > 64)
        volume = 64;
    audio->engine_vehicle = (unsigned char)vehicle;
    start_channel(0, &audio->samples[vehicle], 420, volume);
    audio->engine_started = 1;
}

void slicks_amiga_audio_start_music(struct SlicksAmigaAudio *audio)
{
    if (!audio || !audio->ready || !audio->samples[12].data)
        return;
    start_channel(3, &audio->samples[12], audio->samples[12].period, 32);
    audio->music_started = 1;
}

void slicks_amiga_audio_update(struct SlicksAmigaAudio *audio,
                               short speed, unsigned long collisions,
                               unsigned long trails)
{
    unsigned short period;
    if (!audio || !audio->ready)
        return;
    if (audio->engine_started) {
        if (speed < 0)
            speed = (short)-speed;
        period = (unsigned short)(420 - (speed > 100 ? 200 : speed * 2));
        CUSTOM_WORD(AUDIO_BASE(0) + 6) = period;
    }
    if (collisions != audio->previous_collisions &&
        !audio->collision_ticks) {
        start_channel(1, &audio->samples[10], 322, 48);
        audio->collision_ticks = 3;
    } else if (audio->collision_ticks && !--audio->collision_ticks) {
        CUSTOM_WORD(REG_DMACON) = DMA_AUD0 << 1;
    }
    audio->previous_collisions = collisions;
    if (trails != audio->previous_trails && !audio->trail_ticks) {
        start_channel(2, &audio->samples[11], 322, 30);
        audio->trail_ticks = 3;
    } else if (audio->trail_ticks && !--audio->trail_ticks) {
        CUSTOM_WORD(REG_DMACON) = DMA_AUD0 << 2;
    }
    audio->previous_trails = trails;
}

void slicks_amiga_audio_stop(struct SlicksAmigaAudio *audio)
{
    CUSTOM_WORD(REG_DMACON) = DMA_AUD0 | (DMA_AUD0 << 1) |
                              (DMA_AUD0 << 2) | (DMA_AUD0 << 3);
    if (audio) {
        audio->engine_started = 0;
        audio->music_started = 0;
        audio->collision_ticks = 0;
        audio->trail_ticks = 0;
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
    audio->ready = 0;
}
