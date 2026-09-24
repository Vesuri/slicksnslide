#ifndef SLICKS_AUDIO_CHANNELS_H
#define SLICKS_AUDIO_CHANNELS_H

/* Allocation only: no PCM arithmetic. Each car has a stable home channel.
 * Engines may be borrowed; effects retain DOS signed-priority/flag semantics. */
enum SlicksAudioOwner { SLICKS_AUDIO_IDLE, SLICKS_AUDIO_ENGINE,
    SLICKS_AUDIO_EFFECT, SLICKS_AUDIO_MUSIC };
struct SlicksAudioChannels {
    unsigned char owner[4], engine_mask, next_borrow;
    signed char priority[4];
};
static inline void slicks_audio_channels_engines(struct SlicksAudioChannels *s,unsigned mask)
{
    *s=(struct SlicksAudioChannels){{0,0,0,0},0,0,{0,0,0,0}};
    s->engine_mask=(unsigned char)(mask&15);
    for(unsigned i=0;i<4;++i) if(mask&(1U<<i)) s->owner[i]=SLICKS_AUDIO_ENGINE;
}
static inline int slicks_audio_channels_request(struct SlicksAudioChannels *s,
    unsigned flags,unsigned priority)
{
    unsigned char p=(unsigned char)priority; if(!p) p=1;
    if(flags&2) for(unsigned i=0;i<4;++i)
        if(s->owner[i]==SLICKS_AUDIO_EFFECT && (unsigned char)s->priority[i]==p) return -1;
    int chosen=-1;
    for(unsigned i=0;i<4;++i) if(s->owner[i]==SLICKS_AUDIO_IDLE) { chosen=(int)i; break; }
    /* Deliberate Paula adaptation: lend an engine before interrupting an
     * existing effect. Rotate only on borrowing, never on ordinary updates. */
    if(chosen<0) for(unsigned n=0;n<4;++n) {
        unsigned i=(s->next_borrow+n)&3;
        if(s->owner[i]==SLICKS_AUDIO_ENGINE) {
            chosen=(int)i; s->next_borrow=(unsigned char)((i+1)&3); break;
        }
    }
    /* Original 29832: first eligible, not numerically lowest priority. */
    if(chosen<0) for(unsigned i=0;i<4;++i)
        if(s->owner[i]==SLICKS_AUDIO_EFFECT && s->priority[i]>=0 &&
           s->priority[i]<=(signed char)p) { chosen=(int)i; break; }
    if(chosen>=0) {
        s->owner[chosen]=SLICKS_AUDIO_EFFECT;
        s->priority[chosen]=(signed char)((flags&1)?0xfe:p);
    }
    return chosen;
}
static inline void slicks_audio_channels_finish(struct SlicksAudioChannels *s,unsigned channel)
{
    if(channel>=4) return;
    s->owner[channel]=(s->engine_mask&(1U<<channel))?SLICKS_AUDIO_ENGINE:SLICKS_AUDIO_IDLE;
    s->priority[channel]=0;
}
#endif
