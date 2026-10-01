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
static unsigned operations,fail_at,allocations,allocation_calls,reads;
static int fault(void) { return ++operations==fail_at; }
static BPTR Open(CONST_STRPTR p,int mode)
{ (void)mode; return fault()?0:(BPTR)fopen(p,"rb"); }
static void Close(BPTR p) { fclose((FILE *)p); }
static LONG Read(BPTR p,void *out,LONG n)
{
    ++reads;
    if(fault()) return -1;
    return (LONG)fread(out,1,(size_t)n,(FILE *)p);
}
static LONG Seek(BPTR p,LONG offset,int origin)
{
    if(fault()) return -1;
    FILE *f=(FILE *)p; long old=ftell(f);
    if(old<0 || fseek(f,offset,origin<0?SEEK_SET:origin?SEEK_END:SEEK_CUR)) return -1;
    return old;
}
static void *AllocMem(unsigned long n,int flags)
{ (void)flags; ++allocation_calls; if(fault()) return 0; ++allocations; return malloc(n); }
static void FreeMem(void *p,unsigned long n)
{ (void)n; --allocations; free(p); }
#define SLICKS_ARCHIVE_HOST_TEST
#include "../src/platform/amiga/resource_archive.c"
#include "../src/platform/amiga/menu_resources.h"
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
    unsigned base_allocations=allocations;
    operations=0; reads=0;
    static unsigned char staging[65536];
    struct SlicksResourceCache *cache=slicks_resource_cache_create(&a,
        slicks_menu_resources,SLICKS_MENU_RESOURCE_COUNT,staging,sizeof staging);
    assert(cache);
    unsigned create_operations=operations;
    /* One read per resource: no streamed partial reads. */
    assert(reads==SLICKS_MENU_RESOURCE_COUNT);
    printf("Resident menu cache: %u resources, %lu bytes including host metadata\n",
        (unsigned)SLICKS_MENU_RESOURCE_COUNT,slicks_resource_cache_bytes(cache));
    struct SlicksResourceArchive memory={0};
    operations=0; fail_at=1;
    assert(!slicks_resource_archive_cached(&memory,cache));
    for(unsigned i=0;i<SLICKS_MENU_RESOURCE_COUNT;++i) {
        long size=host_archive_load("ref/SLICKS.000",slicks_menu_resources[i],expected,sizeof expected);
        assert(size>0);
        assert(slicks_resource_archive_load(&memory,slicks_menu_resources[i],actual,sizeof actual)==size);
        assert(!memcmp(expected,actual,(size_t)size));
        assert(slicks_resource_archive_load(&memory,slicks_menu_resources[i],actual,(unsigned long)size-1)==-1);
    }
    assert(slicks_resource_archive_load(&memory,"absent",actual,sizeof actual)==-1);
    slicks_resource_archive_close(&memory);
    assert(!operations); /* No DOS operation or allocation on the cached path. */
    fail_at=0;
    slicks_resource_cache_destroy(cache); assert(allocations==base_allocations);
    for(unsigned fail=1;fail<=create_operations;++fail) {
        operations=0; fail_at=fail;
        cache=slicks_resource_cache_create(&a,slicks_menu_resources,SLICKS_MENU_RESOURCE_COUNT,staging,sizeof staging);
        assert(!cache && allocations==base_allocations);
    }
    operations=0; fail_at=0;
    const char *missing[]={"HELP.TXT","absent"};
    assert(!slicks_resource_cache_create(&a,missing,2,staging,sizeof staging));
    assert(!slicks_resource_cache_create(&a,slicks_menu_resources,SLICKS_MENU_RESOURCE_COUNT,staging,64002));
    assert(allocations==base_allocations);
    struct SlicksArchiveDirectory directory={0};
    unsigned char *retained=a.directory;
    assert(!slicks_resource_directory_adopt(&directory,&a));
    assert(directory.busy && directory.bytes==retained && allocations==1);
    assert(slicks_resource_directory_adopt(&directory,&a)<0);
    assert(slicks_resource_directory_destroy(&directory)<0);
    struct SlicksResourceArchive second={0};
    unsigned reserved_calls=allocation_calls;
    assert(slicks_resource_archive_open_reserved(&second,"ref/SLICKS.000",&directory)<0);
    assert(directory.busy && !second.file);
    slicks_resource_archive_close(&a); assert(allocations==1 && !directory.busy);
    for(unsigned repeat=0;repeat<2;++repeat) {
        assert(!slicks_resource_archive_open_reserved(&a,"ref/SLICKS.000",&directory));
        assert(a.directory==retained && directory.busy);
        for(unsigned i=0;i<a.count;++i) {
            char name[17]={0};memcpy(name,a.directory+19*i,16);
            long wanted=host_archive_load("ref/SLICKS.000",name,expected,sizeof expected);
            long got=slicks_resource_archive_load(&a,name,actual,sizeof actual);
            assert(got==wanted && (got<0 || !memcmp(actual,expected,(size_t)got)));
        }
        slicks_resource_archive_close(&a);
    }
    operations=0;fail_at=1;
    assert(slicks_resource_archive_open_reserved(&a,"ref/SLICKS.000",&directory)<0);
    assert(!a.file && !a.directory && !directory.busy && allocations==1);
    /* Reserved opens reuse the startup directory: only the Open itself. */
    operations=0;fail_at=0;reads=0;
    assert(!slicks_resource_archive_open_reserved(&a,"ref/SLICKS.000",&directory));
    assert(operations==1 && !reads);
    slicks_resource_archive_close(&a);
    assert(allocation_calls==reserved_calls);
    assert(!slicks_resource_directory_destroy(&directory) && !directory.bytes && !allocations);
    puts("Reserved directory: repeated byte-exact loads without rereads, exclusive ownership, open failure cleanup and zero runtime allocation calls pass");
    printf("Cache: byte-identical resources, zero-I/O reads/misses/close, %u injected construction failures unwind\n",create_operations);
    printf("Archive adapter: %u named-resource comparisons, final HELP.TXT, capacity rejection and all four EOF/seek/read faults pass\n",cases);
    return 0;
}
