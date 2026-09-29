/* Create private valid/malformed fixtures without overwriting settings. */
#include <stdio.h>
#include <string.h>
#include "../src/game/track_lists.h"
static const unsigned char *name(void *context,unsigned index)
{ (void)context;(void)index;return (const unsigned char *)"BASIC"; }
int main(int argc,char **argv)
{
    if(argc!=2 && argc!=3)return 2;
    const unsigned char empty[8]={'S','S','T','r','k',26,0,0};
    unsigned char bytes[65537]={0};struct SlicksTrackLists initial,check;
    short track=0;struct SlicksTrackPlaylist playlist={&track,1,1};
    if(slicks_track_lists_open(&initial,empty,sizeof empty))return 1;
    long size=slicks_track_lists_write(&initial,-1,(const unsigned char *)"IO test",
        &playlist,1,name,0,bytes,sizeof bytes);
    if(size<=0 || slicks_track_lists_open(&check,bytes,(unsigned long)size) || check.count!=1)return 1;
    if(argc==3) {
        const char *kind=argv[2];
        if(!strcmp(kind,"short-header"))size=7;
        else if(!strcmp(kind,"short-record"))size=30;
        else if(!strcmp(kind,"short-names"))--size;
        else if(!strcmp(kind,"bad-magic"))bytes[0]='X';
        else if(!strcmp(kind,"list-overflow"))bytes[6]=128;
        else if(!strcmp(kind,"track-overflow"))bytes[8]=128;
        else if(!strcmp(kind,"unterminated"))memset(bytes+10,'X',21);
        else if(!strcmp(kind,"oversize"))size=sizeof bytes;
        else return 2;
        /* Oversize is valid-format data rejected by the disk-size limit;
         * trailing bytes alone are deliberately allowed by the DOS format. */
        int valid=!slicks_track_lists_open(&check,bytes,(unsigned long)size);
        if(valid!=(!strcmp(kind,"oversize")))return 1;
    }
    FILE *file=fopen(argv[1],"wbx");if(!file)return 2;
    int failed=fwrite(bytes,1,(size_t)size,file)!=(size_t)size;
    failed|=fclose(file)!=0;return failed;
}
