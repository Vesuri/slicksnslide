#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/ui/language_table.h"
typedef int BPTR;
typedef const char *CONST_STRPTR;
struct SlicksResourceArchive { int unused; };
static unsigned char language_choice_test;
static unsigned char g_slicks_language_console_modes,g_slicks_language_console_bytes;
static unsigned char input_bytes[32];
static unsigned input_count,input_at,reads,enters,exits;
static int interactive,fail_enter,fail_exit,fail_read,read_result,raw;
static BPTR Input(void) { return 1; }
static int IsInteractive(BPTR input) { assert(input==1); return interactive; }
static int SetMode(BPTR input,int mode)
{
    assert(input==1);
    if(mode) { ++enters; if(fail_enter) return 0; raw=1; }
    else { ++exits; assert(raw); if(fail_exit) return 0; raw=0; }
    return 1;
}
static long Read(BPTR input,void *out,long size)
{
    assert(input==1 && size==1 && raw);
    ++reads;
    if((int)reads==fail_read) return read_result;
    if(input_at==input_count) return 0;
    *(unsigned char *)out=input_bytes[input_at++]; return 1;
}
static void PutStr(CONST_STRPTR text) { assert(text); }
static int language_console_test_key(unsigned step) { (void)step; assert(0); return -1; }
static long slicks_resource_archive_load(struct SlicksResourceArchive *a,
    const char *name,unsigned char *out,unsigned long capacity)
{
    (void)a;
    assert(!strncmp(name,"lang",4) && name[4]>='1' && name[4]<='8');
    assert(capacity>=8); memcpy(out,"label\r\n",8); return 8;
}
#include "../src/platform/amiga/amiga_language_chooser.h"
static void reset(void)
{
    const unsigned char keys[]={0x9b,'B',0x9b,'1','A',0x9b,'B',27};
    memcpy(input_bytes,keys,sizeof keys); input_count=sizeof keys; input_at=0;
    reads=enters=exits=0;interactive=1;fail_enter=fail_exit=fail_read=raw=0;
}
int main(void)
{
    struct SlicksResourceArchive archive={0}; unsigned cases=0;
    reset();assert(choose_startup_language(&archive)==2);
    assert(enters==1 && exits==1 && !raw && reads==input_count); ++cases;
    for(unsigned at=1;at<=8;++at) for(int result=-1;result<=0;++result) {
        reset();fail_read=(int)at;read_result=result;
        assert(choose_startup_language(&archive)==-1);
        assert(reads==at && enters==1 && exits==1 && !raw); ++cases;
        reset();fail_read=(int)at;read_result=result;fail_exit=1;
        assert(choose_startup_language(&archive)==-1);
        assert(reads==at && enters==1 && exits==1 && raw); ++cases;
    }
    reset();interactive=0;assert(choose_startup_language(&archive)==-1);
    assert(!enters && !exits && !reads); ++cases;
    reset();fail_enter=1;assert(choose_startup_language(&archive)==-1);
    assert(enters==1 && !exits && !reads); ++cases;
    reset();fail_exit=1;assert(choose_startup_language(&archive)==-1);
    assert(enters==1 && exits==1 && raw); ++cases;
    printf("Production language console: %u normal/read/EOF/mode-failure cases pass\n",cases);
}
