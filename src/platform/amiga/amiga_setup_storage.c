#ifndef SLICKS_SETUP_STORAGE_HOST_TEST
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#endif
#include "amiga_setup_storage.h"
#include "../../game/track_records.h"
#include "../../ui/screen_capture.h"
unsigned char g_slicks_diag_backup_protect;
unsigned char g_slicks_diag_track_read_fault,g_slicks_diag_track_read_reached;
unsigned char g_slicks_diag_record_write_alloc_fault,g_slicks_diag_record_write_alloc_reached;

static void failure(struct SlicksSetupStorageReport *r,const char *path,LONG error)
{
    if(!r->path) { r->path=path; r->io_error=error; }
}
static int exists(void *context,const char *path)
{
    struct SlicksSetupStorageReport *r=context;
    BPTR lock=Lock((CONST_STRPTR)path,ACCESS_READ);
    if(!lock) {
        LONG error=IoErr();
        if(error==ERROR_OBJECT_NOT_FOUND) return 0;
        failure(r,path,error); return -1;
    }
    struct FileInfoBlock info __attribute__((aligned(4)));
    LONG ok=Examine(lock,&info),error=ok?0:IoErr();
    UnLock(lock);
    if(!ok || info.fib_DirEntryType>=0) {
        failure(r,path,ok?ERROR_OBJECT_WRONG_TYPE:error); return -1;
    }
    return 1;
}
static int write_new(void *context,const char *path,const unsigned char *bytes,unsigned long size)
{
    struct SlicksSetupStorageReport *r=context;
    int present=exists(context,path);
    if(present) { if(present>0) failure(r,path,ERROR_OBJECT_EXISTS); return -2; }
    BPTR file=Open((CONST_STRPTR)path,MODE_NEWFILE);
    if(!file) { failure(r,path,IoErr()); return -2; }
    unsigned long at=0; int failed=0;
    while(at<size) {
        LONG written=Write(file,(APTR)(bytes+at),(LONG)(size-at));
        if(written<=0) { failure(r,path,written<0?IoErr():ERROR_DISK_FULL); failed=1; break; }
        at+=(unsigned long)written;
    }
    if(!failed && !Flush(file)) { failure(r,path,IoErr()); failed=1; }
    if(!Close(file)) { failure(r,path,IoErr()); failed=1; }
    return failed?-1:0;
}
static int rename_file(void *context,const char *from,const char *to)
{
    struct SlicksSetupStorageReport *r=context;
    int present=exists(context,to);
    if(present) { if(present>0) failure(r,to,ERROR_OBJECT_EXISTS); return -1; }
    if(!Rename((CONST_STRPTR)from,(CONST_STRPTR)to)) { failure(r,from,IoErr()); return -1; }
    return 0;
}
static int remove_file(void *context,const char *path)
{
#ifndef SLICKS_SETUP_STORAGE_HOST_TEST
    /* CHAMPSAVB only, in its private fixture directory: make AmigaDOS reject
     * cleanup after the real overwrite transaction has installed the save. */
    if(g_slicks_diag_backup_protect) {
        const char *target="E2E.SSS.bak";
        unsigned i=0; while(path[i] && path[i]==target[i]) ++i;
        if(!path[i] && !target[i] && SetProtection((CONST_STRPTR)path,FIBF_DELETE))
            g_slicks_diag_backup_protect=0;
    }
#endif
    if(DeleteFile((CONST_STRPTR)path)) return 0;
    LONG error=IoErr(); if(error==ERROR_OBJECT_NOT_FOUND) return 0;
    failure(context,path,error); return -1;
}
/* The game owns the failure/retry UI. A DOS write-protection requester can
 * otherwise block Open indefinitely behind the restored system display.
 * Suppress requesters only for the transaction and restore the caller's
 * process setting on every result, including rollback/recovery failures. */
