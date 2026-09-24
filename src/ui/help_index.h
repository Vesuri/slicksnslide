#ifndef SLICKS_HELP_INDEX_H
#define SLICKS_HELP_INDEX_H
struct SlicksHelpLocation { unsigned long chapter; unsigned short page,anchor; };

/* Compact index written by original 3231d: zero + NUL-terminated anchor;
 * 1/2 + little-endian 24-bit chapter/page source offset. Builder appends a
 * final chapter marker. The lookup does not use page offsets directly. */
static inline int slicks_help_index_valid(const unsigned char *index,unsigned size)
{
    if(!index || size<4 || index[0]!=1) return 0;
    unsigned at=0,last=0;
    while(at<size) {
        last=index[at++];
        if(!last) {
            unsigned start=at;
            while(at<size && index[at]) ++at;
            if(at==size || at-start>20) return 0;
            ++at;
        } else {
            if(last>2 || size-at<3) return 0;
            at+=3;
        }
    }
    return last==1;
}
static inline unsigned char slicks_help_upper(unsigned char c)
{ return c>='a' && c<='z'?(unsigned char)(c-'a'+'A'):c; }

struct SlicksHelpIndexInfo {
    unsigned size,records,headers;
    unsigned long body,chapter_capacity;
    /* Source slices; formatting is interpreted by the viewer, not the index. */
    unsigned long header_start[20];
    unsigned short header_length[20];
};
static inline int slicks_help_byte(const unsigned char *source,unsigned long size,unsigned long *at)
{ return *at<size?source[(*at)++]:-1; }
static inline void slicks_help_emit(unsigned char *index,unsigned *used,unsigned value)
{ if(index) index[*used]=(unsigned char)value; ++*used; }

/* Original 32464/3231d. First pass measures; second pass emits only after
 * capacity validation, leaving caller storage untouched on failure. Source
 * offsets are physical bytes, including CR/LF, matching the archive stream. */
static inline int slicks_help_build_index(const unsigned char *source,unsigned long size,
    unsigned char *index,unsigned capacity,struct SlicksHelpIndexInfo *info)
{
    if(!source || !info || size>0xffffffUL) return -1;
    struct SlicksHelpIndexInfo result={0};
    for(unsigned pass=0;pass<2;++pass) {
        result=(struct SlicksHelpIndexInfo){0}; result.chapter_capacity=1000;
        unsigned long at=0,previous=0; unsigned have_previous=0;
        /* fgets(...,200) consumes at most 199 bytes, including newline. The
         * first non-header line is consumed too, exactly as in the DOS code. */
        while(result.headers<20) {
            unsigned long start=at; unsigned n=0;
            result.body=start;
            while(at<size && n<199) { ++n; if(source[at++]=='\n') break; }
            if(!n || source[start]!='!') break;
            result.header_start[result.headers]=start+1;
            result.header_length[result.headers++]=(unsigned short)(n-1);
        }
        unsigned kind=1,done=0;
        for(;;) {
            unsigned char *dest=pass?index:0;
            slicks_help_emit(dest,&result.size,kind); ++result.records;
            if(!kind) {
                for(unsigned n=0;n<20;++n) {
                    int c=slicks_help_byte(source,size,&at);
                    if(c<0) break;
                    c=slicks_help_upper((unsigned char)c);
                    if(c<'A') break;
                    slicks_help_emit(dest,&result.size,(unsigned)c);
                }
                slicks_help_emit(dest,&result.size,0);
            } else {
                for(unsigned shift=0;shift<24;shift+=8)
                    slicks_help_emit(dest,&result.size,(unsigned)(at>>shift));
                if(kind==1) {
                    if(have_previous && at-previous+200>result.chapter_capacity)
                        result.chapter_capacity=at-previous+200;
                    previous=at; have_previous=1;
                }
            }
            if(done) break;
            for(;;) {
                int c=slicks_help_byte(source,size,&at);
                if(c<0 || c==26) { done=1; kind=1; break; }
                if(c=='<') {
                    c=slicks_help_byte(source,size,&at);
                    if(c=='!') { done=1; kind=1; break; }
                    if(c=='#') { kind=0; break; }
                    if(c=='e' || c=='E') {
                        while(slicks_help_byte(source,size,&at)>13) {}
                        kind=1; break;
                    }
                    if(c=='>') { kind=2; break; }
                }
                if(c==12) { kind=2; break; }
            }
        }
        if(!pass && (!index || result.size>capacity)) return -1;
    }
    *info=result; return 0;
}

/* Original 32631..327db: try country-prefixed name first, then bare name.
 * Multiple matching anchors within a chapter select the last one. Stop at
 * the next chapter, not the first anchor. On missing/malformed input leave
 * the destination untouched (malformed DOS indexes otherwise read past end). */
static inline int slicks_help_find(const unsigned char *index,unsigned size,
    const unsigned char *prefix,const unsigned char *topic,struct SlicksHelpLocation *out)
{
    if(!prefix || !topic || !out || !slicks_help_index_valid(index,size)) return -1;
    unsigned p=0,t=0;
    while(p<6 && prefix[p]) ++p;
    while(t<21 && topic[t]) ++t;
    if(p>5 || t>20) return -1;
    for(unsigned pass=0;pass<2;++pass) {
        unsigned at=0,page=0,anchor=0,matched=0;
        unsigned long chapter=0;
        struct SlicksHelpLocation found={0,0,0};
        while(at<size) {
            unsigned kind=index[at++];
            if(kind) {
                unsigned long offset=index[at]|((unsigned long)index[at+1]<<8)|((unsigned long)index[at+2]<<16);
                at+=3;
                if(kind==1) {
                    if(matched) { *out=found; return 0; }
                    chapter=offset; page=0;
                } else ++page;
                anchor=0;
            } else {
                unsigned start=at; while(index[at]) ++at;
                unsigned length=at-start,pre=pass?0:p,match=length==pre+t;
                for(unsigned i=0;match && i<length;++i)
                    if(index[start+i]!=slicks_help_upper(i<pre?prefix[i]:topic[i-pre])) match=0;
                if(match) { found=(struct SlicksHelpLocation){chapter,(unsigned short)page,(unsigned short)anchor}; matched=1; }
                ++anchor; ++at;
            }
        }
    }
    return 1;
}
#endif
