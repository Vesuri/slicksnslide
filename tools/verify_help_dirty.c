#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/ui/help_renderer.h"
static unsigned calls;
static void report(void *context,short l,short t,short r,short b)
{ assert(context==&calls && l<r && t<b); ++calls; }
static short measure(void *context,const unsigned char *f,const unsigned char *s,signed char spacing)
{ (void)context;(void)f;(void)s;(void)spacing;return 1; }
static short text(void *context,struct SlicksChunkyUi *ui,unsigned char *f,
    const unsigned char *s,short x,short y,signed char spacing)
{ (void)context;(void)f;(void)s;(void)spacing;slicks_ui_rectangle(ui,x,y,x+1,y+1,9);return 1; }
int main(void)
{
    static unsigned char pixels[64000],base[64000],saved[64000],palette[768],font[16];
    for(unsigned p=0;p<64000;++p) base[p]=(unsigned char)(p*17+(p/320));
    struct SlicksHelpRenderer r={0};
    r.ui=(struct SlicksChunkyUi){pixels,palette,report,&calls};
    r.font=font;r.text=text;r.measure=measure;font[6]=71;
    for(unsigned round=0;round<20;++round) {
        memcpy(pixels,base,sizeof pixels);
        assert(!slicks_help_renderer_open(&r,saved,sizeof saved,0));
        assert(!r.painted_count);
        /* Two separate pages, then enough disjoint spans to overflow the
         * fixed list. Publication clearing must not clear lifetime bounds. */
        for(unsigned i=0;i<round;++i) {
            short y=(short)(10+7*i);
            slicks_ui_rectangle(&r.ui,33,y,46,y+1,200);
        }
        if(round) assert(r.painted[0].left==32 && r.painted[0].right==48);
        unsigned before=calls;
        assert(!slicks_help_renderer_close(&r));
        assert(!memcmp(pixels,base,sizeof pixels));
        assert(calls-before==r.painted_count && font[6]==71 && !r.active);
        assert(r.ui.dirty==report && r.ui.dirty_context==&calls);
    }
    puts("Help lifetime dirty bounds: empty, disjoint pages, overflow, exact full restore and callback/font reuse pass");
}
