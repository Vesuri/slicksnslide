#ifndef SLICKS_HELP_VIEWER_H
#define SLICKS_HELP_VIEWER_H
#include "help_renderer.h"
#include "help_navigation.h"
struct SlicksHelpViewer {
    struct SlicksHelpRenderer renderer;
    struct SlicksHelpNavigation navigation;
    struct SlicksHelpIndexInfo info;
    unsigned char source[16384],index[8192],chapter[16384],header_storage[20][200];
    const unsigned char *headers[20];
    unsigned source_size,chapter_length;
    unsigned long loaded_chapter;
    short drawn_page,drawn_selection;
};
/* Original 32a35..32abc: reload only on chapter change; a new page/chapter
 * takes the configured initial link, whereas a link-only redraw preserves it. */
static inline int slicks_help_viewer_refresh(struct SlicksHelpViewer *v)
{
    struct SlicksHelpNavigation *n=&v->navigation; struct SlicksHelpStyle *s=&v->renderer.style;
    if(n->chapter!=v->loaded_chapter) {
        if(slicks_help_load_chapter(v->source,v->source_size,n->chapter,v->chapter,sizeof v->chapter,&v->chapter_length)) return -1;
        v->loaded_chapter=n->chapter; n->redraw=-1;
    }
    /* At most one corrective redraw after the original selection clamp.
     * The real help headers provide a Contents link even on empty pages. */
    for(unsigned pass=0;pass<2;++pass) {
        if(v->drawn_page!=n->page || v->drawn_selection!=s->selected || n->redraw) {
            if(v->drawn_page!=n->page || n->redraw<0) s->selected=(short)((signed char)s->link_mode-1);
            if(slicks_help_renderer_page(&v->renderer,v->headers,v->info.headers,v->chapter,v->chapter_length,
                (unsigned short)n->page,&n->next_page)) return -1;
            n->redraw=0;
            if(s->selected>=s->total_links) { s->selected=0; n->redraw=1; }
            v->drawn_selection=s->selected; v->drawn_page=n->page;
        }
        slicks_help_renderer_arrows(&v->renderer,n->page,n->next_page);
        if(!n->redraw) return 0;
    }
    return -1;
}
/* Backing storage belongs to the caller and must outlive the viewer. */
static inline int slicks_help_viewer_open(struct SlicksHelpViewer *v,const unsigned char *topic,short country,
    unsigned char *saved,unsigned capacity)
{
    if(!v || !topic || v->source_size>sizeof v->source ||
       slicks_help_build_index(v->source,v->source_size,v->index,sizeof v->index,&v->info)) return -1;
    for(unsigned h=0;h<v->info.headers;++h) {
        unsigned length=v->info.header_length[h]; if(length>=200) return -1;
        for(unsigned i=0;i<length;++i) v->header_storage[h][i]=v->source[v->info.header_start[h]+i];
        v->header_storage[h][length]=0;
        if(slicks_help_preprocess(v->header_storage[h],200)) return -1;
        v->headers[h]=v->header_storage[h];
    }
    if(slicks_help_renderer_open(&v->renderer,saved,capacity,country)) return -1;
    slicks_help_navigation_init(&v->navigation);
    v->chapter[0]=0;
    /* The original first draws only headers, establishing language/options
     * before it resolves the requested topic. This is not shown separately. */
    if(slicks_help_renderer_page(&v->renderer,v->headers,v->info.headers,v->chapter,0,0,&v->navigation.next_page)) return -1;
    v->navigation.chapter=v->info.body;
    (void)slicks_help_navigate_topic(&v->navigation,&v->renderer.style,v->index,v->info.size,topic);
    v->renderer.style.selected=0; v->renderer.style.target[0]=0;
    v->loaded_chapter=~0UL; v->drawn_page=v->drawn_selection=-1;
    return slicks_help_viewer_refresh(v);
}
static inline int slicks_help_viewer_key(struct SlicksHelpViewer *v,unsigned char ascii,unsigned char scan)
{
    if(!v || !v->renderer.active || v->navigation.done) return -1;
    if(slicks_help_navigation_key(&v->navigation,&v->renderer.style,v->index,v->info.size,v->info.body,ascii,scan)) return -1;
    return v->navigation.done?0:slicks_help_viewer_refresh(v);
}
#endif
