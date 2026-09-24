#ifndef SLICKS_TRACK_MENU_DRAW_H
#define SLICKS_TRACK_MENU_DRAW_H
#include "track_menu.h"
#include "profile_editor_draw.h"
#include "../game/track_playlist.h"

struct SlicksTrackMenuLabels {
    const unsigned char *actions[6],*random_on,*random_off,*separator;
};
struct SlicksTrackMenuDrawOps {
    struct SlicksProfileEditorDrawOps painter;
    void (*tint)(void *,short,short,short,short);
    const unsigned char *(*name)(void *,unsigned);
};

/* 270b9..274d5: use the original saved menu background, fonts and tint
 * table. Preparation owns those resources; this is the redraw stage only. */
static inline int slicks_draw_track_menu(struct SlicksTrackMenu *m,short total,
    const struct SlicksTrackPlaylist *selected,const struct SlicksTrackMenuLabels *labels,
    unsigned char scroll_colour,const struct SlicksTrackMenuDrawOps *ops)
{
    if(!m || total<0 || m->column>6 || m->top<0 || !slicks_track_playlist_valid(selected)) return -1;
    if(m->previous==m->cursor) return 0;
    const struct SlicksProfileEditorDrawOps *p=&ops->painter;
    const struct SlicksPlayerMenuDrawOps *d=&p->menu; void *context=d->context;
    d->restore(context,0,0,0,10,80,182);
    d->restore(context,0,0,180,3,50,10);
    d->restore(context,0,0,127,29,90,100);
    if(!m->column) d->bevel(context,15,(short)(12+8*(m->cursor-m->top)),64,10,50,10,10);
    else d->bevel(context,127,(short)(16+13*m->column),80,11,52,10,10);
    p->colour(context,p->nearest(context,62,50,62));
    for(unsigned i=0;i<6;++i) {
        const unsigned char *text=i==1?(m->random_order?labels->random_on:labels->random_off):labels->actions[i];
        d->text(context,0,text,166,(short)(32+13*i),1);
    }
    if(m->random_count>total) m->random_count=total;
    p->number(context,(unsigned short)m->random_count,198,84,1);
    p->colour(context,p->nearest(context,70,55,70));
    p->number(context,selected->count,210,3,2);
    d->text(context,0,labels->separator,211,3,0);
    p->number(context,(unsigned short)total,216,3,0);
    if(total>22) {
        short y=(short)((long)m->cursor*166/total+15);
        p->rectangle(context,5,y,8,(short)(y+6),scroll_colour);
    }
    for(unsigned row=0;row<22;++row) {
        unsigned track=(unsigned)m->top+row;
        for(unsigned i=0;i<selected->count;++i) if(selected->tracks[i]==(int)track) {
            ops->tint(context,18,(short)(13+8*row),72,(short)(22+8*row)); break;
        }
        if(track<(unsigned)total) d->text(context,0,ops->name(context,track),20,(short)(14+8*row),0);
    }
    m->previous=m->cursor;
    return 0;
}
#endif
