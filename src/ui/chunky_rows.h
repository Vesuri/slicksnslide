#ifndef SLICKS_CHUNKY_ROWS_H
#define SLICKS_CHUNKY_ROWS_H
/* Row helpers for the 320-byte chunky surface, which lives in Chip RAM on
 * the target (about seven cycles per access). Move aligned runs four pixels
 * per access; the result is byte-for-byte that of the plain byte loop. */
typedef unsigned int __attribute__((may_alias)) slicks_chunky_quad;

static inline void slicks_ui_copy_row(unsigned char *destination,const unsigned char *source,int count)
{
    while(count>0 && ((unsigned long)destination&3)) { *destination++=*source++; --count; }
    if(!((unsigned long)source&3))
        for(;count>=4;count-=4,destination+=4,source+=4)
            *(slicks_chunky_quad *)destination=*(const slicks_chunky_quad *)source;
    while(count>0) { *destination++=*source++; --count; }
}
/* Each byte keeps its position, so the byte order of the quad is irrelevant. */
static inline void slicks_ui_remap_row(unsigned char *row,int count,const unsigned char table[256])
{
    while(count>0 && ((unsigned long)row&3)) { *row=table[*row]; ++row; --count; }
    for(;count>=4;count-=4,row+=4) {
        slicks_chunky_quad v=*(slicks_chunky_quad *)row;
        *(slicks_chunky_quad *)row=(slicks_chunky_quad)table[v>>24]<<24|(slicks_chunky_quad)table[(v>>16)&255]<<16|
            (slicks_chunky_quad)table[(v>>8)&255]<<8|table[v&255];
    }
    while(count>0) { *row=table[*row]; ++row; --count; }
}
#endif
