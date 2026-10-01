/* The real adapter and transaction, including two-file rollback, on KickFS. */
#define SLICKS_PAIR_STORAGE_TEST
#define main track_storage_verifier_main
#include "verify_track_storage.c"
#undef main
int main(void)
{
    unsigned char old[96],next[96];
    memset(old,0x31,sizeof old); memset(next,0x72,sizeof next);
    unsigned limit=0,cases=0;
    for(unsigned first=0;first<=limit;++first)
    for(unsigned second=first;second<=limit;++second) {
        initialize(old,sizeof old);
        kickfs_mode=g_slicks_whdload=1;
        files[3]=files[0]; fail_first=first; fail_second=second;
        struct SlicksSetupStorageReport r={SLICKS_SETUP_SAVE_FAILED,0,0};
        const struct SlicksSetupFile pair[]={
            {paths[0],paths[1],paths[2],next,sizeof next},
            {paths[3],paths[4],paths[5],next,sizeof next}};
        const struct SlicksSetupFileOps ops={exists,write_new,rename_file,remove_file,&r};
        r.result=store_files(pair,2,&ops);
        if(!first && !second) limit=operation+20;
        for(unsigned i=0;i<6;i+=3) {
            if(r.result==SLICKS_SETUP_SAVED || r.result==SLICKS_SETUP_SAVED_CLEANUP_PENDING)
                assert(equals(i,next,sizeof next));
            else assert(equals(i,old,sizeof old) || equals(i+2,old,sizeof old));
            if(r.result==SLICKS_SETUP_SAVE_FAILED) assert(equals(i,old,sizeof old));
            if(r.result==SLICKS_SETUP_SAVED || r.result==SLICKS_SETUP_SAVE_FAILED)
                assert(!files[i+1].present && !files[i+2].present);
        }
        assert(!allocations && !allocation_calls); ++cases;
    }
    printf("KickFS paired save: %u single/double-fault cases; no Rename or allocations\n",cases);
}
