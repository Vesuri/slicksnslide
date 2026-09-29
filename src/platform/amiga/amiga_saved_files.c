#ifndef SLICKS_SAVED_FILES_HOST_TEST
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/exec.h>
#endif
#include "amiga_saved_files.h"
unsigned char g_slicks_diag_saved_lock_failure;
long g_slicks_diag_saved_lock_error;
int slicks_saved_file_path(char path[13],const unsigned char *name)
{
    unsigned n=0;
    if(!name || !path) return -1;
    while(n<9 && name[n]) {
        unsigned char c=name[n];
        if(!((c>='A' && c<='Z') || (c>='a' && c<='z') ||
             (c>='0' && c<='9') || c=='_' || c=='-')) return -1;
        ++n;
    }
    if(!n || n>8) return -1;
    for(unsigned i=0;i<n;++i) path[i]=(char)name[i];
    path[n++]='.'; path[n++]='S'; path[n++]='S'; path[n++]='S'; path[n]=0;
    return 0;
}
int slicks_amiga_saved_file_exists(const char *path)
{
    struct Process *process=(struct Process *)FindTask(0);
    APTR window=process->pr_WindowPtr; process->pr_WindowPtr=(APTR)-1;
    BPTR lock=Lock((CONST_STRPTR)path,ACCESS_READ);
    int result=-1;
    if(lock) {
        struct FileInfoBlock info __attribute__((aligned(4)));
        if(Examine(lock,&info) && info.fib_DirEntryType<0) result=1;
        UnLock(lock);
    } else if(IoErr()==ERROR_OBJECT_NOT_FOUND) result=0;
    process->pr_WindowPtr=window; return result;
}
int slicks_amiga_saved_files(unsigned char names[40][9])
{
    struct Process *process=(struct Process *)FindTask(0);
    APTR window=process->pr_WindowPtr; process->pr_WindowPtr=(APTR)-1;
    /* Explicit native fixture only: traverse a regular asset as a directory
     * so AmigaDOS itself rejects Lock, rather than fabricating an I/O result. */
    unsigned char fail_lock=g_slicks_diag_saved_lock_failure;
    g_slicks_diag_saved_lock_failure=0;
    BPTR lock=Lock((CONST_STRPTR)(fail_lock?"SLICKS.000/scan-failure":""),ACCESS_READ);
    if(fail_lock) g_slicks_diag_saved_lock_error=lock?0:IoErr();
    int count=0,result=-1;
    struct FileInfoBlock info __attribute__((aligned(4)));
    if(!lock || !Examine(lock,&info)) goto done;
    while(ExNext(lock,&info)) {
        if(info.fib_DirEntryType>=0) continue;
        unsigned n=0; while(n<13 && info.fib_FileName[n]) ++n;
        if(n<5 || n>12 || info.fib_FileName[n-4]!='.') continue;
        if((info.fib_FileName[n-3]&~32)!='S' || (info.fib_FileName[n-2]&~32)!='S' ||
           (info.fib_FileName[n-1]&~32)!='S') continue;
        unsigned char name[9]={0}; char path[13];
        for(unsigned i=0;i<n-4;++i) name[i]=(unsigned char)info.fib_FileName[i];
        if(slicks_saved_file_path(path,name)) continue;
        if(count==40) { result=-2; goto done; } /* Never silently hide saves. */
        for(unsigned i=0;i<9;++i) names[count][i]=name[i];
        ++count;
    }
    if(IoErr()==ERROR_NO_MORE_ENTRIES) result=count;
done:
    if(lock) UnLock(lock);
    process->pr_WindowPtr=window; return result;
}
void slicks_amiga_saved_files_refresh(struct SlicksAmigaSavedFilesCache *cache)
{
    unsigned char names[40][9];
    int count=slicks_amiga_saved_files(names);
    if(count>=0) {
        for(unsigned i=0;i<40;++i) for(unsigned j=0;j<9;++j)
            cache->names[i][j]=i<(unsigned)count?names[i][j]:0;
    }
    cache->count=count;
}
int slicks_amiga_saved_file_delete(const char *path)
{
    /* Recovery files may contain the last good version: never delete them. */
    char side[17]; unsigned n=0;
    while(path[n] && n<12) { side[n]=path[n]; ++n; }
    if(path[n] || n<5) return -1;
    side[n]='.'; side[n+1]='n'; side[n+2]='e'; side[n+3]='w'; side[n+4]=0;
    if(slicks_amiga_saved_file_exists(side)!=0) return -1;
    side[n+1]='b'; side[n+2]='a'; side[n+3]='k';
    if(slicks_amiga_saved_file_exists(side)!=0) return -1;
    struct Process *process=(struct Process *)FindTask(0);
    APTR window=process->pr_WindowPtr; process->pr_WindowPtr=(APTR)-1;
    int result=DeleteFile((CONST_STRPTR)path)?0:-1;
    process->pr_WindowPtr=window; return result;
}
