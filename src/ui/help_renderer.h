#ifndef SLICKS_HELP_RENDERER_H
#define SLICKS_HELP_RENDERER_H
#include "help_line.h"
#include "chunky_ui.h"
#include "saved_rectangle.h"
#include "menu_dirty.h"

struct SlicksHelpRenderer {
    struct SlicksChunkyUi ui;
    struct SlicksHelpStyle style;
    unsigned char *font;
    short (*measure)(void *,const unsigned char *,const unsigned char *,signed char);
    short (*text)(void *,struct SlicksChunkyUi *,unsigned char *,const unsigned char *,short,short,signed char);
    void *context;
    struct SlicksSavedRectangle saved;
    struct SlicksMenuRect painted[16];
    unsigned short painted_count;
    void (*parent_dirty)(void *,short,short,short,short);
    void *parent_dirty_context;
    unsigned char old_colour,active;
};
static inline void slicks_help_renderer_dirty(void *context,short l,short t,short r,short b)
{
    struct SlicksHelpRenderer *renderer=context;
    slicks_menu_dirty_add(renderer->painted,&renderer->painted_count,l,t,r,b);
    if(renderer->parent_dirty)
        renderer->parent_dirty(renderer->parent_dirty_context,l,t,r,b);
}
/* The original viewer saves the whole visible page before its first draw
 * and restores it and the font colour on exit. Storage is allocated while
 * AmigaOS is available; these operations perform no allocation or OS calls. */
static inline int slicks_help_renderer_open(struct SlicksHelpRenderer *r,
    unsigned char *saved,unsigned long capacity,short country)
{
    if(!r || r->active || !r->font || !r->ui.pixels || !r->ui.palette ||
       !r->measure || !r->text || !saved || saved==r->ui.pixels || capacity<64000) return -1;
    if(slicks_save_rectangle(&r->saved,saved,capacity,&r->ui,0,0,320,200)) return -1;
    r->painted_count=0;
    r->parent_dirty=r->ui.dirty; r->parent_dirty_context=r->ui.dirty_context;
    r->ui.dirty=slicks_help_renderer_dirty; r->ui.dirty_context=r;
    r->old_colour=r->font[6]; r->font[6]=1;
    r->style=(struct SlicksHelpStyle){0};
    r->style.left=20; r->style.top=15; r->style.right=300; r->style.bottom=185;
    r->style.height=r->font[2]; r->style.distance=2; r->style.spacing=1;
    r->style.link_mode=1; r->style.country=country;
    const unsigned char rgb[5][3]={{10,10,30},{45,45,63},{60,60,20},{30,30,50},{40,50,40}};
    for(unsigned i=0;i<5;++i)
        r->style.colours[i]=slicks_ui_nearest(&r->ui,rgb[i][0],rgb[i][1],rgb[i][2]);
    r->active=1; return 0;
}
static inline int slicks_help_renderer_close(struct SlicksHelpRenderer *r)
{
    if(!r || !r->active) return -1;
    /* Keep the original full snapshot, but restore/publish only blocks this
     * modal painted during its entire lifetime, including previous pages. */
    r->ui.dirty=r->parent_dirty; r->ui.dirty_context=r->parent_dirty_context;
    for(unsigned i=0;i<r->painted_count;++i) {
        const struct SlicksMenuRect *p=&r->painted[i];
        if(slicks_restore_rectangle(&r->ui,&r->saved,0,0,p->left,p->top,
            p->right-p->left,p->bottom-p->top)) return -1;
    }
    r->font[6]=r->old_colour; r->active=0; return 0;
}
static inline void slicks_help_renderer_colour(void *context,unsigned char colour)
{ struct SlicksHelpRenderer *r=context; r->font[6]=colour; }
static inline unsigned char slicks_help_renderer_nearest(void *context,unsigned char red,unsigned char green,unsigned char blue)
{ struct SlicksHelpRenderer *r=context; return slicks_ui_nearest(&r->ui,red,green,blue); }
static inline void slicks_help_renderer_rectangle(void *context,short left,short top,short right,short bottom,unsigned char colour)
{ struct SlicksHelpRenderer *r=context; slicks_ui_rectangle(&r->ui,left,top,right,bottom,colour); }
static inline short slicks_help_renderer_measure(void *context,const unsigned char *text,signed char spacing)
{ struct SlicksHelpRenderer *r=context; return r->measure(r->context,r->font,text,spacing); }
static inline short slicks_help_renderer_text(void *context,const unsigned char *text,short x,short y,signed char spacing)
{ struct SlicksHelpRenderer *r=context; return r->text(r->context,&r->ui,r->font,text,x,y,spacing); }
static inline int slicks_help_renderer_line(struct SlicksHelpRenderer *r,const unsigned char *line,short *y)
{
    if(!r || !r->font || !r->ui.pixels || !r->ui.palette || !r->measure || !r->text) return -1;
    const struct SlicksHelpDrawOps ops={r,slicks_help_renderer_colour,slicks_help_renderer_nearest,
        slicks_help_renderer_rectangle,slicks_help_renderer_measure,slicks_help_renderer_text};
    r->style.height=r->font[2];
    return slicks_help_draw_line(&r->style,line,y,&ops);
}
/* Original 32abc..32b5d. Arrow glyphs use the fifth default palette query. */
static inline void slicks_help_renderer_arrows(struct SlicksHelpRenderer *r,short page,short next_page)
{
    struct SlicksHelpStyle *s=&r->style; r->font[6]=s->colours[4];
    short x=(short)(s->right-r->font[1]);
    if(s->previous[0] || page) {
        const unsigned char up[]={24,0};
        (void)r->text(r->context,&r->ui,r->font,up,x,(short)(s->top+1),s->spacing);
    }
    if(s->next[0] || page<next_page) {
        const unsigned char down[]={25,0};
        (void)r->text(r->context,&r->ui,r->font,down,x,(short)(s->bottom-r->font[2]-1),s->spacing);
    }
}

