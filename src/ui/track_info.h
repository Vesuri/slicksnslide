#ifndef SLICKS_TRACK_INFO_H
#define SLICKS_TRACK_INFO_H
#include "chunky_ui.h"
#include "../game/track_playlist.h"

/* Original 1a4ef reads the first description at offset 378, applying 362d4
 * to each byte. Reject missing terminators/capacity before touching output. */
static inline int slicks_track_description(const unsigned char *data,
    unsigned long size,unsigned char *text,unsigned long capacity)
{
    unsigned long end=378;
    if(!data || !text || size<=end) return -1;
    while(end<size && data[end]) ++end;
    if(end==size || end-378>=capacity) return -1;
    for(unsigned long i=378;i<=end;++i) {
        unsigned char c=data[i];
        if(c>='a' && c<='z') c^=0x20;
        else if(c==0x86) c=0x8f;
        else if(c==0x84) c=0x8e;
        else if(c==0x94) c=0x99;
        else if(c==0x82) c=0x90;
        text[i-378]=c;
    }
    return 0;
}

struct SlicksTrackPreview {
    const unsigned char *objects;
    unsigned short count;
};

/* One original 26a53..26b68 shimmer iteration. Never feed the modified
 * screen back into the sample: DOS reads its saved 64x40 preview instead. */
static inline void slicks_track_preview_shimmer(struct SlicksChunkyUi *ui,
    const unsigned char preview[64*40],const unsigned char palette[768],unsigned long *seed)
{
    unsigned x=slicks_track_random_index(64,seed),y=slicks_track_random_index(40,seed);
    unsigned index=preview[y*64+x];
    int delta=(int)slicks_track_random_index(5,seed)-2;
    unsigned char colour=slicks_ui_nearest(ui,(unsigned char)(palette[3*index]+delta),
        (unsigned char)(palette[3*index+1]+delta),(unsigned char)(palette[3*index+2]+delta));
    ui->pixels[mult320[y+20]+x+245]=colour;
    if(ui->dirty) ui->dirty(ui->dirty_context,(short)(245+x),(short)(20+y),(short)(246+x),(short)(21+y));
}

/* 1a32d follows the signed big-endian offset at bytes 6..7. Its object
 * count is signed too: nonpositive counts leave only the background.
 * Return 1 for the original old-format message, -1 for malformed input. */
static inline int slicks_track_preview_open(struct SlicksTrackPreview *view,
    const unsigned char *data,unsigned long size)
{
    if(!view || !data || size<6) return -1;
    if(data[5]!=2) return 1;
    if(size<8) return -1;
    short offset=(short)((unsigned short)data[6]*256U+data[7]);
    if(offset<0 || (unsigned long)offset+2>size) return -1;
    short count=(short)((unsigned short)data[offset]*256U+data[offset+1]);
    unsigned n=count>0?(unsigned short)count:0;
    if((unsigned long)offset+2+5UL*n>size) return -1;
    view->objects=data+offset+2; view->count=(unsigned short)n;
    return 0;
}

/* Calls made to 1a1d4: positions divide by five with signed truncation;
 * rotation is the unmasked original byte. Resource lookup is downstream. */
static inline void slicks_track_preview_object(const struct SlicksTrackPreview *view,
    unsigned i,short x,short y,short *px,short *py,
    unsigned char *type,unsigned char *rotation)
{
    const unsigned char *p=view->objects+5UL*i;
    short ox=(short)((unsigned short)p[0]*256U+p[1]);
    *px=(short)(unsigned short)(x+ox/5);
    *py=(short)(unsigned short)(y+p[2]/5);
    *type=p[3]; *rotation=p[4];
}

/* Original 1a1d4 selects a pre-rotated resource, then calls 35719 with
 * rotation zero and scale five. Sample that oriented image, including the
 * scaler's pre-increment (+1,+1), rather than shrinking a composed frame.
 * The source here stays in its original orientation to avoid another copy. */
static inline int slicks_track_preview_sprite(struct SlicksChunkyUi *ui,
    const unsigned char *pixels,unsigned short width,unsigned short height,
    unsigned char rotation,short x,short y)
{
    if(!ui || !ui->pixels || !pixels || !width || !height || rotation>3) return -1;
    unsigned w=rotation&1?height:width,h=rotation&1?width:height;
    short left=320,top=200,right=0,bottom=0;
    short dy=y;
    for(unsigned sy=0;sy<h;sy+=5) {
        dy=(short)(unsigned short)(dy+1); short dx=x;
        for(unsigned sx=0;sx<w;sx+=5) {
            dx=(short)(unsigned short)(dx+1);
            unsigned ox=sx,oy=sy;
            if(rotation==1) { ox=sy; oy=height-1-sx; }
            else if(rotation==2) { ox=width-1-sx; oy=height-1-sy; }
            else if(rotation==3) { ox=width-1-sy; oy=sx; }
            unsigned char colour=pixels[(unsigned long)oy*width+ox];
            if(!colour || dx<0 || dx>=320 || dy<0 || dy>=200) continue;
            ui->pixels[mult320[(unsigned)dy]+dx]=colour;
            if(dx<left) left=dx;
            if(dy<top) top=dy;
            if(dx+1>right) right=(short)(dx+1);
            if(dy+1>bottom) bottom=(short)(dy+1);
        }
    }
    if(left<right && top<bottom && ui->dirty)
        ui->dirty(ui->dirty_context,left,top,right,bottom);
    return 0;
}
#endif
