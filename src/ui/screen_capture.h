#ifndef SLICKS_SCREEN_CAPTURE_H
#define SLICKS_SCREEN_CAPTURE_H

#define SLICKS_CAPTURE_SIZE (54UL+1024UL+64000UL)
static inline void slicks_capture_u32(unsigned char *p,unsigned long v)
{ for(unsigned i=0;i<4;++i) { p[i]=(unsigned char)v; v>>=8; } }

/* Original 204a0..205c6: indexed BMP, BGR0 DAC palette scaled by four,
 * followed by 320-pixel rows from bottom to top. No image is an input asset. */
static inline int slicks_encode_capture(unsigned char *out,unsigned long capacity,
    const unsigned char *pixels,const unsigned char *palette)
{
    if(!out || !pixels || !palette || capacity<SLICKS_CAPTURE_SIZE) return -1;
    for(unsigned i=0;i<54;++i) out[i]=0;
    out[0]='B'; out[1]='M';
    slicks_capture_u32(out+2,SLICKS_CAPTURE_SIZE);
    slicks_capture_u32(out+10,1078);
    slicks_capture_u32(out+14,40);
    slicks_capture_u32(out+18,320);
    slicks_capture_u32(out+22,200);
    out[26]=1; out[28]=8;
    slicks_capture_u32(out+34,64000);
    for(unsigned i=0;i<256;++i) {
        out[54+4*i]=(unsigned char)(palette[3*i+2]<<2);
        out[55+4*i]=(unsigned char)(palette[3*i+1]<<2);
        out[56+4*i]=(unsigned char)(palette[3*i]<<2);
        out[57+4*i]=0;
    }
    unsigned long at=1078;
    for(int y=199;y>=0;--y) {
        const unsigned char *row=pixels+(unsigned)y*320UL;
        for(unsigned x=0;x<320;++x) out[at++]=row[x];
    }
    return 0;
}
#endif
