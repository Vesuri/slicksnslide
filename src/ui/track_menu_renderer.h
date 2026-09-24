#ifndef SLICKS_TRACK_MENU_RENDERER_H
#define SLICKS_TRACK_MENU_RENDERER_H
#include "track_menu_draw.h"
#include "track_menu_prepare.h"
#include "profile_editor_renderer.h"

struct SlicksTrackRenderer {
    struct SlicksPlayerMenuRenderer *surface;
    unsigned char tint[256],scroll_colour;
    const unsigned char *(*name)(void *,unsigned);
    void *name_context;
};
static inline void slicks_tracks_restore(void *p,short x,short y,short sx,short sy,short w,short h)
{ struct SlicksTrackRenderer *r=p; slicks_player_renderer_restore(r->surface,x,y,sx,sy,w,h); }
static inline void slicks_tracks_bevel(void *p,short x,short y,short w,short h,unsigned char red,unsigned char green,unsigned char blue)
{ struct SlicksTrackRenderer *r=p; slicks_player_renderer_bevel(r->surface,x,y,w,h,red,green,blue); }
static inline void slicks_tracks_text(void *p,unsigned font,const unsigned char *text,short x,short y,unsigned char flags)
{ struct SlicksTrackRenderer *r=p; slicks_player_renderer_text(r->surface,font,text,x,y,flags); }
static inline void slicks_tracks_colour(void *p,unsigned char colour)
{ struct SlicksTrackRenderer *r=p; slicks_editor_renderer_colour(r->surface,colour); }
static inline unsigned char slicks_tracks_nearest(void *p,unsigned char red,unsigned char green,unsigned char blue)
{ struct SlicksTrackRenderer *r=p; return slicks_editor_renderer_nearest(r->surface,red,green,blue); }
static inline void slicks_tracks_rectangle(void *p,short l,short t,short right,short b,unsigned char colour)
{ struct SlicksTrackRenderer *r=p; slicks_editor_renderer_rectangle(r->surface,l,t,right,b,colour); }
static inline void slicks_tracks_tint(void *p,short l,short t,short right,short b)
{
    struct SlicksTrackRenderer *r=p;
    if(slicks_ui_remap(&r->surface->ui,l,t,right,b,r->tint)) r->surface->error=-1;
}
static inline const unsigned char *slicks_tracks_name(void *p,unsigned index)
{ struct SlicksTrackRenderer *r=p; return r->name(r->name_context,index); }
static inline void slicks_tracks_number(void *p,unsigned short value,short x,short y,unsigned char flags)
{
    /* Original 302b6 interprets its argument as a signed word. Unlike the
     * profile editor's byte percentages, track counts need more digits. */
    unsigned char digits[7]; unsigned at=6,magnitude=value;
    int negative=value>=0x8000;
    if(negative) magnitude=0x10000U-value;
    digits[at]=0;
    do { digits[--at]=(unsigned char)('0'+magnitude%10); magnitude/=10; } while(magnitude);
    if(negative) digits[--at]='-';
    slicks_tracks_text(p,0,digits+at,x,y,flags);
}
static inline int slicks_track_renderer_init(struct SlicksTrackRenderer *r,
    struct SlicksPlayerMenuRenderer *surface,unsigned char percent,
    const unsigned char *(*name)(void *,unsigned),void *name_context)
{
    if(!r || !surface || !surface->ui.pixels || !surface->ui.palette ||
       !surface->saved || surface->saved==surface->ui.pixels || !surface->fonts[0] ||
       !surface->text || !name) return -1;
    r->surface=surface; r->name=name; r->name_context=name_context;
    slicks_ui_tint_table(surface->ui.palette,r->tint,45,10,45,percent);
    r->scroll_colour=slicks_ui_nearest(&surface->ui,50,10,10);
    return 0;
}
static inline int slicks_track_renderer_prepare(struct SlicksTrackRenderer *r,
    const unsigned char *title,const unsigned char *footer,short total,unsigned char percent)
{
    if(!r || !r->surface || !title || !footer || total<0 ||
       !r->surface->fonts[1] || !r->surface->fonts[2]) return -1;
    struct SlicksPlayerMenuRenderer *s=r->surface; s->error=0;
    const struct SlicksPlayerMenuPrepareOps ops={slicks_player_renderer_text,s};
    slicks_prepare_track_menu(&s->ui,s->fonts,title,footer,total,percent,&ops);
    if(s->error) return s->error;
    for(unsigned long i=0;i<64000;++i) s->saved[i]=s->ui.pixels[i];
    return 0;
}
static inline int slicks_track_renderer_draw(struct SlicksTrackRenderer *r,
    struct SlicksTrackMenu *menu,short total,const struct SlicksTrackPlaylist *selected,
    const struct SlicksTrackMenuLabels *labels)
{
    if(!r || !r->surface || !r->name) return -1;
    r->surface->error=0;
    const struct SlicksTrackMenuDrawOps ops={{{slicks_tracks_restore,slicks_tracks_bevel,
        0,slicks_tracks_text,r},slicks_tracks_colour,slicks_tracks_number,
        slicks_tracks_nearest,slicks_tracks_rectangle},slicks_tracks_tint,slicks_tracks_name};
    if(slicks_draw_track_menu(menu,total,selected,labels,r->scroll_colour,&ops)) return -1;
    return r->surface->error;
}
#endif
