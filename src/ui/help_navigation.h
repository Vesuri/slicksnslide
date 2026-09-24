#ifndef SLICKS_HELP_NAVIGATION_H
#define SLICKS_HELP_NAVIGATION_H
#include "help_line.h"
struct SlicksHelpNavigation {
    unsigned long chapter;
    short page,next_page;
    signed char redraw;
    unsigned char done;
    /* Original history is 101 slots; page/selection values are signed bytes. */
    unsigned long chapters[101];
    signed char pages[101],selections[101];
};
static inline void slicks_help_navigation_init(struct SlicksHelpNavigation *n)
{
    *n=(struct SlicksHelpNavigation){0};
    for(unsigned i=0;i<101;++i) n->selections[i]=-1;
    n->redraw=1;
}
static inline int slicks_help_navigate_topic(struct SlicksHelpNavigation *n,struct SlicksHelpStyle *s,
    const unsigned char *index,unsigned size,const unsigned char *topic)
{
    struct SlicksHelpLocation found={n->chapter,(unsigned short)n->page,(unsigned short)s->selected};
    int result=slicks_help_find(index,size,s->prefix,topic,&found);
    if(!result) { n->chapter=found.chapter; n->page=(short)found.page; s->selected=(short)found.anchor; }
    return result;
}
/* Original 32b81..32e20, after the DOS getch pair. Preserve its exact keys:
 * uppercase ASCII K moves up; scan-code Left is not treated as Up. */
static inline int slicks_help_navigation_key(struct SlicksHelpNavigation *n,struct SlicksHelpStyle *s,
    const unsigned char *index,unsigned size,unsigned long body,unsigned char ascii,unsigned char scan)
{
    if(!n || !s || !slicks_help_index_valid(index,size)) return -1;
    if(scan==0x44 || ascii==27) n->done=0xe5;
    if(scan==0x3b) { n->chapter=body; n->page=0; s->selected=0; }
    if(scan==0x49) {
        if(n->page>0) { s->selected=0; --n->page; }
        else if(s->previous[0]) (void)slicks_help_navigate_topic(n,s,index,size,s->previous);
    }
    if(scan==0x51) {
        if(n->page<n->next_page) { s->selected=0; ++n->page; }
        else if(s->next[0]) (void)slicks_help_navigate_topic(n,s,index,size,s->next);
    }
    if((scan==0x48 || ascii=='K') && s->total_links>1) {
        --s->selected; if(s->selected<0) s->selected=(short)(s->total_links-1);
    }
    if((scan==0x50 || scan==0x4d || ascii==9) && s->total_links>1) {
        ++s->selected; if(s->selected>=s->total_links) s->selected=0;
    }
    if((ascii==8 || ascii=='b') && n->selections[0]>=0) {
        n->chapter=n->chapters[0]; n->page=n->pages[0]; s->selected=n->selections[0];
        for(unsigned i=0;i<100;++i) {
            n->chapters[i]=n->chapters[i+1]; n->pages[i]=n->pages[i+1]; n->selections[i]=n->selections[i+1];
        }
        n->selections[100]=-1;
    }
    if((ascii==13 || ascii==32) && s->total_links>0) {
        n->redraw=1;
        /* $ commands and inline-information links do not navigate here. */
        if(s->target[0]=='$' || !s->link_type) return 0;
        for(unsigned i=100;i>0;--i) {
            n->chapters[i]=n->chapters[i-1]; n->pages[i]=n->pages[i-1]; n->selections[i]=n->selections[i-1];
        }
        n->chapters[0]=n->chapter; n->pages[0]=(signed char)n->page; n->selections[0]=(signed char)s->selected;
        (void)slicks_help_navigate_topic(n,s,index,size,s->target);
    }
    return 0;
}
#endif
