/* Compile the actual adapter. Only allocation and custom registers are mocked. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "host_archive.h"
enum { MEMF_CHIP=1,MEMF_CLEAR=2 };
static unsigned long allocated_bytes;
static long fail_after=-1;
static void *AllocMem(unsigned long n,unsigned flags)
{ (void)flags; if(!fail_after) return 0; if(fail_after>0) --fail_after;
  void *p=calloc(1,n); if(p) allocated_bytes+=n; return p; }
static void FreeMem(void *p,unsigned long n)
{ assert(allocated_bytes>=n); allocated_bytes-=n; free(p); }
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

static void verify_extreme_pitches(struct SlicksAmigaAudio *a)
{
    unsigned cases=0,banks=0; double worst=0;
    for(unsigned vehicle=0;vehicle<10;++vehicle) {
        unsigned short vehicles[4]={(unsigned short)vehicle,0,0,0};
        slicks_amiga_audio_start_engines(a,vehicles,1);
        for(unsigned speed=0;speed<65536;++speed) {
            unsigned long speeds[4]={speed,0,0,0};
            slicks_amiga_audio_update_speeds(a,speeds);
            slicks_amiga_audio_tick(a); slicks_amiga_audio_tick(a);
            const struct SlicksAmigaSample *sample=engine_sample(a,0);
            const struct SlicksAmigaSample *base=&a->samples[engine_sample_block[vehicle]];
            unsigned period=a->engine_periods[0],frequency=a->engine_frequencies[0];
            assert(sample->data && sample->bytes && !(sample->bytes&1));
            assert(period>=124 && period<=65535);
            assert(words[(AUDIO_BASE(0)+6)/2]==period);
            assert(longs[AUDIO_BASE(0)/4]==(unsigned long)sample->data);
            assert(words[(AUDIO_BASE(0)+4)/2]==sample->bytes/2);
            banks|=1U<<a->engine_levels[0];
            if(frequency>=55) {
                double actual=(double)PAL_AUDIO_CLOCK*base->bytes/(period*(double)sample->bytes);
                double error=actual/frequency-1; if(error<0) error=-error;
                if(error>worst) worst=error;
                assert(error<0.009); /* Integer period/rate quantization, not clamping. */
            }
            ++cases;
        }
    }
    assert(banks==7);
    /* Borrow an engine, change bank while inaudible, resume the latest bank. */
    const unsigned short vehicles[4]={0,0,0,0};
    slicks_amiga_audio_start_engines(a,vehicles,15);
    slicks_amiga_audio_play_effect(a,1,0,100);
    unsigned long speeds[4]={8500,0,0,0};
    slicks_amiga_audio_update_speeds(a,speeds);
    assert(a->engine_levels[0]==2 && a->channels.owner[0]==SLICKS_AUDIO_EFFECT);
    for(unsigned i=0;i<100;++i) slicks_amiga_audio_tick(a);
    assert(a->channels.owner[0]==SLICKS_AUDIO_ENGINE);
    assert(longs[AUDIO_BASE(0)/4]==(unsigned long)engine_sample(a,0)->data);
    slicks_amiga_audio_stop(a);
    printf("Paula limits: %u vehicle/speed cases, all three waveform banks, worst representable pitch error %.4f%%; borrowed engine resumes latest bank\n",cases,worst*100);
}

static void verify_bank_gain(void)
{
    unsigned char bank[27*14]; unsigned at=0;
    struct SlicksAmigaAudio a;
    for(unsigned i=0;i<27;++i) {
        unsigned gain=i*9;
        /* Every second entry inherits the previous entry's explicit gain. */
        unsigned char header[]={ 't','S',0,0,0,4,0,(unsigned char)gain,0x2b,0x11 };
        unsigned length=i%2?8:10;
        if(i%2) {header[6]=0x2b;header[7]=0x11;}
        memcpy(bank+at,header,length); at+=length;
        bank[at++]=0;bank[at++]=127;bank[at++]=128;bank[at++]=255;
    }
    assert(!slicks_amiga_audio_create(&a,bank,at));
    for(unsigned i=0;i<27;++i) {
        unsigned gain=(i&~1U)*9;
        assert(a.samples[i].data[0]==-(int)(128*gain/255));
        assert(a.samples[i].data[1]==-(int)(gain/255));
        assert(a.samples[i].data[2]==0);
        assert(a.samples[i].data[3]==(int)(127*gain/255));
    }
    slicks_amiga_audio_destroy(&a); assert(!allocated_bytes);
    puts("Sample bank gains: explicit and inherited gains retain signed PCM semantics");
}

