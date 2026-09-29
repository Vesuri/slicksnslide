#ifndef SLICKS_HELP_TEXT_DIRTY_H
#define SLICKS_HELP_TEXT_DIRTY_H
#include "chunky_ui.h"
/* Bounds of the unaligned, unshadowed Help font bridge. Follow the string
 * renderer's individual glyph advances, not whole-string measured width:
 * glyph zero, signed spacing, tabs and trailing glyph overhang differ. */
static inline void slicks_help_text_dirty(struct SlicksChunkyUi *ui,
    const unsigned char *font,const unsigned char *text,short x,short y,signed char spacing)
{
    const unsigned char *codes=font+6+font[5],*widths=codes+font[0];
    short anchor=x;
    int left=320,top=200,right=0,bottom=0;
    for(unsigned n=0;n<1000 && text[n];++n) {
        unsigned c=text[n],glyph=0;
        if(c==13 || c==10) { x=anchor; y=(short)(y+font[2]+1); continue; }
        while(glyph<font[0] && codes[glyph]!=c) ++glyph;
        if(glyph<font[0] && widths[glyph] && font[2]) {
            int l=x,t=y,r=x+widths[glyph],b=y+font[2];
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
#endif
