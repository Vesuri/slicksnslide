/* Create a new local-only title fixture; never overwrite user settings. */
#include <stdio.h>
#include "../src/gen/setup_defaults.h"
int main(int argc,char **argv)
{
    if(argc!=4 || argv[2][0]<'0' || argv[2][0]>'5' || argv[2][1] ||
       argv[3][0]<'1' || argv[3][0]>'8' || argv[3][1])return 2;
    struct SlicksConfiguration c=slicks_original_configuration;
    c.options[0]=(short)(argv[2][0]-'0');
    c.field_05e1=(unsigned char)(argv[3][0]-'0');
    unsigned char bytes[142];
    if(slicks_save_configuration(&c,bytes,sizeof bytes,0xa1)!=sizeof bytes)return 1;
    FILE *f=fopen(argv[1],"wbx");if(!f)return 2;
    int failed=fwrite(bytes,1,sizeof bytes,f)!=sizeof bytes;
    failed|=fclose(f)!=0;return failed;
}
