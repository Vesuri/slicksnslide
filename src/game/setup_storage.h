#ifndef SLICKS_SETUP_STORAGE_H
#define SLICKS_SETUP_STORAGE_H

/* Platform file transaction for one track or the two original setup streams. No game
 * state or serialization policy lives here. Run only with the OS available.
 * Temporary/backup names must be distinct and in the destination directory.
 * A leftover temporary/backup file is never silently overwritten. */
struct SlicksSetupFile {
    const char *path,*temporary,*backup;
    const unsigned char *bytes;
    unsigned long size;
};
struct SlicksSetupFileOps {
    int (*exists)(void *,const char *); /* 0 absent, 1 present, -1 error */
    /* Create a previously absent file, write all bytes, flush/close and
     * report errors: -1 may leave our partial file, -2 created nothing.
     * The distinction prevents cleanup removing a file we never owned. */
    int (*write)(void *,const char *,const unsigned char *,unsigned long);
    int (*rename)(void *,const char *,const char *); /* must not replace */
    int (*remove)(void *,const char *); /* absent is success */
    void *context;
};
enum SlicksSetupSaveResult {
    SLICKS_SETUP_SAVED=0,
    SLICKS_SETUP_SAVE_FAILED=1, /* previous pair restored, may retry */
    SLICKS_SETUP_RECOVERY_REQUIRED=2, /* preserve files for recovery */
    SLICKS_SETUP_SAVED_CLEANUP_PENDING=3 /* new pair committed, old backup remains */
};

/* One or two caller-owned streams. Transactional under reported I/O failures;
 * count is bounded so temporary transaction state stays on the small stack.
 * This is not a claim of atomic
 * two-file publication during a power loss. Startup must detect leftovers
 * and report recovery rather than loading a possibly mixed generation. */
static inline enum SlicksSetupSaveResult slicks_store_files(
    const struct SlicksSetupFile *files,unsigned count,const struct SlicksSetupFileOps *ops)
{
    if(!files || !ops || count<1 || count>2) return SLICKS_SETUP_SAVE_FAILED;
    unsigned char existed[2]={0},staged[2]={0},backed[2]={0},installed[2]={0};
    void *c=ops->context;
    for(unsigned i=0;i<count;++i) {
        if(!files[i].bytes || !files[i].size) return SLICKS_SETUP_SAVE_FAILED;
        int old=ops->exists(c,files[i].path);
        int temporary=ops->exists(c,files[i].temporary);
        int backup=ops->exists(c,files[i].backup);
        if(old<0 || temporary<0 || backup<0) return SLICKS_SETUP_SAVE_FAILED;
        if(temporary || backup) return SLICKS_SETUP_RECOVERY_REQUIRED;
        existed[i]=(unsigned char)old;
    }
    for(unsigned i=0;i<count;++i) {
        int result=ops->write(c,files[i].temporary,files[i].bytes,files[i].size);
        staged[i]=(unsigned char)(result!=-2);
        if(result) goto rollback;
    }
    for(unsigned i=0;i<count;++i) if(existed[i]) {
        if(ops->rename(c,files[i].path,files[i].backup)) goto rollback;
        backed[i]=1;
    }
    for(unsigned i=0;i<count;++i) {
        if(ops->rename(c,files[i].temporary,files[i].path)) goto rollback;
        staged[i]=0; installed[i]=1;
    }
    {
        unsigned cleanup=0;
        for(unsigned i=0;i<count;++i) if(backed[i])
            if(ops->remove(c,files[i].backup)) cleanup=1;
        return cleanup?SLICKS_SETUP_SAVED_CLEANUP_PENDING:SLICKS_SETUP_SAVED;
    }
rollback:
    {
        unsigned recovery=0;
        for(unsigned j=count;j>0;--j) {
            unsigned i=j-1;
            if(installed[i] && ops->remove(c,files[i].path)) {
                recovery=1; /* Do not overwrite a destination whose removal failed. */
            } else if(backed[i] && ops->rename(c,files[i].backup,files[i].path)) recovery=1;
            if(staged[i] && ops->remove(c,files[i].temporary)) recovery=1;
        }
        return recovery?SLICKS_SETUP_RECOVERY_REQUIRED:SLICKS_SETUP_SAVE_FAILED;
    }
}
static inline enum SlicksSetupSaveResult slicks_store_setup_pair(
    const struct SlicksSetupFile files[2],const struct SlicksSetupFileOps *ops)
{ return slicks_store_files(files,2,ops); }
#endif
