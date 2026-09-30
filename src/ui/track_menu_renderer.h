#ifndef SLICKS_TRACK_MENU_RENDERER_H
#define SLICKS_TRACK_MENU_RENDERER_H
#include "track_menu_draw.h"
#include "track_menu_prepare.h"
#include "profile_editor_renderer.h"

#define SLICKS_TRACK_PARTIAL_TRACKS 512
struct SlicksTrackRect { short left,top,right,bottom; };
struct SlicksTrackRenderer {
    struct SlicksPlayerMenuRenderer *surface;
    unsigned char tint[256],scroll_colour;
    const unsigned char *(*name)(void *,unsigned);
    void *name_context;
    /* Cursor-only redraw. The original redraws the whole list for every
     * move; with the page, playlist and counters unchanged the pixels differ
     * only inside the old and new bevel and scroll-marker rectangles. Every
     * draw operation still runs in order. Operations outside that damage are
     * skipped; others run in full and their pixels outside it are put back. */
    void (*parent_dirty)(void *,short,short,short,short);
    void *parent_dirty_context;
    unsigned long serial,drawn_serial;
    struct SlicksTrackMenu drawn;
    short drawn_total,drawn_tracks[SLICKS_TRACK_PARTIAL_TRACKS];
    unsigned short drawn_count;
    unsigned char drawn_valid,allow_partial,partial,partial_failed,mute;
    struct SlicksTrackRect damage[4],saved_area;
    unsigned damage_count;
    unsigned char scratch[4096];
};
/* Every painter reports through the surface's dirty callback, so a report
 * the renderer did not make means the shown list is no longer its own. */
static inline void slicks_tracks_dirty(void *p,short l,short t,short right,short b)
{
    struct SlicksTrackRenderer *r=p;
    if(r->mute) return;
    ++r->serial;
    if(r->parent_dirty) r->parent_dirty(r->parent_dirty_context,l,t,right,b);
}
static inline int slicks_tracks_intersect(const struct SlicksTrackRect *a,const struct SlicksTrackRect *b,struct SlicksTrackRect *out)
{
    struct SlicksTrackRect c={a->left>b->left?a->left:b->left,a->top>b->top?a->top:b->top,
        a->right<b->right?a->right:b->right,a->bottom<b->bottom?a->bottom:b->bottom};
    if(c.left>=c.right || c.top>=c.bottom) return 0;
    if(out) *out=c;
    return 1;
}
/* Returns 1 when the caller should run the operation with bounds b. */
static inline int slicks_tracks_begin(struct SlicksTrackRenderer *r,struct SlicksTrackRect b)
{
    if(!r->partial) return 1;
    if(r->partial_failed) return 0;
    if(b.left<0) b.left=0;
    if(b.top<0) b.top=0;
    if(b.right>320) b.right=320;
    if(b.bottom>200) b.bottom=200;
    int hit=0;
    for(unsigned i=0;i<r->damage_count;++i) hit|=slicks_tracks_intersect(&b,&r->damage[i],0);
    if(!hit) return 0;
    unsigned width=(unsigned)(b.right-b.left);
    if(width*(unsigned)(b.bottom-b.top)>sizeof r->scratch) { r->partial_failed=1; return 0; }
    const unsigned char *pixels=r->surface->ui.pixels; unsigned at=0;
    for(short y=b.top;y<b.bottom;++y,at+=width)
        slicks_ui_copy_row(r->scratch+at,pixels+mult320[(unsigned)y]+b.left,(int)width);
    r->saved_area=b; r->mute=1;
    return 1;
}
static inline void slicks_tracks_end(struct SlicksTrackRenderer *r)
{
    if(!r->partial || !r->mute) return;
    const struct SlicksTrackRect b=r->saved_area;
    unsigned char *pixels=r->surface->ui.pixels; unsigned width=(unsigned)(b.right-b.left),at=0;
    for(short y=b.top;y<b.bottom;++y,at+=width) {
        /* Put back the runs of this row that lie outside every damage rect. */
        short cuts[8]; unsigned count=0;
        for(unsigned i=0;i<r->damage_count;++i) {
            const struct SlicksTrackRect *d=&r->damage[i];
            if(y<d->top || y>=d->bottom || d->right<=b.left || d->left>=b.right) continue;
            short l=d->left<b.left?b.left:d->left,rt=d->right>b.right?b.right:d->right;
            unsigned j=count; count+=2;
            while(j && cuts[j-2]>l) { cuts[j]=cuts[j-2]; cuts[j+1]=cuts[j-1]; j-=2; }
            cuts[j]=l; cuts[j+1]=rt;
        }
        short x=b.left;
        for(unsigned i=0;i<=count;i+=2) {
            short end=i<count?cuts[i]:b.right;
            if(end>x) slicks_ui_copy_row(pixels+mult320[(unsigned)y]+x,r->scratch+at+(x-b.left),end-x);
            if(i<count && cuts[i+1]>x) x=cuts[i+1];
        }
    }
    r->mute=0;
    for(unsigned i=0;i<r->damage_count;++i) {
        struct SlicksTrackRect c;
        if(slicks_tracks_intersect(&b,&r->damage[i],&c) && r->parent_dirty)
            r->parent_dirty(r->parent_dirty_context,c.left,c.top,c.right,c.bottom);
    }
}
static inline void slicks_tracks_restore(void *p,short x,short y,short sx,short sy,short w,short h)
{
    struct SlicksTrackRenderer *r=p;
    if(!r->partial) { slicks_player_renderer_restore(r->surface,x,y,sx,sy,w,h); return; }
    if(r->partial_failed) return;
    /* Same page-origin mapping as slicks_restore_menu_background: pixel
     * (dx,dy) receives saved(dx-x,dy-y); restoring is idempotent. */
    if(x<0 || y<0 || sx<0 || sy<0 || w<0 || w>320 || h<0 || h>255) { r->partial_failed=1; return; }
    int ax=sx&~3,left=x+ax,top=y+sy,columns=(w+3)&~3,rows=h;
    if(!w || !h || left>=320 || top>=200 || ax>=320 || sy>=200) return;
    if(columns>320-left) columns=320-left;
    if(rows>200-top) rows=200-top;
    const struct SlicksTrackRect area={(short)left,(short)top,(short)(left+columns),(short)(top+rows)};
    unsigned char *pixels=r->surface->ui.pixels; const unsigned char *saved=r->surface->saved;
    for(unsigned i=0;i<r->damage_count;++i) {
        struct SlicksTrackRect c;
        if(!slicks_tracks_intersect(&area,&r->damage[i],&c)) continue;
        for(short py=c.top;py<c.bottom;++py)
            slicks_ui_copy_row(pixels+mult320[(unsigned)py]+c.left,saved+mult320[(unsigned)(py-y)]+(c.left-x),c.right-c.left);
        if(r->parent_dirty) r->parent_dirty(r->parent_dirty_context,c.left,c.top,c.right,c.bottom);
    }
}
static inline void slicks_tracks_bevel(void *p,short x,short y,short w,short h,unsigned char red,unsigned char green,unsigned char blue)
{
    struct SlicksTrackRenderer *r=p;
    if(!slicks_tracks_begin(r,(struct SlicksTrackRect){x,y,(short)(x+w+1),(short)(y+h+1)})) return;
    slicks_player_renderer_bevel(r->surface,x,y,w,h,red,green,blue);
    slicks_tracks_end(r);
}
/* Painter bounds with a margin of the widest glyph on each side: the pen
 * advances as in sui_font_measure (spacing 1), aligned by that width. */
