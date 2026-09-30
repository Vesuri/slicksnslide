#ifndef SLICKS_PALETTE_REMAP_H
#define SLICKS_PALETTE_REMAP_H
#include "chunky_ui.h"
#include "chunky_rows.h"

/* Original 344c5: build an indexed-colour tint table. Keep the signed byte
 * percentage, truncated /25, byte narrowing, and 16-bit blended sum. This
 * is not a clamped RGB interpolation: overflow is observable in DOS. */
static inline void slicks_ui_tint_table(const unsigned char palette[768],
    unsigned char table[256],short red,short green,short blue,unsigned char percent)
{
    struct SlicksChunkyUi ui={0,palette,0,0};
    const short colour[3]={red,green,blue};
    int weight=(signed char)((signed char)percent*32/25);
    int remaining=128-weight;
    for(unsigned i=0;i<256;++i) {
        unsigned char query[3];
        for(unsigned c=0;c<3;++c) {
            short sum=(short)(unsigned short)(colour[c]*weight+palette[3*i+c]*remaining);
            /* Explicit arithmetic shift, also for negative nonmultiples. */
            int shifted=sum<0?-((-sum+127)/128):sum/128;
            query[c]=(unsigned char)shifted;
        }
        table[i]=slicks_ui_nearest(&ui,query[0],query[1],query[2]);
    }
}

/* Original 34599, specialized to the visible chunky page. The DOS routine
 * visits four VGA planes; every pixel in the half-open rectangle is remapped
 * once. Unlike the DOS raw routine, reject out-of-surface rectangles rather
 * than permitting wrapped VGA addresses. Player-menu preparation is in bounds. */
static inline int slicks_ui_remap(struct SlicksChunkyUi *ui,
    short left,short top,short right,short bottom,const unsigned char table[256])
{
    if(left<0 || top<0 || right>320 || bottom>200 || right<left || bottom<top)
        return -1;
    if(left==right || top==bottom) return 0;
    unsigned char *row=ui->pixels+mult320[(unsigned)top]+left;
    for(short y=top;y<bottom;++y,row+=320) slicks_ui_remap_row(row,right-left,table);
    if(ui->dirty) ui->dirty(ui->dirty_context,left,top,right,bottom);
    return 0;
}
#endif
