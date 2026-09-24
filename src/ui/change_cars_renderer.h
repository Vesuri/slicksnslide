#ifndef SLICKS_CHANGE_CARS_RENDERER_H
#define SLICKS_CHANGE_CARS_RENDERER_H
#include "change_cars_dialog.h"
#include "player_menu_renderer.h"
#include "saved_rectangle.h"

/* Original 24862..24972 / 24af9..24b55. Caller owns two compact 40x100
 * snapshots and decoded icons/fonts. No allocation while hardware is owned. */
struct SlicksChangeCarsRenderer {
    struct SlicksPlayerMenuRenderer *surface;
    struct SlicksSavedRectangle original,decorated;
    short x,y;
    unsigned char old_colour,active;
};
static inline void slicks_change_cars_restore(void *p,short x,short y,short sx,short sy,short w,short h)
{
    struct SlicksChangeCarsRenderer *r=p;
    if(slicks_restore_rectangle(&r->surface->ui,&r->decorated,x,y,sx,sy,w,h)) r->surface->error=-1;
}
static inline void slicks_change_cars_bevel(void *p,short x,short y,short w,short h,
    unsigned char red,unsigned char green,unsigned char blue)
{ struct SlicksChangeCarsRenderer *r=p; slicks_player_renderer_bevel(r->surface,x,y,w,h,red,green,blue); }
static inline void slicks_change_cars_sprite(void *p,signed char vehicle,short x,short y)
{ struct SlicksChangeCarsRenderer *r=p; slicks_player_renderer_sprite(r->surface,vehicle,x,y); }
static inline int slicks_change_cars_renderer_close(struct SlicksChangeCarsRenderer *r)
{
    if(!r || !r->active) return -1;
    int result=slicks_restore_rectangle(&r->surface->ui,&r->original,r->x,r->y,0,0,40,100);
    r->surface->fonts[0][6]=r->old_colour;
    r->active=0;
    return result;
}
static inline int slicks_change_cars_renderer_open(struct SlicksChangeCarsRenderer *r,
    struct SlicksChangeCarsDialog *d,short x,short y,unsigned count,
    const unsigned char *title,unsigned char *original,unsigned long original_size,
    unsigned char *decorated,unsigned long decorated_size)
{
    if(!r || r->active || !r->surface || !d || !title || !count || count>4 ||
       !original || !decorated || original==decorated ||
       original_size<4000 || decorated_size<4000 || x<0 || x>280 || y<0 || y>100) return -1;
    struct SlicksPlayerMenuRenderer *s=r->surface;
    if(!s->ui.pixels || !s->ui.palette || !s->fonts[0] || !s->text || !s->icon || !s->icons) return -1;
    if(slicks_save_rectangle(&r->original,original,original_size,&s->ui,x,y,40,100)) return -1;
    r->x=x; r->y=y; r->old_colour=s->fonts[0][6]; r->active=1;
    unsigned char table[256]; slicks_ui_tint_table(s->ui.palette,table,15,15,50,60);
    s->error=slicks_ui_remap(&s->ui,x,y+3,x+40,(short)(y+count*15+5),table);
    s->fonts[0][6]=slicks_ui_nearest(&s->ui,40,40,60);
    slicks_player_renderer_text(s,0,title,x,y,0);
    if(s->error || slicks_save_rectangle(&r->decorated,decorated,decorated_size,&s->ui,x,y,40,100)) {
        (void)slicks_change_cars_renderer_close(r); return -1;
    }
    *d=(struct SlicksChangeCarsDialog){0,0};
    return 0;
}
static inline int slicks_change_cars_renderer_draw(struct SlicksChangeCarsRenderer *r,
    const struct SlicksChangeCarsDialog *d,const signed char roles[4],const signed char vehicles[4])
{
    if(!r || !r->active || !d || !roles || !vehicles) return -1;
    struct SlicksPlayerMenuRenderer *s=r->surface;
    /* Validate all icons before changing any pixels. */
    for(unsigned i=0;i<4;++i) if(roles[i]) {
        int index=vehicles[i]+1;
        if(index<1 || (unsigned)index>=s->icon_count || !s->icons[index].pixels) return -1;
    }
    if(slicks_change_cars_driver(roles,d->row)<0) return -1;
    s->error=0;
    const struct SlicksChangeCarsDrawOps ops={slicks_change_cars_restore,
        slicks_change_cars_bevel,slicks_change_cars_sprite,r};
    (void)slicks_change_cars_draw(d,roles,vehicles,r->x,r->y,&ops);
    return s->error;
}
#endif
