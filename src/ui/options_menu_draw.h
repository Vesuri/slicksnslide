#ifndef SLICKS_OPTIONS_MENU_DRAW_H
#define SLICKS_OPTIONS_MENU_DRAW_H
#include "options_menu.h"
#include "player_menu_draw.h"

struct SlicksOptionsLabels {
    const unsigned char (*rows)[64],(*suffixes)[64],(*modes)[64];
    const unsigned char *off,*on;
};
struct SlicksOptionsDrawOps {
    struct SlicksPlayerMenuDrawOps menu;
    void (*colour)(void *,unsigned char);
    unsigned char (*measure)(void *,const unsigned char *);
    void (*pattern)(void *,short,short,short,short);
    void (*rectangle)(void *,short,short,short,short,unsigned char);
};
static inline void slicks_option_number(unsigned char text[64],short value,
    const unsigned char *suffix)
{
    unsigned char reverse[6]; unsigned count=0,at=0;
    unsigned magnitude=value<0?(unsigned)(-(int)value):(unsigned)value;
    if(value<0) text[at++]='-';
    do { reverse[count++]=(unsigned char)('0'+magnitude%10); magnitude/=10; } while(magnitude);
    while(count) text[at++]=reverse[--count];
    while(*suffix && at<63) text[at++]=*suffix++;
    text[at]=0;
}

/* Original redraw loop 28f9d..293ef. Palette indices are the four nearest
 * colours prepared by 28e19. Restore source coordinates/rounding belong to
 * the existing saved-background renderer, not a newly painted substitute. */
static inline int slicks_draw_options_menu(struct SlicksOptionsMenu *m,
    const struct SlicksConfiguration *c,const struct SlicksOptionSpec specs[15],
    const struct SlicksOptionsLabels *labels,const unsigned char colours[4],
    const struct SlicksOptionsDrawOps *ops)
{
    if(m->row>17 || c->options[0]<0 || c->options[0]>5) return -1;
    for(unsigned i=0;i<15;++i) if(specs[i].maximum<=0) return -1;
    if(m->redraw==123) return 0;
    const struct SlicksPlayerMenuDrawOps *p=&ops->menu; void *context=p->context;
    short y=40; unsigned following=0;
    if(m->redraw<0) p->restore(context,0,0,2,38,177,162);
    for(unsigned i=0;i<18;++i) {
        unsigned enabled=(unsigned)slicks_option_enabled(specs,i,(unsigned)c->options[0]);
        ops->colour(context,enabled?colours[i==m->row?1:0]:colours[2]);
        if(m->redraw<0 || i==(unsigned char)m->redraw || following) {
            short width=(signed char)ops->measure(context,labels->rows[i]);
            if(enabled) {
                if(m->redraw>=0) p->restore(context,0,0,2,(short)(y-2),177,11);
                if(i==m->row) p->bevel(context,(short)(71-width),(short)(y-2),
                    (short)(width+9),9,50,10,10);
                if(i<15) ops->pattern(context,80,(short)(y-2),130,(short)(y+6));
                p->text(context,0,labels->rows[i],77,y,2);
                following=0;
            }
            if(i==(unsigned char)m->redraw) following=1;
        }
        if(i<15 && enabled) {
            short x=(short)(unsigned short)((long)c->options[i]*50/specs[i].maximum+80);
            if(i) {
                p->restore(context,0,0,x,y,(short)(140-x),10);
                ops->rectangle(context,80,y,x,(short)(y+8),colours[3]);
                unsigned char text[64]; const unsigned char *value;
                if(specs[i].maximum==1 || (!c->options[i] && (specs[i].modes&128))) {
                    if(c->options[i]<0 || c->options[i]>1) return -1;
                    value=c->options[i]?labels->on:labels->off;
                } else {
                    slicks_option_number(text,c->options[i],labels->suffixes[i]); value=text;
                }
                p->text(context,0,value,107,(short)(y+1),1);
            } else {
                p->restore(context,0,0,80,y,70,10);
                p->text(context,0,labels->modes[c->options[0]],82,y,0);
            }
        }
        if(enabled) y=(short)(y+10);
        if(i==14) y=(short)(y+2);
    }
    m->redraw=123;
    return 0;
}
#endif
