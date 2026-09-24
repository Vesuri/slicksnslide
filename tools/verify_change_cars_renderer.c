#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/ui/change_cars_renderer.h"
static unsigned dirty_calls;
static void dirty(void *p,short l,short t,short r,short b)
{ (void)p; if(l<0 || t<0 || r>320 || b>200 || l>=r || t>=b) abort(); ++dirty_calls; }
static void text(void *p,struct SlicksChunkyUi *ui,unsigned char *font,
    const unsigned char *value,short x,short y,unsigned char flags)
{
    (void)p;
    if(strcmp((const char *)value,"CARS") || flags) abort();
    slicks_ui_rectangle(ui,x,y,x+16,y+3,font[6]);
}
static void failing_text(void *p,struct SlicksChunkyUi *ui,unsigned char *font,
    const unsigned char *value,short x,short y,unsigned char flags)
{ text(p,ui,font,value,x,y,flags); ((struct SlicksPlayerMenuRenderer *)p)->error=-1; }
static void icon(void *p,struct SlicksChunkyUi *ui,const struct SlicksMenuIcon *i,short x,short y)
{ (void)p; slicks_ui_rectangle(ui,x,y,x+i->width,y+i->height,*i->pixels); }
int main(void)
{
    static unsigned char pixels[64000],baseline[64000],palette[768],original[4000],decorated[4000];
    for(unsigned i=0;i<64000;++i) baseline[i]=(unsigned char)(i*17+i/320);
    for(unsigned i=0;i<768;++i) palette[i]=(unsigned char)((i*13)%64);
    unsigned char font[8]={0},dot=99;
    struct SlicksMenuIcon icons[11];
    for(unsigned i=0;i<11;++i) icons[i]=(struct SlicksMenuIcon){&dot,8,6};
    struct SlicksPlayerMenuRenderer surface={0};
    surface.ui=(struct SlicksChunkyUi){pixels,palette,dirty,NULL}; surface.fonts[0]=font;
    surface.icons=icons; surface.icon_count=11; surface.text=text; surface.icon=icon; surface.context=&surface;
    unsigned cases=0;
    for(unsigned mask=1;mask<16;++mask) for(unsigned repeat=0;repeat<3;++repeat) {
        signed char roles[4],vehicles[4]={0,3,6,9}; unsigned count=0;
        for(unsigned i=0;i<4;++i) { roles[i]=(mask&(1U<<i))?(i&1?-1:1):0; if(roles[i]) ++count; }
        short x=repeat==0?210:repeat==1?0:280,y=repeat==0?71:repeat==1?0:100;
        memcpy(pixels,baseline,sizeof pixels); font[6]=77; dirty_calls=0;
        struct SlicksChangeCarsRenderer r={0}; r.surface=&surface;
        struct SlicksChangeCarsDialog d={0};
        if(slicks_change_cars_renderer_open(&r,&d,x,y,count,(const unsigned char *)"CARS",
            original,sizeof original,decorated,sizeof decorated) || !r.active) return 1;
        for(unsigned row=0;row<count;++row) {
            d.row=(signed char)row;
            if(slicks_change_cars_renderer_draw(&r,&d,roles,vehicles)) return 1;
            if(slicks_change_cars_key(&d,roles,vehicles,10,77)) return 1;
            if(slicks_change_cars_renderer_draw(&r,&d,roles,vehicles)) return 1;
        }
        if(slicks_change_cars_renderer_close(&r) || r.active || font[6]!=77 ||
           memcmp(pixels,baseline,sizeof pixels) || !dirty_calls) return 1;
        if(slicks_change_cars_renderer_close(&r)!=-1) return 1;
        surface.text=failing_text;
        if(slicks_change_cars_renderer_open(&r,&d,x,y,count,(const unsigned char *)"CARS",
            original,sizeof original,decorated,sizeof decorated)!=-1 || r.active ||
            font[6]!=77 || memcmp(pixels,baseline,sizeof pixels)) return 1;
        surface.text=text;
        if(slicks_change_cars_renderer_open(&r,&d,x,y,count,(const unsigned char *)"CARS",
            original,3999,decorated,sizeof decorated)!=-1 || r.active ||
            memcmp(pixels,baseline,sizeof pixels)) return 1;
        ++cases;
    }
    printf("Change Cars renderer: %u open/edit/restore/reopen-failure lifetimes pass (stub text/icons; not a pixel oracle)\n",cases);
    return 0;
}
