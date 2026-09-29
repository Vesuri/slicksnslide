#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/ui/menu_dirty.h"
static unsigned seed=12345;
static unsigned random_value(void) { seed=seed*1664525U+1013904223U; return seed; }
int main(void)
{
    struct SlicksMenuRect r[16]; unsigned short count=0;
    slicks_menu_dirty_add(r,&count,17,40,30,48);
    slicks_menu_dirty_add(r,&count,193,40,207,48);
    assert(count==2 && r[0].left==16 && r[0].right==32);
    slicks_menu_dirty_add(r,&count,32,40,192,48);
    assert(count==1 && r[0].left==16 && r[0].right==208);
    count=0;
    for(int i=0;i<17;++i) slicks_menu_dirty_add(r,&count,35,i*3,40,i*3+1);
    assert(count==1 && r[0].left==32 && r[0].right==48 && r[0].bottom==49);
    static unsigned char expected[64000],covered[64000];
    for(unsigned trial=0;trial<1000;++trial) {
        count=0; memset(expected,0,sizeof expected);
        for(unsigned n=0;n<40;++n) {
            int l=(int)(random_value()%440)-60,t=(int)(random_value()%280)-40;
            int right=l+(int)(random_value()%40),bottom=t+(int)(random_value()%12);
            slicks_menu_dirty_add(r,&count,l,t,right,bottom);
            for(int y=0;y<200;++y) for(int x=0;x<320;++x)
                if(x>=l && x<right && y>=t && y<bottom) expected[y*320+x]=1;
            memset(covered,0,sizeof covered);
            assert(count<=16);
            for(unsigned i=0;i<count;++i) {
                assert(r[i].left<r[i].right && r[i].right<=320 && !(r[i].left%16) && !(r[i].right%16));
                assert(r[i].top<r[i].bottom && r[i].bottom<=200);
                for(unsigned y=r[i].top;y<r[i].bottom;++y)
                    for(unsigned x=r[i].left;x<r[i].right;++x) covered[y*320+x]=1;
            }
            for(unsigned p=0;p<64000;++p) assert(!expected[p] || covered[p]);
        }
    }
    puts("Menu rectangles: narrow/disjoint, chained merge, bounded overflow and 40000 clipped coverage steps pass");
}
