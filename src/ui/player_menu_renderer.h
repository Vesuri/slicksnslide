#ifndef SLICKS_PLAYER_MENU_RENDERER_H
#define SLICKS_PLAYER_MENU_RENDERER_H
#include "player_menu_prepare.h"
#include "player_menu_draw.h"
#include "menu_background.h"

struct SlicksMenuIcon {
    const unsigned char *pixels;
    unsigned short width,height;
};
/* Fonts/icons have already been decoded from original resources. The source
 * snapshot is storage for our own prepared screen, never a DOS frame dump.
 * Native text/icon callbacks draw into ui.pixels. Their adapter must report
 * text/icon dirty bounds through ui.dirty; the painters below do so directly.
 * Font slots: kirj, pieni, iso. Icon slots: computer then vehicle indices. */
struct SlicksPlayerMenuRenderer {
    struct SlicksChunkyUi ui;
    unsigned char *saved;
    unsigned char *fonts[3];
    const struct SlicksMenuIcon *icons;
    unsigned icon_count;
    void (*text)(void *,struct SlicksChunkyUi *,unsigned char *,const unsigned char *,short,short,unsigned char);
    void (*icon)(void *,struct SlicksChunkyUi *,const struct SlicksMenuIcon *,short,short);
    void *context;
    int error;
};
static inline void slicks_player_renderer_text(void *context,unsigned font,
    const unsigned char *text,short x,short y,unsigned char flags)
{
    struct SlicksPlayerMenuRenderer *r=context;
    if(font>=3 || !r->fonts[font]) { r->error=-1; return; }
    r->text(r->context,&r->ui,r->fonts[font],text,x,y,flags);
}
static inline void slicks_player_renderer_redraw_text(void *context,unsigned font,
    const unsigned char *text,short x,short y,unsigned char flags)
{
    /* Original redraw callback uses slot 1 for iso, not pieni. */
    slicks_player_renderer_text(context,font?2:0,text,x,y,flags);
}
static inline void slicks_player_renderer_restore(void *context,short x,short y,
    short sx,short sy,short width,short height)
{
    struct SlicksPlayerMenuRenderer *r=context;
    if(slicks_restore_menu_background(&r->ui,r->saved,x,y,sx,sy,width,height)) r->error=-1;
}
static inline void slicks_player_renderer_bevel(void *context,short x,short y,
    short width,short height,unsigned char red,unsigned char green,unsigned char blue)
{
    struct SlicksPlayerMenuRenderer *r=context;
    slicks_ui_bevel(&r->ui,x,y,width,height,red,green,blue);
}
static inline void slicks_player_renderer_sprite(void *context,short sprite,short x,short y)
{
    struct SlicksPlayerMenuRenderer *r=context;
    unsigned index=(unsigned)(sprite+1);
    if(index>=r->icon_count || !r->icons[index].pixels) { r->error=-1; return; }
    r->icon(r->context,&r->ui,&r->icons[index],x,y);
}
static inline int slicks_player_renderer_prepare(struct SlicksPlayerMenuRenderer *r,
    const unsigned char *title,const unsigned char *footer,unsigned char footer_percent)
{
    if(!r || !r->ui.pixels || !r->ui.palette || !r->saved || r->saved==r->ui.pixels ||
       !r->text || !r->fonts[0] || !r->fonts[1] || !r->fonts[2]) return -1;
    r->error=0;
    const struct SlicksPlayerMenuPrepareOps ops={slicks_player_renderer_text,r};
    slicks_prepare_player_menu(&r->ui,r->fonts,title,footer,footer_percent,&ops);
    if(r->error) return r->error;
    for(unsigned long i=0;i<64000;++i) r->saved[i]=r->ui.pixels[i];
    return 0;
}
static inline int slicks_player_renderer_draw(struct SlicksPlayerMenuRenderer *r,
    unsigned row,const short selected[4],const signed char participation[4],
    const struct SlicksPlayerProfiles *profiles,short vehicle_count,
    const struct SlicksPlayerMenuLabels *labels)
{
    if(!r || !r->ui.pixels || !r->ui.palette || !r->saved || !r->text ||
       !r->icon || !r->icons || vehicle_count<0 || (unsigned)vehicle_count+1>r->icon_count) return -1;
    r->error=0;
    const struct SlicksPlayerMenuDrawOps ops={slicks_player_renderer_restore,
        slicks_player_renderer_bevel,slicks_player_renderer_sprite,
        slicks_player_renderer_redraw_text,r};
    if(slicks_draw_player_menu(row,selected,participation,profiles,vehicle_count,labels,&ops)) return -1;
    return r->error;
}
#endif
