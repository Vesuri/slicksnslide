#ifndef SLICKS_HUD_BACKGROUND_H
#define SLICKS_HUD_BACKGROUND_H

/* HUD asset subset of 2e51a: original tI/legacy header and escape RLE.
 * Supports the background and variable-sized weapon/tuning icons.
 * Produce its authoritative chunky pixels instead of four VGA byte banks.
 * Unknown geometry/palette modes and truncated input fail closed. */
static inline int slicks_decode_hud_image(const unsigned char *source,
    unsigned long size, unsigned char *pixels, unsigned long capacity,
    unsigned short *image_width, unsigned short *image_height)
{
    unsigned long at=3, made=0;
    unsigned type,width,height;
    if(!source || !pixels || size<3) return -1;
    if(source[0]==0x74 && source[1]==0x49 && source[2]==0x1a) {
        if(size<7) return -1;
        type=source[3];
        width=((unsigned)(source[4]&0xf0)<<4)|source[5];
        height=((unsigned)(source[4]&15)<<8)|source[6];
        at=7;
    } else {
        if(source[0]>3) return -1;
        type=source[0]>>1;
        width=((unsigned)(source[0]&1)<<8)|source[1]; height=source[2];
    }
    if(!width || width>320 || !height || height>200 || type>1) return -1;
    unsigned long total=(unsigned long)width*height;
    if(total>capacity) return -1;
    if(!type) {
        if(size-at!=total) return -1;
        for(;made<total;++made) pixels[made]=source[at++];
    } else {
        if(at>=size) return -1;
        unsigned char escape=source[at++];
        while(made<total) {
            unsigned count=1;
            if(at>=size) return -1;
            unsigned char value=source[at++];
            if(value==escape) {
                if(at>=size) return -1;
                count=source[at++];
                if(!count) count=1;
                else {
                    if(at>=size) return -1;
                    value=source[at++];
                }
            }
            while(count--) {
                if(made<total) pixels[made]=value;
                else {
                    /* 2e7ae subtracts the complete final run before testing
                     * signed remaining size. vir7/vir10 contain one excess
                     * pixel, which spills into the next VGA bank's first
                     * byte. Preserve in-allocation writes, never overflow
                     * the native destination. */
                    unsigned long stride=(width+3)/4,plane_size=stride*height;
                    unsigned long x=made%width,y=made/width;
                    unsigned long offset=(x&3)*plane_size+y*stride+x/4;
                    if(offset>=plane_size*4) return -1;
                    unsigned long plane=offset/plane_size;
                    offset%=plane_size;
                    x=(offset%stride)*4+plane; y=offset/stride;
                    if(x<width) pixels[y*width+x]=value;
                }
                ++made;
            }
        }
    }
    if(at!=size) return -1;
    *image_width=(unsigned short)width; *image_height=(unsigned short)height;
    return 0;
}

static inline int slicks_decode_hud_background(const unsigned char *source,
    unsigned long size, unsigned char pixels[320*16])
{
    unsigned short width,height;
    if(slicks_decode_hud_image(source,size,pixels,320*16,&width,&height)) return -1;
    return width==320 && height==16?0:-1;
}
#endif