static inline struct SlicksTrackRect slicks_tracks_text_bounds(const unsigned char *font,const unsigned char *text,
    short x,short y,unsigned char flags)
{
    const unsigned char *codes=font+6+font[5],*widths=codes+font[0];
    int widest=font[1],width=0,span=0,overhang=font[3]<128?font[3]:256-font[3];
    for(unsigned i=0;i<font[0];++i) if(widths[i]>widest) widest=widths[i];
    for(unsigned n=0;text[n];++n) {
        unsigned g=0; while(g<font[0] && codes[g]!=text[n]) ++g;
        int step=g>0 && g<font[0]?widths[g]+(signed char)font[3]:font[1];
        width+=step; span+=step<0?-step:step;
    }
    if((flags&3)==1) x=(short)(x-width/2);
    else if((flags&3)==2) x=(short)(x-width);
    int margin=widest+overhang+2;
    return (struct SlicksTrackRect){(short)(x-margin-span),(short)(y-1),(short)(x+span+margin),(short)(y+font[2]+2)};
}
static inline void slicks_tracks_text(void *p,unsigned font,const unsigned char *text,short x,short y,unsigned char flags)
{
    struct SlicksTrackRenderer *r=p;
    if(r->partial && !r->partial_failed) {
        if(font>=3 || !r->surface->fonts[font]) { r->partial_failed=1; return; }
        /* Most text lies on rows outside the damage: skip it before any
         * glyph lookup (bounds rows are y-1..y+height+2). */
        short top=(short)(y-1),bottom=(short)(y+r->surface->fonts[font][2]+2); int rows=0;
        for(unsigned i=0;i<r->damage_count;++i) rows|=top<r->damage[i].bottom && bottom>r->damage[i].top;
        if(!rows) return;
        for(unsigned i=0;text[i];++i) if(text[i]==13 || text[i]==10 || text[i]==8 || text[i]==207) { r->partial_failed=1; return; }
        if(!slicks_tracks_begin(r,slicks_tracks_text_bounds(r->surface->fonts[font],text,x,y,flags))) return;
    } else if(r->partial) return;
    slicks_player_renderer_text(r->surface,font,text,x,y,flags);
    slicks_tracks_end(r);
}
static inline void slicks_tracks_colour(void *p,unsigned char colour)
{ struct SlicksTrackRenderer *r=p; slicks_editor_renderer_colour(r->surface,colour); }
static inline unsigned char slicks_tracks_nearest(void *p,unsigned char red,unsigned char green,unsigned char blue)
{ struct SlicksTrackRenderer *r=p; return slicks_editor_renderer_nearest(r->surface,red,green,blue); }
static inline void slicks_tracks_rectangle(void *p,short l,short t,short right,short b,unsigned char colour)
{
    struct SlicksTrackRenderer *r=p;
    if(!slicks_tracks_begin(r,(struct SlicksTrackRect){l,t,right,b})) return;
    slicks_editor_renderer_rectangle(r->surface,l,t,right,b,colour);
    slicks_tracks_end(r);
}
static inline void slicks_tracks_tint(void *p,short l,short t,short right,short b)
{
    struct SlicksTrackRenderer *r=p;
    if(!slicks_tracks_begin(r,(struct SlicksTrackRect){l,t,right,b})) return;
    if(slicks_ui_remap(&r->surface->ui,l,t,right,b,r->tint)) r->surface->error=-1;
    slicks_tracks_end(r);
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
    r->parent_dirty=surface->ui.dirty; r->parent_dirty_context=surface->ui.dirty_context;
    surface->ui.dirty=slicks_tracks_dirty; surface->ui.dirty_context=r;
    r->drawn_valid=r->allow_partial=r->partial=r->mute=0;
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
    const struct SlicksTrackMenu *d=&r->drawn;
    unsigned char partial=(unsigned char)(r->allow_partial && r->drawn_valid && r->serial==r->drawn_serial &&
        menu && selected && menu->previous!=menu->cursor && menu->top==d->top &&
        menu->random_count==d->random_count && menu->random_order==d->random_order &&
        total==r->drawn_total && selected->count==r->drawn_count && selected->count<=SLICKS_TRACK_PARTIAL_TRACKS &&
        slicks_track_playlist_valid(selected));
    for(unsigned i=0;partial && i<selected->count;++i) if(selected->tracks[i]!=r->drawn_tracks[i]) partial=0;
    r->allow_partial=0;
    short previous=menu?menu->previous:0;
    /* 270c9 draws nothing for an unchanged cursor; keep the snapshot. */
    if(menu && menu->previous==menu->cursor) return slicks_draw_track_menu(menu,total,selected,labels,r->scroll_colour,&ops);
    if(partial) {
        /* Bevels (270f5..27121) and scroll marker (2718c..271b8) are the
         * only cursor- and column-dependent pixels. */
        struct SlicksTrackRect areas[4]; unsigned count=0;
        const struct SlicksTrackMenu *states[2]={d,menu};
        for(unsigned i=0;i<2;++i) {
            const struct SlicksTrackMenu *m=states[i];
            if(!m->column) areas[count++]=(struct SlicksTrackRect){15,(short)(12+8*(m->cursor-m->top)),80,(short)(12+8*(m->cursor-m->top)+11)};
            else areas[count++]=(struct SlicksTrackRect){127,(short)(16+13*m->column),208,(short)(16+13*m->column+12)};
            if(total>22) {
                short y=(short)((long)m->cursor*166/total+15);
                areas[count++]=(struct SlicksTrackRect){5,y,8,(short)(y+6)};
            }
        }
        /* Merge overlaps so tinting never visits a pixel twice. */
        for(unsigned merged=1;merged;) {
            merged=0;
            for(unsigned i=0;i<count && !merged;++i) for(unsigned j=i+1;j<count && !merged;++j)
                if(slicks_tracks_intersect(&areas[i],&areas[j],0)) {
                    struct SlicksTrackRect *a=&areas[i]; const struct SlicksTrackRect *b=&areas[j];
                    if(b->left<a->left) a->left=b->left;
                    if(b->top<a->top) a->top=b->top;
                    if(b->right>a->right) a->right=b->right;
                    if(b->bottom>a->bottom) a->bottom=b->bottom;
                    areas[j]=areas[--count]; merged=1;
                }
        }
        for(unsigned i=0;i<count;++i) r->damage[i]=areas[i];
        r->damage_count=count; r->partial=1; r->partial_failed=0;
        int failed=slicks_draw_track_menu(menu,total,selected,labels,r->scroll_colour,&ops);
        r->partial=0; r->mute=0;
        if(failed) return -1;
        if(r->partial_failed || r->surface->error) {
            /* Pixels outside the damage are untouched; redraw everything. */
            menu->previous=previous; r->surface->error=0;
        } else partial=2;
    }
    if(partial!=2 && slicks_draw_track_menu(menu,total,selected,labels,r->scroll_colour,&ops)) return -1;
    if(r->surface->error) { r->drawn_valid=0; return r->surface->error; }
    if(menu) {
        r->drawn=*menu; r->drawn_total=total; r->drawn_count=selected->count;
        r->drawn_valid=(unsigned char)(selected->count<=SLICKS_TRACK_PARTIAL_TRACKS);
        for(unsigned i=0;r->drawn_valid && i<selected->count;++i) r->drawn_tracks[i]=selected->tracks[i];
        r->drawn_serial=r->serial;
    }
    return 0;
}
#endif
