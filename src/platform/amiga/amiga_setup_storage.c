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
unsigned char g_slicks_whdload;
/* Explicit native fixtures only: the next whole-file write targets a path
 * through a regular file, so AmigaDOS itself rejects it. Zero normally. */
unsigned char g_slicks_diag_write_fault;
#ifndef SLICKS_SETUP_STORAGE_HOST_TEST
struct WhdStorage {
    unsigned char magic[8]; unsigned short version,size;
    LONG (*save)(const char *,const unsigned char *,unsigned long,LONG *);
    volatile unsigned long *switches;
    unsigned long transactions,total_switches,max_switches,writes,max_write;
};
_Static_assert(sizeof(struct WhdStorage)==40,"WHDLoad storage descriptor ABI");
extern struct WhdStorage slicks_whd_storage;
int slicks_amiga_storage_create(void)
{
    return g_slicks_whdload && !slicks_whd_storage.save?-1:0;
}
static LONG whole_file_write(const char *path,const unsigned char *bytes,unsigned long size,LONG *error)
{
    ++slicks_whd_storage.writes;
    if(size>slicks_whd_storage.max_write) slicks_whd_storage.max_write=size;
    return slicks_whd_storage.save(path,bytes,size,error);
}
#endif
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
/* Create or replace the file with one complete write. Under WHDLoad this is
 * a single resload_SaveFile: no KickFS packets, existence checks or renames. */
static int write_whole(void *context,const char *path,const unsigned char *bytes,unsigned long size)
{
    struct SlicksSetupStorageReport *r=context;
    const char *target=path;
    if(g_slicks_diag_write_fault) { g_slicks_diag_write_fault=0; target="SLICKS.000/write-fault"; }
    if(g_slicks_whdload) {
        LONG error=0;
        if(whole_file_write(target,bytes,size,&error)) return 0;
        failure(r,path,error); return -1;
    }
    BPTR file=Open((CONST_STRPTR)target,MODE_NEWFILE);
    if(!file) { failure(r,path,IoErr()); return -1; }
    unsigned long at=0; int failed=0;
    while(at<size) {
        LONG written=Write(file,(APTR)(bytes+at),(LONG)(size-at));
        if(written<=0) { failure(r,path,written<0?IoErr():ERROR_DISK_FULL); failed=1; break; }
        at+=(unsigned long)written;
    }
    if(!Close(file) && !failed) { failure(r,path,IoErr()); failed=1; }
    return failed?-1:0;
}
/* The game owns the failure/retry UI. A DOS write-protection requester can
 * otherwise block Open indefinitely behind the restored system display.
 * Suppress requesters only for the writes and restore the caller's
 * process setting on every result. */
