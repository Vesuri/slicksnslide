#ifndef SLICKS_HOST_ARCHIVE_H
#define SLICKS_HOST_ARCHIVE_H
#include <stdio.h>
#include <string.h>

/* Read original archive resources directly; never generate extracted assets. */
static long host_archive_load(const char *path, const char *name,
                               unsigned char *bytes, unsigned long capacity)
{
    unsigned char header[5], entry[19], next[19];
    FILE *file = fopen(path, "rb");
    long result = -1;
    if (!file) return -1;
    if (fread(header,1,5,file)!=5 || memcmp(header,"MF\032",3)) goto done;
    unsigned count = ((unsigned)header[3]<<8)|header[4];
    for (unsigned i=0; i<count; ++i) {
        if (fread(entry,1,19,file)!=19) goto done;
        if (strncmp((const char *)entry,name,16)) continue;
        unsigned long start=((unsigned long)entry[16]<<16)|((unsigned)entry[17]<<8)|entry[18];
        unsigned long end;
        if(i+1<count) {
            if(fread(next,1,19,file)!=19) goto done;
            end=((unsigned long)next[16]<<16)|((unsigned)next[17]<<8)|next[18];
        } else {
            if(fseek(file,0,SEEK_END)) goto done;
            long position=ftell(file); if(position<0) goto done;
            end=(unsigned long)position;
        }
        /* Like the target loader, skip zero-size name markers (e.g. car9)
         * and continue looking for the later payload with the same name. */
        if (end==start) {
            if(i+1==count || fseek(file,-19L,SEEK_CUR)) goto done;
            continue;
        }
        if (end<start || end-start>capacity || fseek(file,(long)start,SEEK_SET)) goto done;
        if (fread(bytes,1,end-start,file)!=end-start) goto done;
        result=(long)(end-start);
        break;
    }
done:
    fclose(file);
    return result;
}
#endif
