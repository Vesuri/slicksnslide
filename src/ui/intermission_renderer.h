#ifndef SLICKS_INTERMISSION_RENDERER_H
#define SLICKS_INTERMISSION_RENDERER_H
#include "intermission_draw.h"
#include "intermission_prepare.h"
#include "track_records_renderer.h"
#include "saved_rectangle.h"

/* Icon IDs 1..10 are current vehicles, as in the existing records painter.
 * The distinct fastest-lap marker is supplied by the platform owner. */
struct SlicksIntermissionRenderer {
    struct SlicksRecordsRenderer *surface;
    struct SlicksSavedRectangle buttons,cars;
    short fastest_icon;
    unsigned char participants,active,expose_actions;
};
struct SlicksIntermissionContent {
    signed char roles[4],vehicles[4];
    short points[4],track_index,track_total;
    signed int laps[4],fastest;
    const unsigned char *names[4],*labels[4],*track_name,*slash;
};
static inline void slicks_intermission_colour(void *p,unsigned char colour)
{ struct SlicksIntermissionRenderer *r=p; r->surface->fonts[0][6]=colour; }
static inline void slicks_intermission_text(void *p,const unsigned char *name,short x,short y,unsigned char flags)
{
    struct SlicksIntermissionRenderer *r=p; struct SlicksRecordsRenderer *s=r->surface;
    if(!s->error) s->error=s->text(s->context,&s->ui,s->fonts[0],name,x,y,flags,s->highlight);
}
static inline void slicks_intermission_number(void *p,unsigned short value,short x,short y,unsigned char flags)
{
    unsigned char text[7]; slicks_records_decimal((short)value,text);
    slicks_intermission_text(p,text,x,y,flags);
}
static inline void slicks_intermission_icon(void *p,short id,short x,short y)
{
    struct SlicksIntermissionRenderer *r=p; struct SlicksRecordsRenderer *s=r->surface;
    if(!s->error) s->error=s->icon(s->context,&s->ui,id,x,y);
}
static inline void slicks_intermission_fastest(void *p,short x,short y)
{ struct SlicksIntermissionRenderer *r=p; slicks_intermission_icon(p,r->fastest_icon,x,y); }
static inline unsigned char slicks_intermission_nearest(void *p,unsigned char red,unsigned char green,unsigned char blue)
{ struct SlicksIntermissionRenderer *r=p; return slicks_ui_nearest(&r->surface->ui,red,green,blue); }
static inline void slicks_intermission_restore(void *p,short x,short y,short sx,short sy,short w,short h)
{
    struct SlicksIntermissionRenderer *r=p;
    if(!r->surface->error) r->surface->error=slicks_restore_rectangle(&r->surface->ui,
        x==95?&r->cars:&r->buttons,x,y,sx,sy,w,h);
}
static inline void slicks_intermission_bevel(void *p,short x,short y,short w,short h,
    unsigned char red,unsigned char green,unsigned char blue)
{
    struct SlicksIntermissionRenderer *r=p;
    if(!r->surface->error) slicks_ui_bevel(&r->surface->ui,x,y,w,h,red,green,blue);
}
static inline void slicks_intermission_car(void *p,short vehicle,short x,short y)
{ slicks_intermission_icon(p,(short)(vehicle+1),x,y); }
static inline void slicks_intermission_label(void *p,unsigned font,const unsigned char *text,short x,short y,unsigned char flags)
{ (void)font; slicks_intermission_text(p,text,x,y,flags); }
static inline int slicks_intermission_renderer_draw(struct SlicksIntermissionRenderer *r,
    struct SlicksIntermissionMenu *m,const struct SlicksIntermissionContent *c)
{
    if(!r || !r->active || !m || !c) return -1;
    r->surface->error=0;
    const struct SlicksPlayerMenuDrawOps ops={slicks_intermission_restore,slicks_intermission_bevel,
        slicks_intermission_car,slicks_intermission_label,r};
    slicks_intermission_car_rows(m,c->roles,c->vehicles,1,&ops);
    /* The supplied DOS build hides these two implemented actions. Native
     * championship support exposes Change Cars and Save Game explicitly. */
    slicks_intermission_action_rows_from(m,r->participants,1,c->labels,&ops,r->expose_actions?0:2);
    return r->surface->error;
}
/* Owner loads all assets first and retains/restores its full parent page on
 * failure. Preview callback paints real track objects at (25,35), not a dump.
 * Compact snapshots are taken before drivers/headers, exactly as in 241e2. */
static inline int slicks_intermission_renderer_open(struct SlicksIntermissionRenderer *r,
    struct SlicksIntermissionMenu *m,const struct SlicksIntermissionContent *c,
    const unsigned char source_palette[768],unsigned char *buttons,unsigned long button_size,
    unsigned char *cars,unsigned long car_size,
    int (*preview)(void *,struct SlicksChunkyUi *,short,short),void *preview_context)
{
    if(!r || r->active || !r->surface || !m || !c || !source_palette || !buttons || !cars ||
       buttons==cars || button_size<3024 || car_size<320 || !preview || !c->track_name ||
       !c->slash || !c->labels[2] || !c->labels[3] ||
       (r->expose_actions && (!c->labels[0] || !c->labels[1]))) return -1;
    struct SlicksRecordsRenderer *s=r->surface;
    if(!s->ui.pixels || !s->ui.palette || !s->fonts[0] || !s->text || !s->icon) return -1;
    unsigned count=0;
    for(unsigned i=0;i<4;++i) if(c->roles[i]) {
        if(!c->names[i] || c->vehicles[i]<0 || c->vehicles[i]>=10) return -1;
        ++count;
    }
    if(!count) return -1;
    s->error=0; unsigned char table[256];
    if(slicks_intermission_prepare_panel(&s->ui,source_palette,table,count)) return -1;
    s->highlight=slicks_ui_nearest(&s->ui,5,5,15);
    if(slicks_save_rectangle(&r->buttons,buttons,button_size,&s->ui,125,(short)(77+10*count),70,42) ||
       slicks_save_rectangle(&r->cars,cars,car_size,&s->ui,95,77,8,40)) return -1;
    const struct SlicksIntermissionRowsOps rows={slicks_intermission_colour,slicks_intermission_number,
        slicks_intermission_text,slicks_intermission_fastest,r};
    slicks_intermission_driver_rows(c->roles,c->points,c->names,c->laps,c->fastest,&rows);
    if(s->error || slicks_intermission_prepare_preview(&s->ui,table)) return -1;
    const struct SlicksIntermissionHeaderOps heading={rows,slicks_intermission_nearest};
    slicks_intermission_track_header(c->track_index,c->track_total,c->track_name,c->slash,&heading);
    if(s->error || preview(preview_context,&s->ui,25,35)) return -1;
    r->participants=(unsigned char)count; r->active=1; slicks_intermission_init(m);
    int result=slicks_intermission_renderer_draw(r,m,c);
    if(result) r->active=0;
    return result;
}
#endif
