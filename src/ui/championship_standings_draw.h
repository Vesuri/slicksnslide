#ifndef SLICKS_CHAMPIONSHIP_STANDINGS_DRAW_H
#define SLICKS_CHAMPIONSHIP_STANDINGS_DRAW_H
#include "../game/championship_standings.h"
struct SlicksStandingsDrawOps {
    unsigned char (*nearest)(void *,unsigned char,unsigned char,unsigned char);
    void (*colour)(void *,unsigned char,unsigned char);
    void (*rectangle)(void *,short,short,short,short,unsigned char);
    void (*text)(void *,const unsigned char *,short,short,unsigned char);
    void (*number)(void *,short,short,short,unsigned char);
    void *context;
};
/* Original 2a7a1..2aa77: signed points, original row geometry, eight
 * gradient stripes, font colour slots 4 and 0, tied rank suppression.
 * Font shadow is (1,1) in the owning final-standings screen. */
static inline void slicks_draw_championship_standings(
    const struct SlicksChampionshipStandings *table,const unsigned char colours[4][6],
    const unsigned char *const names[4],const struct SlicksStandingsDrawOps *ops)
{
    void *p=ops->context;
    for(unsigned row=0;row<4;++row) {
        ops->colour(p,4,ops->nearest(p,50,50,50));
        if(table->points[row]<0) continue;
        unsigned driver=table->driver[row];
        for(unsigned stripe=0;stripe<8;++stripe) {
            unsigned char rgb[3];
            for(unsigned c=0;c<3;++c)
                rgb[c]=(unsigned char)(((7-(int)stripe)*(signed char)colours[driver][c]+
                    (int)stripe*(signed char)colours[driver][c+3])/10);
            unsigned char colour=ops->nearest(p,rgb[0],rgb[1],rgb[2]);
            short y=(short)(91+row*10+stripe);
            ops->rectangle(p,85,y,235,(short)(y+1),colour);
        }
        unsigned char gray=(unsigned char)(60-row*2);
        ops->colour(p,0,ops->nearest(p,gray,gray,gray));
        short y=(short)(92+row*10);
        if(!row || table->points[row]!=table->points[row-1])
            ops->number(p,(short)table->place[row],86,y,4);
        ops->text(p,names[driver],98,y,4);
        ops->number(p,table->points[row],233,y,6);
    }
}
#endif
