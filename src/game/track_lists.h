#ifndef SLICKS_TRACK_LISTS_H
#define SLICKS_TRACK_LISTS_H
#include "track_playlist.h"

/* SLICKS.TRK, original reader 2d28a: SSTrk/1a, big-endian list count,
 * then count + 21-byte title + count eight-byte DOS base names per list.
 * Views borrow caller-owned file bytes. No captured frames or guest RAM. */
struct SlicksTrackLists { const unsigned char *bytes; unsigned long size; unsigned short count; };
struct SlicksTrackListView { const unsigned char *title,*names; unsigned short count; };
static inline unsigned slicks_track_list_word(const unsigned char *p)
{ return (unsigned)p[0]*256+p[1]; }
static inline int slicks_track_lists_open(struct SlicksTrackLists *out,
    const unsigned char *bytes,unsigned long size)
{
    if(!out || !bytes || size<8 || bytes[0]!='S' || bytes[1]!='S' || bytes[2]!='T' || bytes[3]!='r') return -1;
    /* DOS checks only the first four magic bytes and skips bytes 4 and 5. */
    unsigned count=slicks_track_list_word(bytes+6);
    if(count>32767) return -1;
    unsigned long at=8;
    for(unsigned i=0;i<count;++i) {
        if(size-at<23) return -1;
        unsigned tracks=slicks_track_list_word(bytes+at),length=0;
        if(tracks>32767) return -1;
        while(length<21 && bytes[at+2+length]) ++length;
        if(length==21) return -1;
        unsigned long record=23UL+8UL*tracks;
        if(record>size-at) return -1;
        at+=record;
    }
    /* Like DOS, ignore bytes beyond the declared records. Unlike DOS,
     * reject truncation before publishing a partial catalogue. */
    *out=(struct SlicksTrackLists){bytes,size,(unsigned short)count}; return 0;
}
static inline int slicks_track_lists_get(const struct SlicksTrackLists *lists,
    unsigned index,struct SlicksTrackListView *out)
{
    if(!lists || !lists->bytes || !out || index>=lists->count) return -1;
    unsigned long at=8;
    for(unsigned i=0;i<index;++i) at+=23UL+8UL*slicks_track_list_word(lists->bytes+at);
    *out=(struct SlicksTrackListView){lists->bytes+at+2,lists->bytes+at+23,
        (unsigned short)slicks_track_list_word(lists->bytes+at)};
    return 0;
}
static inline int slicks_track_list_name_equal(const unsigned char *stored,const unsigned char *name)
{
    unsigned i=0;
    while(i<8 && stored[i] && name[i]) {
        unsigned a=stored[i],b=name[i];
        if(a>='a' && a<='z') a-=32;
        if(b>='a' && b<='z') b-=32;
        if(a!=b) return 0;
        ++i;
    }
    if(i==8) return !name[8];
    return !stored[i] && !name[i];
}
/* Original 11035 ASCII-case-insensitive first-match lookup. Missing tracks are skipped;
 * duplicates and file order survive. Check capacity before changing state. */
static inline int slicks_track_lists_select(const struct SlicksTrackLists *lists,unsigned index,
    struct SlicksTrackPlaylist *playlist,unsigned total,
    const unsigned char *(*name)(void *,unsigned),void *context)
{
    struct SlicksTrackListView view;
    if(!slicks_track_playlist_valid(playlist) || !name || total>32767 ||
       slicks_track_lists_get(lists,index,&view)) return -1;
    for(unsigned pass=0;pass<2;++pass) {
        unsigned count=0;
        for(unsigned i=0;i<view.count;++i) for(unsigned j=0;j<total;++j)
            if(slicks_track_list_name_equal(view.names+8UL*i,name(context,j))) {
                if(count==playlist->capacity) return -1;
                if(pass) playlist->tracks[count]=(short)j;
                ++count; break;
            }
        if(pass) playlist->count=(unsigned short)count;
    }
    return 0;
}

/* Original 2d4db byte layout. Serialize into separate caller-owned storage;
 * the platform installs this with the existing safe-save transaction. Append
 * uses remove=-1 and title; delete uses a valid remove and no title. */
static inline long slicks_track_lists_write(const struct SlicksTrackLists *lists,int remove,
    const unsigned char *title,const struct SlicksTrackPlaylist *playlist,unsigned total,
    const unsigned char *(*name)(void *,unsigned),void *context,
    unsigned char *out,unsigned long capacity)
{
    if(!lists || !lists->bytes || !out || out==lists->bytes || capacity<8 ||
       (title?(remove!=-1 || lists->count==32767):
        (remove<0 || (unsigned)remove>=lists->count))) return -1;
    unsigned title_length=0;
    unsigned long need=8;
    if(title) {
        if(!slicks_track_playlist_valid(playlist) || playlist->count>32767 || !name) return -1;
        while(title_length<21 && title[title_length]) ++title_length;
        if(title_length>20) return -1;
        for(unsigned i=0;i<playlist->count;++i)
            if(playlist->tracks[i]<0 || (unsigned)playlist->tracks[i]>=total) return -1;
        need+=23UL+8UL*playlist->count;
    }
    if(need>capacity) return -1;
    for(unsigned i=0;i<lists->count;++i) if((int)i!=remove) {
        struct SlicksTrackListView v; if(slicks_track_lists_get(lists,i,&v)) return -1;
        unsigned long record=23UL+8UL*v.count;
        if(record>capacity-need) return -1;
        need+=record;
    }
    if(need>capacity) return -1;
    const unsigned char header[6]={'S','S','T','r','k',26};
    for(unsigned i=0;i<6;++i) out[i]=header[i];
    unsigned count=title?lists->count+1:lists->count-1;
    out[6]=(unsigned char)(count>>8); out[7]=(unsigned char)count;
    unsigned long at=8;
    for(unsigned i=0;i<lists->count;++i) if((int)i!=remove) {
        struct SlicksTrackListView v;
        /* Already checked in preflight; keep the success dependency explicit
         * for the target compiler's definite-initialization analysis. */
        if(slicks_track_lists_get(lists,i,&v)) return -1;
        const unsigned char *source=v.title-2; unsigned long length=23UL+8UL*v.count;
        for(unsigned long j=0;j<length;++j) out[at++]=source[j];
    }
    if(title) {
        out[at++]=(unsigned char)(playlist->count>>8); out[at++]=(unsigned char)playlist->count;
        for(unsigned i=0;i<21;++i) out[at++]=i<title_length?title[i]:0;
        for(unsigned i=0;i<playlist->count;++i) {
            const unsigned char *s=name(context,(unsigned)playlist->tracks[i]); unsigned j=0;
            for(;j<8 && s[j];++j) out[at++]=s[j];
            for(;j<8;++j) out[at++]=0;
        }
    }
    return (long)at;
}
#endif
