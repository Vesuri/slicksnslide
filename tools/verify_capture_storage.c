#define SLICKS_CAPTURE_STORAGE_TEST
#define main track_storage_main
#include "verify_track_storage.c"
#undef main

int main(void)
{
    static unsigned char pixels[64000],palette[768],encoded[SLICKS_CAPTURE_SIZE],scratch[SLICKS_CAPTURE_SIZE];
    for(unsigned i=0;i<64000;++i) pixels[i]=(unsigned char)(i*17);
    for(unsigned i=0;i<768;++i) palette[i]=(unsigned char)(i&63);
    assert(!slicks_encode_capture(encoded,sizeof encoded,pixels,palette));
    assert(slicks_amiga_store_capture(pixels,palette,0,sizeof scratch).io_error==ERROR_NO_FREE_STORE);
    assert(slicks_amiga_store_capture(pixels,palette,scratch,sizeof scratch-1).io_error==ERROR_NO_FREE_STORE);
    assert(!operation && !allocations);
    unsigned calls=0;
    for(unsigned fail=0;fail<=calls+1;++fail) {
        memset(files,0,sizeof files); operation=error=0;
        fail_first=fail; fail_second=0;
        files[0].present=1; files[0].size=1; files[0].bytes[0]=123;
        struct SlicksSetupStorageReport report=slicks_amiga_store_capture(pixels,palette,scratch,sizeof scratch);
        assert(!allocations && files[0].present && files[0].size==1 && files[0].bytes[0]==123);
        if(!fail) { calls=operation; assert(report.result==SLICKS_SETUP_SAVED); }
        if(report.result==SLICKS_SETUP_SAVED || report.result==SLICKS_SETUP_SAVED_CLEANUP_PENDING)
            assert(equals(3,encoded,sizeof encoded));
        if(files[3].present) assert(equals(3,encoded,sizeof encoded));
    }
    memset(files,0,sizeof files); operation=error=fail_first=fail_second=0;
    files[1].present=1; files[1].size=1; files[1].bytes[0]=42;
    assert(slicks_amiga_store_capture(pixels,palette,scratch,sizeof scratch).result==SLICKS_SETUP_RECOVERY_REQUIRED);
    assert(!allocations && files[1].size==1 && files[1].bytes[0]==42 && !files[0].present);
    printf("Capture storage: %u single-fault points, no overwrite, retained recovery file pass\n",calls);
    return 0;
}
