#ifndef SLICKS_MENU_BITMAP_H
#define SLICKS_MENU_BITMAP_H
static inline unsigned long slicks_bmp_word32(const unsigned char *p)
{ return (unsigned long)p[0]|((unsigned long)p[1]<<8)|((unsigned long)p[2]<<16)|((unsigned long)p[3]<<24); }

/* Original 2ec59's 8-bit menu BMP path, producing chunky instead of its
 * four-bank sprite. Restrict to native screen dimensions and width divisible
 * by four; reject malformed/truncated input rather than following DOS EOF or
 * out-of-allocation writes. Output may be partial on a rejected stream.
 * RLE delta deliberately ignores dx, as DOS does, and restarts at column 0.
 * Caller supplies 64000 pixel bytes and 768 palette bytes. */
static inline int slicks_decode_menu_bitmap(const unsigned char *data,unsigned long size,
    unsigned char *pixels,unsigned char *palette,unsigned *width,unsigned *height,
    unsigned long *consumed)
{
    if(!data || !pixels || !palette || !width || !height || size<54 || data[0]!='B' || data[1]!='M') return -1;
    unsigned long header=slicks_bmp_word32(data+14),offset=slicks_bmp_word32(data+10);
    unsigned long w=slicks_bmp_word32(data+18),h=slicks_bmp_word32(data+22),compression=slicks_bmp_word32(data+30);
    if(header<40 || header>size-14 || size-14-header<1024 || w==0 || w>320 || (w&3) ||
        h==0 || h>200 || data[26]!=1 || data[27] || data[28]!=8 || data[29] || compression>1 ||
        offset<14+header+1024 || offset>size) return -1;
    for(unsigned i=0;i<256;++i) {
        const unsigned char *p=data+14+header+4*i;
        palette[3*i]=p[2]>>2; palette[3*i+1]=p[1]>>2; palette[3*i+2]=p[0]>>2;
    }
    for(unsigned long i=0;i<w*h;++i) pixels[i]=0;
    unsigned long at=offset; unsigned y=0;
    while(y<h) {
        unsigned x=0;
        if(!compression) {
            if(size-at<w) return -1;
            for(;x<w;++x) pixels[(h-y-1)*w+x]=data[at++];
            ++y;
        } else {
            while(x<w) {
                if(size-at<2) return -1;
                unsigned count=data[at++],value=data[at++];
                if(count) {
                    if(count>w-x) return -1;
                    for(unsigned i=0;i<count;++i) pixels[(h-y-1)*w+x++]=(unsigned char)value;
                } else if(value==0) { ++y; break; }
                else if(value==1) { y=(unsigned)h; break; }
                else if(value==2) {
                    if(size-at<2) return -1;
                    ++at; y+=data[at++]; break;
                } else {
                    if(value>w-x || size-at<value+(value&1)) return -1;
                    for(unsigned i=0;i<value;++i) pixels[(h-y-1)*w+x++]=data[at++];
                    at+=value&1;
                }
            }
        }
    }
    *width=(unsigned)w; *height=(unsigned)h; if(consumed) *consumed=at;
    return 0;
}
#endif
