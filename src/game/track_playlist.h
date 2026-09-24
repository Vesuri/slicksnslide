#ifndef SLICKS_TRACK_PLAYLIST_H
#define SLICKS_TRACK_PLAYLIST_H

/* DS:0090 count and DS:062a far array, supplied as native caller-owned
 * storage. Preserve selection order and unused tail words like DOS. */
struct SlicksTrackPlaylist { short *tracks; unsigned short count,capacity; };
static inline int slicks_track_playlist_valid(const struct SlicksTrackPlaylist *p)
{ return p && p->tracks && p->count<=p->capacity; }

/* 27583..2765f: first occurrence toggles out, otherwise append a named track. */
static inline int slicks_track_playlist_toggle(struct SlicksTrackPlaylist *p,
    short track,unsigned char has_name)
{
    if(!slicks_track_playlist_valid(p) || track<0) return -1;
    unsigned i=0; while(i<p->count && p->tracks[i]!=track) ++i;
    if(i<p->count) {
        --p->count;
        for(;i<p->count;++i) p->tracks[i]=p->tracks[i+1];
    } else if(has_name) {
        if(p->count==p->capacity) return -1;
        p->tracks[p->count++]=track;
    }
    return 0;
}
static inline int slicks_track_playlist_all(struct SlicksTrackPlaylist *p,unsigned count)
{
    if(!slicks_track_playlist_valid(p) || count>p->capacity || count>32767) return -1;
    p->count=0;
    while(p->count<count) { p->tracks[p->count]=(short)p->count; ++p->count; }
    return 0;
}
static inline unsigned slicks_track_random_index(unsigned count,unsigned long *seed)
{
    *seed=(*seed*0x015a4e35UL+1)&0xffffffffUL;
    return (unsigned)(count*((*seed>>16)&0x7fffUL)/32768UL);
}
/* 26d34: swap every entry with a draw over the FULL list, not Fisher-Yates. */
static inline int slicks_track_playlist_shuffle(struct SlicksTrackPlaylist *p,unsigned long *seed)
{
    if(!slicks_track_playlist_valid(p) || !seed || p->count>32767) return -1;
    for(unsigned i=0;i<p->count;++i) {
        unsigned j=slicks_track_random_index(p->count,seed);
        short track=p->tracks[i]; p->tracks[i]=p->tracks[j]; p->tracks[j]=track;
    }
    return 0;
}
/* 2771c..277ff: reject duplicate draws; each retry still advances the RNG. */
static inline int slicks_track_playlist_random(struct SlicksTrackPlaylist *p,
    unsigned tracks,unsigned wanted,unsigned long *seed)
{
    if(!slicks_track_playlist_valid(p) || !seed || tracks>32767 || wanted>tracks || wanted>p->capacity) return -1;
    p->count=0;
    while(p->count<wanted) {
        short candidate=(short)slicks_track_random_index(tracks,seed);
        p->tracks[p->count]=candidate;
        unsigned i=0; while(i<p->count && p->tracks[i]!=candidate) ++i;
        if(i==p->count) ++p->count;
    }
    return 0;
}
#endif
