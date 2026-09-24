#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../src/game/setup_storage.h"
#include "../src/game/configuration.h"
#include "../src/game/player_profiles.h"

struct File { unsigned size; unsigned char bytes[6000]; unsigned present; };
struct Store { struct File files[6]; uint64_t fail; unsigned calls,fail_before_create; };
static const char *paths[]={"SLICKS.CFG","SLICKS.CFG.new","SLICKS.CFG.bak",
    "SLICKS.PLR","SLICKS.PLR.new","SLICKS.PLR.bak"};
static unsigned path_index(const char *path)
{ for(unsigned i=0;i<6;++i) if(!strcmp(path,paths[i])) return i; abort(); }
static int fails(struct Store *s)
{ if(s->calls>=64) abort(); return (s->fail>>(s->calls++))&1; }
static int exists(void *p,const char *path)
{ struct Store *s=p; return fails(s)?-1:(int)s->files[path_index(path)].present; }
static int write_file(void *p,const char *path,const unsigned char *bytes,unsigned long size)
{
    struct Store *s=p; struct File *f=&s->files[path_index(path)];
    if(f->present || size>sizeof f->bytes) abort();
    int fail=fails(s); if(fail && s->fail_before_create) return -2;
    f->present=1; f->size=fail?(unsigned)size/2:(unsigned)size;
    memcpy(f->bytes,bytes,f->size); return fail?-1:0;
}
static int rename_file(void *p,const char *from,const char *to)
{
    struct Store *s=p; struct File *a=&s->files[path_index(from)],*b=&s->files[path_index(to)];
    if(fails(s)) return -1;
    if(!a->present || b->present) abort();
    *b=*a; a->present=0; return 0;
}
static int remove_file(void *p,const char *path)
{ struct Store *s=p; if(fails(s)) return -1; s->files[path_index(path)].present=0; return 0; }
static int equal(const struct File *f,const unsigned char *bytes,unsigned size)
{ return f->present && f->size==size && !memcmp(f->bytes,bytes,size); }
static int same(const struct File *a,const struct File *b)
{ return a->present==b->present && (!a->present || equal(a,b->bytes,b->size)); }
int main(void)
{
    struct SlicksConfiguration config={0}; struct SlicksPlayerProfiles profiles={0};
    profiles.count=5;
    for(unsigned i=0;i<4;++i) config.selected_profile[i]=(short)(i%2+3);
    for(unsigned i=0;i<15;++i) config.options[i]=(short)(i*7);
    for(unsigned i=3;i<5;++i) {
        snprintf((char *)profiles.names[i],21,"PLAYER %u",i);
        profiles.setup[i].vehicle=(unsigned char)i; profiles.setting[i]=100;
        for(unsigned j=0;j<6;++j) profiles.setup[i].colours[j]=(unsigned char)(i*3+j);
    }
    unsigned char cfg[142],plr[6000];
    int cfgsize=slicks_save_configuration(&config,cfg,sizeof cfg,0x5a);
    int plrsize=slicks_save_player_profiles(&profiles,plr,sizeof plr);
    if(cfgsize!=142 || plrsize!=119) abort();
    struct SlicksSetupFile files[2]={{paths[0],paths[1],paths[2],cfg,(unsigned)cfgsize},
        {paths[3],paths[4],paths[5],plr,(unsigned)plrsize}};
    unsigned cases=0,results[4]={0};
    for(unsigned early=0;early<2;++early) for(unsigned presence=0;presence<4;++presence) for(unsigned first=0;first<=24;++first)
    for(unsigned second=first;second<=24;++second) {
        struct Store s={0};
        s.fail_before_create=early;
        if(first<24) s.fail|=UINT64_C(1)<<first;
        if(second<24) s.fail|=UINT64_C(1)<<second;
        for(unsigned i=0;i<2;++i) if(presence&(1U<<i)) {
            struct File *f=&s.files[i*3]; f->present=1; f->size=17+i;
            memset(f->bytes,0x40+i,f->size);
        }
        struct File old[2]={s.files[0],s.files[3]};
        struct SlicksSetupFileOps ops={exists,write_file,rename_file,remove_file,&s};
        enum SlicksSetupSaveResult result=slicks_store_setup_pair(files,&ops);
        if(result>SLICKS_SETUP_SAVED_CLEANUP_PENDING) abort();
        ++results[result]; ++cases;
        for(unsigned i=0;i<2;++i) {
            if(result==SLICKS_SETUP_SAVED || result==SLICKS_SETUP_SAVED_CLEANUP_PENDING) {
                if(!equal(&s.files[i*3],files[i].bytes,(unsigned)files[i].size)) abort();
            } else if(old[i].present) {
                if(!same(&s.files[i*3],&old[i]) && !same(&s.files[i*3+2],&old[i])) abort();
            }
            if(result==SLICKS_SETUP_SAVE_FAILED && !same(&s.files[i*3],&old[i])) abort();
            if(result==SLICKS_SETUP_SAVE_FAILED || result==SLICKS_SETUP_SAVED)
                if(s.files[i*3+1].present || s.files[i*3+2].present) abort();
        }
    }
    /* Pre-existing temporary/backup artifacts are evidence, not scratch. */
    for(unsigned i=0;i<6;++i) if(i%3) {
        struct Store s={0}; s.files[i].present=1; s.files[i].size=1; s.files[i].bytes[0]=123;
        struct Store before=s; struct SlicksSetupFileOps ops={exists,write_file,rename_file,remove_file,&s};
        if(slicks_store_setup_pair(files,&ops)!=SLICKS_SETUP_RECOVERY_REQUIRED ||
           memcmp(s.files,before.files,sizeof s.files)) abort();
    }
    for(unsigned i=0;i<4;++i) if(!results[i]) abort();
    printf("Setup storage: %u success/single/double I/O-failure cases pass; all old files remain recoverable, pair rollback and leftover guards pass\n",cases);
    return 0;
}
