#ifndef SLICKS_ARCADE_TITLE_PAINTER_H
#define SLICKS_ARCADE_TITLE_PAINTER_H
#include "arcade_title_draw.h"
#include "chunky_ui.h"
struct SlicksArcadeTitlePainter {
    unsigned char *logical,*fonts[4];
    const unsigned char *background,*palette,*players,*settings,*summary;
    short seconds,tracks;
    unsigned char shadow,failed;
    void (*text)(void *,unsigned char *,const unsigned char *,const unsigned char *,short,short,unsigned short,unsigned short);
    void (*dirty)(void *,short,short,short,short);
    void *context;
};
static inline unsigned char slicks_arcade_paint_nearest(void *p,unsigned char r,unsigned char g,unsigned char b)
{ struct SlicksArcadeTitlePainter *s=p;struct SlicksChunkyUi ui={.palette=s->palette};return slicks_ui_nearest(&ui,r,g,b); }
static inline void slicks_arcade_paint_colour(void *p,unsigned font,unsigned char c)
{ ((struct SlicksArcadeTitlePainter *)p)->fonts[font][6]=c; }
static inline void slicks_arcade_paint_shadow(void *p,unsigned char c)
{ ((struct SlicksArcadeTitlePainter *)p)->shadow=c; }
static inline void slicks_arcade_paint_restore(void *p,short x,short y,short w,short h)
{
    struct SlicksArcadeTitlePainter *s=p;x&=(short)~3;w&=(short)~3;
    for(unsigned dy=(unsigned)y;dy<(unsigned)(y+h);++dy)for(unsigned dx=(unsigned)x;dx<(unsigned)(x+w);++dx)
        s->logical[(dx&3)*65536UL+dy*100+(dx>>2)]=s->background[2+(dx&3)*16000UL+dy*80+(dx>>2)];
    if(s->dirty)s->dirty(s->context,x,y,(short)(x+w),(short)(y+h));
}
static inline void slicks_arcade_paint_rectangle(void *p,short l,short t,short r,short b,unsigned char c)
{
    struct SlicksArcadeTitlePainter *s=p;
    for(unsigned y=(unsigned)t;y<(unsigned)b;++y)for(unsigned x=(unsigned)l;x<(unsigned)r;++x)
        s->logical[(x&3)*65536UL+y*100+(x>>2)]=c;
    if(s->dirty)s->dirty(s->context,l,t,r,b);
}
static inline void slicks_arcade_paint_text(void *p,unsigned font,unsigned id,short x,short y,unsigned char flags)
{
    struct SlicksArcadeTitlePainter *s=p;unsigned char buffer[96];const unsigned char *text;
    if(id==0 || id==5)text=id?s->settings:s->players;
    else {
        short values[2]={(short)id,0};const unsigned char *format=(const unsigned char *)"%dP";
        if(id==6){values[0]=s->seconds;values[1]=s->tracks;format=s->summary;}
        if(slicks_arcade_title_format(buffer,sizeof buffer,format,values,id==6?2:1)){s->failed=1;return;}
        text=buffer;
    }
    s->text(s->context,s->logical,s->fonts[font],text,x,y,flags,s->shadow);
}
static inline int slicks_arcade_title_paint(struct SlicksArcadeTitlePainter *s,
    unsigned char *counter,unsigned char *refresh,unsigned char selection,short players,const signed char colours[4][6])
{
    const struct SlicksArcadeTitleDrawOps ops={slicks_arcade_paint_nearest,slicks_arcade_paint_colour,
        slicks_arcade_paint_shadow,slicks_arcade_paint_restore,slicks_arcade_paint_rectangle,slicks_arcade_paint_text,s};
    s->failed=0;slicks_arcade_title_draw(counter,refresh,selection,players,colours,&ops);return s->failed?-1:0;
}
#endif
