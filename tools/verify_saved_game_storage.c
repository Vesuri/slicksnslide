#define SLICKS_SAVED_STORAGE_TEST
#define main track_storage_verifier_main
#include "verify_track_storage.c"
#undef main
int main(void)
{
    unsigned char scratch[SLICKS_SAVED_GAME_MAX_BYTES];
    paths[0]="RACE.SSS"; paths[1]="RACE.SSS.new"; paths[2]="RACE.SSS.bak";
    unsigned char tracks[256][8],expected[SLICKS_SAVED_GAME_MAX_BYTES],old[8]={0x53,8,0,0,0,0,0,0};
    memset(tracks,'T',sizeof tracks);
    struct SlicksSavedGame game={.track_count=256,.next_track=93,.tracks=tracks};
    memset(game.names,'N',sizeof game.names);
    long size=slicks_save_game_bytes(&game,expected,sizeof expected);
    assert(size==6+8*256+4*53);
    unsigned cases=0,results[4]={0};
    for(unsigned invalid=0;invalid<2;++invalid) {
        initialize(old,sizeof old);
        struct SlicksSetupStorageReport report=slicks_amiga_store_saved_game(paths[0],&game,
            invalid?scratch:0,invalid?(unsigned long)size-1:sizeof scratch);
        assert(report.result==SLICKS_SETUP_SAVE_FAILED && report.io_error==ERROR_NO_FREE_STORE);
        assert(!operation && !allocations && equals(0,old,sizeof old));
        struct SlicksSavedGame untouched; memset(&untouched,0xa5,sizeof untouched);
        unsigned char names[256][8]; memset(names,0xa5,sizeof names);
        struct SlicksSetupLoadReport load=slicks_amiga_load_saved_game(paths[0],&untouched,names,256,
            invalid?scratch:0,invalid?(unsigned long)size-1:sizeof scratch);
        assert(load.result==SLICKS_SETUP_LOAD_IO_ERROR && load.io_error==ERROR_NO_FREE_STORE && !operation);
        for(unsigned i=0;i<sizeof untouched;++i) assert(((unsigned char *)&untouched)[i]==0xa5);
        for(unsigned i=0;i<sizeof names;++i) assert(((unsigned char *)names)[i]==0xa5);
    }
    initialize(old,sizeof old); fail_first=fail_second=0;
    struct SlicksSetupStorageReport report=slicks_amiga_store_saved_game(paths[0],&game,scratch,sizeof scratch);
    assert(report.result==SLICKS_SETUP_SAVED && equals(0,expected,(unsigned)size) && !allocations);
    unsigned normal=operation;
    for(unsigned first=0;first<=normal+5;++first) for(unsigned second=first;second<=normal+5;++second) {
        initialize(old,sizeof old); fail_first=first; fail_second=second;
        report=slicks_amiga_store_saved_game(paths[0],&game,scratch,sizeof scratch);
        assert(!allocations && report.path==paths[0]); ++cases; ++results[report.result];
        if(report.result==SLICKS_SETUP_SAVED || report.result==SLICKS_SETUP_SAVED_CLEANUP_PENDING)
            assert(equals(0,expected,(unsigned)size));
        else assert(equals(0,old,sizeof old) || equals(2,old,sizeof old));
        if(report.result==SLICKS_SETUP_SAVE_FAILED) assert(equals(0,old,sizeof old));
        if(report.result==SLICKS_SETUP_SAVED || report.result==SLICKS_SETUP_SAVE_FAILED)
            assert(!files[1].present && !files[2].present);
    }
    fail_first=fail_second=0;
    for(unsigned artifact=1;artifact<3;++artifact) {
        initialize(old,sizeof old); files[artifact].present=1; files[artifact].size=3;
        struct File before[3]; memcpy(before,files,sizeof before);
        report=slicks_amiga_store_saved_game(paths[0],&game,scratch,sizeof scratch);
        assert(report.result==SLICKS_SETUP_RECOVERY_REQUIRED && !memcmp(before,files,sizeof before) && !allocations);
    }
    initialize(old,sizeof old); files[0].present=0;
    report=slicks_amiga_store_saved_game(paths[0],&game,scratch,sizeof scratch);
    assert(report.result==SLICKS_SETUP_SAVED && equals(0,expected,(unsigned)size) && !allocations);
    game.track_count=SLICKS_SAVED_GAME_TRACK_MAX+1; operation=0;
    report=slicks_amiga_store_saved_game(paths[0],&game,scratch,sizeof scratch);
    assert(report.result==SLICKS_SETUP_SAVE_FAILED && !operation && !allocations);
    for(unsigned i=0;i<4;++i) assert(results[i]);
    struct SlicksSavedGame loaded;
    unsigned char loaded_tracks[256][8],encoded[SLICKS_SAVED_GAME_MAX_BYTES];
    fail_first=fail_second=0; initialize(expected,(unsigned)size);
    struct SlicksSetupLoadReport load=slicks_amiga_load_saved_game(paths[0],&loaded,loaded_tracks,256,scratch,sizeof scratch);
    assert(load.result==SLICKS_SETUP_LOADED && !allocations);
    assert(slicks_save_game_bytes(&loaded,encoded,sizeof encoded)==size && !memcmp(encoded,expected,(size_t)size));
    unsigned reads=operation,load_cases=0;
    for(unsigned fault_at=1;fault_at<=reads;++fault_at) {
        initialize(expected,(unsigned)size); fail_first=fault_at;
        memset(&loaded,0xa5,sizeof loaded); memset(loaded_tracks,0xa5,sizeof loaded_tracks);
        load=slicks_amiga_load_saved_game(paths[0],&loaded,loaded_tracks,256,scratch,sizeof scratch);
        assert(load.result==SLICKS_SETUP_LOAD_IO_ERROR && load.path==paths[0] && !allocations);
        for(unsigned i=0;i<sizeof loaded;++i) assert(((unsigned char *)&loaded)[i]==0xa5);
        for(unsigned i=0;i<sizeof loaded_tracks;++i) assert(((unsigned char *)loaded_tracks)[i]==0xa5);
        assert(equals(0,expected,(unsigned)size)); ++load_cases;
    }
    fail_first=0;
    for(unsigned cut=0;cut<(unsigned)size;++cut) {
        initialize(expected,cut);
        load=slicks_amiga_load_saved_game(paths[0],&loaded,loaded_tracks,256,scratch,sizeof scratch);
        assert(load.result==SLICKS_SETUP_LOAD_INVALID && !allocations);
        for(unsigned i=0;i<sizeof loaded;++i) assert(((unsigned char *)&loaded)[i]==0xa5);
        for(unsigned i=0;i<sizeof loaded_tracks;++i) assert(((unsigned char *)loaded_tracks)[i]==0xa5);
        ++load_cases;
    }
    initialize(expected,(unsigned)size); files[0].size++;
    load=slicks_amiga_load_saved_game(paths[0],&loaded,loaded_tracks,256,scratch,sizeof scratch);
    assert(load.result==SLICKS_SETUP_LOAD_INVALID && !allocations);
    initialize(expected,(unsigned)size); files[0].present=0;
    load=slicks_amiga_load_saved_game(paths[0],&loaded,loaded_tracks,256,scratch,sizeof scratch);
    assert(load.result==SLICKS_SETUP_LOAD_IO_ERROR && load.io_error==ERROR_OBJECT_NOT_FOUND && !allocations);
    for(unsigned artifact=1;artifact<3;++artifact) {
        initialize(expected,(unsigned)size); files[artifact].present=1;
        load=slicks_amiga_load_saved_game(paths[0],&loaded,loaded_tracks,256,scratch,sizeof scratch);
        assert(load.result==SLICKS_SETUP_LOAD_RECOVERY && !allocations);
    }
    printf("Amiga saved-game storage: %u single/double faults, short writes, missing-file creation and recovery guards pass\n",cases);
    printf("Amiga saved-game load: %u I/O-fault and truncation cases preserve destinations; overflow/missing/recovery cases pass\n",load_cases);
    unsigned char large_tracks[300][8],large_loaded[300][8];
    for(unsigned i=0;i<sizeof large_tracks;++i) ((unsigned char *)large_tracks)[i]=(unsigned char)(i*31);
    game.track_count=300; game.tracks=large_tracks;
    size=slicks_save_game_bytes(&game,expected,sizeof expected); assert(size>0);
    initialize(old,sizeof old); fail_first=fail_second=0;
    report=slicks_amiga_store_saved_game(paths[0],&game,scratch,sizeof scratch);
    assert(report.result==SLICKS_SETUP_SAVED && equals(0,expected,(unsigned)size) && !allocations);
    memset(&loaded,0xa5,sizeof loaded); memset(large_loaded,0xa5,sizeof large_loaded);
    load=slicks_amiga_load_saved_game(paths[0],&loaded,large_loaded,256,scratch,sizeof scratch);
    assert(load.result==SLICKS_SETUP_LOAD_INVALID && !allocations);
    for(unsigned i=0;i<sizeof loaded;++i) assert(((unsigned char *)&loaded)[i]==0xa5);
    for(unsigned i=0;i<sizeof large_loaded;++i) assert(((unsigned char *)large_loaded)[i]==0xa5);
    load=slicks_amiga_load_saved_game(paths[0],&loaded,large_loaded,300,scratch,sizeof scratch);
    assert(load.result==SLICKS_SETUP_LOADED && !allocations);
    assert(slicks_save_game_bytes(&loaded,encoded,sizeof encoded)==size && !memcmp(encoded,expected,(size_t)size));
    operation=0;
    load=slicks_amiga_load_saved_game(paths[0],&loaded,large_loaded,SLICKS_SAVED_GAME_TRACK_MAX+1,scratch,sizeof scratch);
    assert(load.result==SLICKS_SETUP_LOAD_INVALID && !operation && !allocations);
    puts("Amiga saved-game storage: 300-track write/read and undersized/invalid capacity guards pass");
    static unsigned char maximum_tracks[SLICKS_SAVED_GAME_TRACK_MAX][8];
    static unsigned char maximum_loaded[SLICKS_SAVED_GAME_TRACK_MAX][8];
    for(unsigned i=0;i<sizeof maximum_tracks;++i) ((unsigned char *)maximum_tracks)[i]=(unsigned char)(i*31+7);
    game.track_count=SLICKS_SAVED_GAME_TRACK_MAX; game.tracks=maximum_tracks;
    size=slicks_save_game_bytes(&game,expected,sizeof expected);
    assert(size==SLICKS_SAVED_GAME_MAX_BYTES);
    initialize(old,sizeof old);
    report=slicks_amiga_store_saved_game(paths[0],&game,scratch,sizeof scratch);
    assert(report.result==SLICKS_SETUP_SAVED && equals(0,expected,(unsigned)size));
    load=slicks_amiga_load_saved_game(paths[0],&loaded,maximum_loaded,SLICKS_SAVED_GAME_TRACK_MAX,scratch,sizeof scratch);
    assert(load.result==SLICKS_SETUP_LOADED && !memcmp(maximum_tracks,maximum_loaded,sizeof maximum_tracks));
    assert(slicks_save_game_bytes(&loaded,encoded,sizeof encoded)==size && !memcmp(encoded,expected,(size_t)size));
    assert(!allocation_calls && !allocations);
    puts("Maximum 10000-track save/load roundtrip uses caller scratch with zero allocation calls");
    return 0;
}
