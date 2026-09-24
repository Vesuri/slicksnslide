#ifndef SLICKS_HELP_TEXT_H
#define SLICKS_HELP_TEXT_H
#include "help_index.h"

/* Original 31923, in place. LF is removed, CR remains a line separator,
 * '<' becomes the command introducer 11, and '<>' becomes page separator 12.
 * Preserve the original's treatment of '<<' and its existing tail bytes:
 * it does not append a new terminator at the compacted write cursor. */
static inline int slicks_help_preprocess(unsigned char *text,unsigned capacity)
{
    if(!text) return -1;
    unsigned length=0;
    while(length<capacity && text[length]) ++length;
    if(length==capacity) return -1;
    unsigned read=0,write=0;
    while(read<length) {
        unsigned c=text[read];
        if(c=='<') {
            if(text[read+1]=='<') { text[write]='<'; ++read; }
            if(text[read+1]=='>') { text[write]=12; ++read; }
            else text[write]=11;
            ++write;
        } else if(c==10) text[write]=0;
        else text[write++]=(unsigned char)c;
        ++read;
    }
    return 0;
}

/* Original 32048: load until <e, <E, <!, DOS EOF (26) or stream EOF.
 * The byte after '<' is sufficient: this is not a general XML/tag parser.
 * Measure before writing so a small destination cannot corrupt the viewer. */
static inline int slicks_help_load_chapter(const unsigned char *source,unsigned long size,
    unsigned long offset,unsigned char *chapter,unsigned capacity,unsigned *length)
{
    if(!source || !chapter || !length || offset>size) return -1;
    unsigned long at=offset,end;
    int previous=-1;
    for(;;) {
        int c=slicks_help_byte(source,size,&at);
        if(c<0 || c==26 || (previous=='<' && (c=='e' || c=='E' || c=='!'))) {
            end=at-(c<0?0:1);
            if(previous=='<') --end;
            break;
        }
        previous=c;
    }
    unsigned long bytes=end-offset;
    if(bytes+2>capacity) return -1;
    for(unsigned long i=0;i<bytes;++i) chapter[i]=source[offset+i];
    chapter[bytes]=chapter[bytes+1]=0;
    if(slicks_help_preprocess(chapter,(unsigned)bytes+2)) return -1;
    unsigned n=0; while(chapter[n]) ++n;
    *length=n; return 0;
}
#endif
