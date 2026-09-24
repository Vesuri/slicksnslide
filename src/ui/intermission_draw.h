#ifndef SLICKS_INTERMISSION_DRAW_H
#define SLICKS_INTERMISSION_DRAW_H
#include "intermission_menu.h"
#include "player_menu_draw.h"

/* Original 2453a..245bb: repaint current vehicles after Change Cars. */
static inline void slicks_intermission_car_rows(struct SlicksIntermissionMenu *m,
    const signed char roles[4],const signed char vehicles[4],unsigned char has_background,
    const struct SlicksPlayerMenuDrawOps *ops)
{
    if(!m->cars_redraw) return;
    if(has_background) ops->restore(ops->context,95,77,0,0,8,40);
    m->cars_redraw=0;
    short row=0;
    for(unsigned slot=0;slot<4;++slot) if(roles[slot])
        ops->sprite(ops->context,vehicles[slot],95,(short)(77+10*row++));
}

/* Original 245da..24702: rows 0/1 stay hidden even after F2 selects row 0.
 * Labels 2/3 are already resolved through nexttrack/mainmenu with the
 * original NEXT TRACK/END MATCH strings as fallbacks. */
static inline void slicks_intermission_action_rows_from(struct SlicksIntermissionMenu *m,
    unsigned char count,unsigned char has_background,const unsigned char *const labels[4],
    const struct SlicksPlayerMenuDrawOps *ops,short first)
{
    if(!m->redraw) return;
    if(has_background) ops->restore(ops->context,125,(short)(77+10*count),0,0,72,42);
    for(short row=first;row<4;++row) {
        if(row==m->selected)
            ops->bevel(ops->context,125,(short)(77+10*(count+row)),70,11,50,10,10);
        ops->text(ops->context,0,labels[row],160,(short)(80+10*(count+row)),1);
    }
    m->redraw=0;
}
static inline void slicks_intermission_action_rows(struct SlicksIntermissionMenu *m,
    unsigned char count,unsigned char has_background,const unsigned char *const labels[4],
    const struct SlicksPlayerMenuDrawOps *ops)
{ slicks_intermission_action_rows_from(m,count,has_background,labels,ops,2); }

struct SlicksIntermissionRowsOps {
    void (*colour)(void *,unsigned char);
    void (*number)(void *,unsigned short,short,short,unsigned char);
    void (*name)(void *,const unsigned char *,short,short,unsigned char);
    void (*fastest)(void *,short,short);
    void *context;
};

struct SlicksIntermissionHeaderOps {
    struct SlicksIntermissionRowsOps draw;
    unsigned char (*nearest)(void *,unsigned char,unsigned char,unsigned char);
};
/* Original 243e9..244fb. Owner resolves the playlist name and total using
 * the original playlist/mode rules; numbering wraps as a signed DOS word. */
static inline void slicks_intermission_track_header(short track_index,short total,
    const unsigned char *track_name,const unsigned char *slash,
    const struct SlicksIntermissionHeaderOps *ops)
{
    const struct SlicksIntermissionRowsOps *d=&ops->draw;
    d->colour(d->context,ops->nearest(d->context,40,40,50));
    d->name(d->context,track_name,58,77,1);
    d->colour(d->context,ops->nearest(d->context,50,50,30));
    d->name(d->context,slash,250,65,0);
    d->number(d->context,(unsigned short)((unsigned short)track_index+1),248,64,6);
    d->number(d->context,(unsigned short)total,254,64,4);
}
/* Original 242ca..243c9: compact active slots, preserving slot colours and
 * championship points. Every tied best lap receives the marker; there is no
 * implicit sorting and no human-only filter here. Names are resolved from
 * the selected profiles by the owner. This routine does not award points. */
static inline void slicks_intermission_driver_rows(const signed char roles[4],
    const short points[4],const unsigned char *const names[4],
    const signed int laps[4],signed int fastest,const struct SlicksIntermissionRowsOps *ops)
{
    short row=0;
    for(unsigned slot=0;slot<4;++slot) if(roles[slot]) {
        short y=(short)(77+row*10);
        ops->colour(ops->context,(unsigned char)(4+slot*5));
        ops->number(ops->context,(unsigned short)points[slot],249,y,6);
        ops->name(ops->context,names[slot],105,y,4);
        if(laps[slot]==fastest) ops->fastest(ops->context,253,y);
        ++row;
    }
}
#endif
