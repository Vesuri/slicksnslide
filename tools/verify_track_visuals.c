#include <stdio.h>
#include <string.h>
#include "../src/game/track_scene.c"
static unsigned char old_planes[0x40000],new_planes[0x40000],lower[60800],upper[60800],pixels[4096];
int main(void)
{
    unsigned random=1234,cases=0;
    const unsigned short positions[]={0,1,3,160,300,319,320,65530};
    for(unsigned rotation=0;rotation<4;++rotation)
    for(unsigned width=1;width<=64;width+=7)
    for(unsigned height=1;height<=64;height+=9)
    for(unsigned where=0;where<8;++where) {
        struct TrackSprite sprite={pixels,width,height};
        for(unsigned i=0;i<width*height;++i) { random=random*1664525U+1; pixels[i]=(random&7)?random>>24:0; }
        memset(old_planes,117,sizeof old_planes); memset(new_planes,117,sizeof new_planes);
        unsigned short x=positions[where],y=where==7?65530:where==6?190:where==5?189:20;
        draw_sprite(old_planes,lower,upper,&sprite,x,y,rotation,0);
        draw_visual(new_planes,&sprite,x,y,rotation);
        if(memcmp(old_planes,new_planes,sizeof old_planes)) {
            fprintf(stderr,"visual mismatch rotation=%u size=%ux%u position=%u,%u\n",rotation,width,height,x,y); return 1;
        }
        ++cases;
    }
    printf("Track visual fast path: %u whole-plane comparisons pass\n",cases);
    return 0;
}
