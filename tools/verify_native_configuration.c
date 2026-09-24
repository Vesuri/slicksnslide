#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "../src/game/configuration.h"

/* The Amiga struct has an 86-byte byte-field prefix followed by 28
 * big-endian words. Convert representation only, then use the already
 * original-code-verified CFG writer to compare every persisted field. */
_Static_assert(sizeof(short)==2,"16-bit short required");
_Static_assert(offsetof(struct SlicksConfiguration,date_code)==86,"unexpected byte prefix");
_Static_assert(sizeof(struct SlicksConfiguration)==142,"unexpected native configuration layout");

static int read_exact(const char *path,unsigned char *data,size_t size)
{
    FILE *f=fopen(path,"rb");
    if(!f) { perror(path); return -1; }
    size_t got=fread(data,1,size,f);
    int extra=fgetc(f),failed=ferror(f);
    if(fclose(f)) failed=1;
    if(got!=size || extra!=EOF || failed) {
        fprintf(stderr,"Expected exactly %zu bytes: %s\n",size,path); return -1;
    }
    return 0;
}
int main(int argc,char **argv)
{
    if(argc!=3) { fprintf(stderr,"usage: %s native-configuration-dump SLICKS.CFG\n",argv[0]); return 2; }
    unsigned char native[142],file[142],encoded[142];
    struct SlicksConfiguration configuration;
    if(read_exact(argv[1],native,sizeof native) || read_exact(argv[2],file,sizeof file)) return 2;
    memcpy(&configuration,native,86);
    for(unsigned at=86;at<142;at+=2) {
        short value=(short)((unsigned)native[at]*256U+native[at+1]);
        memcpy((unsigned char *)&configuration+at,&value,sizeof value);
    }
    if(file[0]!=15 || slicks_save_configuration(&configuration,encoded,sizeof encoded,file[1])!=142) return 1;
    for(unsigned at=0;at<142;++at) if(encoded[at]!=file[at]) {
        fprintf(stderr,"Reloaded configuration differs at CFG byte %u: native=%u saved=%u\n",at,encoded[at],file[at]); return 1;
    }
    puts("Native reloaded configuration: all 142 saved CFG bytes match");
    return 0;
}
