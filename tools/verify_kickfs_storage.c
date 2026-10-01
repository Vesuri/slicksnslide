/* The real adapter's CFG/PLR pair on the WHDLoad whole-file bridge, plus the
 * unchanged-file skip shared with standalone saves. */
#define SLICKS_PAIR_STORAGE_TEST
#define main track_storage_verifier_main
#include "verify_track_storage.c"
#undef main
int main(void)
{
    unsigned char next[96];
    memset(next,0x72,sizeof next);
    unsigned limit=0,cases=0;
    for(unsigned first=0;first<=limit;++first)
    for(unsigned second=first;second<=limit;++second) {
        initialize(next,1);
        kickfs_mode=g_slicks_whdload=1;
        fail_first=first; fail_second=second;
        struct SlicksSetupStorageReport r={SLICKS_SETUP_SAVE_FAILED,0,0,0};
        const struct SlicksSetupFile pair[]={{paths[0],next,sizeof next},{paths[1],next,sizeof next}};
        r.result=store_files(pair,2,&r);
        if(!first && !second) limit=operation+4;
        if(r.result==SLICKS_SETUP_SAVED) assert(writes==2 && equals(0,next,sizeof next) && equals(1,next,sizeof next));
        else assert(r.path && writes>=1);
        assert(!allocations && !allocation_calls); ++cases;
    }
    printf("KickFS paired save: %u single/double-fault cases; two whole-file writes, no DOS file creation or allocations\n",cases);
    /* Load remembers the files on disk; a save rewrites only changed files. */
    fail_first=fail_second=0;
    for(unsigned mode=0;mode<2;++mode) {
        struct SlicksConfiguration configuration; struct SlicksPlayerProfiles profiles;
        memset(&configuration,0,sizeof configuration); memset(&profiles,0,sizeof profiles); profiles.count=4;
        static unsigned char buffer[SLICKS_AMIGA_SETUP_BYTES];
        int cfg=slicks_save_configuration(&configuration,buffer,142,SLICKS_AMIGA_CONFIG_SIGNATURE);
        int plr=slicks_save_player_profiles(&profiles,buffer+142,sizeof buffer-142);
        assert(cfg==142 && plr>0);
        initialize(buffer,142); kickfs_mode=g_slicks_whdload=(unsigned char)mode;
        memcpy(files[1].bytes,buffer+142,(size_t)plr); files[1].size=(unsigned)plr; files[1].present=1;
        struct SlicksSetupLoadReport load=slicks_amiga_load_setup(&configuration,&profiles,24,9,2026);
        assert(load.result==SLICKS_SETUP_LOADED && !allocations);
        /* Loading stamps the date into the configuration: the first save of a
         * day may differ; reloading that save must then be write-free. */
        struct SlicksSetupStorageReport saved=slicks_amiga_store_setup(&configuration,&profiles,
            SLICKS_AMIGA_CONFIG_SIGNATURE,buffer,sizeof buffer);
        assert(saved.result==SLICKS_SETUP_SAVED);
        load=slicks_amiga_load_setup(&configuration,&profiles,24,9,2026);
        assert(load.result==SLICKS_SETUP_LOADED && !allocations);
        writes=0; operation=0;
        saved=slicks_amiga_store_setup(&configuration,&profiles,SLICKS_AMIGA_CONFIG_SIGNATURE,buffer,sizeof buffer);
        assert(saved.result==SLICKS_SETUP_SAVED && !writes && !operation);
        configuration.options[3]^=1;
        saved=slicks_amiga_store_setup(&configuration,&profiles,SLICKS_AMIGA_CONFIG_SIGNATURE,buffer,sizeof buffer);
        assert(saved.result==SLICKS_SETUP_SAVED && writes==1 && equals(0,buffer,142) && equals(1,buffer+142,(unsigned)plr));
        saved=slicks_amiga_store_setup(&configuration,&profiles,SLICKS_AMIGA_CONFIG_SIGNATURE,buffer,sizeof buffer);
        assert(saved.result==SLICKS_SETUP_SAVED && writes==1);
        /* A failed save forgets both files, so Retry writes the pair. */
        profiles.setting[3]^=1; g_slicks_diag_write_fault=1;
        saved=slicks_amiga_store_setup(&configuration,&profiles,SLICKS_AMIGA_CONFIG_SIGNATURE,buffer,sizeof buffer);
        assert(saved.result==SLICKS_SETUP_SAVE_FAILED && !g_slicks_diag_write_fault);
        writes=0;
        saved=slicks_amiga_store_setup(&configuration,&profiles,SLICKS_AMIGA_CONFIG_SIGNATURE,buffer,sizeof buffer);
        assert(saved.result==SLICKS_SETUP_SAVED && writes==2 && equals(1,buffer+142,(unsigned)plr));
    }
    puts("Setup save: unchanged CFG/PLR are not rewritten; one changed file costs one write; failure retries both");
    return 0;
}