static enum SlicksSetupSaveResult store_files(const struct SlicksSetupFile *files,
    unsigned count,const struct SlicksSetupFileOps *ops)
{
#ifndef SLICKS_SETUP_STORAGE_HOST_TEST
    struct Process *process=(struct Process *)FindTask(0);
    APTR window=process->pr_WindowPtr;
    process->pr_WindowPtr=(APTR)-1;
#endif
    enum SlicksSetupSaveResult result=slicks_store_files(files,count,ops);
#ifndef SLICKS_SETUP_STORAGE_HOST_TEST
    process->pr_WindowPtr=window;
#endif
    return result;
}
struct SlicksSetupStorageReport slicks_amiga_store_capture(
    const unsigned char *pixels,const unsigned char *palette,
    unsigned char *buffer,unsigned long capacity)
{
    static char path[]="TUNING00.BMP";
    char temporary[]="TUNING00.BMP.new",backup[]="TUNING00.BMP.bak";
    struct SlicksSetupStorageReport report={SLICKS_SETUP_SAVE_FAILED,0,path};
    if(!buffer || capacity<SLICKS_CAPTURE_SIZE) {
        report.io_error=ERROR_NO_FREE_STORE; return report;
    }
#ifndef SLICKS_SETUP_STORAGE_HOST_TEST
    struct Process *process=(struct Process *)FindTask(0);
    APTR window=process->pr_WindowPtr; process->pr_WindowPtr=(APTR)-1;
#endif
    if(slicks_encode_capture(buffer,SLICKS_CAPTURE_SIZE,pixels,palette)) goto done;
    for(unsigned n=0;n<99;++n) {
        path[6]=temporary[6]=backup[6]=(char)('0'+n/10);
        path[7]=temporary[7]=backup[7]=(char)('0'+n%10);
        int present=exists(&report,path);
        if(present<0) goto done;
        if(present) continue;
        const struct SlicksSetupFile file={path,temporary,backup,buffer,SLICKS_CAPTURE_SIZE};
        const struct SlicksSetupFileOps ops={exists,write_new,rename_file,remove_file,&report};
        report.result=store_files(&file,1,&ops);
        goto done;
    }
    report.io_error=ERROR_OBJECT_EXISTS;
done:
    report.path=path;
#ifndef SLICKS_SETUP_STORAGE_HOST_TEST
    process->pr_WindowPtr=window;
#endif
    return report;
}

struct SlicksSetupStorageReport slicks_amiga_store_setup(
    const struct SlicksConfiguration *configuration,const struct SlicksPlayerProfiles *profiles,unsigned char signature,
    unsigned char *buffer,unsigned long buffer_size)
{
    struct SlicksSetupStorageReport report={SLICKS_SETUP_SAVE_FAILED,0,0};
    const unsigned long capacity=3UL+58UL*(SLICKS_PROFILE_MAX-3);
    if(!buffer || buffer_size<SLICKS_AMIGA_SETUP_BYTES) {
        report.io_error=ERROR_NO_FREE_STORE; return report;
    }
    int cfg=slicks_save_configuration(configuration,buffer,142,signature);
    int plr=slicks_save_player_profiles(profiles,buffer+142,capacity);
    if(cfg>0 && plr>0) {
        const struct SlicksSetupFile files[2]={
            {"SLICKS.CFG","SLICKS.CFG.new","SLICKS.CFG.bak",buffer,(unsigned long)cfg},
            {"SLICKS.PLR","SLICKS.PLR.new","SLICKS.PLR.bak",buffer+142,(unsigned long)plr}};
        const struct SlicksSetupFileOps ops={exists,write_new,rename_file,remove_file,&report};
        report.result=store_files(files,2,&ops);
    }
    return report;
}

/* Read to EOF, including a one-byte overflow probe. Do not mistake an I/O
 * error for a missing file, or a maximum-length prefix for a complete file. */
static long read_file(struct SlicksSetupLoadReport *r,const char *path,
    unsigned char *buffer,unsigned long capacity)
{
    BPTR file=Open((CONST_STRPTR)path,MODE_OLDFILE);
    if(!file) {
        LONG error=IoErr();
        if(error==ERROR_OBJECT_NOT_FOUND) return -1;
        r->result=SLICKS_SETUP_LOAD_IO_ERROR; r->io_error=error; r->path=path;
        return -2;
    }
    unsigned long at=0;
    while(at<capacity) {
        LONG got=Read(file,buffer+at,(LONG)(capacity-at));
#ifndef SLICKS_SETUP_STORAGE_HOST_TEST
        /* Armed immediately before the isolated track-list startup load.
         * Real data reaches staging first; none may reach the cached view. */
        if(g_slicks_diag_track_read_fault==1 && got>0) {
            g_slicks_diag_track_read_fault=0;g_slicks_diag_track_read_reached=1;
            got=-1;SetIoErr(ERROR_SEEK_ERROR);
        }
#endif
        if(got<0) {
            r->result=SLICKS_SETUP_LOAD_IO_ERROR; r->io_error=IoErr(); break;
        }
        if(!got) break;
        at+=(unsigned long)got;
    }
    if(r->result==SLICKS_SETUP_LOADED && at==capacity) {
        unsigned char extra;
        LONG got=Read(file,&extra,1);
        if(got<0) { r->result=SLICKS_SETUP_LOAD_IO_ERROR; r->io_error=IoErr(); }
        else if(got) r->result=SLICKS_SETUP_LOAD_INVALID;
    }
    LONG closed=Close(file);
#ifndef SLICKS_SETUP_STORAGE_HOST_TEST
    /* Always close the actual handle, even when simulating a failed result. */
    if(g_slicks_diag_track_read_fault==2) {
        g_slicks_diag_track_read_fault=0;g_slicks_diag_track_read_reached=2;
        closed=0;SetIoErr(ERROR_SEEK_ERROR);
    }
#endif
    if(!closed && r->result==SLICKS_SETUP_LOADED) {
        r->result=SLICKS_SETUP_LOAD_IO_ERROR; r->io_error=IoErr();
    }
    if(r->result!=SLICKS_SETUP_LOADED) { r->path=path; return -2; }
    return (long)at;
}

