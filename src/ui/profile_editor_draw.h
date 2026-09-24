#ifndef SLICKS_PROFILE_EDITOR_DRAW_H
#define SLICKS_PROFILE_EDITOR_DRAW_H
#include "profile_editor.h"
#include "player_menu_draw.h"

struct SlicksProfileEditorLabels {
    const unsigned char *rows[6],*roles[2],*percent;
    const unsigned char *random,*random_each,*unavailable;
};
struct SlicksProfileEditorDrawOps {
    struct SlicksPlayerMenuDrawOps menu;
    void (*colour)(void *,unsigned char);
    void (*number)(void *,unsigned short,short,short,unsigned char);
    unsigned char (*nearest)(void *,unsigned char,unsigned char,unsigned char);
    void (*rectangle)(void *,short,short,short,short,unsigned char);
};

/* Original 27bea..27f53. The name is the editor's working copy, not the
 * persisted profile name. Limits run separately before this redraw stage.
 * Restore uses the original saved-rectangle source offsets (120,y).
 * All nonzero redraw values redraw the whole dialog in the DOS code. */
static inline int slicks_draw_profile_editor(struct SlicksProfileEditor *e,
    const struct SlicksPlayerProfiles *profiles,unsigned index,const unsigned char *name,
    short vehicle_count,unsigned char label_colour,unsigned char value_colour,
    const struct SlicksProfileEditorLabels *labels,const struct SlicksProfileEditorDrawOps *ops)
{
    if(!e || !profiles || index>=SLICKS_PROFILE_MAX || e->row>5 || !name || vehicle_count<0) return -1;
    unsigned n=0; while(n<21 && name[n]) ++n;
    if(n==21) return -1;
    if(!e->redraw) return 0;
    const struct SlicksPlayerMenuDrawOps *m=&ops->menu;
    void *c=m->context;
    ops->colour(c,label_colour);
    for(unsigned i=0;i<6;++i) {
        short y=(short)(100+12*i);
        m->restore(c,0,0,120,y,200,12);
        if(i==e->row) m->bevel(c,120,y,50,11,50,10,10);
        m->text(c,0,labels->rows[i],145,(short)(y+3),1);
    }
    ops->colour(c,value_colour);
    m->text(c,0,name,180,103,0);
    m->text(c,0,labels->roles[profiles->setup[index].flags&1],180,115,0);
    m->text(c,0,labels->percent,266,115,0);
    ops->number(c,profiles->setting[index],264,115,2);
    short vehicle=(signed char)profiles->setup[index].vehicle;
    if(vehicle<0 || vehicle==vehicle_count) m->text(c,0,labels->random,180,127,0);
    else if(vehicle==(short)(unsigned short)(vehicle_count+1)) m->text(c,0,labels->random_each,180,127,0);
    else if(vehicle>vehicle_count) m->text(c,0,labels->unavailable,180,127,0);
    else m->sprite(c,vehicle,180,126);
    for(unsigned i=0;i<2;++i) {
        const unsigned char *rgb=profiles->setup[index].colours+i*3;
        unsigned char colour=ops->nearest(c,rgb[0],rgb[1],rgb[2]);
        ops->rectangle(c,180,(short)(138+11*i),191,(short)(149+11*i),colour);
    }
    e->redraw=0;
    return 0;
}
#endif
