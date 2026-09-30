/* Actual Amiga adapter, deterministic in-memory DOS filesystem/faults. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
typedef long LONG;
typedef intptr_t BPTR;
typedef void *APTR;
typedef const char *CONST_STRPTR;
struct FileInfoBlock { long fib_DirEntryType; };
enum { MEMF_ANY,ACCESS_READ,MODE_NEWFILE,MODE_OLDFILE,
    ERROR_OBJECT_NOT_FOUND=205,ERROR_OBJECT_WRONG_TYPE=212,
    ERROR_OBJECT_EXISTS=203,ERROR_DISK_FULL=221,ERROR_NO_FREE_STORE=103 };
#ifdef SLICKS_CAPTURE_STORAGE_TEST
struct File { unsigned char bytes[65536]; unsigned size,offset,present; };
static struct File files[6];
static const char *paths[]={"TUNING00.BMP","TUNING00.BMP.new","TUNING00.BMP.bak",
    "TUNING01.BMP","TUNING01.BMP.new","TUNING01.BMP.bak"};
#elif defined(SLICKS_SAVED_STORAGE_TEST) || defined(SLICKS_TRACK_LIST_STORAGE_TEST)
struct File { unsigned char bytes[80219]; unsigned size,offset,present; };
static struct File files[3];
static const char *paths[]={"RACE.SSS","RACE.SSS.new","RACE.SSS.bak"};
#else
struct File { unsigned char bytes[9000]; unsigned size,offset,present; };
static struct File files[3];
static const char *paths[]={"TRACKS/TEST.SS","TRACKS/TEST.SS.new","TRACKS/TEST.SS.bak"};
#endif
static unsigned operation,fail_first,fail_second,allocations,allocation_calls;
static LONG error;
static int fault(void)
{ ++operation; if(operation==fail_first || operation==fail_second) { error=999; return 1; } return 0; }
static unsigned index_of(const char *path)
{ for(unsigned i=0;i<sizeof paths/sizeof *paths;++i) if(!strcmp(path,paths[i])) return i; abort(); }
static LONG IoErr(void) { return error; }
static void *AllocMem(unsigned long n,int flags)
{ (void)flags; ++allocation_calls; if(fault()) return 0; ++allocations; return malloc(n); }
static void FreeMem(void *p,unsigned long n) { (void)n; assert(allocations); --allocations; free(p); }
static BPTR Lock(CONST_STRPTR p,int mode)
{ (void)mode; if(fault()) return 0; unsigned i=index_of(p); if(files[i].present) return i+1; error=ERROR_OBJECT_NOT_FOUND; return 0; }
static LONG Examine(BPTR lock,struct FileInfoBlock *info)
{ (void)lock; if(fault()) return 0; info->fib_DirEntryType=-1; return 1; }
static void UnLock(BPTR lock) { (void)lock; }
static BPTR Open(CONST_STRPTR p,int mode)
{
    if(fault()) return 0; unsigned i=index_of(p); struct File *f=&files[i];
    if(mode==MODE_NEWFILE) { assert(!f->present); f->present=1; f->size=0; }
    else if(!f->present) { error=ERROR_OBJECT_NOT_FOUND; return 0; }
    f->offset=0; return i+1;
}
static LONG Read(BPTR file,APTR out,LONG n)
{
    if(fault()) return -1; struct File *f=&files[file-1];
    if(n>61) n=61; if(n>(LONG)(f->size-f->offset)) n=f->size-f->offset;
    memcpy(out,f->bytes+f->offset,(size_t)n); f->offset+=(unsigned)n; return n;
}
static LONG Write(BPTR file,APTR bytes,LONG n)
{
    if(fault()) return -1; struct File *f=&files[file-1];
    if(n>67) n=67; assert(f->offset+(unsigned)n<=sizeof f->bytes);
    memcpy(f->bytes+f->offset,bytes,(size_t)n); f->offset+=(unsigned)n; f->size=f->offset; return n;
}
static LONG Close(BPTR file) { (void)file; return !fault(); }
static LONG Flush(BPTR file) { (void)file; return !fault(); }
static LONG Rename(CONST_STRPTR a,CONST_STRPTR b)
{
    if(fault()) return 0; unsigned from=index_of(a),to=index_of(b);
    assert(files[from].present && !files[to].present); files[to]=files[from]; files[from].present=0; return 1;
}
static LONG DeleteFile(CONST_STRPTR path)
{ if(fault()) return 0; files[index_of(path)].present=0; return 1; }
#define SLICKS_SETUP_STORAGE_HOST_TEST
#include "../src/platform/amiga/amiga_setup_storage.c"
static int equals(unsigned i,const unsigned char *bytes,unsigned size)
{ return files[i].present && files[i].size==size && !memcmp(files[i].bytes,bytes,size); }
static void initialize(const unsigned char *bytes,unsigned size)
{
    memset(files,0,sizeof files); memcpy(files[0].bytes,bytes,size); files[0].size=size; files[0].present=1;
    operation=error=0; assert(!allocations);
}
int main(void)
{
    unsigned char scratch[8192];
    unsigned char before[512],after[512];
    for(unsigned i=0;i<sizeof before;++i) before[i]=(unsigned char)(i*17+11);
    before[2]='S'; before[3]='S'; before[4]=0x7e; before[5]=2;
    memcpy(after,before,sizeof after); struct SlicksTrackRecords records; slicks_clear_track_records(&records);
    assert(slicks_write_track_records(after,sizeof after,&records)==1);
    unsigned cases=0,results[4]={0};
    /* Post-race publication preserves non-record bytes and never mutates
     * the caller's insertion result, even when record one is promoted.
     * Every one/two-fault transaction retains the old or complete new file. */
    struct SlicksTrackRecords inserted=records;
    for(unsigned i=0;i<20;++i) inserted.entries[1][i]=(unsigned char)('A'+i);
    inserted.entries[1][20]=123; inserted.entries[1][26]=4; inserted.trailer=17;
    struct SlicksTrackRecords encoded=inserted;
    unsigned char published[512]; memcpy(published,before,sizeof published);
    assert(slicks_write_track_records(published,sizeof published,&encoded)==1);
    for(unsigned invalid=0;invalid<2;++invalid) {
        initialize(before,sizeof before); unsigned char changed=9;
        struct SlicksSetupStorageReport report=slicks_amiga_store_track_records(paths[0],&inserted,&changed,
            invalid?scratch:0,invalid?sizeof scratch-1:sizeof scratch);
        assert(report.result==SLICKS_SETUP_SAVE_FAILED && report.io_error==ERROR_NO_FREE_STORE);
        assert(!changed && !operation && !allocations && equals(0,before,sizeof before));
        report=slicks_amiga_clear_track_records(paths[0],&changed,
            invalid?scratch:0,invalid?sizeof scratch-1:sizeof scratch);
        assert(report.result==SLICKS_SETUP_SAVE_FAILED && !changed && !operation && !allocations);
    }
    initialize(before,sizeof before); unsigned char inserted_changed=0;
    struct SlicksSetupStorageReport inserted_report=slicks_amiga_store_track_records(paths[0],&inserted,&inserted_changed,scratch,sizeof scratch);
    assert(inserted_report.result==SLICKS_SETUP_SAVED && inserted_changed && equals(0,published,sizeof published));
    unsigned inserted_calls=operation,inserted_cases=0;
    struct SlicksTrackRecords retained=inserted;
    for(unsigned first=0;first<=inserted_calls+10;++first)
    for(unsigned second=first;second<=inserted_calls+10;++second) {
        initialize(before,sizeof before); fail_first=first; fail_second=second;
        inserted_report=slicks_amiga_store_track_records(paths[0],&inserted,&inserted_changed,scratch,sizeof scratch);
        assert(!allocations && !memcmp(&inserted,&retained,sizeof inserted));
        unsigned committed=inserted_report.result==SLICKS_SETUP_SAVED || inserted_report.result==SLICKS_SETUP_SAVED_CLEANUP_PENDING;
        assert(inserted_changed==committed);
        if(committed) assert(equals(0,published,sizeof published));
        else assert(equals(0,before,sizeof before) || equals(2,before,sizeof before));
        ++inserted_cases;
    }
    printf("Post-race record publication: %u single/double-fault cases preserve source records and complete old/new files\n",inserted_cases);
    fail_first=fail_second=0;
    initialize(before,sizeof before);
    g_slicks_diag_record_write_alloc_fault=1; inserted_changed=9;
    inserted_report=slicks_amiga_store_track_records(paths[0],&inserted,&inserted_changed,scratch,sizeof scratch);
    assert(!g_slicks_diag_record_write_alloc_fault && g_slicks_diag_record_write_alloc_reached);
    assert(inserted_report.result==SLICKS_SETUP_SAVE_FAILED && inserted_report.io_error==ERROR_NO_FREE_STORE);
    assert(!inserted_changed && !allocations && equals(0,before,sizeof before) && !files[1].present && !files[2].present);
    assert(!memcmp(&inserted,&retained,sizeof inserted));
    inserted_report=slicks_amiga_store_track_records(paths[0],&inserted,&inserted_changed,scratch,sizeof scratch);
    assert(inserted_report.result==SLICKS_SETUP_SAVED && inserted_changed && !allocations && equals(0,published,sizeof published));
    assert(!memcmp(&inserted,&retained,sizeof inserted));
    puts("Record save scratch rejection leaves files/source intact; retry commits the retained table");
    fail_first=fail_second=0;
    initialize(before,sizeof before); unsigned char changed=0;
    struct SlicksSetupStorageReport report=slicks_amiga_clear_track_records(paths[0],&changed,scratch,sizeof scratch);
    assert(report.result==SLICKS_SETUP_SAVED && changed && equals(0,after,sizeof after));
    unsigned normal_calls=operation;
    for(unsigned first=0;first<=normal_calls+10;++first) for(unsigned second=first;second<=normal_calls+10;++second) {
        initialize(before,sizeof before); fail_first=first; fail_second=second; changed=9;
        report=slicks_amiga_clear_track_records(paths[0],&changed,scratch,sizeof scratch);
        assert(!allocations && report.path==paths[0]); ++cases; ++results[report.result];
        unsigned committed=report.result==SLICKS_SETUP_SAVED || report.result==SLICKS_SETUP_SAVED_CLEANUP_PENDING;
        assert(changed==committed);
        if(committed) assert(equals(0,after,sizeof after));
        else assert(equals(0,before,sizeof before) || equals(2,before,sizeof before));
        if(report.result==SLICKS_SETUP_SAVE_FAILED) assert(equals(0,before,sizeof before));
        if(report.result==SLICKS_SETUP_SAVED || report.result==SLICKS_SETUP_SAVE_FAILED)
            assert(!files[1].present && !files[2].present);
    }
    fail_first=fail_second=0;
    initialize(before,sizeof before); files[0].present=0;
    report=slicks_amiga_clear_track_records(paths[0],&changed,scratch,sizeof scratch);
    assert(report.result==SLICKS_SETUP_SAVE_FAILED && report.io_error==ERROR_OBJECT_NOT_FOUND && !changed && !files[0].present);
    initialize(before,sizeof before); files[0].bytes[2]='X';
    unsigned char invalid[512]; memcpy(invalid,files[0].bytes,sizeof invalid);
    report=slicks_amiga_clear_track_records(paths[0],&changed,scratch,sizeof scratch);
    assert(report.result==SLICKS_SETUP_SAVE_FAILED && !changed && equals(0,invalid,sizeof invalid));
    for(unsigned artifact=1;artifact<3;++artifact) {
        initialize(before,sizeof before); files[artifact].present=1; files[artifact].size=7; memset(files[artifact].bytes,0x77,7);
        struct File snapshot[sizeof files/sizeof *files]; memcpy(snapshot,files,sizeof files);
        report=slicks_amiga_clear_track_records(paths[0],&changed,scratch,sizeof scratch);
        assert(report.result==SLICKS_SETUP_RECOVERY_REQUIRED && !changed && !memcmp(files,snapshot,sizeof files));
    }
    for(unsigned size=0;size<363;++size) {
        initialize(before,size); report=slicks_amiga_clear_track_records(paths[0],&changed,scratch,sizeof scratch);
        assert(report.result==SLICKS_SETUP_SAVE_FAILED && !changed && equals(0,before,size));
    }
    before[5]=1; initialize(before,sizeof before);
    report=slicks_amiga_clear_track_records(paths[0],&changed,scratch,sizeof scratch);
    assert(report.result==SLICKS_SETUP_SAVED && !changed && equals(0,before,sizeof before));
    unsigned char oversized[8193]={0}; oversized[2]='S'; oversized[3]='S'; oversized[4]=0x7e; oversized[5]=2;
    initialize(oversized,sizeof oversized); report=slicks_amiga_clear_track_records(paths[0],&changed,scratch,sizeof scratch);
    assert(report.result==SLICKS_SETUP_SAVE_FAILED && !changed && equals(0,oversized,sizeof oversized));
    for(unsigned i=0;i<4;++i) assert(results[i]);
    assert(!allocation_calls);
    printf("Amiga track storage: %u single/double fault cases preserve old or committed tracks; short reads/writes, truncation, oversized and recovery guards pass\n",cases);
    return 0;
}
