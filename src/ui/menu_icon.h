#ifndef SLICKS_MENU_ICON_H
#define SLICKS_MENU_ICON_H
#include "chunky_ui.h"

/* Original 2e51a legacy indexed resource variant, used by ohj_*.@I.
 * Keep logical width: DOS pads its four source planes to a multiple of four.
 * Validate the entire stream before publishing pixels or dimensions. */
static inline int slicks_decode_indexed_menu_icon(const unsigned char *resource,
    unsigned long size,unsigned char *pixels,unsigned long capacity,
    unsigned short *width,unsigned short *height)
{
    if(!resource || !pixels || !width || !height || size<3 || resource[0]>3) return -1;
    unsigned w=resource[1]+((resource[0]&1U)<<8),h=resource[2];
    unsigned long total=(unsigned long)w*h;
    if(!w || !h || total>capacity) return -1;
    for(unsigned pass=0;pass<2;++pass) {
        unsigned long at=3,out=0;
        unsigned escape=0;
        if(resource[0]&2) { if(at==size) return -1; escape=resource[at++]; }
        while(out<total) {
            if(at==size) return -1;
            unsigned value=resource[at++],count=1;
            if((resource[0]&2) && value==escape) {
                if(at==size) return -1;
                count=resource[at++];
                if(!count) count=1;
                else { if(at==size) return -1; value=resource[at++]; }
            }
            /* Original assets can finish with an overlong zero run (keyboard
             * icon: 222 decoded bytes for 221 logical pixels). Keep only the
             * image; unlike the DOS loader never spill into adjacent planes. */
            if(count>total-out) count=(unsigned)(total-out);
            if(pass) for(unsigned i=0;i<count;++i) pixels[out+i]=(unsigned char)value;
            out+=count;
        }
        if(at!=size) return -1;
    }
    *width=(unsigned short)w; *height=(unsigned short)h;
    return 0;
}

/* Original 2e0f8/2e2d2 tB1 resource and first-draw palette conversion.
 * Decode once for a palette, as the original B3 -> B2 cached sprite does;
 * subsequent draws retain these indices even if the palette changes.
 * Bit 15 means transparent. RGB fields expand by doubling, not bit-repeat.
 * Supports on-screen, byte-sized dimensions used by original menu icons.
 * Invalid/truncated input leaves all outputs untouched. */
static inline int slicks_decode_menu_icon(const unsigned char *resource,
    unsigned long size,const unsigned char palette[768],unsigned char *pixels,
    unsigned long capacity,unsigned short *width,unsigned short *height)
{
    if(!resource || !palette || !pixels || !width || !height || size<8 ||
       resource[0]!=0x74 || resource[1]!=0xb1 || resource[2]!=0x1a || resource[3] ||
       resource[4] || resource[6]) return -1;
    unsigned w=resource[5],h=resource[7]; unsigned long total=(unsigned long)w*h;
    if(!w || !h || h>200 || total>capacity || size!=8+2*total) return -1;
    struct SlicksChunkyUi ui={0,palette,0,0};
    for(unsigned long i=0;i<total;++i) {
        unsigned a=resource[8+2*i],b=resource[9+2*i];
        pixels[i]=(a&128)?0:slicks_ui_nearest(&ui,(unsigned char)((a&31)*2),
            (unsigned char)(((a&96)>>1)+((b&224)>>4)),(unsigned char)((b&31)*2));
    }
    *width=(unsigned short)w; *height=(unsigned short)h;
    return 0;
}
#endif
