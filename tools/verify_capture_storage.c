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
    assert(slicks_amiga_store_setup(0,0,0,0,SLICKS_AMIGA_SETUP_BYTES).io_error==ERROR_NO_FREE_STORE);
    assert(slicks_amiga_store_setup(0,0,0,scratch,SLICKS_AMIGA_SETUP_BYTES-1).io_error==ERROR_NO_FREE_STORE);
    assert(!operation && !allocations);
    unsigned calls=0,failed=0;
    for(unsigned fail=0;fail<=calls+1;++fail) {
        initialize((const unsigned char *)"\173",1);
        fail_first=fail; fail_second=0;
        struct SlicksSetupStorageReport report=slicks_amiga_store_capture(pixels,palette,scratch,sizeof scratch);
        /* An existing capture is never replaced. */
        assert(!allocations && files[0].present && files[0].size==1 && files[0].bytes[0]==123 && writes<=1);
        if(!fail) { calls=operation; assert(report.result==SLICKS_SETUP_SAVED && writes==1); }
        if(report.result==SLICKS_SETUP_SAVED) assert(equals(1,encoded,sizeof encoded));
        else ++failed;
    }
    assert(failed);
    printf("Capture storage: %u single-fault points, one complete write, never replaces a capture\n",calls);
    return 0;
}
