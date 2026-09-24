/* Compile the actual Amiga adapter against FILE-backed DOS calls. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
typedef intptr_t BPTR;
typedef long LONG;
typedef const char *CONST_STRPTR;
enum { MEMF_ANY,MODE_OLDFILE,OFFSET_BEGINNING=-1,OFFSET_CURRENT=0,OFFSET_END=1 };
static unsigned operations,fail_at,allocations;
static int fault(void) { return ++operations==fail_at; }
static BPTR Open(CONST_STRPTR p,int mode)
{ (void)mode; return fault()?0:(BPTR)fopen(p,"rb"); }
static void Close(BPTR p) { fclose((FILE *)p); }
static LONG Read(BPTR p,void *out,LONG n)
{ return fault()?-1:(LONG)fread(out,1,(size_t)n,(FILE *)p); }
static LONG Seek(BPTR p,LONG offset,int origin)
{
    if(fault()) return -1;
    FILE *f=(FILE *)p; long old=ftell(f);
    if(old<0 || fseek(f,offset,origin<0?SEEK_SET:origin?SEEK_END:SEEK_CUR)) return -1;
    return old;
}
static void *AllocMem(unsigned long n,int flags)
{ (void)flags; if(fault()) return 0; ++allocations; return malloc(n); }
static void FreeMem(void *p,unsigned long n)
{ (void)n; --allocations; free(p); }
#define SLICKS_ARCHIVE_HOST_TEST
#include "../src/platform/amiga/resource_archive.c"
#include "host_archive.h"
int main(void)
{
    struct SlicksResourceArchive a={0};
    assert(!slicks_resource_archive_open(&a,"ref/SLICKS.000"));
    unsigned char expected[262144],actual[262144]; unsigned cases=0;
    for(unsigned i=0;i<a.count;++i) {
        char name[17]={0}; memcpy(name,a.directory+19*i,16);
        long n=host_archive_load("ref/SLICKS.000",name,expected,sizeof expected);
        long got=slicks_resource_archive_load(&a,name,actual,sizeof actual);
        assert(got==n);
        if(n>=0) { assert(!memcmp(expected,actual,(size_t)n)); ++cases; }
    }
    long n=slicks_resource_archive_load(&a,"HELP.TXT",actual,sizeof actual);
    assert(n==13104 && !memcmp(actual,"!<-window",9));
    assert(slicks_resource_archive_load(&a,"HELP.TXT",actual,13103)==-1);
    for(unsigned fail=1;fail<=4;++fail) {
        operations=0; fail_at=fail;
        assert(slicks_resource_archive_load(&a,"HELP.TXT",actual,sizeof actual)==-1);
    }
    fail_at=0;
    assert(slicks_resource_archive_load(&a,"HELP.TXT",actual,sizeof actual)==13104);
    slicks_resource_archive_close(&a); assert(!allocations);
    printf("Archive adapter: %u named-resource comparisons, final HELP.TXT, capacity rejection and all four EOF/seek/read faults pass\n",cases);
    return 0;
}
