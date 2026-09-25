#ifndef SLICKS_AMIGA_AUDIO_H
#define SLICKS_AMIGA_AUDIO_H
#include "../../game/audio_channels.h"
#ifdef __cplusplus
extern "C" {
#endif

#define SLICKS_AUDIO_ENGINE_COUNT 10
#define SLICKS_AUDIO_SAMPLE_COUNT 27
#define SLICKS_AUDIO_EFFECT_CHANNELS 4
#define SLICKS_AUDIO_ONE_SHOT_CHANNELS 4

struct SlicksAmigaSample {
    signed char *data;
    unsigned short bytes;
    unsigned short period;
};

struct SlicksAmigaAudio {
    struct SlicksAmigaSample samples[SLICKS_AUDIO_SAMPLE_COUNT];
    struct SlicksAmigaSample reduced_engines[8][2]; /* IDs 17..24, half/quarter. */
    struct SlicksAmigaSample music;
    signed char *silence;
    struct SlicksAudioChannels channels;
    const struct SlicksAmigaSample *pending_sample[4];
    unsigned short pending_period[4],pending_volume[4];
    unsigned char pending_start[4]; /* 1: stop at VBI, 2: start at following VBI. */
    unsigned char engine_vehicles[4];
    unsigned char engine_levels[4];
    unsigned short engine_frequencies[4], engine_periods[4];
    unsigned short effect_ticks[SLICKS_AUDIO_EFFECT_CHANNELS];
    unsigned char silent_reload_ticks[SLICKS_AUDIO_ONE_SHOT_CHANNELS];
    unsigned short engine_frequency;
    unsigned short engine_period;
    unsigned char effect_priority[SLICKS_AUDIO_EFFECT_CHANNELS];
    unsigned char engine_vehicle;
    unsigned char engine_sample_block;
    unsigned char last_effect_sample_block;
    unsigned char last_effect_priority;
    unsigned char engine_started;
    unsigned char music_started;
    unsigned short sound_volume;
    unsigned short music_volume;
    unsigned char volume_dirty;
    unsigned char ready;
};

int slicks_amiga_audio_create(struct SlicksAmigaAudio *audio,
                              const unsigned char *resource,
                              unsigned long resource_size);
int slicks_amiga_audio_add_music(struct SlicksAmigaAudio *audio,
                                 const unsigned char *wave,
                                 unsigned long wave_size);
/* No hardware writes; call after loading/editing configuration. */
void slicks_amiga_audio_set_volume(struct SlicksAmigaAudio *audio,
                                   short sounds,short background);
void slicks_amiga_audio_start_music(struct SlicksAmigaAudio *audio);
void slicks_amiga_audio_start_engine(struct SlicksAmigaAudio *audio,
                                     unsigned short vehicle,
                                     unsigned short priority);
void slicks_amiga_audio_start_engines(struct SlicksAmigaAudio *audio,
                                     const unsigned short vehicles[4],unsigned mask);
void slicks_amiga_audio_update_engines(struct SlicksAmigaAudio *audio,
                                      const long vx[4],const long vy[4]);
void slicks_amiga_audio_update_speeds(struct SlicksAmigaAudio *audio,
                                     const unsigned long speeds[4]);
void slicks_amiga_audio_update(struct SlicksAmigaAudio *audio,
                               long velocity_x, long velocity_y);
void slicks_amiga_audio_play_effect(struct SlicksAmigaAudio *audio,
                                    unsigned short sample_block,
                                    unsigned short flags,
                                    unsigned short priority);
void slicks_amiga_audio_stop(struct SlicksAmigaAudio *audio);
void slicks_amiga_audio_destroy(struct SlicksAmigaAudio *audio);
/* Called once by the takeover VBI, independently of game updates. */
void slicks_amiga_audio_vblank(void);
void slicks_amiga_audio_tick(struct SlicksAmigaAudio *audio);

#ifdef __cplusplus
}
#endif
#endif
