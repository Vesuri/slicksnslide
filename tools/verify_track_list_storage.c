/* Reuse the existing short-read/write and two-fault AmigaDOS mock, but
 * execute the actual production SLICKS.TRK load/store adapter. */
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
    paths[0]="SLICKS.TRK"; paths[1]="SLICKS.TRK.new"; paths[2]="SLICKS.TRK.bak";
    unsigned char empty[8]={'S','S','T','r','k',26,0,0},old[1024],next[1024],loaded[1024];
    struct SlicksTrackLists initial,lists;
    assert(!slicks_track_lists_open(&initial,empty,sizeof empty));
    short indices[]={2,0,2,1}; struct SlicksTrackPlaylist selected={indices,4,4};
    long old_size=slicks_track_lists_write(&initial,-1,(const unsigned char *)"First",&selected,3,track_name,0,old,sizeof old);
    assert(old_size>0 && !slicks_track_lists_open(&lists,old,(unsigned long)old_size));
    unsigned cases=0,results[4]={0};
    for(unsigned action=0;action<2;++action) {
        int remove=action?0:-1; const unsigned char *title=action?0:(const unsigned char *)"Second";
        long next_size=slicks_track_lists_write(&lists,remove,title,&selected,3,track_name,0,next,sizeof next);
        assert(next_size>0);
        fail_first=fail_second=0; initialize(old,(unsigned)old_size);
        struct SlicksSetupStorageReport report=slicks_amiga_store_track_lists(&lists,remove,title,&selected,3,track_name,0,transaction_work,sizeof transaction_work);
        assert(report.result==SLICKS_SETUP_SAVED && equals(0,next,(unsigned)next_size));
        unsigned calls=operation;
        for(unsigned first=0;first<=calls+2;++first) for(unsigned second=first;second<=calls+2;++second) {
            initialize(old,(unsigned)old_size); fail_first=first; fail_second=second;
            report=slicks_amiga_store_track_lists(&lists,remove,title,&selected,3,track_name,0,transaction_work,sizeof transaction_work);
            assert(!allocations); ++cases; ++results[report.result];
            if(report.result==SLICKS_SETUP_SAVED || report.result==SLICKS_SETUP_SAVED_CLEANUP_PENDING)
                assert(equals(0,next,(unsigned)next_size));
            else assert(equals(0,old,(unsigned)old_size) || equals(2,old,(unsigned)old_size));
            if(report.result==SLICKS_SETUP_SAVE_FAILED) assert(equals(0,old,(unsigned)old_size));
            if(report.result==SLICKS_SETUP_SAVED || report.result==SLICKS_SETUP_SAVE_FAILED)
                assert(!files[1].present && !files[2].present);
        }
    }
    fail_first=fail_second=0; initialize(old,(unsigned)old_size);
    struct SlicksTrackLists view={0,17,19};
    struct SlicksSetupLoadReport load=slicks_amiga_load_track_lists(loaded,sizeof loaded,&view);
    assert(load.result==SLICKS_SETUP_LOADED && view.bytes==loaded && view.count==1 &&
        view.size==(unsigned long)old_size && !memcmp(loaded,old,(size_t)old_size) && !allocations);
    unsigned calls=operation;
    for(unsigned fault_at=1;fault_at<=calls;++fault_at) {
        initialize(old,(unsigned)old_size); fail_first=fault_at; fail_second=0;
        memset(loaded,0xa5,sizeof loaded); view=(struct SlicksTrackLists){0,17,19};
        load=slicks_amiga_load_track_lists(loaded,sizeof loaded,&view);
        assert(load.result!=SLICKS_SETUP_LOADED && !view.bytes && view.size==17 && view.count==19 && !allocations);
        for(unsigned i=0;i<sizeof loaded;++i) assert(loaded[i]==0xa5);
    }
    fail_first=fail_second=0;
    for(unsigned capacity=8;capacity<(unsigned)old_size;++capacity) {
        initialize(old,(unsigned)old_size); view=(struct SlicksTrackLists){0,17,19}; memset(loaded,0xa5,sizeof loaded);
        load=slicks_amiga_load_track_lists(loaded,capacity,&view);
        assert(load.result==SLICKS_SETUP_LOAD_INVALID && !view.bytes && view.size==17 && view.count==19 && !allocations);
        for(unsigned i=0;i<sizeof loaded;++i) assert(loaded[i]==0xa5);
    }
    for(unsigned cut=0;cut<(unsigned)old_size;++cut) {
        initialize(old,cut); view=(struct SlicksTrackLists){0,17,19}; memset(loaded,0xa5,sizeof loaded);
        load=slicks_amiga_load_track_lists(loaded,sizeof loaded,&view);
        assert(load.result==SLICKS_SETUP_LOAD_INVALID && !view.bytes && !allocations);
        for(unsigned i=0;i<sizeof loaded;++i) assert(loaded[i]==0xa5);
    }
    for(unsigned artifact=1;artifact<3;++artifact) {
        initialize(old,(unsigned)old_size); files[artifact].present=1; files[artifact].size=5;
        memset(files[artifact].bytes,0x77,5);
        struct File snapshot[3]; memcpy(snapshot,files,sizeof files);
        load=slicks_amiga_load_track_lists(loaded,sizeof loaded,&view);
        assert(load.result==SLICKS_SETUP_LOAD_RECOVERY && !memcmp(snapshot,files,sizeof files));
        struct SlicksSetupStorageReport report=slicks_amiga_store_track_lists(&lists,0,0,0,0,0,0,transaction_work,sizeof transaction_work);
        assert(report.result==SLICKS_SETUP_RECOVERY_REQUIRED && !memcmp(snapshot,files,sizeof files) && !allocations);
    }
    initialize(empty,sizeof empty); files[0].present=0;
    load=slicks_amiga_load_track_lists(loaded,sizeof loaded,&view);
    assert(load.result==SLICKS_SETUP_LOADED && !view.count && view.size==8 && !files[0].present && !allocations);
    struct SlicksSetupStorageReport report=slicks_amiga_store_track_lists(&view,-1,(const unsigned char *)"First",&selected,3,track_name,0,transaction_work,sizeof transaction_work);
    assert(report.result==SLICKS_SETUP_SAVED && equals(0,old,(unsigned)old_size) && !allocations);
    memset(loaded,0,sizeof loaded); view=(struct SlicksTrackLists){0,0,0};
    load=slicks_amiga_load_track_lists(loaded,sizeof loaded,&view);
    short recovered[4]={-1,-1,-1,-1}; struct SlicksTrackPlaylist restored={recovered,0,4};
    assert(load.result==SLICKS_SETUP_LOADED && !slicks_track_lists_select(&view,0,&restored,3,track_name,0));
    assert(restored.count==4 && !memcmp(recovered,indices,sizeof recovered) && !allocations);
    for(unsigned i=0;i<4;++i) assert(results[i]);
    initialize(old,(unsigned)old_size);
    report=slicks_amiga_store_track_lists(&view,-1,(const unsigned char *)"First",&selected,3,track_name,0,0,sizeof transaction_work);
    assert(report.result==SLICKS_SETUP_SAVE_FAILED && report.io_error==ERROR_NO_FREE_STORE && !operation && !allocations);
    report=slicks_amiga_store_track_lists(&view,-1,(const unsigned char *)"First",&selected,3,track_name,0,transaction_work,sizeof transaction_work-1);
    assert(report.result==SLICKS_SETUP_SAVE_FAILED && report.io_error==ERROR_NO_FREE_STORE && !operation && !allocations);
    struct SlicksAmigaTrackListCache cache={0};
    initialize(old,(unsigned)old_size); fail_first=fail_second=0;
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
    for(unsigned artifact=1;artifact<3;++artifact) {
        initialize_with_cache(old,(unsigned)old_size); files[artifact].present=1;
        slicks_amiga_track_list_cache_refresh(&cache,transaction_work,sizeof transaction_work);
        assert(cache.report.result==SLICKS_SETUP_LOAD_RECOVERY && cache.view.bytes==retained && allocations==1);
    }
    initialize_with_cache(empty,sizeof empty); files[0].present=0;
    slicks_amiga_track_list_cache_refresh(&cache,transaction_work,sizeof transaction_work);
    assert(cache.report.result==SLICKS_SETUP_LOADED && cache.view.size==8 &&
        !cache.view.count && !memcmp(cache.view.bytes,empty,8) && allocations==1);
    slicks_amiga_track_list_cache_free(&cache);
    assert(!allocations && !cache.view.bytes && cache.report.result==SLICKS_SETUP_LOAD_INVALID);
    puts("Resident track-list cache: exact-size replacement, all refresh faults, recovery retention and cleanup pass");
    printf("Amiga saved-list storage: %u single/double save faults, load/close/allocation failures, truncation, recovery guards and missing-file first save pass\n",cases);
    return 0;
}
