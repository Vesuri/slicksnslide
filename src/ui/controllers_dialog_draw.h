#ifndef SLICKS_CONTROLLERS_DIALOG_DRAW_H
#define SLICKS_CONTROLLERS_DIALOG_DRAW_H
#include "controllers_dialog.h"
#include "player_menu_draw.h"

struct SlicksControllersLabels {
    const unsigned char (*key_names)[64];
    const unsigned char *supported,*defaults,*exit;
};
struct SlicksControllersDrawOps {
    struct SlicksPlayerMenuDrawOps menu;
    void (*colour)(void *,unsigned char);
    void (*number)(void *,unsigned short,short,short,unsigned char);
};

/* Original 2da76..2dd89. Sprite slots refer to the three controller icons,
 * not car icons. The saved bitmap is the original 200x80 dialog rectangle;
 * preserve source-crop coordinates when restoring rows. */
static inline int slicks_draw_controllers(struct SlicksControllersDialog *d,
    const struct SlicksConfiguration *c,short x,short y,
    const struct SlicksControllersLabels *labels,const unsigned char colours[2],
    const struct SlicksControllersDrawOps *ops)
{
    if(d->row>4 || d->column>5) return -1;
    /* Invalid signed device bytes would index before DOS's icon table. */
    for(unsigned i=0;i<4;++i) if((signed char)c->player_input[i]<0) return -1;
    slicks_controllers_limits(d);
    if(d->redraw==111) return 0;
    const struct SlicksPlayerMenuDrawOps *p=&ops->menu; void *context=p->context;
    for(unsigned row=0;row<5;++row) {
        short py=(short)(y+16+12*row);
        ops->colour(context,colours[0]);
        if(d->redraw>=0 && row!=(unsigned char)d->redraw) continue;
        p->restore(context,x,y,0,(short)(15+12*row),200,12);
        if(row<4) {
            if(row==d->row) p->bevel(context,(short)(x+19+28*d->column),py,35,10,50,10,10);
            unsigned device=c->player_input[row],icon=device,number=device;
            if(device>2) { icon=2; number=device-2; }
            else if(device) icon=1;
            p->sprite(context,(short)icon,(short)(x+29),py);
            if(number) ops->number(context,(unsigned short)number,(short)(x+42),(short)(py+1),1);
            if(device) ops->colour(context,colours[1]);
            for(unsigned i=0;i<5;++i) {
                unsigned char scan=c->keys[5*row+i];
                if(scan) p->text(context,0,labels->key_names[slicks_controller_key_index(labels->supported,scan)],
                    (short)(x+64+28*i),(short)(py+2),1);
            }
        } else {
            if(row==d->row) p->bevel(context,(short)(x+19+80*d->column),py,60,10,50,10,10);
            p->text(context,0,labels->defaults,(short)(x+50),(short)(py+2),1);
            p->text(context,0,labels->exit,(short)(x+130),(short)(py+2),1);
        }
    }
    d->redraw=111;
    return 0;
}
#endif
