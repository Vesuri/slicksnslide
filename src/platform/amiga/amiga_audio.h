#ifndef SLICKS_AMIGA_AUDIO_H
#define SLICKS_AMIGA_AUDIO_H

#define SLICKS_AUDIO_ENGINE_COUNT 10
#define SLICKS_AUDIO_SAMPLE_COUNT 13

struct SlicksAmigaSample {
    signed char *data;
    unsigned short bytes;
    unsigned short period;
};

struct SlicksAmigaAudio {
    struct SlicksAmigaSample samples[SLICKS_AUDIO_SAMPLE_COUNT];
    unsigned long previous_collisions;
    unsigned long previous_trails;
    unsigned short collision_ticks;
    unsigned short trail_ticks;
    unsigned char engine_vehicle;
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
                                     unsigned short volume);
void slicks_amiga_audio_update(struct SlicksAmigaAudio *audio,
                               short speed, unsigned long collisions,
                               unsigned long trails);
void slicks_amiga_audio_stop(struct SlicksAmigaAudio *audio);
void slicks_amiga_audio_destroy(struct SlicksAmigaAudio *audio);

#endif
