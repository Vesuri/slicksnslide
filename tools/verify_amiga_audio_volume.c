/* Compile the actual adapter. Only allocation and custom registers are mocked. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "host_archive.h"
enum { MEMF_CHIP=1,MEMF_CLEAR=2 };
static void *AllocMem(unsigned long n,unsigned flags) { (void)flags; return calloc(1,n); }
static void FreeMem(void *p,unsigned long n) { (void)n; free(p); }
static unsigned short words[256];
static unsigned long longs[128];
static unsigned accesses[256];
static volatile unsigned short *custom_word(unsigned offset)
{
    assert(offset<512 && !(offset&1)); ++accesses[offset/2];
    if(offset==6) words[3]+=256; /* Raster progress for the DMA startup wait. */
    return &words[offset/2];
}
static volatile unsigned long *custom_long(unsigned offset)
{ assert(offset<512 && !(offset&3)); return &longs[offset/4]; }
#define CUSTOM_WORD(offset) (*custom_word(offset))
#define CUSTOM_LONG(offset) (*custom_long(offset))
#define SLICKS_AUDIO_HOST_TEST
#include "../src/platform/amiga/amiga_audio.c"

static void verify_lifecycle(void)
{
    signed char sample[1000]={0};
    const unsigned short vehicles[4]={0,2,6,9};
    const unsigned long speeds[4]={50,100,150,200};
    unsigned cases=0;
    /* Stop/pause may arrive before either staged VBI, during playback,
     * after the silent reload, or after an effect has returned its engine. */
    for(unsigned kind=0;kind<3;++kind) for(unsigned ticks=0;ticks<16;++ticks) {
        struct SlicksAmigaAudio a={0};
        a.ready=1; a.silence=sample; a.sound_volume=64; a.music_volume=32;
        a.music=(struct SlicksAmigaSample){sample,1000,322};
        for(unsigned i=0;i<SLICKS_AUDIO_SAMPLE_COUNT;++i) a.samples[i]=a.music;
        slicks_amiga_audio_start_engines(&a,vehicles,15);
        if(kind==1) for(unsigned i=0;i<4;++i)
            slicks_amiga_audio_play_effect(&a,i,0,20+i);
        if(kind==2) slicks_amiga_audio_start_music(&a);
        for(unsigned i=0;i<ticks;++i) slicks_amiga_audio_tick(&a);
        slicks_amiga_audio_stop(&a);
        assert(words[REG_DMACON/2]==15); /* Clear all four audio DMA bits. */
        unsigned dma=accesses[REG_DMACON/2];
        for(unsigned i=0;i<32;++i) slicks_amiga_audio_tick(&a);
        assert(accesses[REG_DMACON/2]==dma); /* No stale pending restart. */
        assert(!a.engine_started && !a.music_started && !a.channels.engine_mask);
        for(unsigned i=0;i<4;++i) {
            assert(a.channels.owner[i]==SLICKS_AUDIO_IDLE);
            assert(!a.pending_start[i] && !a.effect_ticks[i] && !a.silent_reload_ticks[i]);
            assert(!words[(AUDIO_BASE(i)+8)/2]);
        }
        /* Resume/new race rebuilds every engine at its latest measured speed. */
        slicks_amiga_audio_start_engines(&a,vehicles,15);
        slicks_amiga_audio_update_speeds(&a,speeds);
        slicks_amiga_audio_tick(&a); slicks_amiga_audio_tick(&a);
        for(unsigned i=0;i<4;++i) {
            assert(a.channels.owner[i]==SLICKS_AUDIO_ENGINE && !a.pending_start[i]);
            assert(words[(AUDIO_BASE(i)+6)/2]==a.engine_periods[i]);
            assert(words[(AUDIO_BASE(i)+8)/2]==64);
        }
        /* Results must discard race owners and their pending requests. */
        slicks_amiga_audio_start_music(&a);
        slicks_amiga_audio_tick(&a); slicks_amiga_audio_tick(&a);
        assert(!a.channels.engine_mask && a.channels.owner[3]==SLICKS_AUDIO_MUSIC);
        for(unsigned i=0;i<3;++i) assert(a.channels.owner[i]==SLICKS_AUDIO_IDLE);
        for(unsigned i=0;i<16;++i) slicks_amiga_audio_tick(&a);
        for(unsigned i=0;i<4;++i) assert(a.channels.owner[i]==SLICKS_AUDIO_IDLE);
        slicks_amiga_audio_stop(&a);
        ++cases;
    }
    printf("Audio lifecycle: %u stop/pause/resume/results/restart phase cases pass\n",cases);
}

