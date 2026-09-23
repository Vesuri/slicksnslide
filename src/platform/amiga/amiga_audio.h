#ifndef SLICKS_AMIGA_AUDIO_H
#define SLICKS_AMIGA_AUDIO_H

#define SLICKS_AUDIO_ENGINE_COUNT 10
#define SLICKS_AUDIO_SAMPLE_COUNT 26
#define SLICKS_AUDIO_EFFECT_CHANNELS 2

struct SlicksAmigaSample {
    signed char *data;
    unsigned short bytes;
    unsigned short period;
};

struct SlicksAmigaAudio {
    struct SlicksAmigaSample samples[SLICKS_AUDIO_SAMPLE_COUNT];
    struct SlicksAmigaSample music;
    signed char *silence;
    unsigned short effect_ticks[SLICKS_AUDIO_EFFECT_CHANNELS];
    unsigned short engine_frequency;
    unsigned short engine_period;
    unsigned char effect_priority[SLICKS_AUDIO_EFFECT_CHANNELS];
    unsigned char engine_vehicle;
    unsigned char engine_sample_block;
    unsigned char last_effect_sample_block;
    unsigned char last_effect_priority;
    unsigned char engine_started;
    unsigned char music_started;
    unsigned char ready;
};

int slicks_amiga_audio_create(struct SlicksAmigaAudio *audio,
                              const unsigned char *resource,
                              unsigned long resource_size);
int slicks_amiga_audio_add_music(struct SlicksAmigaAudio *audio,
                                 const unsigned char *wave,
                                 unsigned long wave_size);
void slicks_amiga_audio_start_music(struct SlicksAmigaAudio *audio);
void slicks_amiga_audio_start_engine(struct SlicksAmigaAudio *audio,
                                     unsigned short vehicle,
                                     unsigned short priority);
void slicks_amiga_audio_update(struct SlicksAmigaAudio *audio,
                               long velocity_x, long velocity_y);
void slicks_amiga_audio_play_effect(struct SlicksAmigaAudio *audio,
                                    unsigned short sample_block,
                                    unsigned short flags,
                                    unsigned short priority);
void slicks_amiga_audio_stop(struct SlicksAmigaAudio *audio);
void slicks_amiga_audio_destroy(struct SlicksAmigaAudio *audio);

#endif
