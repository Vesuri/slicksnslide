/* Synthetic catalogue for native UI testing. Never modifies reference data. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/game/track_lists.h"
int main(int argc,char **argv)
{
    if(argc!=2 && argc!=3) return 2;
    unsigned char bytes[65536]={0};
    unsigned count=2848;
    if(argc==3) {
        char *end; unsigned long value=strtoul(argv[2],&end,10);
        if(*end || value<1 || value>2848) return 2;
        count=(unsigned)value;
    }
    memcpy(bytes,"SSTrk\032",6); bytes[6]=count>>8; bytes[7]=count&255;
    unsigned long at=8;
    for(unsigned i=0;i<count;++i) {
        bytes[at+1]=(i==count-1)?2:0;
        snprintf((char *)bytes+at+2,21,"LIST %04u",i);
        at+=23;
        if(i==count-1) { memcpy(bytes+at,"1WAY",4); memcpy(bytes+at+8,"BASIC",5); at+=16; }
    }
    struct SlicksTrackLists lists; struct SlicksTrackListView last;
    char expected[21]; snprintf(expected,sizeof expected,"LIST %04u",count-1);
    if(slicks_track_lists_open(&lists,bytes,at) || lists.count!=count ||
       slicks_track_lists_get(&lists,count-1,&last) || last.count!=2 ||
       strcmp((const char *)last.title,expected)) return 1;
    FILE *f=fopen(argv[1],"wbx"); /* Refuse to replace an existing catalogue. */
    if(!f) { perror(argv[1]); return 1; }
    int failed=fwrite(bytes,1,at,f)!=at;
    failed|=fclose(f)!=0;
    if(failed) return 1;
    printf("Synthetic catalogue: %u lists, %lu bytes; last selects 1WAY/BASIC\n",count,at);
    return 0;
}