/* Original 32165..32301. Headers and chapter are already preprocessed.
 * Page selection counts explicit form feeds, not a reflowed host layout.
 * next_page is the original 6ff2 in/out value; caller clears it before draw. */
static inline int slicks_help_renderer_page(struct SlicksHelpRenderer *r,
    const unsigned char *const *headers,unsigned header_count,
    const unsigned char *chapter,unsigned length,unsigned page,short *next_page)
{
    if(!r || !r->font || !r->ui.pixels || !r->ui.palette || !r->measure || !r->text ||
       !chapter || !next_page || page>32767 || header_count>20 || (header_count && !headers)) return -1;
    for(unsigned h=0;h<header_count;++h) {
        if(!headers[h]) return -1;
        unsigned n=0; while(n<200 && headers[h][n]) ++n;
        if(n==200) return -1;
    }
    /* The DOS scratch line is limited to 200 bytes. Reject a missing line
     * boundary instead of reproducing its unterminated stack-buffer read. */
    unsigned run=0;
    for(unsigned i=0;i<length;++i) {
        if(!chapter[i]) return -1;
        if(chapter[i]==13 || chapter[i]==12) run=0;
        else if(++run>=200) return -1;
    }
    struct SlicksHelpStyle *s=&r->style;
    s->next[0]=s->previous[0]=0; s->centred=0; s->total_links=s->links=0;
    short y=(short)(s->top+4);
    slicks_ui_rectangle(&r->ui,s->left,s->top,s->right,y,s->colours[0]);
    for(unsigned h=0;h<header_count;++h)
        if(slicks_help_renderer_line(r,headers[h],&y)) return -1;
    unsigned at=0,current_page=0;
    while(at<length && current_page!=page) { if(chapter[at]==12) ++current_page; ++at; }
    for(;;) {
        unsigned char line[201]={0}; unsigned n=0;
        for(;;) {
            unsigned char c=at<length?chapter[at]:0; ++at;
            line[n]=c;
            if(c==13 || (n && line[n-1]==12)) { line[n]=0; break; }
            if(++n==200) break;
        }
        if(slicks_help_renderer_line(r,line,&y)) return -1;
        if(y>(short)(s->bottom-4) || at>=length || line[0]==12) {
            if(at<length) *next_page=(short)(page+1);
            break;
        }
    }
    slicks_ui_rectangle(&r->ui,s->left,y,s->right,(short)(s->bottom+4),s->colours[0]);
    return 0;
}
#endif
