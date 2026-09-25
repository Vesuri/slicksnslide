/* Check a read-only GDB table capture against the actual AmigaDOS output.
 * Encoding itself is independently checked by verify-track-record-write. */
#include <stdio.h>
#include <string.h>
#include "../src/game/track_records.h"
static long load(const char *path,unsigned char *bytes,unsigned capacity)
{
    FILE *f=fopen(path,"rb"); if(!f) return -1;
    size_t size=fread(bytes,1,capacity,f); int error=ferror(f),extra=fgetc(f);
    fclose(f); return error || extra!=EOF?-1:(long)size;
}
int main(int argc,char **argv)
{
    unsigned char before[8192],after[8192],capture[322];
    if(argc!=4) { fputs("usage: verify_target_records original.SS saved.SS target.records\n",stderr); return 2; }
    long size=load(argv[1],before,sizeof before),written=load(argv[2],after,sizeof after);
    if(size<363 || written!=size || load(argv[3],capture,sizeof capture)!=322) return 2;
    struct SlicksTrackRecords records;
    memcpy(records.entries,capture,319);
    /* 68020 ABI: one padding byte, then a big-endian native trailer word. */
    records.trailer=(unsigned short)(capture[320]*256U+capture[321]);
    if(slicks_write_track_records(before,(unsigned long)size,&records)!=1 || memcmp(before,after,(size_t)size)) {
        fputs("Target record publication differs from the original-format encoder\n",stderr); return 1;
    }
    printf("Target record publication: all %ld bytes match; track payload preserved\n",size);
    return 0;
}
