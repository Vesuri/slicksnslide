#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/ui/intermission_renderer.h"
struct Test { unsigned previews,markers,cars,texts; int fail_preview; };
static int text(void *p,struct SlicksChunkyUi *ui,unsigned char *font,const unsigned char *s,
    short x,short y,unsigned char flags,unsigned char highlight)
{
    struct Test *t=p; ++t->texts; if(!s || !font || (flags&4 && !highlight)) abort();
    slicks_ui_rectangle(ui,x,y,x+3,y+3,font[6]); return 0;
}
static int icon(void *p,struct SlicksChunkyUi *ui,short id,short x,short y)
{
    struct Test *t=p; if(id==11) ++t->markers; else if(id>=1 && id<=10) ++t->cars; else abort();
    slicks_ui_rectangle(ui,x,y,x+2,y+2,(unsigned char)id); return 0;
}
static int preview(void *p,struct SlicksChunkyUi *ui,short x,short y)
{
    struct Test *t=p; if(x!=25 || y!=35 || t->cars) abort(); ++t->previews;
    if(t->fail_preview) return -1;
    slicks_ui_rectangle(ui,x,y,x+65,y+40,21); return 0;
}
int main(void)
{
    static unsigned char pixels[64000],drawn[64000],palette[768],buttons[3024],cars[320];
    unsigned char font[8]={0};
    for(unsigned i=0;i<768;++i) palette[i]=(unsigned char)((i*13)%64);
    unsigned cases=0;
    for(unsigned mask=1;mask<16;++mask) for(unsigned failure=0;failure<2;++failure) {
        for(unsigned i=0;i<64000;++i) pixels[i]=(unsigned char)(i*19+i/320);
        struct Test test={0}; test.fail_preview=(int)failure;
        struct SlicksRecordsRenderer surface={.ui={pixels,palette,0,0},.fonts={font,0},.text=text,.icon=icon,.context=&test};
        struct SlicksIntermissionRenderer r={.surface=&surface,.fastest_icon=11};
        struct SlicksIntermissionMenu m;
        struct SlicksIntermissionContent c={.labels={(const unsigned char *)"CHANGE CARS",(const unsigned char *)"SAVE GAME",
            (const unsigned char *)"NEXT TRACK",(const unsigned char *)"END MATCH"},
            .track_name=(const unsigned char *)"BASIC",.slash=(const unsigned char *)"/",.track_total=2,.fastest=100};
        unsigned active=0;
        for(unsigned i=0;i<4;++i) {
            c.roles[i]=(mask&(1U<<i))?(i&1?-1:1):0; active+=c.roles[i]!=0;
            c.vehicles[i]=(signed char)(i*3); c.names[i]=(const unsigned char *)"DRIVER";
            c.points[i]=(short)(i*7); c.laps[i]=100;
        }
        int result=slicks_intermission_renderer_open(&r,&m,&c,palette,buttons,sizeof buttons,cars,sizeof cars,preview,&test);
        if(test.previews!=1 || test.markers!=active || (failure?(result!=-1 || r.active):(result || !r.active))) return 1;
        if(!failure) {
            if(test.cars!=active || m.redraw || m.cars_redraw || m.selected!=2) return 1;
            memcpy(drawn,pixels,sizeof drawn);
            m.redraw=-1; m.cars_redraw=-1;
            if(slicks_intermission_renderer_draw(&r,&m,&c) || memcmp(drawn,pixels,sizeof drawn)) return 1;
            if(slicks_intermission_key(&m,80)!=SLICKS_INTERMISSION_NONE || m.selected!=3 ||
                slicks_intermission_renderer_draw(&r,&m,&c)) return 1;
            if(slicks_intermission_key(&m,60)!=SLICKS_INTERMISSION_CHANGE_CARS || m.selected!=0) return 1;
            c.vehicles[0]=1;
            if(slicks_intermission_renderer_draw(&r,&m,&c)) return 1;
        }
        ++cases;
    }
    printf("Intermission renderer: %u composition/redraw/preview-failure cases pass (stub assets; not a pixel oracle)\n",cases);
    return 0;
}
