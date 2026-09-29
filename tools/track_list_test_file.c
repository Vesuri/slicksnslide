/* Create a valid, nonempty track-list fixture without overwriting settings. */
#include <stdio.h>
#include "../src/game/track_lists.h"
static const unsigned char *name(void *context,unsigned index)
{ (void)context;(void)index;return (const unsigned char *)"BASIC"; }
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    const unsigned char empty[8]={'S','S','T','r','k',26,0,0};
    unsigned char bytes[64];struct SlicksTrackLists initial,check;
    short track=0;struct SlicksTrackPlaylist playlist={&track,1,1};
    if(slicks_track_lists_open(&initial,empty,sizeof empty))return 1;
    long size=slicks_track_lists_write(&initial,-1,(const unsigned char *)"IO test",
        &playlist,1,name,0,bytes,sizeof bytes);
    if(size<=0 || slicks_track_lists_open(&check,bytes,(unsigned long)size) || check.count!=1)return 1;
    FILE *file=fopen(argv[1],"wbx");if(!file)return 2;
    int failed=fwrite(bytes,1,(size_t)size,file)!=(size_t)size;
    failed|=fclose(file)!=0;return failed;
}