/* Decode into unpublished caller-owned staging. The caller decides whether
 * and when to publish it; failure may change staging, never the live cache. */
static struct SlicksSetupLoadReport load_track_lists_work(
    unsigned char *buffer,unsigned long capacity,struct SlicksTrackLists *view)
{
    struct SlicksSetupLoadReport report={SLICKS_SETUP_LOADED,0,0,0,0};
    struct SlicksSetupStorageReport io={SLICKS_SETUP_SAVE_FAILED,0,0};
    if(!buffer || !view || capacity<8) { report.result=SLICKS_SETUP_LOAD_INVALID; return report; }
    if(capacity>SLICKS_AMIGA_TRACK_LIST_BYTES) capacity=SLICKS_AMIGA_TRACK_LIST_BYTES;
    const char *leftovers[]={"SLICKS.TRK.new","SLICKS.TRK.bak"};
    for(unsigned i=0;i<2;++i) {
        int present=exists(&io,leftovers[i]);
        if(present) {
            report.result=present>0?SLICKS_SETUP_LOAD_RECOVERY:SLICKS_SETUP_LOAD_IO_ERROR;
            report.io_error=io.io_error; report.path=leftovers[i]; return report;
        }
    }
    long size=read_file(&report,"SLICKS.TRK",buffer,capacity);
    if(size==-1) {
        const unsigned char empty[8]={'S','S','T','r','k',26,0,0};
        for(unsigned i=0;i<8;++i) buffer[i]=empty[i];
        size=8;
    }
    struct SlicksTrackLists next;
    if(size>=0 && slicks_track_lists_open(&next,buffer,(unsigned long)size)) {
        report.result=SLICKS_SETUP_LOAD_INVALID; report.path="SLICKS.TRK";
    }
    if(size>=0 && report.result==SLICKS_SETUP_LOADED) {
        *view=next;
    }
    return report;
}
struct SlicksSetupLoadReport slicks_amiga_load_track_lists(
    unsigned char *out,unsigned long capacity,struct SlicksTrackLists *view)
{
    struct SlicksSetupLoadReport report={SLICKS_SETUP_LOAD_INVALID,0,0,0,0};
    if(!out || !view || capacity<8) return report;
    if(capacity>SLICKS_AMIGA_TRACK_LIST_BYTES) capacity=SLICKS_AMIGA_TRACK_LIST_BYTES;
    unsigned char *buffer=AllocMem(capacity,MEMF_ANY);
    if(!buffer) {
        report.result=SLICKS_SETUP_LOAD_IO_ERROR; report.io_error=ERROR_NO_FREE_STORE;
        report.path="SLICKS.TRK"; return report;
    }
    struct SlicksTrackLists next;
    report=load_track_lists_work(buffer,capacity,&next);
    if(report.result==SLICKS_SETUP_LOADED) {
        for(unsigned long i=0;i<next.size;++i) out[i]=buffer[i];
        next.bytes=out; *view=next;
    }
    FreeMem(buffer,capacity);
    return report;
}
void slicks_amiga_track_list_cache_free(struct SlicksAmigaTrackListCache *cache)
{
    if(cache->view.bytes) FreeMem((APTR)cache->view.bytes,cache->view.size);
    cache->view=(struct SlicksTrackLists){0,0,0};
    cache->report=(struct SlicksSetupLoadReport){SLICKS_SETUP_LOAD_INVALID,0,"SLICKS.TRK",0,0};
}
void slicks_amiga_track_list_cache_refresh(struct SlicksAmigaTrackListCache *cache,
    unsigned char *work,unsigned long capacity)
{
    struct SlicksTrackLists next;
    cache->report=(struct SlicksSetupLoadReport){SLICKS_SETUP_LOAD_IO_ERROR,ERROR_NO_FREE_STORE,"SLICKS.TRK",0,0};
    if(!work || capacity<SLICKS_AMIGA_TRACK_LIST_BYTES) return;
    cache->report=load_track_lists_work(work,SLICKS_AMIGA_TRACK_LIST_BYTES,&next);
    if(cache->report.result==SLICKS_SETUP_LOADED) {
        unsigned char *bytes=AllocMem(next.size,MEMF_ANY);
        if(!bytes) {
            cache->report.result=SLICKS_SETUP_LOAD_IO_ERROR;
            cache->report.io_error=ERROR_NO_FREE_STORE; cache->report.path="SLICKS.TRK";
        } else {
            for(unsigned long i=0;i<next.size;++i) bytes[i]=work[i];
            if(cache->view.bytes) FreeMem((APTR)cache->view.bytes,cache->view.size);
            next.bytes=bytes; cache->view=next;
        }
    }
}
struct SlicksSetupStorageReport slicks_amiga_store_track_lists(
    const struct SlicksTrackLists *lists,int remove,const unsigned char *title,
    const struct SlicksTrackPlaylist *playlist,unsigned total,
    const unsigned char *(*name)(void *,unsigned),void *context,
    unsigned char *buffer,unsigned long capacity)
{
    struct SlicksSetupStorageReport report={SLICKS_SETUP_SAVE_FAILED,0,0};
    if(!buffer || capacity<SLICKS_AMIGA_TRACK_LIST_BYTES) { report.io_error=ERROR_NO_FREE_STORE; report.path="SLICKS.TRK"; return report; }
    long size=slicks_track_lists_write(lists,remove,title,playlist,total,name,context,
        buffer,SLICKS_AMIGA_TRACK_LIST_BYTES);
    if(size<0) { report.path="SLICKS.TRK"; report.io_error=ERROR_OBJECT_WRONG_TYPE; }
    else {
        const struct SlicksSetupFile file={"SLICKS.TRK","SLICKS.TRK.new","SLICKS.TRK.bak",buffer,(unsigned long)size};
        const struct SlicksSetupFileOps ops={exists,write_new,rename_file,remove_file,&report};
        report.result=store_files(&file,1,&ops);
    }
    return report;
}

