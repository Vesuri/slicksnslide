/* Cursor-only Tracks redraw: drive a partial and a full-redraw renderer with
 * the same keys, playlist toggles and foreign repaints. Pixels and menu state
 * must match after every draw. Real fonts supply glyph widths; the painter is
 * a deterministic stand-in (the 68020 text path has its own oracle gates:
 * verify-track-prepare, verify-font-glyph, verify-standings-dirty). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "host_archive.h"
#include "../src/ui/font_resource.h"
#include "../src/ui/track_menu_renderer.h"

static unsigned long reported;
static void report(void *context,short l,short t,short r,short b)
{ (void)context; reported+=(unsigned long)(r-l)*(unsigned long)(b-t); }
static int advance(const unsigned char *font,unsigned c,short x,short anchor,unsigned *glyph)
{
    const unsigned char *codes=font+6+font[5],*widths=codes+font[0];
    unsigned g=0; while(g<font[0] && codes[g]!=c) ++g;
    *glyph=g;
    if(g>0 && g<font[0]) return widths[g]+font[3];
    if(c==8 || c==207) return 10-(short)(x-anchor)%10;
    return font[1];
}
static void text(void *context,struct SlicksChunkyUi *ui,unsigned char *font,
    const unsigned char *s,short x,short y,unsigned char flags)
{
    (void)context; short width=0,anchor=x; unsigned g;
    for(unsigned i=0;s[i];++i) width=(short)(width+advance(font,s[i],(short)(x+width),x,&g));
    if((flags&3)==1) x=(short)(x-width/2);
    else if((flags&3)==2) x=(short)(x-width);
    const unsigned char *widths=font+6+font[5]+font[0];
    for(unsigned i=0;s[i];++i) {
        int step=advance(font,s[i],x,anchor,&g);
        if(g>0 && g<font[0])
            for(int py=0;py<font[2];++py) for(int px=0;px<widths[g];++px)
                if((px+py+s[i])%3) {
                    int ax=x+px,ay=y+py;
                    if(ax>=0 && ax<320 && ay>=0 && ay<200) ui->pixels[ay*320+ax]=(unsigned char)(font[6]^s[i]^px);
                }
        x=(short)(x+step);
    }
    if(ui->dirty) ui->dirty(ui->dirty_context,0,(short)(y<0?0:y),320,(short)(y+font[2]>200?200:y+font[2]));
}
static const unsigned char *name(void *p,unsigned index)
{ (void)p; static unsigned char out[16]; snprintf((char *)out,sizeof out,"TRK%03uWX",index); return out; }

struct Side {
    struct SlicksPlayerMenuRenderer surface;
    struct SlicksTrackRenderer renderer;
    struct SlicksTrackMenu menu;
    short tracks[256];
    struct SlicksTrackPlaylist playlist;
    unsigned char pixels[64000],fonts[3][8192];
};
static void side_open(struct Side *s,const unsigned char *saved,const unsigned char *palette,
    unsigned char fonts[3][8192],short random_count)
{
    memcpy(s->pixels,saved,64000); memcpy(s->fonts,fonts,sizeof s->fonts);
    memset(&s->surface,0,sizeof s->surface);
    s->surface.ui=(struct SlicksChunkyUi){s->pixels,palette,report,0};
    for(unsigned f=0;f<3;++f) s->surface.fonts[f]=s->fonts[f];
    s->surface.saved=(unsigned char *)saved; s->surface.text=text;
    if(slicks_track_renderer_init(&s->renderer,&s->surface,66,name,0)) abort();
    s->menu=(struct SlicksTrackMenu){0,0,-1,random_count,0,0,0};
    s->playlist=(struct SlicksTrackPlaylist){s->tracks,0,256};
}
static int draw(struct Side *s,short total,unsigned char allow,const struct SlicksTrackMenuLabels *labels)
{
    s->renderer.allow_partial=allow;
    return slicks_track_renderer_draw(&s->renderer,&s->menu,total,&s->playlist,labels);
}
int main(void)
{
    static unsigned char resource[16384],saved[64000],palette[768],fonts[3][8192];
    static const char *const font_names[3]={"kirj.@f","iso.@f","pieni.@f"};
    for(unsigned f=0;f<3;++f) {
        long size=host_archive_load("ref/SLICKS.000",font_names[f],resource,sizeof resource);
        if(size<=0 || slicks_decode_font_resource(resource,(unsigned long)size,fonts[f],sizeof fonts[f])<0) return 2;
    }
    for(unsigned i=0;i<64000;++i) saved[i]=(unsigned char)((i*37+i/320*11)%251+1);
    for(unsigned i=0;i<768;++i) palette[i]=(unsigned char)(i*29%64);
    const struct SlicksTrackMenuLabels labels={
        {(const unsigned char *)"TRACK RECORDS",(const unsigned char *)"",(const unsigned char *)"SELECT ALL",
         (const unsigned char *)"CLEAR ALL",(const unsigned char *)"RANDOM",(const unsigned char *)"OK"},
        (const unsigned char *)"RANDOM ORDER: ON",(const unsigned char *)"RANDOM ORDER: OFF",(const unsigned char *)"/"};
    static const unsigned char scans[]={0x50,0x48,0x50,0x48,0x50,0x48,0x49,0x51,0x47,0x4f,0x4b,0x4d,0x1c,0x39};
    static struct Side partial,full;
    const short totals[]={195,23,22,10,1};
    unsigned long seed=4242,draws=0,partials=0;
    for(unsigned t=0;t<sizeof totals/sizeof totals[0];++t) {
        short total=totals[t];
        side_open(&partial,saved,palette,fonts,3); side_open(&full,saved,palette,fonts,3);
        if(draw(&partial,total,0,&labels) || draw(&full,total,0,&labels)) abort();
        for(unsigned step=0;step<6000;++step) {
            seed=seed*1103515245UL+12345UL; unsigned pick=(unsigned)(seed>>16);
            if(pick%97==0) {
                /* A foreign painter must force the next draw to be full. */
                slicks_ui_rectangle(&partial.surface.ui,40,60,90,70,7);
                slicks_ui_rectangle(&full.surface.ui,40,60,90,70,7);
            }
            unsigned char scan=scans[pick%sizeof scans];
            struct Side *sides[2]={&partial,&full};
            for(unsigned i=0;i<2;++i) {
                struct Side *s=sides[i];
                enum SlicksTrackMenuAction action=slicks_track_menu_key(&s->menu,total,scan);
                if(action==SLICKS_TRACK_MENU_TOGGLE && s->menu.cursor>=0)
                    (void)slicks_track_playlist_toggle(&s->playlist,s->menu.cursor,1);
                if(s->menu.done) { s->menu.done=0; s->menu.previous=-1; }
            }
            reported=0;
            if(draw(&partial,total,1,&labels)) abort();
            unsigned long area=reported;
            if(draw(&full,total,0,&labels)) abort();
            if(memcmp(partial.pixels,full.pixels,64000) || memcmp(&partial.menu,&full.menu,sizeof partial.menu)) {
                for(unsigned i=0;i<64000;++i) if(partial.pixels[i]!=full.pixels[i]) {
                    fprintf(stderr,"Tracks partial mismatch total=%d step=%u scan=%02x cursor=%d top=%d column=%u at %u,%u\n",
                        total,step,scan,partial.menu.cursor,partial.menu.top,partial.menu.column,i%320,i/320);
                    return 1;
                }
                fprintf(stderr,"Tracks partial state mismatch total=%d step=%u\n",total,step); return 1;
            }
            ++draws;
            if(area && area<12000) ++partials;
        }
    }
    if(partials<3000) { fprintf(stderr,"Tracks partial: only %lu cursor-only draws exercised\n",partials); return 1; }
    printf("Tracks cursor-only redraw: %lu keys (%lu partial draws) match full redraw pixels and state\n",draws,partials);
    return 0;
}
