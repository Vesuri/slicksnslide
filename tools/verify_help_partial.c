/* Link-only Help redraw: drive two viewers over the real HELP.TXT with the
 * same keys. One uses the production partial redraw, the other the original
 * full-page redraw. Pixels, formatting and navigation state must match after
 * every key. The painter is a deterministic stand-in; the real font path has
 * its own differential gate (verify-help-pixels, verify-font-glyph). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "host_archive.h"
#include "../src/ui/help_viewer.h"

static short measure(void *context,const unsigned char *font,const unsigned char *s,signed char spacing)
{ (void)context; (void)font; return (short)(strlen((const char *)s)*(6+spacing)); }
static short text(void *context,struct SlicksChunkyUi *ui,unsigned char *font,
    const unsigned char *s,short x,short y,signed char spacing)
{
    (void)context; short start=x;
    for(unsigned i=0;s[i];++i,x=(short)(x+6+spacing))
        slicks_ui_rectangle(ui,x,y,(short)(x+1+s[i]%5),(short)(y+font[2]-s[i]%3),(unsigned char)(font[6]^s[i]));
    return (short)(x-start);
}
struct Side {
    struct SlicksHelpViewer viewer;
    unsigned char pixels[64000],saved[64000],font[16];
};
static int open_side(struct Side *side,const unsigned char *help,unsigned size,const unsigned char *palette,
    const unsigned char *topic,short country,unsigned char no_partial)
{
    struct SlicksHelpViewer *v=&side->viewer;
    for(unsigned p=0;p<64000;++p) side->pixels[p]=(unsigned char)(p*7+p/320);
    memset(side->font,0,sizeof side->font); side->font[2]=7;
    memset(v,0,sizeof *v); memcpy(v->source,help,size); v->source_size=size; v->no_partial=no_partial;
    v->renderer.ui=(struct SlicksChunkyUi){side->pixels,palette,0,0};
    v->renderer.font=side->font; v->renderer.measure=measure; v->renderer.text=text;
    return slicks_help_viewer_open(v,topic,country,side->saved,sizeof side->saved);
}
static int same(const struct Side *a,const struct Side *b)
{
    const struct SlicksHelpViewer *x=&a->viewer,*y=&b->viewer;
    return !memcmp(a->pixels,b->pixels,64000) && !memcmp(&x->renderer.style,&y->renderer.style,sizeof x->renderer.style) &&
        !memcmp(&x->navigation,&y->navigation,sizeof x->navigation) && a->font[6]==b->font[6] &&
        x->drawn_page==y->drawn_page && x->drawn_selection==y->drawn_selection && x->loaded_chapter==y->loaded_chapter;
}
int main(void)
{
    static unsigned char help[16384],palette[768];
    static struct Side partial,full;
    long size=host_archive_load("ref/SLICKS.000","HELP.TXT",help,sizeof help); if(size<=0) return 2;
    for(unsigned i=0;i<768;++i) palette[i]=(unsigned char)(i*13%64);
    static const struct { unsigned char ascii,scan; } keys[]={
        {0,0x50},{0,0x48},{9,0},{'K',0},{0,0x4d},{0,0x51},{0,0x49},{13,0},{32,0},{8,0},{'b',0},{0,0x3b}};
    const char *topics[]={"","options","players","tracks","reg","FI_OPTIONS","main"};
    unsigned long seed=12345,cases=0,partials=0;
    for(unsigned t=0;t<7;++t) for(unsigned c=0;c<2;++c) {
        short country=c?358:0;
        if(open_side(&partial,help,(unsigned)size,palette,(const unsigned char *)topics[t],country,0) ||
           open_side(&full,help,(unsigned)size,palette,(const unsigned char *)topics[t],country,1)) abort();
        if(!same(&partial,&full)) { fprintf(stderr,"Help partial mismatch at open topic=%s\n",topics[t]); return 1; }
        for(unsigned step=0;step<4000;++step) {
            /* Favour link movement, the case the partial redraw serves. */
            seed=seed*1103515245UL+12345UL; unsigned pick=(unsigned)(seed>>16)%24;
            unsigned k=pick<12?pick%5:pick-12;
            short before=partial.viewer.drawn_selection,page=partial.viewer.drawn_page;
            int a=slicks_help_viewer_key(&partial.viewer,keys[k].ascii,keys[k].scan);
            int b=slicks_help_viewer_key(&full.viewer,keys[k].ascii,keys[k].scan);
            if(a!=b || a) { fprintf(stderr,"Help key result %d/%d topic=%s step=%u\n",a,b,topics[t],step); return 1; }
            if(partial.viewer.drawn_page==page && partial.viewer.drawn_selection!=before) ++partials;
            if(!same(&partial,&full)) {
                fprintf(stderr,"Help partial mismatch topic=%s country=%d step=%u key=%u page=%d selected=%d\n",
                    topics[t],country,step,k,partial.viewer.drawn_page,partial.viewer.drawn_selection);
                return 1;
            }
            ++cases;
        }
        if(slicks_help_renderer_close(&partial.viewer.renderer) || slicks_help_renderer_close(&full.viewer.renderer) ||
           memcmp(partial.pixels,full.pixels,64000)) abort();
    }
    if(partials<1000) { fprintf(stderr,"Help partial: only %lu link-only redraws exercised\n",partials); return 1; }
    printf("Help link-only redraw: %lu keys (%lu link-only redraws) match full-page redraw pixels and state\n",cases,partials);
    return 0;
}