struct SlicksSetupLoadReport slicks_amiga_load_saved_game(const char *path,
    struct SlicksSavedGame *game,unsigned char (*tracks)[8],unsigned capacity)
{
    struct SlicksSetupLoadReport report={SLICKS_SETUP_LOADED,0,path,0,0};
    struct SlicksSetupStorageReport io={SLICKS_SETUP_SAVE_FAILED,0,0};
    unsigned length=0;
    if(path) while(length<120 && path[length]) ++length;
    if(!game || !tracks || !length || length>=120 || capacity>SLICKS_SAVED_GAME_TRACK_MAX) {
        report.result=SLICKS_SETUP_LOAD_INVALID; return report;
    }
    char leftover[124];
    for(unsigned i=0;i<length;++i) leftover[i]=path[i];
    const char *suffixes[]={".new",".bak"};
#ifndef SLICKS_SETUP_STORAGE_HOST_TEST
    struct Process *process=(struct Process *)FindTask(0);
    APTR window=process->pr_WindowPtr;
    process->pr_WindowPtr=(APTR)-1;
#endif
    for(unsigned suffix=0;suffix<2;++suffix) {
        for(unsigned i=0;i<5;++i) leftover[length+i]=suffixes[suffix][i];
        int present=exists(&io,leftover);
        if(present) {
            report.result=present>0?SLICKS_SETUP_LOAD_RECOVERY:SLICKS_SETUP_LOAD_IO_ERROR;
            report.io_error=io.io_error; goto done;
        }
    }
    unsigned long buffer_size=6UL+8UL*capacity+4*53;
    unsigned char *buffer=AllocMem(buffer_size,MEMF_ANY);
    if(!buffer) {
        report.result=SLICKS_SETUP_LOAD_IO_ERROR; report.io_error=ERROR_NO_FREE_STORE;
        goto done;
    }
    long size=read_file(&report,path,buffer,buffer_size);
    if(size==-1) {
        report.result=SLICKS_SETUP_LOAD_IO_ERROR; report.io_error=ERROR_OBJECT_NOT_FOUND;
    } else if(size>=0 && slicks_load_game_bytes(game,tracks,capacity,buffer,(unsigned long)size))
        report.result=SLICKS_SETUP_LOAD_INVALID;
    FreeMem(buffer,buffer_size);
done:
#ifndef SLICKS_SETUP_STORAGE_HOST_TEST
    process->pr_WindowPtr=window;
#endif
    report.path=path;
    return report;
}

