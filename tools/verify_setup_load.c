/* Exercise the actual Amiga adapter with deterministic DOS boundary faults. */
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
static unsigned char cfg[143],plr[6000];
static long sizes[2],offsets[2],io_error;
static unsigned operation,fail_at,allocations;
static int fault(void) { if(++operation==fail_at) { io_error=999; return 1; } return 0; }
static LONG IoErr(void) { return io_error; }
static void *AllocMem(unsigned long n,int flags)
{ (void)flags; if(fault()) return 0; ++allocations; return malloc(n); }
static void FreeMem(void *p,unsigned long n) { (void)n; --allocations; free(p); }
/* Loading needs no existence checks: Lock/Examine must never be called. */
static BPTR Lock(CONST_STRPTR p,int mode) { (void)p;(void)mode; abort(); }
static LONG Examine(BPTR lock,struct FileInfoBlock *info) { (void)lock;(void)info; abort(); }
static void UnLock(BPTR lock) { (void)lock; abort(); }
static BPTR Open(CONST_STRPTR p,int mode)
{ assert(mode==MODE_OLDFILE); if(fault()) return 0;
  unsigned i=!strcmp(p,"SLICKS.PLR"); offsets[i]=0;
  if(sizes[i]<0) { io_error=ERROR_OBJECT_NOT_FOUND; return 0; } return i+1; }
static LONG Read(BPTR file,APTR out,LONG n)
{ if(fault()) return -1; unsigned i=(unsigned)file-1;
  if(n>7) n=7; /* Force short, non-EOF reads. */
  if(n>sizes[i]-offsets[i]) n=sizes[i]-offsets[i];
  memcpy(out,(i?plr:cfg)+offsets[i],(size_t)n); offsets[i]+=n; return n; }
static LONG Close(BPTR file) { (void)file; return !fault(); }
static LONG Write(BPTR f,APTR b,LONG n) { (void)f;(void)b;(void)n; abort(); }
static LONG whole_file_write(const char *p,const unsigned char *b,unsigned long n,LONG *e)
{ (void)p;(void)b;(void)n;(void)e; abort(); }
#define SLICKS_SETUP_STORAGE_HOST_TEST
#include "../src/platform/amiga/amiga_setup_storage.c"

static struct SlicksConfiguration initial;
static struct SlicksPlayerProfiles profiles;
static unsigned checks;
static struct SlicksSetupLoadReport run(unsigned fail)
{
    struct SlicksConfiguration c=initial;
    struct SlicksPlayerProfiles p=profiles;
    fail_at=fail; operation=0;
    struct SlicksSetupLoadReport r=slicks_amiga_load_setup(&c,&p,24,9,2026);
    assert(!allocations);
    if(r.result!=SLICKS_SETUP_LOADED) {
        assert(!memcmp(&c,&initial,sizeof c)); assert(!memcmp(&p,&profiles,sizeof p));
    } else {
        struct SlicksConfiguration expected=initial;
        struct SlicksPlayerProfiles ep=profiles;
        slicks_load_configuration(&expected,sizes[0]<0?0:cfg,sizes[0]<0?0:sizes[0],
            SLICKS_AMIGA_CONFIG_SIGNATURE,24,9,2026);
        slicks_load_player_profiles(&ep,sizes[1]<0?0:plr,sizes[1]<0?0:sizes[1]);
        assert(!memcmp(&c,&expected,sizeof c)); assert(!memcmp(&p,&ep,sizeof p));
    }
    ++checks; return r;
}
int main(void)
{
    memset(&initial,0x12,sizeof initial);
    memset(&profiles,0,sizeof profiles); profiles.count=4;
    sizes[0]=slicks_save_configuration(&initial,cfg,sizeof cfg,SLICKS_AMIGA_CONFIG_SIGNATURE);
    sizes[1]=slicks_save_player_profiles(&profiles,plr,sizeof plr);
    assert(run(0).result==SLICKS_SETUP_LOADED);
    unsigned calls=operation;
    for(unsigned i=1;i<=calls;++i) assert(run(i).result==SLICKS_SETUP_LOAD_IO_ERROR);
    long valid_plr=sizes[1];
    for(long n=0;n<142;++n) { sizes[0]=n; assert(run(0).result==SLICKS_SETUP_LOAD_INVALID); }
    sizes[0]=143; assert(run(0).result==SLICKS_SETUP_LOAD_INVALID); sizes[0]=142;
    cfg[1]^=1; assert(run(0).result==SLICKS_SETUP_LOAD_FOREIGN); cfg[1]^=1;
    for(long n=0;n<valid_plr;++n) { sizes[1]=n; assert(run(0).result==SLICKS_SETUP_LOAD_INVALID); }
    sizes[1]=valid_plr+1; assert(run(0).result==SLICKS_SETUP_LOAD_INVALID);
    sizes[1]=valid_plr;
    for(unsigned mask=0;mask<4;++mask) {
        sizes[0]=mask&1?142:-1; sizes[1]=mask&2?valid_plr:-1;
        struct SlicksSetupLoadReport r=run(0);
        assert(r.result==SLICKS_SETUP_LOADED);
        assert(r.configuration_present==!!(mask&1)); assert(r.profiles_present==!!(mask&2));
    }
    printf("Setup load: %u adapter checks pass (short reads, faults, truncation, foreign tag, missing files)\n",checks);
}