int main(void)
{
    verify_lifecycle();
    unsigned char bank[131691]; struct SlicksAmigaAudio bank_audio;
    assert(host_archive_load("ref/SLICKS.000","samples.dat",bank,sizeof bank)==sizeof bank);
    assert(!slicks_amiga_audio_create(&bank_audio,bank,sizeof bank));
    for(unsigned i=0;i<26;++i) {
        assert(bank_audio.samples[i].period==322); /* All archive rates are 11025 Hz. */
        assert((unsigned long)bank_audio.samples[i].bytes*322UL>3546895UL/25UL);
    }
    slicks_amiga_audio_destroy(&bank_audio);
    signed char sample[1000]={0}; unsigned cases=0;
    for(unsigned mask=0;mask<16;++mask) {
        struct SlicksAmigaAudio a={0}; a.ready=1; a.silence=sample; a.sound_volume=64;
        for(unsigned i=0;i<SLICKS_AUDIO_SAMPLE_COUNT;++i)
            a.samples[i]=(struct SlicksAmigaSample){sample,1000,322};
        const unsigned short vehicles[4]={0,2,6,9};
        const long vx[4]={10,20,30,40},vy[4]={1,2,3,4};
        slicks_amiga_audio_start_engines(&a,vehicles,mask);
        slicks_amiga_audio_update_engines(&a,vx,vy);
        slicks_amiga_audio_tick(&a); slicks_amiga_audio_tick(&a);
        for(unsigned i=0;i<4;++i) {
            assert(a.channels.owner[i]==((mask&(1U<<i))?SLICKS_AUDIO_ENGINE:SLICKS_AUDIO_IDLE));
            if(mask&(1U<<i)) assert(words[(AUDIO_BASE(i)+6)/2]==a.engine_periods[i]);
        }
        for(unsigned i=0;i<4;++i) slicks_amiga_audio_play_effect(&a,i,0,20+i);
        for(unsigned i=0;i<4;++i) assert(a.channels.owner[i]==SLICKS_AUDIO_EFFECT);
        struct SlicksAudioChannels before=a.channels;
        slicks_amiga_audio_play_effect(&a,5,0,10); /* Too weak to replace any effect. */
        assert(!memcmp(&before,&a.channels,sizeof before));
        slicks_amiga_audio_play_effect(&a,5,2,22); /* Original priority duplicate rule. */
        assert(!memcmp(&before,&a.channels,sizeof before));
        long fast[4]={100,200,300,400};
        for(unsigned n=0;n<100;++n) slicks_amiga_audio_update_engines(&a,fast,vy);
        /* Game updates alone must not age one-shots. Conversely the VBI
         * releases them without any further simulation or rendering. */
        for(unsigned i=0;i<4;++i) assert(a.channels.owner[i]==SLICKS_AUDIO_EFFECT);
        for(unsigned n=0;n<12;++n) slicks_amiga_audio_tick(&a);
        for(unsigned i=0;i<4;++i) {
            assert(!a.effect_ticks[i] && !a.silent_reload_ticks[i]);
            assert(a.channels.owner[i]==((mask&(1U<<i))?SLICKS_AUDIO_ENGINE:SLICKS_AUDIO_IDLE));
            if(mask&(1U<<i)) {
                assert(words[(AUDIO_BASE(i)+6)/2]==a.engine_periods[i]);
                assert(longs[AUDIO_BASE(i)/4]==(unsigned long)sample);
                assert(words[(AUDIO_BASE(i)+4)/2]==500);
            }
        }
        slicks_amiga_audio_stop(&a);
        assert(!a.engine_started && !a.channels.engine_mask);
        for(unsigned i=0;i<4;++i) assert(a.channels.owner[i]==SLICKS_AUDIO_IDLE);
    }
    for(unsigned old=0;old<256;++old) for(unsigned incoming=0;incoming<256;++incoming) {
        struct SlicksAudioChannels s={0};
        for(unsigned i=0;i<4;++i) { s.owner[i]=SLICKS_AUDIO_EFFECT; s.priority[i]=(signed char)old; }
        unsigned p=incoming?incoming:1;
        int expected=(signed char)old>=0 && (signed char)old<=(signed char)p?0:-1;
        assert(slicks_audio_channels_request(&s,0,incoming)==expected);
    }
    struct SlicksAudioChannels loops;
    slicks_audio_channels_engines(&loops,15);
    for(unsigned i=0;i<4;++i) assert(slicks_audio_channels_request(&loops,1,10)==(int)i);
    assert(slicks_audio_channels_request(&loops,0,127)==-1);
    puts("Direct audio: all 16 active-car masks, borrowing/resume/latest pitches, 65536 priority pairs and protected loops pass");
    for(unsigned sound=0;sound<=100;++sound) for(unsigned bg=0;bg<=100;++bg) {
        struct SlicksAmigaAudio a={0}; a.ready=1;
        a.music=(struct SlicksAmigaSample){sample,1000,322}; a.silence=sample;
        for(unsigned i=0;i<SLICKS_AUDIO_SAMPLE_COUNT;++i) a.samples[i]=a.music;
        memset(accesses,0,sizeof accesses);
        slicks_amiga_audio_set_volume(&a,sound,bg);
        for(unsigned i=0;i<256;++i) assert(!accesses[i]);
        unsigned effects=64*sound/100,music=64*sound*(bg*5/2)/25500;
        assert(a.sound_volume==effects && a.music_volume==music);
        slicks_amiga_audio_start_engine(&a,0,100);
        slicks_amiga_audio_play_effect(&a,0,0,10);
        slicks_amiga_audio_play_effect(&a,1,0,11);
        slicks_amiga_audio_start_music(&a);
        slicks_amiga_audio_update(&a,10,20);
        slicks_amiga_audio_tick(&a); slicks_amiga_audio_tick(&a);
        assert(words[(AUDIO_BASE(3)+8)/2]==music);
        slicks_amiga_audio_update(&a,10,20);
        assert(!a.volume_dirty);
        /* Runtime mute must affect already-playing channels, without DMA changes. */
        slicks_amiga_audio_set_volume(&a,0,bg);
        unsigned dma=accesses[REG_DMACON/2];
        slicks_amiga_audio_update(&a,10,20);
        assert(accesses[REG_DMACON/2]==dma);
        for(unsigned c=0;c<4;++c) assert(!words[(AUDIO_BASE(c)+8)/2]);
        unsigned before[4];
        for(unsigned c=0;c<4;++c) before[c]=accesses[(AUDIO_BASE(c)+8)/2];
        slicks_amiga_audio_set_volume(&a,0,bg);
        slicks_amiga_audio_update(&a,10,20);
        for(unsigned c=0;c<4;++c) assert(accesses[(AUDIO_BASE(c)+8)/2]==before[c]);
        ++cases;
    }
    printf("Amiga audio volume: %u combinations cover engine, both effects, music, live mute, deferred and unchanged writes\n",cases);
    return 0;
}