struct SlicksSetupStorageReport slicks_amiga_store_saved_game(const char *path,const struct SlicksSavedGame *game)
{
    struct SlicksSetupStorageReport report={SLICKS_SETUP_SAVE_FAILED,0,path};
    long size=slicks_saved_game_size(game);
    if(!path || size<0) { report.io_error=ERROR_OBJECT_WRONG_TYPE; return report; }
    unsigned length=0; while(length<120 && path[length]) ++length;
    if(!length || length>=120) { report.io_error=ERROR_OBJECT_WRONG_TYPE; return report; }
    char temporary[124],backup[124];
    for(unsigned i=0;i<length;++i) temporary[i]=backup[i]=path[i];
    const char *suffixes[]={".new",".bak"};
    for(unsigned i=0;i<5;++i) { temporary[length+i]=suffixes[0][i]; backup[length+i]=suffixes[1][i]; }
    unsigned char *bytes=AllocMem((unsigned long)size,MEMF_ANY);
    if(!bytes) { report.io_error=ERROR_NO_FREE_STORE; return report; }
    if(slicks_save_game_bytes(game,bytes,(unsigned long)size)!=size) report.io_error=ERROR_OBJECT_WRONG_TYPE;
    else {
        const struct SlicksSetupFile file={path,temporary,backup,bytes,(unsigned long)size};
        const struct SlicksSetupFileOps ops={exists,write_new,rename_file,remove_file,&report};
        report.result=store_files(&file,1,&ops);
    }
    FreeMem(bytes,(unsigned long)size);
    /* The filesystem callbacks may have recorded a stack-local suffix path. */
    report.path=path;
    return report;
}

struct SlicksSetupStorageReport slicks_amiga_store_track_records(const char *path,
    const struct SlicksTrackRecords *source,unsigned char *changed,
    unsigned char *buffer,unsigned long capacity)
{
    struct SlicksSetupStorageReport report={SLICKS_SETUP_SAVE_FAILED,0,path};
    if(changed) *changed=0;
    if(!path || !source || !changed) return report;
    if(g_slicks_diag_record_write_alloc_fault) {
        g_slicks_diag_record_write_alloc_fault=0;
        g_slicks_diag_record_write_alloc_reached=1; buffer=0;
    }
    if(!buffer || capacity<8192) { report.io_error=ERROR_NO_FREE_STORE; return report; }
    unsigned length=0; while(length<120 && path[length]) ++length;
    if(!length || length>=120) return report;
    char temporary[124],backup[124];
    for(unsigned i=0;i<length;++i) temporary[i]=backup[i]=path[i];
    const char *suffixes[]={".new",".bak"};
    for(unsigned i=0;i<5;++i) { temporary[length+i]=suffixes[0][i]; backup[length+i]=suffixes[1][i]; }
    report.path=0;
    int staged=exists(&report,temporary),backed=exists(&report,backup);
    if(staged<0 || backed<0) goto done;
    if(staged || backed) { report.result=SLICKS_SETUP_RECOVERY_REQUIRED; goto done; }
    int present=exists(&report,path);
    if(present!=1) { if(!present) report.io_error=ERROR_OBJECT_NOT_FOUND; goto done; }
    struct SlicksSetupLoadReport load={SLICKS_SETUP_LOADED,0,0,0,0};
    long size=read_file(&load,path,buffer,8192);
    if(size<6 || load.result!=SLICKS_SETUP_LOADED || buffer[2]!='S' || buffer[3]!='S' || buffer[4]!=0x7e) {
        report.io_error=load.io_error?load.io_error:ERROR_OBJECT_WRONG_TYPE;
    } else {
        struct SlicksTrackRecords records=*source;
        int encoded=slicks_write_track_records(buffer,(unsigned long)size,&records);
        if(encoded<0) report.io_error=ERROR_OBJECT_WRONG_TYPE;
        else if(!encoded) report.result=SLICKS_SETUP_SAVED; /* Original old-format no-op. */
        else {
            const struct SlicksSetupFile file={path,temporary,backup,buffer,(unsigned long)size};
            const struct SlicksSetupFileOps ops={exists,write_new,rename_file,remove_file,&report};
            report.result=store_files(&file,1,&ops);
            *changed=(unsigned char)(report.result==SLICKS_SETUP_SAVED || report.result==SLICKS_SETUP_SAVED_CLEANUP_PENDING);
        }
    }
done:
    report.path=path; /* Never return pointers into local suffix buffers. */
    return report;
}