static void verify_bank_one_shots(struct SlicksAmigaAudio *a)
{
    const unsigned short vehicles[4]={0,2,6,9};
    for(unsigned sample=0;sample<SLICKS_AUDIO_SAMPLE_COUNT;++sample) {
        slicks_amiga_audio_start_engines(a,vehicles,15);
        slicks_amiga_audio_tick(a); slicks_amiga_audio_tick(a);
        unsigned channel=a->channels.next_borrow;
        slicks_amiga_audio_play_effect(a,(unsigned short)sample,0,30);
        assert(a->channels.owner[channel]==SLICKS_AUDIO_EFFECT);
        slicks_amiga_audio_tick(a); slicks_amiga_audio_tick(a);
        assert(longs[AUDIO_BASE(channel)/4]==(unsigned long)a->samples[sample].data);
        slicks_amiga_audio_tick(a);
        assert(longs[AUDIO_BASE(channel)/4]==(unsigned long)a->silence);
        assert(words[(AUDIO_BASE(channel)+4)/2]==1);
        unsigned frames=0;
        while(a->channels.owner[channel]==SLICKS_AUDIO_EFFECT || a->pending_start[channel]) {
            assert(++frames<500);
            slicks_amiga_audio_tick(a);
        }
        assert(a->channels.owner[channel]==SLICKS_AUDIO_ENGINE);
        assert(longs[AUDIO_BASE(channel)/4]==(unsigned long)engine_sample(a,channel)->data);
        assert(!a->silent_reload_ticks[channel] && !a->effect_ticks[channel]);
        slicks_amiga_audio_stop(a);
    }
    puts("All 27 supplied samples: one-shot silent reload and borrowed-engine return pass");
}

int main(void)
{
    verify_bank_gain();
    verify_lifecycle();
    unsigned char bank[131691]; struct SlicksAmigaAudio bank_audio;
    assert(host_archive_load("ref/SLICKS.000","samples.dat",bank,sizeof bank)==sizeof bank);
    assert(!slicks_amiga_audio_create(&bank_audio,bank,sizeof bank));
    for(unsigned i=0;i<SLICKS_AUDIO_SAMPLE_COUNT;++i) {
        assert(bank_audio.samples[i].period==322); /* All archive rates are 11025 Hz. */
        assert((unsigned long)bank_audio.samples[i].bytes*322UL>3546895UL/25UL);
    }
    /* Original ID 1 is the embedded WAV; engine ID 17 is NOT tS block 17. */
    assert(bank_audio.samples[1].bytes==5534);
    assert(bank_audio.samples[17].bytes==2050);
    assert(bank_audio.samples[18].bytes==2700);
    assert(bank_audio.samples[26].bytes==5960);
    verify_extreme_pitches(&bank_audio);
    verify_bank_one_shots(&bank_audio);
    slicks_amiga_audio_destroy(&bank_audio);
    assert(!allocated_bytes);
    const unsigned truncated[]={0,1,7,8,8715,8726,8759,sizeof bank-1};
    for(unsigned i=0;i<sizeof truncated/sizeof truncated[0];++i) {
        assert(slicks_amiga_audio_create(&bank_audio,bank,truncated[i])==-1);
        assert(!allocated_bytes);
    }
    const unsigned corrupt[]={8715,8715+8,8715+20,8715+22,8715+34};
    for(unsigned i=0;i<sizeof corrupt/sizeof corrupt[0];++i) {
        unsigned char before=bank[corrupt[i]]; bank[corrupt[i]]=0xff;
        assert(slicks_amiga_audio_create(&bank_audio,bank,sizeof bank)==-1);
        assert(!allocated_bytes); bank[corrupt[i]]=before;
    }
    puts("Mixed sample bank: 27 IDs including WAV, correct engine slot and malformed/truncated cleanup pass");
    for(unsigned failure=0;failure<44;++failure) {
        fail_after=failure;
        assert(slicks_amiga_audio_create(&bank_audio,bank,sizeof bank)==-1);
        assert(!allocated_bytes);
    }
    fail_after=-1;
    puts("Audio allocation: all 44 bank/variant/silence allocation failures clean up completely");
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
