#include <exec/memory.h>
#include <proto/exec.h>
#include "amiga_shop.h"
#include "../../ui/menu_icon.h"
#include "../../ui/menu_background.h"
#include "../../ui/help_text_dirty.h"
extern void slicks_records_text(unsigned char *,const unsigned char *,const unsigned char *,short,short,unsigned short,unsigned short);
extern short slicks_menu_measure(const unsigned char *,const unsigned char *);

struct ShopPainter {
    struct SlicksAmigaPlayerMenu *menu;
    struct SlicksResourceArchive *archive;
    unsigned char *resource;
    unsigned char tint[256];
};
static void label(void *p,unsigned font,const unsigned char *s,short x,short y,unsigned char flags)
{
    struct SlicksAmigaPlayerMenu *m=((struct ShopPainter *)p)->menu;
    /* REGISTER! uses the original shadow flag. */
    slicks_records_text(m->renderer.ui.pixels,m->fonts[font],s,x,y,flags,0);
    slicks_font_text_dirty(&m->renderer.ui,m->fonts[font],s,x,y,1,flags,
        slicks_menu_measure(m->fonts[font],s),0);
    __asm__ volatile("" ::: "memory");
}
static void number(void *p,unsigned font,short value,short x,short y,unsigned char flags)
{ unsigned char s[7]; slicks_records_decimal(value,s); label(p,font,s,x,y,flags); }
static void restore(void *p,short x,short y,short sx,short sy,short w,short h)
{
    struct SlicksAmigaPlayerMenu *m=((struct ShopPainter *)p)->menu;
    if(slicks_restore_menu_background(&m->renderer.ui,m->saved,x,y,sx,sy,w,h)) m->error=-1;
}
static void bevel(void *p,short x,short y,short w,short h,unsigned char r,unsigned char g,unsigned char b)
{ slicks_ui_bevel(&((struct ShopPainter *)p)->menu->renderer.ui,x,y,w,h,r,g,b); }
static void rectangle(void *p,short l,short t,short r,short b,unsigned char colour)
{ slicks_ui_rectangle(&((struct ShopPainter *)p)->menu->renderer.ui,l,t,r,b,colour); }
static void colour(void *p,unsigned font,unsigned char r,unsigned char g,unsigned char b)
{
    struct SlicksAmigaPlayerMenu *m=((struct ShopPainter *)p)->menu;
    m->fonts[font][6]=slicks_ui_nearest(&m->renderer.ui,r,g,b);
}
static void tint(void *p,short l,short t,short r,short b)
{
    struct ShopPainter *s=p;
    if(slicks_ui_remap(&s->menu->renderer.ui,l,t,r,b,s->tint)) s->menu->error=-1;
}
static void sprite(void *p,short id,short left,short top)
{
    struct ShopPainter *s=p;
    char item_name[]="vir00.@16",car_name[]="auto01.@16";
    const char *name;
    if(id<13) { item_name[3]=(char)('0'+id/10);item_name[4]=(char)('0'+id%10);name=item_name; }
    else { unsigned car=id-13;car_name[5]=(char)('0'+car);name=car?car_name:"carimage16"; }
    long size=slicks_resource_archive_load(s->archive,name,s->resource,65536);
    unsigned char pixels[256];unsigned short width,height;
    if(size<0 || slicks_decode_menu_icon(s->resource,(unsigned long)size,s->menu->renderer.ui.palette,
        pixels,sizeof pixels,&width,&height) || left<0 || top<0 || left+width>320 || top+height>200) {
        s->menu->error=-1;return;
    }
    for(unsigned y=0;y<height;++y) for(unsigned x=0;x<width;++x)
        if(pixels[y*width+x]) s->menu->renderer.ui.pixels[mult320[top+y]+left+x]=pixels[y*width+x];
    if(s->menu->renderer.ui.dirty)
        s->menu->renderer.ui.dirty(s->menu->renderer.ui.dirty_context,
            left,top,(short)(left+width),(short)(top+height));
}
static struct SlicksShopDrawOps draw_ops(struct ShopPainter *p)
{ return (struct SlicksShopDrawOps){{restore,bevel,sprite,label,p},number,rectangle}; }

int slicks_amiga_shop_draw(struct SlicksAmigaPlayerMenu *m,const struct SlicksShopContent *c,
    const struct SlicksShopMenu *state)
{
    struct ShopPainter painter={.menu=m};
    struct SlicksShopDrawOps ops=draw_ops(&painter);
    slicks_amiga_player_menu_restore(m);
    signed char column=0;
    for(unsigned d=0;d<4 && (int)d<state->driver;++d) column+=c->session->players.selected[d]!=0;
    if(slicks_draw_shop_values(c->session,c->rules,c->extra,column,state->row,-1,-1,state->count,
        slicks_ui_nearest(&m->renderer.ui,50,10,10),slicks_ui_nearest(&m->renderer.ui,70,70,10),
        c->exit_label,&ops)<0) return -1;
    return m->error || m->renderer.error?-1:0;
}

struct SlicksAmigaPlayerMenu *slicks_amiga_shop_create(struct SlicksResourceArchive *archive,
    unsigned char *chunky,const struct SlicksShopContent *c,struct SlicksShopMenu *state)
{
    static unsigned char palette[768];
    if(slicks_resource_archive_load(archive,"tuning.@p",palette,sizeof palette)!=sizeof palette) return 0;
    struct SlicksAmigaPlayerMenu *m=slicks_amiga_race_surface_create(archive,chunky,palette);
    unsigned char *resource=AllocMem(65536,MEMF_ANY);
    if(!m || !resource) goto failed;
    long size=slicks_resource_archive_load(archive,"tuning.@I",resource,65536);
    unsigned short width,height;
    if(size<0 || slicks_decode_indexed_menu_icon(resource,(unsigned long)size,chunky,64000,
        &width,&height) || width!=320 || height!=200) goto failed;
    *state=(struct SlicksShopMenu){.driver=slicks_shop_driver(c->session->players.participation,-1,1)};
    while(state->count<13 && slicks_shop_item(c->rules,&c->session->options,c->session->inventory[0],
        c->session->players.participation[0],c->session->players.vehicle[0],c->extra,state->count)>=0) ++state->count;
    struct ShopPainter painter={.menu=m,.archive=archive,.resource=resource};
    slicks_ui_tint_table(palette,painter.tint,10,10,30,50);
    struct SlicksShopStaticOps ops={draw_ops(&painter),colour,tint};
    if(slicks_draw_shop_background(c,state->count,&ops) || m->error) goto failed;
    /* Original 2cf21 saves the fully decorated screen, not the raw image. */
    for(unsigned i=0;i<64000;++i) m->saved[i]=chunky[i];
    m->saved_dirty_count=0; m->track_saved_dirty=1;
    FreeMem(resource,65536);resource=0;
    if(slicks_amiga_shop_draw(m,c,state)) goto failed;
    return m;
failed:
    if(resource) FreeMem(resource,65536);
    slicks_amiga_player_menu_destroy(m); return 0;
}