struct SlicksSetupStorageReport slicks_amiga_clear_track_records(const char *path,unsigned char *changed,
    unsigned char *buffer,unsigned long capacity)
{
    struct SlicksTrackRecords records;
    slicks_clear_track_records(&records);
    return slicks_amiga_store_track_records(path,&records,changed,buffer,capacity);
}

struct SlicksSetupLoadReport slicks_amiga_load_setup(
    struct SlicksConfiguration *configuration,struct SlicksPlayerProfiles *profiles,
    unsigned short date_first,unsigned short date_second,unsigned short date_third)
{
    struct SlicksSetupLoadReport report={SLICKS_SETUP_LOADED,0,0,0,0};
    struct SlicksSetupStorageReport io={SLICKS_SETUP_SAVE_FAILED,0,0};
    const char *leftovers[]={"SLICKS.CFG.new","SLICKS.CFG.bak",
        "SLICKS.PLR.new","SLICKS.PLR.bak"};
    for(unsigned i=0;i<4;++i) {
        int present=exists(&io,leftovers[i]);
        if(present) {
            report.result=present>0?SLICKS_SETUP_LOAD_RECOVERY:SLICKS_SETUP_LOAD_IO_ERROR;
            report.path=leftovers[i]; report.io_error=io.io_error; return report;
        }
    }
    const unsigned long capacity=3UL+58UL*(SLICKS_PROFILE_MAX-3);
    unsigned char *buffer=AllocMem(capacity,MEMF_ANY);
    struct SlicksPlayerProfiles *next=AllocMem(sizeof(*next),MEMF_ANY);
    struct SlicksConfiguration config=*configuration;
    if(!buffer || !next) {
        report.result=SLICKS_SETUP_LOAD_IO_ERROR; report.io_error=ERROR_NO_FREE_STORE;
        goto done;
    }
    *next=*profiles;
    long size=read_file(&report,"SLICKS.CFG",buffer,142);
    if(size==-2) goto done;
    report.configuration_present=size>=0;
    if(size>=0 && (size!=142 || buffer[0]!=15)) {
        report.result=SLICKS_SETUP_LOAD_INVALID; report.path="SLICKS.CFG"; goto done;
    }
    if(size>=0 && buffer[1]!=SLICKS_AMIGA_CONFIG_SIGNATURE) {
        report.result=SLICKS_SETUP_LOAD_FOREIGN; report.path="SLICKS.CFG"; goto done;
    }
    slicks_load_configuration(&config,size<0?0:buffer,size<0?0:(unsigned long)size,
        SLICKS_AMIGA_CONFIG_SIGNATURE,date_first,date_second,date_third);
    size=read_file(&report,"SLICKS.PLR",buffer,capacity);
    if(size==-2) goto done;
    report.profiles_present=size>=0;
    if(size>=0) {
        unsigned count=size>=3?((unsigned)buffer[1]<<8)|buffer[2]:0;
        count=(unsigned short)(count+3);
        unsigned long expected=3UL+58UL*(count>3?count-3:0);
        if(size<3 || buffer[0]!=0x97 || count<1 || count>SLICKS_PROFILE_MAX ||
            (unsigned long)size!=expected) {
            report.result=SLICKS_SETUP_LOAD_INVALID; report.path="SLICKS.PLR"; goto done;
        }
    }
    if(slicks_load_player_profiles(next,size<0?0:buffer,size<0?0:(unsigned long)size)<0) {
        report.result=SLICKS_SETUP_LOAD_INVALID; report.path="SLICKS.PLR"; goto done;
    }
    *configuration=config; *profiles=*next;
done:
    if(next) FreeMem(next,sizeof(*next));
    if(buffer) FreeMem(buffer,capacity);
    return report;
}
