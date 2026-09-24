#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef void *APTR;
typedef const char *CONST_STRPTR;
typedef int BPTR;
enum {ACCESS_READ=0,ERROR_OBJECT_NOT_FOUND=205,ERROR_NO_MORE_ENTRIES=232};
struct Process { APTR pr_WindowPtr; };
struct FileInfoBlock { int fib_DirEntryType; char fib_FileName[108]; };
static struct Process process;
static char entries[43][108];
static int count,cursor,error,fault,leftover,deleted;
static void *FindTask(void *p) { (void)p; return &process; }
static BPTR Lock(const char *p,int mode)
{
    (void)mode; assert(process.pr_WindowPtr==(APTR)-1);
    if(!*p) return fault==1?0:1;
    if(strstr(p,".new") || strstr(p,".bak")) { error=205; return leftover?2:0; }
    if(!strcmp(p,"E2E.SSS")) return 2;
    error=205; return 0;
}
static void UnLock(BPTR lock) { assert(lock); }
static int Examine(BPTR lock,struct FileInfoBlock *info)
{ info->fib_DirEntryType=lock==1?1:-1; cursor=0; return fault!=2; }
static int ExNext(BPTR lock,struct FileInfoBlock *info)
{
    assert(lock==1);
    if(cursor==count || fault==3) { error=fault==3?99:232; return 0; }
    info->fib_DirEntryType=entries[cursor][0]=='D'?1:-1;
    strcpy(info->fib_FileName,entries[cursor++]); return 1;
}
static int IoErr(void) { return error; }
static int DeleteFile(const char *path)
{ assert(process.pr_WindowPtr==(APTR)-1); assert(!strcmp(path,"E2E.SSS")); ++deleted; return fault!=4; }
#define SLICKS_SAVED_FILES_HOST_TEST
#include "../src/platform/amiga/amiga_saved_files.c"
int main(void)
{
    char path[13]; unsigned char names[40][9];
    assert(!slicks_saved_file_path(path,(const unsigned char *)"A-b_12")); assert(!strcmp(path,"A-b_12.SSS"));
    const char *bad[]={"","123456789","../A","DH0:A","A.SSS","A/B","A B","*","?"};
    for(unsigned i=0;i<sizeof bad/sizeof *bad;++i) {
        strcpy(path,"UNCHANGED"); assert(slicks_saved_file_path(path,(const unsigned char *)bad[i])); assert(!strcmp(path,"UNCHANGED"));
    }
    strcpy(entries[0],"E2E.SSS"); strcpy(entries[1],"lower.sss"); strcpy(entries[2],"DIR.SSS");
    strcpy(entries[3],"TOOLONGNAME.SSS"); strcpy(entries[4],"E2E.SSS.new"); strcpy(entries[5],"NOPE.SST"); count=6;
    assert(slicks_amiga_saved_files(names)==2 && !strcmp((char *)names[1],"lower"));
    assert(!process.pr_WindowPtr);
    for(fault=1;fault<=3;++fault) { assert(slicks_amiga_saved_files(names)<0); assert(!process.pr_WindowPtr); }
    fault=0;
    for(unsigned i=0;i<41;++i) snprintf(entries[i],sizeof entries[i],"S%u.SSS",i);
    count=40; assert(slicks_amiga_saved_files(names)==40);
    count=41; assert(slicks_amiga_saved_files(names)==-2);
    assert(slicks_amiga_saved_file_exists("E2E.SSS")==1);
    assert(slicks_amiga_saved_file_exists("NONE.SSS")==0);
    leftover=1; assert(slicks_amiga_saved_file_delete("E2E.SSS")<0 && !deleted);
    leftover=0; assert(!slicks_amiga_saved_file_delete("E2E.SSS") && deleted==1);
    fault=4; assert(slicks_amiga_saved_file_delete("E2E.SSS")<0);
    assert(!process.pr_WindowPtr);
    puts("Native saved-file catalogue/path/delete: bounds, filtering, faults, recovery retention and requester restoration PASS");
}
