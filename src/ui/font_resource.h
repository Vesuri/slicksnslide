#ifndef SLICKS_FONT_RESOURCE_H
#define SLICKS_FONT_RESOURCE_H

/* Original 2fc0c font loader: archive wrapper -> runtime header, palette,
 * character codes, widths, then glyph rows padded to four bytes. This loader
 * is independent of the smaller HUD cache and supports all menu fonts.
 * Returns runtime byte count, or -1 without output writes on invalid input. */
static inline long slicks_font_resource_size(const unsigned char *resource,unsigned long size)
{
    if(!resource || size<10 || resource[0]!=0xc3) return -1;
    unsigned count=resource[4],height=resource[6],colours=resource[9];
    unsigned long header=6UL+colours+2UL*count,packed=4UL+header,padded=header;
    if(!count || !height || packed>size) return -1;
    for(unsigned i=0;i<count;++i) {
        unsigned width=resource[10+colours+count+i];
        packed+=(unsigned long)width*height;
        padded+=(unsigned long)((width+3)&~3U)*height;
    }
    return packed==size?(long)padded:-1;
}
static inline long slicks_decode_font_resource(const unsigned char *resource,
    unsigned long size,unsigned char *runtime,unsigned long capacity)
{
    long required=slicks_font_resource_size(resource,size);
    if(required<0 || !runtime || (unsigned long)required>capacity) return -1;
    unsigned count=resource[4],height=resource[6],colours=resource[9];
    unsigned long header=6UL+colours+2UL*count;
    for(unsigned long i=0;i<header;++i) runtime[i]=resource[i+4];
    unsigned long source=4+header,destination=header;
    for(unsigned i=0;i<count;++i) {
        unsigned width=resource[10+colours+count+i],stride=(width+3)&~3U;
        for(unsigned row=0;row<height;++row) {
            for(unsigned x=0;x<width;++x) runtime[destination+x]=resource[source++];
            for(unsigned x=width;x<stride;++x) runtime[destination+x]=0;
            destination+=stride;
        }
    }
    return required;
}
#endif
