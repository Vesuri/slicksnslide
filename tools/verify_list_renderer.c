#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/ui/list_renderer.h"
#define abort() do { fprintf(stderr,"List renderer assertion at line %d\n",__LINE__); exit(1); } while(0)

/* Composition/lifetime checks only, not an original-DOS pixel oracle.
 * Synthetic font callbacks make every draw observable while checking bounds. */
struct Fixture { unsigned texts,dirty; };
static short measure(void *p,const unsigned char *font,const unsigned char *s)
{ (void)p; (void)font; return (short)(strlen((const char *)s)*4); }
static void dirty(void *p,short l,short t,short r,short b)
{ struct Fixture *f=p; if(l<0 || t<0 || r>320 || b>200 || l>=r || t>=b) abort(); ++f->dirty; }
static void text(void *p,struct SlicksChunkyUi *ui,unsigned char *font,const unsigned char *s,short x,short y,unsigned char flags)
{
    struct Fixture *f=p; short w=measure(p,font,s); if(flags==2) x-=w;
    if(x<0 || x+w>320 || y<0 || y+font[2]>200) abort();
    if(w) slicks_ui_rectangle(ui,x,y,x+w,y+font[2],font[6]);
    ++f->texts;
}
int main(void)
{
    unsigned char screen[64000],before[64000],palette[768],font[8]={0},names[2849][21];
    unsigned char original[64000],tinted[64000],caption[64000];
    for(unsigned i=0;i<768;++i) palette[i]=(unsigned char)((i*17+i/5)%64);
    for(unsigned i=0;i<2849;++i) snprintf((char *)names[i],21,"PLAYER %u",i);
    const unsigned char *labels[]={ (const unsigned char *)"SELECT",(const unsigned char *)"MODIFY,REMOVE,CANCEL" };
    unsigned cases=0;
    const unsigned counts[]={3,100,101,127,128,255,256,512,1000,1560,2849};
    for(unsigned c=0;c<sizeof counts/sizeof counts[0];++c) for(unsigned row=0;row<4;++row)
    for(unsigned kind=0;kind<2;++kind) {
        unsigned count=counts[c];
        struct Fixture f={0}; font[2]=6; font[6]=71;
        for(unsigned i=0;i<64000;++i) screen[i]=before[i]=(unsigned char)(i*31+i/320);
        struct SlicksListRenderer r={0}; r.ui=(struct SlicksChunkyUi){screen,palette,dirty,&f};
        r.font=font; r.names=&names[0][0]; r.stride=21;
        r.left=160; r.top=(short)(30+row*16); r.right=310; r.bottom=r.top+100;
        r.measure=measure; r.text=text; r.context=&f;
        unsigned char scrollbar[800]; struct SlicksSavedRectangle saved_scrollbar;
        if(slicks_list_save_scrollbar(&r,&saved_scrollbar,scrollbar,sizeof scrollbar)) abort();
        if(slicks_list_renderer_open(&r,1,(short)count,labels[kind],66,1,
            original,sizeof original,tinted,sizeof tinted,caption,sizeof caption)) abort();
        const unsigned char keys[]={0x50,0x51,0x4f,0x48,0x49,0x47,0x4b,0x0f,0x4d};
        slicks_list_renderer_draw(&r,1);
        for(unsigned k=0;k<sizeof keys;++k) {
            slicks_list_dialog_key(&r.state,keys[k]);
            slicks_list_renderer_draw(&r,k+2);
            slicks_list_renderer_draw(&r,k+2); /* unchanged tick, active repaint */
        }
        slicks_list_dialog_key(&r.state,1);
        short result=slicks_list_renderer_close(&r);
        if(count==3 && (r.scrollbar_top!=r.top || r.scrollbar_bottom!=r.bottom+2)) abort();
        if(slicks_list_restore_scrollbar(&r,&saved_scrollbar)) abort();
        if(result!=1 || r.active || font[6]!=71 ||
           memcmp(screen,before,sizeof screen) || !f.texts || !f.dirty) {
            fprintf(stderr,"count=%u row=%u kind=%u result=%d active=%u colour=%u\n",count,row,kind,result,r.active,font[6]);
            for(unsigned i=0;i<64000;++i) if(screen[i]!=before[i]) { fprintf(stderr,"changed pixel %u,%u\n",i%320,i/320); break; }
            abort();
        }
        /* Insufficient storage must not change any screen/font bytes. */
        if(slicks_list_renderer_open(&r,1,(short)count,labels[kind],66,1,
            original,1,tinted,sizeof tinted,caption,sizeof caption)!=-1 ||
           memcmp(screen,before,sizeof screen) || font[6]!=71 || r.active) abort();
        ++cases;
    }
    printf("List renderer composition: %u open/navigation/pulse/cancel cycles restore all 64000 pixels and font state; capacity failures do not paint\n",cases);
    return 0;
}
