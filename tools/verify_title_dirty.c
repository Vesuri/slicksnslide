#include <stdio.h>
#include <string.h>
#include "../src/ui/title_dirty.h"
int main(void)
{
    struct SlicksTitleDirty d={0};
    slicks_title_dirty_add(&d,200,96,244,140);
    slicks_title_dirty_add(&d,108,77,208,174);
    if(d.count!=1 || d.rects[0].left!=96 || d.rects[0].right!=256 ||
       d.rects[0].top!=77 || d.rects[0].bottom!=174) return 1;
    slicks_title_dirty_add(&d,0,190,320,200);
    if(d.count!=2) return 1;
    static unsigned char logical[262144],chunky[64000];
    for(unsigned i=0;i<sizeof logical;++i) logical[i]=(unsigned char)(i*37+(i>>16));
    memset(chunky,0xa5,sizeof chunky);
    slicks_title_dirty_unpack(&d,logical,chunky);
    for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;++x) {
        unsigned hit=(x>=96 && x<256 && y>=77 && y<174)||y>=190;
        unsigned expected=hit?logical[(x&3)*65536UL+y*100+(x>>2)]:0xa5;
        if(chunky[y*320+x]!=expected) return 1;
    }
    slicks_title_dirty_add(&d,0,0,320,200);
    if(d.count!=1 || d.rects[0].left || d.rects[0].top ||
       d.rects[0].right!=320 || d.rects[0].bottom!=200) return 1;
    d.count=0;
    for(unsigned i=0;i<5;++i) slicks_title_dirty_add(&d,i*32,0,i*32+16,1);
    if(d.count!=1 || d.rects[0].right!=320 || d.rects[0].bottom!=200) return 1;
    d.count=0;
    slicks_title_dirty_add(&d,320,0,400,10);slicks_title_dirty_add(&d,0,200,10,201);
    if(d.count) return 1;
    puts("Title dirty bounds: merge, alignment, full invalidation, overflow, clipping and 64000 unpack/untouched pixels pass");
    return 0;
}
