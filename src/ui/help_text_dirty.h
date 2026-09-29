#ifndef SLICKS_HELP_TEXT_DIRTY_H
#define SLICKS_HELP_TEXT_DIRTY_H
#include "chunky_ui.h"
/* Painter bounds for the Help and standings font bridges. Follow individual
 * glyph advances, not whole-string measured width: glyph zero, signed spacing,
 * tabs and trailing glyph overhang differ. Alignment uses the native measured
 * width, but newlines and tabs retain the original (unaligned) anchor. These
 * bridges use a rightward one-pixel shadow; shadow_y selects its vertical offset. */
static inline void slicks_font_text_dirty(struct SlicksChunkyUi *ui,
    const unsigned char *font,const unsigned char *text,short x,short y,signed char spacing,
    unsigned char flags,short measured_width,unsigned char shadow_y)
{
    const unsigned char *codes=font+6+font[5],*widths=codes+font[0];
    short anchor=x;
    if((flags&3)==1) x=(short)(x-measured_width/2);
    else if((flags&3)==2) x=(short)(x-measured_width);
    int left=320,top=200,right=0,bottom=0;
    for(unsigned n=0;n<1000 && text[n];++n) {
        unsigned c=text[n],glyph=0;
        if(c==13 || c==10) { x=anchor; y=(short)(y+font[2]+1); continue; }
        while(glyph<font[0] && codes[glyph]!=c) ++glyph;
        if(glyph<font[0] && widths[glyph] && font[2]) {
            int l=x,t=y,r=x+widths[glyph]+((flags&4)!=0),
                b=y+font[2]+((flags&4)?shadow_y:0);
            if(l<0) l=0;
            if(t<0) t=0;
            if(r>320) r=320;
            if(b>200) b=200;
            if(l<r && t<b) {
                if(l<left) left=l;
                if(t<top) top=t;
                if(r>right) right=r;
                if(b>bottom) bottom=b;
            }
        }
        int advance;
        if(glyph>0 && glyph<font[0]) advance=widths[glyph]+font[3]+spacing-1;
        else if(c==8 || c==207) advance=10-(short)(x-anchor)%10;
        else advance=font[1];
        x=(short)(x+advance);
    }
    if(left<right && top<bottom && ui->dirty)
        ui->dirty(ui->dirty_context,left,top,right,bottom);
}
static inline void slicks_help_text_dirty(struct SlicksChunkyUi *ui,
    const unsigned char *font,const unsigned char *text,short x,short y,signed char spacing)
{
    slicks_font_text_dirty(ui,font,text,x,y,spacing,0,0,0);
}
#endif
