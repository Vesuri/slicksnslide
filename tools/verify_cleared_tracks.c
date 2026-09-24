/* Read-only end-to-end disk check. Expected record bytes use the serializer
 * independently checked against DOS by verify_track_record_write.c. */
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "../src/game/track_records.h"

static int read_track(const char *directory,const char *name,unsigned char data[8193])
{
    char path[4096];
    if(snprintf(path,sizeof path,"%s/%s",directory,name)>=(int)sizeof path) return -1;
    FILE *file=fopen(path,"rb");
    if(!file) { perror(path); return -1; }
    size_t size=fread(data,1,8193,file);
    int bad=ferror(file) || size>8192;
    if(fclose(file)) bad=1;
    return bad?-1:(int)size;
}
int main(int argc,char **argv)
{
    if(argc!=3) { fprintf(stderr,"usage: %s reference-tracks cleared-tracks\n",argv[0]); return 2; }
    DIR *dir=opendir(argv[1]); if(!dir) return 2;
    unsigned count=0,rewritten=0; struct dirent *entry;
    while((entry=readdir(dir))) {
        size_t length=strlen(entry->d_name);
        if(length<3 || strcmp(entry->d_name+length-3,".SS")) continue;
        unsigned char expected[8193],actual[8193];
        int size=read_track(argv[1],entry->d_name,expected);
        int got=read_track(argv[2],entry->d_name,actual);
        struct SlicksTrackRecords records; slicks_clear_track_records(&records);
        int changed=size<0?-1:slicks_write_track_records(expected,(unsigned long)size,&records);
        if(changed<0 || got!=size || memcmp(expected,actual,(size_t)size)) {
            fprintf(stderr,"Cleared file mismatch: %s\n",entry->d_name); closedir(dir); return 1;
        }
        rewritten+=(unsigned)changed; ++count;
    }
    closedir(dir);
    dir=opendir(argv[2]); if(!dir) return 2;
    while((entry=readdir(dir))) {
        size_t length=strlen(entry->d_name);
        if(length>=4 && (!strcmp(entry->d_name+length-4,".new") || !strcmp(entry->d_name+length-4,".bak"))) {
            fprintf(stderr,"Unexpected recovery artifact: %s\n",entry->d_name); closedir(dir); return 1;
        }
    }
    closedir(dir);
    if(!count) return 1;
    printf("CLEARED_TRACKS_DISK_OK files=%u rewritten=%u; complete files match, no recovery artifacts\n",count,rewritten);
    return 0;
}
