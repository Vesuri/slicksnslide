#ifndef SLICKS_SETUP_STORAGE_H
#define SLICKS_SETUP_STORAGE_H

/* Platform file writes for one track, a save or the two original setup
 * streams. No game state or serialization policy lives here. Run only with
 * the OS available. Like the DOS original, each file is replaced in place
 * with one complete write: no temporary, backup or recovery files. */
struct SlicksSetupFile {
    const char *path;
    const unsigned char *bytes;
    unsigned long size;
};
struct SlicksSetupFileOps {
    /* Create or replace the file with all bytes in one operation, report
     * errors. A failure may leave the destination incomplete. */
    int (*write)(void *,const char *,const unsigned char *,unsigned long);
    void *context;
};
enum SlicksSetupSaveResult {
    SLICKS_SETUP_SAVED=0,
    SLICKS_SETUP_SAVE_FAILED=1 /* in-memory state retained, may retry */
};

/* One or two caller-owned streams, written in order. Stops at the first
 * failure; an earlier file of the pair may already be replaced. */
static inline enum SlicksSetupSaveResult slicks_store_files(
    const struct SlicksSetupFile *files,unsigned count,const struct SlicksSetupFileOps *ops)
{
    if(!files || !ops || count<1 || count>2) return SLICKS_SETUP_SAVE_FAILED;
    for(unsigned i=0;i<count;++i)
        if(!files[i].bytes || !files[i].size) return SLICKS_SETUP_SAVE_FAILED;
    for(unsigned i=0;i<count;++i)
        if(ops->write(ops->context,files[i].path,files[i].bytes,files[i].size))
            return SLICKS_SETUP_SAVE_FAILED;
    return SLICKS_SETUP_SAVED;
}
static inline enum SlicksSetupSaveResult slicks_store_setup_pair(
    const struct SlicksSetupFile files[2],const struct SlicksSetupFileOps *ops)
{ return slicks_store_files(files,2,ops); }
#endif
