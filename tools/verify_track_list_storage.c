/* Reuse the existing short-read/write and two-fault AmigaDOS mock, but
 * execute the actual production SLICKS.TRK load/store adapter. */
#define SLICKS_TRACK_LIST_STORAGE_TEST
#define main track_records_storage_verifier_main
#include "verify_track_storage.c"
#undef main
static unsigned char transaction_work[SLICKS_AMIGA_TRACK_LIST_BYTES];
static const unsigned char *track_name(void *p,unsigned index)
{ (void)p; static const unsigned char n[][9]={"BASIC","1WAY","ABCDEFGH"}; return n[index]; }
static void initialize_with_cache(const unsigned char *bytes,unsigned size)
{
    assert(allocations==1);
    memset(files,0,sizeof files); memcpy(files[0].bytes,bytes,size);
    files[0].size=size; files[0].present=1; operation=error=0;
}
int main(void)
{
    paths[0]="SLICKS.TRK";
    unsigned char empty[8]={'S','S','T','r','k',26,0,0},old[1024],next[1024],loaded[1024];
    struct SlicksTrackLists initial,lists;
    assert(!slicks_track_lists_open(&initial,empty,sizeof empty));
    short indices[]={2,0,2,1}; struct SlicksTrackPlaylist selected={indices,4,4};
    long old_size=slicks_track_lists_write(&initial,-1,(const unsigned char *)"First",&selected,3,track_name,0,old,sizeof old);
    assert(old_size>0 && !slicks_track_lists_open(&lists,old,(unsigned long)old_size));
    unsigned cases=0,results[2]={0};
    for(unsigned action=0;action<2;++action) {
        int remove=action?0:-1; const unsigned char *title=action?0:(const unsigned char *)"Second";
        long next_size=slicks_track_lists_write(&lists,remove,title,&selected,3,track_name,0,next,sizeof next);
        assert(next_size>0);
        fail_first=fail_second=0; initialize(old,(unsigned)old_size);
        struct SlicksSetupStorageReport report=slicks_amiga_store_track_lists(&lists,remove,title,&selected,3,track_name,0,transaction_work,sizeof transaction_work);
        assert(report.result==SLICKS_SETUP_SAVED && equals(0,next,(unsigned)next_size) && writes==1 &&
            report.size==(unsigned long)next_size && !memcmp(transaction_work,next,(size_t)next_size));
        unsigned calls=operation;
        for(unsigned first=0;first<=calls+2;++first) for(unsigned second=first;second<=calls+2;++second) {
            initialize(old,(unsigned)old_size); fail_first=first; fail_second=second;
            report=slicks_amiga_store_track_lists(&lists,remove,title,&selected,3,track_name,0,transaction_work,sizeof transaction_work);
            assert(!allocations && writes<=1); ++cases; ++results[report.result];
            if(report.result==SLICKS_SETUP_SAVED) assert(equals(0,next,(unsigned)next_size));
        }
    }
    fail_first=fail_second=0; initialize(old,(unsigned)old_size);
    struct SlicksTrackLists view={0,17,19};
    struct SlicksSetupLoadReport load=slicks_amiga_load_track_lists(loaded,sizeof loaded,&view,transaction_work,sizeof transaction_work);
    assert(load.result==SLICKS_SETUP_LOADED && view.bytes==loaded && view.count==1 &&
        view.size==(unsigned long)old_size && !memcmp(loaded,old,(size_t)old_size) && !allocations);
    unsigned calls=operation;
    for(unsigned fault_at=1;fault_at<=calls;++fault_at) {
        initialize(old,(unsigned)old_size); fail_first=fault_at; fail_second=0;
        memset(loaded,0xa5,sizeof loaded); view=(struct SlicksTrackLists){0,17,19};
        load=slicks_amiga_load_track_lists(loaded,sizeof loaded,&view,transaction_work,sizeof transaction_work);
        assert(load.result!=SLICKS_SETUP_LOADED && !view.bytes && view.size==17 && view.count==19 && !allocations);
        for(unsigned i=0;i<sizeof loaded;++i) assert(loaded[i]==0xa5);
    }
    fail_first=fail_second=0;
    for(unsigned capacity=8;capacity<(unsigned)old_size;++capacity) {
        initialize(old,(unsigned)old_size); view=(struct SlicksTrackLists){0,17,19}; memset(loaded,0xa5,sizeof loaded);
        load=slicks_amiga_load_track_lists(loaded,capacity,&view,transaction_work,sizeof transaction_work);
        assert(load.result==SLICKS_SETUP_LOAD_INVALID && !view.bytes && view.size==17 && view.count==19 && !allocations);
        for(unsigned i=0;i<sizeof loaded;++i) assert(loaded[i]==0xa5);
    }
    for(unsigned cut=0;cut<(unsigned)old_size;++cut) {
        initialize(old,cut); view=(struct SlicksTrackLists){0,17,19}; memset(loaded,0xa5,sizeof loaded);
        load=slicks_amiga_load_track_lists(loaded,sizeof loaded,&view,transaction_work,sizeof transaction_work);
        assert(load.result==SLICKS_SETUP_LOAD_INVALID && !view.bytes && !allocations);
        for(unsigned i=0;i<sizeof loaded;++i) assert(loaded[i]==0xa5);
    }
    initialize(empty,sizeof empty); files[0].present=0;
    load=slicks_amiga_load_track_lists(loaded,sizeof loaded,&view,transaction_work,sizeof transaction_work);
    assert(load.result==SLICKS_SETUP_LOADED && !view.count && view.size==8 && !files[0].present && !allocations);
    struct SlicksSetupStorageReport report=slicks_amiga_store_track_lists(&view,-1,(const unsigned char *)"First",&selected,3,track_name,0,transaction_work,sizeof transaction_work);
    assert(report.result==SLICKS_SETUP_SAVED && equals(0,old,(unsigned)old_size) && !allocations);
    memset(loaded,0,sizeof loaded); view=(struct SlicksTrackLists){0,0,0};
    load=slicks_amiga_load_track_lists(loaded,sizeof loaded,&view,transaction_work,sizeof transaction_work);
    short recovered[4]={-1,-1,-1,-1}; struct SlicksTrackPlaylist restored={recovered,0,4};
    assert(load.result==SLICKS_SETUP_LOADED && !slicks_track_lists_select(&view,0,&restored,3,track_name,0));
    assert(restored.count==4 && !memcmp(recovered,indices,sizeof recovered) && !allocations);
    for(unsigned i=0;i<2;++i) assert(results[i]);
    initialize(old,(unsigned)old_size);
    report=slicks_amiga_store_track_lists(&view,-1,(const unsigned char *)"First",&selected,3,track_name,0,0,sizeof transaction_work);
    assert(report.result==SLICKS_SETUP_SAVE_FAILED && report.io_error==ERROR_NO_FREE_STORE && !operation && !allocations);
    report=slicks_amiga_store_track_lists(&view,-1,(const unsigned char *)"First",&selected,3,track_name,0,transaction_work,sizeof transaction_work-1);
    assert(report.result==SLICKS_SETUP_SAVE_FAILED && report.io_error==ERROR_NO_FREE_STORE && !operation && !allocations);
    struct SlicksAmigaTrackListCache cache={0};
    initialize(old,(unsigned)old_size); fail_first=fail_second=0;
    slicks_amiga_track_list_cache_refresh(&cache,transaction_work,sizeof transaction_work);
    assert(cache.report.result==SLICKS_SETUP_LOAD_IO_ERROR && !operation && !allocations);
    fail_first=1;
    assert(slicks_amiga_track_list_cache_create(&cache)<0 && !cache.storage && !allocations);
    slicks_amiga_track_list_cache_free(&cache);
    fail_first=0;operation=0;
    assert(!slicks_amiga_track_list_cache_create(&cache) && allocations==1);
    unsigned startup_calls=allocation_calls;
    assert(slicks_amiga_track_list_cache_create(&cache)<0 && allocation_calls==startup_calls);
    operation=0;
    slicks_amiga_track_list_cache_refresh(&cache,transaction_work,sizeof transaction_work);
    assert(cache.report.result==SLICKS_SETUP_LOADED && cache.view.size==(unsigned long)old_size &&
        cache.view.count==1 && !memcmp(cache.view.bytes,old,(size_t)old_size) && allocations==1);
    calls=operation;
    const unsigned char *retained=cache.view.bytes;
    for(unsigned size=0;size<2;++size) {
        initialize_with_cache(old,(unsigned)old_size);
        slicks_amiga_track_list_cache_refresh(&cache,size?transaction_work:0,sizeof transaction_work-size);
        assert(cache.report.result==SLICKS_SETUP_LOAD_IO_ERROR && cache.report.io_error==ERROR_NO_FREE_STORE &&
            cache.view.bytes==retained && !memcmp(retained,old,(size_t)old_size) && !operation && allocations==1);
    }
    for(unsigned fault_at=1;fault_at<=calls;++fault_at) {
        initialize_with_cache(old,(unsigned)old_size); fail_first=fault_at; fail_second=0;
        slicks_amiga_track_list_cache_refresh(&cache,transaction_work,sizeof transaction_work);
        assert(cache.report.result!=SLICKS_SETUP_LOADED && cache.view.bytes==retained &&
            !memcmp(cache.view.bytes,old,(size_t)old_size) && allocations==1);
    }
    fail_first=fail_second=0;
    for(unsigned cut=0;cut<(unsigned)old_size;++cut) {
        initialize_with_cache(old,cut);
        slicks_amiga_track_list_cache_refresh(&cache,transaction_work,sizeof transaction_work);
        assert(cache.report.result==SLICKS_SETUP_LOAD_INVALID && cache.view.bytes==retained &&
            !memcmp(cache.view.bytes,old,(size_t)old_size) && allocations==1);
    }
    /* A successful save publishes the bytes it wrote without a reread. */
    {
        long next_size=slicks_track_lists_write(&cache.view,-1,(const unsigned char *)"Second",&selected,3,track_name,0,next,sizeof next);
        assert(next_size>0);
        initialize_with_cache(old,(unsigned)old_size);
        struct SlicksSetupStorageReport saved=slicks_amiga_store_track_lists(&cache.view,-1,(const unsigned char *)"Second",
            &selected,3,track_name,0,transaction_work,sizeof transaction_work);
        assert(saved.result==SLICKS_SETUP_SAVED); operation=0;
        slicks_amiga_track_list_cache_publish(&cache,transaction_work,saved.size);
        assert(!operation && cache.report.result==SLICKS_SETUP_LOADED && cache.view.bytes==retained &&
            cache.view.count==2 && cache.view.size==(unsigned long)next_size && !memcmp(retained,next,(size_t)next_size));
        transaction_work[0]='X';
        slicks_amiga_track_list_cache_publish(&cache,transaction_work,saved.size);
        assert(cache.report.result==SLICKS_SETUP_LOAD_INVALID && !memcmp(retained,next,(size_t)next_size));
    }
    initialize_with_cache(empty,sizeof empty); files[0].present=0;
    slicks_amiga_track_list_cache_refresh(&cache,transaction_work,sizeof transaction_work);
    assert(cache.report.result==SLICKS_SETUP_LOADED && cache.view.size==8 &&
        !cache.view.count && !memcmp(cache.view.bytes,empty,8) && allocations==1);
    static unsigned char maximum[SLICKS_AMIGA_TRACK_LIST_BYTES];
    memcpy(maximum,empty,8); maximum[7]=1;
    maximum[8]=8188>>8;maximum[9]=8188&255;maximum[10]='X';
    for(unsigned i=31;i<sizeof maximum;++i) maximum[i]=(unsigned char)(i*37);
    for(unsigned repeat=0;repeat<3;++repeat) {
        initialize_with_cache(maximum,sizeof maximum);
        slicks_amiga_track_list_cache_refresh(&cache,transaction_work,sizeof transaction_work);
        assert(cache.report.result==SLICKS_SETUP_LOADED && cache.view.bytes==retained &&
            cache.view.size==sizeof maximum && !memcmp(retained,maximum,sizeof maximum));
        initialize_with_cache(maximum,sizeof maximum);files[0].bytes[0]='X';
        slicks_amiga_track_list_cache_refresh(&cache,transaction_work,sizeof transaction_work);
        assert(cache.report.result==SLICKS_SETUP_LOAD_INVALID && !memcmp(retained,maximum,sizeof maximum));
        initialize_with_cache(empty,sizeof empty);
        slicks_amiga_track_list_cache_refresh(&cache,transaction_work,sizeof transaction_work);
        assert(cache.report.result==SLICKS_SETUP_LOADED && cache.view.bytes==retained && cache.view.size==8);
    }
    slicks_amiga_track_list_cache_free(&cache);
    assert(!allocations && !cache.storage && !cache.view.bytes && cache.report.result==SLICKS_SETUP_LOAD_INVALID);
    assert(allocation_calls==startup_calls);
    puts("Resident track-list cache: startup reservation, allocation-free refresh and publish, all refresh faults and cleanup pass");
    printf("Amiga saved-list storage: %u single/double save faults, load/close/allocation failures, truncation and missing-file first save pass; one complete write\n",cases);
    return 0;
}
