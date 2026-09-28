/* Create a fresh, local-only expired-trial CFG. Never modify user settings. */
#include <stdio.h>
#include "../src/gen/setup_defaults.h"
int main(int argc,char **argv)
{
    if(argc!=2) return 2;
    struct SlicksConfiguration c=slicks_original_configuration;
    c.date_code=0;
    unsigned char bytes[142];
    if(slicks_save_configuration(&c,bytes,sizeof bytes,0xa1)!=sizeof bytes) return 1;
    FILE *f=fopen(argv[1],"wbx");if(!f) return 2;
    int failed=fwrite(bytes,1,sizeof bytes,f)!=sizeof bytes;
    failed|=fclose(f)!=0;return failed;
}
