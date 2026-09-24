#ifndef SLICKS_CONTROLLERS_DIALOG_RENDERER_H
#define SLICKS_CONTROLLERS_DIALOG_RENDERER_H
#include "controllers_dialog_draw.h"
#include "profile_editor_renderer.h"
#include "saved_rectangle.h"
#include "palette_remap.h"

/* Original modal 2d8ac..2dfd4. Caller loads three indexed device icons and
 * five RGB action icons, and supplies both compact rectangle buffers before
 * entering hardware takeover. The underlying Options snapshot is untouched. */
struct SlicksControllersRenderer {
    struct SlicksPlayerMenuRenderer *surface;
    struct SlicksSavedRectangle original,tinted;
    const struct SlicksMenuIcon *icons; /* keyboard, joystick, LPT, five actions */
    short x,y;
    unsigned char colours[2],old_colours[2],active;
};
static inline void slicks_controllers_restore(void *p,short x,short y,short sx,short sy,short w,short h)
{
    struct SlicksControllersRenderer *r=p;
    if(slicks_restore_rectangle(&r->surface->ui,&r->tinted,x,y,sx,sy,w,h)) r->surface->error=-1;
}
static inline void slicks_controllers_bevel(void *p,short x,short y,short w,short h,unsigned char red,unsigned char green,unsigned char blue)
{ struct SlicksControllersRenderer *r=p; slicks_player_renderer_bevel(r->surface,x,y,w,h,red,green,blue); }
static inline void slicks_controllers_text(void *p,unsigned font,const unsigned char *text,short x,short y,unsigned char flags)
{ struct SlicksControllersRenderer *r=p; slicks_player_renderer_text(r->surface,font,text,x,y,flags); }
static inline void slicks_controllers_colour(void *p,unsigned char colour)
{ struct SlicksControllersRenderer *r=p; slicks_editor_renderer_colour(r->surface,colour); }
static inline void slicks_controllers_number(void *p,unsigned short value,short x,short y,unsigned char flags)
{ struct SlicksControllersRenderer *r=p; slicks_editor_renderer_number(r->surface,value,x,y,flags); }
static inline void slicks_controllers_icon(void *p,short index,short x,short y)
{
    struct SlicksControllersRenderer *r=p;
    if(index<0 || index>=8) { r->surface->error=-1; return; }
    r->surface->icon(r->surface->context,&r->surface->ui,&r->icons[index],x,y);
}
static inline int slicks_controllers_renderer_open(struct SlicksControllersRenderer *r,
    struct SlicksControllersDialog *d,short x,short y,
    unsigned char *original,unsigned long original_size,
    unsigned char *tinted,unsigned long tinted_size)
{
    if(!r || r->active || !d || !r->surface || !r->icons ||
       !original || !tinted || original==tinted || original_size<16800 || tinted_size<16000 ||
       x<0 || x>120 || y<2 || y>118) return -1;
    struct SlicksPlayerMenuRenderer *s=r->surface;
    if(!s->ui.pixels || !s->ui.palette || !s->fonts[0] || !s->fonts[1] || !s->text || !s->icon) return -1;
    for(unsigned i=0;i<8;++i)
        if(!r->icons[i].pixels || !r->icons[i].width || !r->icons[i].height) return -1;
    if(slicks_save_rectangle(&r->original,original,original_size,&s->ui,x,y-2,200,84)) return -1;
    r->old_colours[0]=s->fonts[0][6]; r->old_colours[1]=s->fonts[1][6];
    s->fonts[1][6]=slicks_ui_nearest(&s->ui,50,50,40); s->fonts[0][6]=1;
    unsigned char table[256]; slicks_ui_tint_table(s->ui.palette,table,45,10,45,75);
    slicks_ui_remap(&s->ui,x,y,x+200,y+80,table);
    s->error=0;
    for(unsigned i=0;i<5;++i) slicks_controllers_icon(r,(short)(3+i),(short)(x+55+28*i),y+6);
    for(unsigned i=0;i<4;++i) {
        unsigned char text[2]={(unsigned char)('1'+i),0};
        slicks_player_renderer_text(s,1,text,x+10,(short)(y+15+12*i),1);
    }
    if(slicks_save_rectangle(&r->tinted,tinted,tinted_size,&s->ui,x,y,200,80)) return -1;
    r->colours[0]=slicks_ui_nearest(&s->ui,55,55,50);
    r->colours[1]=slicks_ui_nearest(&s->ui,38,38,38);
    r->x=x; r->y=y; r->active=1;
    *d=(struct SlicksControllersDialog){0,0,0,0,-1};
    return s->error;
}
static inline int slicks_controllers_renderer_draw(struct SlicksControllersRenderer *r,
    struct SlicksControllersDialog *d,const struct SlicksConfiguration *c,
    const struct SlicksControllersLabels *labels)
{
    if(!r || !r->active || !d || !c || !labels) return -1;
    r->surface->error=0;
    struct SlicksControllersDrawOps ops={{slicks_controllers_restore,slicks_controllers_bevel,
        slicks_controllers_icon,slicks_controllers_text,r},slicks_controllers_colour,slicks_controllers_number};
    if(slicks_draw_controllers(d,c,r->x,r->y,labels,r->colours,&ops)) return -1;
    return r->surface->error;
}
static inline int slicks_controllers_renderer_capture(struct SlicksControllersRenderer *r,
    const struct SlicksControllersDialog *d,const unsigned char *prompt)
{
    if(!r || !r->active || !d || !d->capturing || d->row>=4 ||
       d->column<1 || d->column>5 || !prompt) return -1;
    slicks_player_renderer_text(r->surface,1,prompt,
        (short)(r->x+64+28*(d->column-1)),(short)(r->y+17+12*d->row),1);
    return r->surface->error;
}
static inline int slicks_controllers_renderer_close(struct SlicksControllersRenderer *r)
{
    if(!r || !r->active) return -1;
    if(slicks_restore_rectangle(&r->surface->ui,&r->original,r->x,r->y-2,0,0,200,84)) return -1;
    r->surface->fonts[0][6]=r->old_colours[0]; r->surface->fonts[1][6]=r->old_colours[1];
    r->active=0;
    return 0;
}
#endif
