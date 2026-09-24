#ifndef SLICKS_OPTIONS_MENU_RENDERER_H
#define SLICKS_OPTIONS_MENU_RENDERER_H
#include "options_menu_draw.h"
#include "profile_editor_renderer.h"

/* Uses an original prepared Options background and decoded kirj font. The
 * caller owns the surrounding screen, resource preparation and saved bitmap. */
struct SlicksOptionsRenderer {
    struct SlicksPlayerMenuRenderer *surface;
    unsigned char tint[256],colours[4];
    short (*measure)(const unsigned char *,const unsigned char *);
};
static inline void slicks_options_restore(void *p,short x,short y,short sx,short sy,short w,short h)
{ struct SlicksOptionsRenderer *r=p; slicks_player_renderer_restore(r->surface,x,y,sx,sy,w,h); }
static inline void slicks_options_bevel(void *p,short x,short y,short w,short h,unsigned char red,unsigned char green,unsigned char blue)
{ struct SlicksOptionsRenderer *r=p; slicks_player_renderer_bevel(r->surface,x,y,w,h,red,green,blue); }
static inline void slicks_options_text(void *p,unsigned font,const unsigned char *text,short x,short y,unsigned char flags)
{ struct SlicksOptionsRenderer *r=p; slicks_player_renderer_text(r->surface,font,text,x,y,flags); }
static inline void slicks_options_colour(void *p,unsigned char colour)
{ struct SlicksOptionsRenderer *r=p; slicks_editor_renderer_colour(r->surface,colour); }
static inline unsigned char slicks_options_measure(void *p,const unsigned char *text)
{ struct SlicksOptionsRenderer *r=p; return (unsigned char)r->measure(r->surface->fonts[0],text); }
static inline void slicks_options_pattern(void *p,short l,short t,short right,short b)
{
    struct SlicksOptionsRenderer *r=p;
    if(slicks_ui_remap(&r->surface->ui,l,t,right,b,r->tint)) r->surface->error=-1;
}
static inline void slicks_options_rectangle(void *p,short l,short t,short right,short b,unsigned char colour)
{ struct SlicksOptionsRenderer *r=p; slicks_editor_renderer_rectangle(r->surface,l,t,right,b,colour); }
static inline int slicks_options_renderer_init(struct SlicksOptionsRenderer *r,
    struct SlicksPlayerMenuRenderer *surface,
    short (*measure)(const unsigned char *,const unsigned char *))
{
    if(!r || !surface || !surface->ui.pixels || !surface->ui.palette ||
       !surface->saved || !surface->fonts[0] || !surface->text || !measure) return -1;
    r->surface=surface; r->measure=measure;
    r->colours[0]=slicks_ui_nearest(&surface->ui,70,70,60);
    r->colours[1]=slicks_ui_nearest(&surface->ui,70,70,40);
    r->colours[2]=slicks_ui_nearest(&surface->ui,30,30,30);
    r->colours[3]=slicks_ui_nearest(&surface->ui,50,45,20);
    slicks_ui_tint_table(surface->ui.palette,r->tint,30,10,30,75);
    return 0;
}
static inline int slicks_options_renderer_draw(struct SlicksOptionsRenderer *r,
    struct SlicksOptionsMenu *menu,const struct SlicksConfiguration *configuration,
    const struct SlicksOptionSpec specs[15],const struct SlicksOptionsLabels *labels)
{
    if(!r || !r->surface || !r->measure) return -1;
    r->surface->error=0;
    const struct SlicksOptionsDrawOps ops={{slicks_options_restore,slicks_options_bevel,
        0,slicks_options_text,r},slicks_options_colour,slicks_options_measure,
        slicks_options_pattern,slicks_options_rectangle};
    if(slicks_draw_options_menu(menu,configuration,specs,labels,r->colours,&ops)) return -1;
    return r->surface->error;
}

/* Original static preparation 28e7f..28f6b on the existing menu surface.
 * DOS tints to row 220; only rows 0..199 belong to the visible Amiga page.
 * Title is the resolved original "settings" string, not a replacement. */
static inline int slicks_options_renderer_prepare(struct SlicksOptionsRenderer *r,
    const unsigned char *title)
{
    if(!r || !r->surface || !title || !r->surface->fonts[1]) return -1;
    struct SlicksPlayerMenuRenderer *s=r->surface;
    if(!s->saved || s->saved==s->ui.pixels) return -1;
    s->error=0;
    s->fonts[1][6]=slicks_ui_nearest(&s->ui,60,60,60);
    unsigned char table[256];
    slicks_ui_tint_table(s->ui.palette,table,10,10,50,66);
    if(slicks_ui_remap(&s->ui,5,30,140,200,table)) return -1;
    slicks_player_renderer_text(s,1,title,75,25,1);
    if(s->error) return s->error;
    for(unsigned long i=0;i<64000;++i) s->saved[i]=s->ui.pixels[i];
    return 0;
}
#endif