static enum SlicksSetupSaveResult store_files(const struct SlicksSetupFile *files,
    unsigned count,struct SlicksSetupStorageReport *report)
{
    const struct SlicksSetupFileOps ops={write_whole,report};
#ifndef SLICKS_SETUP_STORAGE_HOST_TEST
    struct Process *process=(struct Process *)FindTask(0);
    APTR window=process->pr_WindowPtr;
    process->pr_WindowPtr=(APTR)-1;
    unsigned long before=g_slicks_whdload?*slicks_whd_storage.switches:0;
#endif
    enum SlicksSetupSaveResult result=slicks_store_files(files,count,&ops);
#ifndef SLICKS_SETUP_STORAGE_HOST_TEST
    if(g_slicks_whdload) {
        unsigned long switches=*slicks_whd_storage.switches-before;
        ++slicks_whd_storage.transactions;
        slicks_whd_storage.total_switches+=switches;
        if(switches>slicks_whd_storage.max_switches) slicks_whd_storage.max_switches=switches;
    }
    process->pr_WindowPtr=window;
#endif
    return result;
}
struct SlicksSetupStorageReport slicks_amiga_store_capture(
    const unsigned char *pixels,const unsigned char *palette,
    unsigned char *buffer,unsigned long capacity)
{
    static char path[]="TUNING00.BMP";
    struct SlicksSetupStorageReport report={SLICKS_SETUP_SAVE_FAILED,0,path,0};
    if(!buffer || capacity<SLICKS_CAPTURE_SIZE) {
        report.io_error=ERROR_NO_FREE_STORE; return report;
    }
#ifndef SLICKS_SETUP_STORAGE_HOST_TEST
    struct Process *process=(struct Process *)FindTask(0);
    APTR window=process->pr_WindowPtr; process->pr_WindowPtr=(APTR)-1;
#endif
    report.path=0;
    if(slicks_encode_capture(buffer,SLICKS_CAPTURE_SIZE,pixels,palette)) goto done;
    for(unsigned n=0;n<99;++n) {
        path[6]=(char)('0'+n/10);
        path[7]=(char)('0'+n%10);
        int present=exists(&report,path);
        if(present<0) goto done;
        if(present) continue;
        const struct SlicksSetupFile file={path,buffer,SLICKS_CAPTURE_SIZE};
        report.result=store_files(&file,1,&report);
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

/* Last CFG/PLR bytes known to be on disk, from load or a successful save.
 * Quit and standings saves rewrite only a file whose bytes changed: under
 * WHDLoad every physical write costs an OS switch and its write delay. */
static struct { unsigned long size,crc; } setup_known[2];
static unsigned long setup_crc(const unsigned char *bytes,unsigned long size)
{
    unsigned long crc=0xffffffffUL;
    for(unsigned long i=0;i<size;++i) {
        crc^=bytes[i];
        for(unsigned bit=0;bit<8;++bit) crc=(crc>>1)^(0xedb88320UL&-(crc&1));
    }
    return ~crc&0xffffffffUL;
}
static void setup_remember(unsigned i,const unsigned char *bytes,unsigned long size)
{ setup_known[i].size=size; setup_known[i].crc=setup_crc(bytes,size); }
struct SlicksSetupStorageReport slicks_amiga_store_setup(
    const struct SlicksConfiguration *configuration,const struct SlicksPlayerProfiles *profiles,unsigned char signature,
    unsigned char *buffer,unsigned long buffer_size)
{
    struct SlicksSetupStorageReport report={SLICKS_SETUP_SAVE_FAILED,0,0,0};
    const unsigned long capacity=3UL+58UL*(SLICKS_PROFILE_MAX-3);
    if(!buffer || buffer_size<SLICKS_AMIGA_SETUP_BYTES) {
        report.io_error=ERROR_NO_FREE_STORE; return report;
    }
    int cfg=slicks_save_configuration(configuration,buffer,142,signature);
    int plr=slicks_save_player_profiles(profiles,buffer+142,capacity);
    if(cfg>0 && plr>0) {
        const struct SlicksSetupFile both[2]={
            {"SLICKS.CFG",buffer,(unsigned long)cfg},
            {"SLICKS.PLR",buffer+142,(unsigned long)plr}};
        struct SlicksSetupFile files[2]; unsigned char index[2]; unsigned count=0;
        for(unsigned i=0;i<2;++i)
            if(setup_known[i].size!=both[i].size || setup_known[i].crc!=setup_crc(both[i].bytes,both[i].size)) {
                index[count]=(unsigned char)i; files[count++]=both[i];
            }
        report.result=count?store_files(files,count,&report):SLICKS_SETUP_SAVED;
        /* A failed pair may be half written: forget both so Retry rewrites. */
        if(report.result!=SLICKS_SETUP_SAVED) setup_known[0].size=setup_known[1].size=0;
        else for(unsigned i=0;i<count;++i) setup_remember(index[i],files[i].bytes,files[i].size);
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
    if(!buffer || !view || capacity<8) { report.result=SLICKS_SETUP_LOAD_INVALID; return report; }
    if(capacity>SLICKS_AMIGA_TRACK_LIST_BYTES) capacity=SLICKS_AMIGA_TRACK_LIST_BYTES;
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
    unsigned char *out,unsigned long capacity,struct SlicksTrackLists *view,
    unsigned char *buffer,unsigned long buffer_capacity)
{
    struct SlicksSetupLoadReport report={SLICKS_SETUP_LOAD_INVALID,0,0,0,0};
    if(!out || !view || capacity<8) return report;
    if(capacity>SLICKS_AMIGA_TRACK_LIST_BYTES) capacity=SLICKS_AMIGA_TRACK_LIST_BYTES;
    if(!buffer || buffer_capacity<capacity) {
        report.result=SLICKS_SETUP_LOAD_IO_ERROR; report.io_error=ERROR_NO_FREE_STORE;
        report.path="SLICKS.TRK"; return report;
    }
    struct SlicksTrackLists next;
    report=load_track_lists_work(buffer,capacity,&next);
    if(report.result==SLICKS_SETUP_LOADED) {
        for(unsigned long i=0;i<next.size;++i) out[i]=buffer[i];
        next.bytes=out; *view=next;
    }
    return report;
}
int slicks_amiga_track_list_cache_create(struct SlicksAmigaTrackListCache *cache)
{
    if(!cache || cache->storage) return -1;
    cache->storage=AllocMem(SLICKS_AMIGA_TRACK_LIST_BYTES,MEMF_ANY);
    return cache->storage?0:-1;
}
void slicks_amiga_track_list_cache_free(struct SlicksAmigaTrackListCache *cache)
{
    if(cache->storage) FreeMem(cache->storage,SLICKS_AMIGA_TRACK_LIST_BYTES);
    cache->storage=0;
    cache->view=(struct SlicksTrackLists){0,0,0};
    cache->report=(struct SlicksSetupLoadReport){SLICKS_SETUP_LOAD_INVALID,0,"SLICKS.TRK",0,0};
}
void slicks_amiga_track_list_cache_refresh(struct SlicksAmigaTrackListCache *cache,
    unsigned char *work,unsigned long capacity)
{
    struct SlicksTrackLists next;
    cache->report=(struct SlicksSetupLoadReport){SLICKS_SETUP_LOAD_IO_ERROR,ERROR_NO_FREE_STORE,"SLICKS.TRK",0,0};
    if(!cache->storage || !work || work==cache->storage || capacity<SLICKS_AMIGA_TRACK_LIST_BYTES) return;
    cache->report=load_track_lists_work(work,SLICKS_AMIGA_TRACK_LIST_BYTES,&next);
    if(cache->report.result==SLICKS_SETUP_LOADED) {
        for(unsigned long i=0;i<next.size;++i) cache->storage[i]=work[i];
        next.bytes=cache->storage; cache->view=next;
    }
}
void slicks_amiga_track_list_cache_publish(struct SlicksAmigaTrackListCache *cache,
    const unsigned char *bytes,unsigned long size)
{
    struct SlicksTrackLists next;
    cache->report=(struct SlicksSetupLoadReport){SLICKS_SETUP_LOAD_INVALID,0,"SLICKS.TRK",0,0};
    if(!cache->storage || !bytes || bytes==cache->storage || size>SLICKS_AMIGA_TRACK_LIST_BYTES ||
       slicks_track_lists_open(&next,bytes,size)) return;
    for(unsigned long i=0;i<size;++i) cache->storage[i]=bytes[i];
    next.bytes=cache->storage; cache->view=next;
    cache->report.result=SLICKS_SETUP_LOADED; cache->report.path=0;
}
struct SlicksSetupStorageReport slicks_amiga_store_track_lists(
    const struct SlicksTrackLists *lists,int remove,const unsigned char *title,
    const struct SlicksTrackPlaylist *playlist,unsigned total,
    const unsigned char *(*name)(void *,unsigned),void *context,
    unsigned char *buffer,unsigned long capacity)
{
    struct SlicksSetupStorageReport report={SLICKS_SETUP_SAVE_FAILED,0,0,0};
    if(!buffer || capacity<SLICKS_AMIGA_TRACK_LIST_BYTES) { report.io_error=ERROR_NO_FREE_STORE; report.path="SLICKS.TRK"; return report; }
    long size=slicks_track_lists_write(lists,remove,title,playlist,total,name,context,
        buffer,SLICKS_AMIGA_TRACK_LIST_BYTES);
    if(size<0) { report.path="SLICKS.TRK"; report.io_error=ERROR_OBJECT_WRONG_TYPE; }
    else {
        const struct SlicksSetupFile file={"SLICKS.TRK",buffer,(unsigned long)size};
        report.result=store_files(&file,1,&report);
        report.size=(unsigned long)size;
    }
    return report;
}

struct SlicksSetupLoadReport slicks_amiga_load_saved_game(const char *path,
    struct SlicksSavedGame *game,unsigned char (*tracks)[8],unsigned capacity,
    unsigned char *buffer,unsigned long buffer_capacity)
{
    struct SlicksSetupLoadReport report={SLICKS_SETUP_LOADED,0,path,0,0};
    unsigned length=0;
    if(path) while(length<120 && path[length]) ++length;
    if(!game || !tracks || !length || length>=120 || capacity>SLICKS_SAVED_GAME_TRACK_MAX) {
        report.result=SLICKS_SETUP_LOAD_INVALID; return report;
    }
    unsigned long buffer_size=6UL+8UL*capacity+4*53;
    if(!buffer || buffer_capacity<buffer_size) {
        report.result=SLICKS_SETUP_LOAD_IO_ERROR; report.io_error=ERROR_NO_FREE_STORE;
        return report;
    }
#ifndef SLICKS_SETUP_STORAGE_HOST_TEST
    struct Process *process=(struct Process *)FindTask(0);
    APTR window=process->pr_WindowPtr;
    process->pr_WindowPtr=(APTR)-1;
#endif
    long size=read_file(&report,path,buffer,buffer_size);
    if(size==-1) {
        report.result=SLICKS_SETUP_LOAD_IO_ERROR; report.io_error=ERROR_OBJECT_NOT_FOUND;
    } else if(size>=0 && slicks_load_game_bytes(game,tracks,capacity,buffer,(unsigned long)size))
        report.result=SLICKS_SETUP_LOAD_INVALID;
#ifndef SLICKS_SETUP_STORAGE_HOST_TEST
    process->pr_WindowPtr=window;
#endif
    report.path=path;
    return report;
}

struct SlicksSetupStorageReport slicks_amiga_store_saved_game(const char *path,const struct SlicksSavedGame *game,
    unsigned char *bytes,unsigned long capacity)
{
    struct SlicksSetupStorageReport report={SLICKS_SETUP_SAVE_FAILED,0,path,0};
    long size=slicks_saved_game_size(game);
    if(!path || size<0) { report.io_error=ERROR_OBJECT_WRONG_TYPE; return report; }
    if(!bytes || capacity<(unsigned long)size) { report.io_error=ERROR_NO_FREE_STORE; return report; }
    unsigned length=0; while(length<120 && path[length]) ++length;
    if(!length || length>=120) { report.io_error=ERROR_OBJECT_WRONG_TYPE; return report; }
    if(slicks_save_game_bytes(game,bytes,(unsigned long)size)!=size) report.io_error=ERROR_OBJECT_WRONG_TYPE;
    else {
        const struct SlicksSetupFile file={path,bytes,(unsigned long)size};
        report.path=0;
        report.result=store_files(&file,1,&report);
    }
    report.path=path;
    return report;
}

struct SlicksSetupStorageReport slicks_amiga_store_track_records(const char *path,
    const struct SlicksTrackRecords *source,unsigned char *changed,
    unsigned char *buffer,unsigned long capacity)
{
    struct SlicksSetupStorageReport report={SLICKS_SETUP_SAVE_FAILED,0,path,0};
    if(changed) *changed=0;
    if(!path || !source || !changed) return report;
    if(g_slicks_diag_record_write_alloc_fault) {
        g_slicks_diag_record_write_alloc_fault=0;
        g_slicks_diag_record_write_alloc_reached=1; buffer=0;
    }
    if(!buffer || capacity<8192) { report.io_error=ERROR_NO_FREE_STORE; return report; }
    report.path=0;
    struct SlicksSetupLoadReport load={SLICKS_SETUP_LOADED,0,0,0,0};
    long size=read_file(&load,path,buffer,8192);
    if(size==-1) report.io_error=ERROR_OBJECT_NOT_FOUND;
    else if(size<6 || load.result!=SLICKS_SETUP_LOADED || buffer[2]!='S' || buffer[3]!='S' || buffer[4]!=0x7e) {
        report.io_error=load.io_error?load.io_error:ERROR_OBJECT_WRONG_TYPE;
    } else {
        /* The record block is bytes 8..362. Rewrite the complete file only
         * when it changes, so clearing already-clear tracks costs no write. */
        unsigned char before[355];
        unsigned long span=(unsigned long)size<363?0:355;
        for(unsigned long i=0;i<span;++i) before[i]=buffer[8+i];
        struct SlicksTrackRecords records=*source;
        int encoded=slicks_write_track_records(buffer,(unsigned long)size,&records);
        unsigned char same=encoded>0;
        for(unsigned long i=0;same && i<span;++i) same=before[i]==buffer[8+i];
        if(encoded<0) report.io_error=ERROR_OBJECT_WRONG_TYPE;
        else if(!encoded || same) report.result=SLICKS_SETUP_SAVED; /* Old format: original no-op. */
        else {
            const struct SlicksSetupFile file={path,buffer,(unsigned long)size};
            report.result=store_files(&file,1,&report);
            *changed=(unsigned char)(report.result==SLICKS_SETUP_SAVED);
        }
    }
    report.path=path;
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
    const unsigned long capacity=3UL+58UL*(SLICKS_PROFILE_MAX-3);
    unsigned char *buffer=AllocMem(capacity,MEMF_ANY);
    struct SlicksPlayerProfiles *next=AllocMem(sizeof(*next),MEMF_ANY);
    struct SlicksConfiguration config=*configuration;
    if(!buffer || !next) {
        report.result=SLICKS_SETUP_LOAD_IO_ERROR; report.io_error=ERROR_NO_FREE_STORE;
        goto done;
    }
    *next=*profiles;
    setup_known[0].size=setup_known[1].size=0;
    long size=read_file(&report,"SLICKS.CFG",buffer,142);
    if(size==-2) goto done;
    if(size>0) setup_remember(0,buffer,(unsigned long)size);
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
    if(size>0) setup_remember(1,buffer,(unsigned long)size);
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
