#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../src/game/setup_storage.h"
#include "../src/game/configuration.h"
#include "../src/game/player_profiles.h"

struct File { unsigned size; unsigned char bytes[6000]; unsigned present; };
struct Store { struct File files[2]; unsigned calls,fail_at; };
static const char *paths[]={"SLICKS.CFG","SLICKS.PLR"};
static unsigned path_index(const char *path)
{ for(unsigned i=0;i<2;++i) if(!strcmp(path,paths[i])) return i; abort(); }
/* Replace in place with one complete write; a failed write may truncate. */
static int write_file(void *p,const char *path,const unsigned char *bytes,unsigned long size)
{
    struct Store *s=p; struct File *f=&s->files[path_index(path)];
    if(size>sizeof f->bytes) abort();
    int fail=++s->calls==s->fail_at;
    f->present=1; f->size=fail?(unsigned)size/2:(unsigned)size;
    memcpy(f->bytes,bytes,f->size); return fail?-1:0;
}
static int equal(const struct File *f,const unsigned char *bytes,unsigned size)
{ return f->present && f->size==size && !memcmp(f->bytes,bytes,size); }
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
    struct SlicksSetupFile files[2]={{paths[0],cfg,(unsigned)cfgsize},{paths[1],plr,(unsigned)plrsize}};
    unsigned cases=0;
    for(unsigned presence=0;presence<4;++presence) for(unsigned fail=0;fail<=3;++fail) {
        struct Store s={0}; s.fail_at=fail;
        for(unsigned i=0;i<2;++i) if(presence&(1U<<i)) {
            struct File *f=&s.files[i]; f->present=1; f->size=17+i; memset(f->bytes,0x40+i,f->size);
        }
        struct SlicksSetupFileOps ops={write_file,&s};
        enum SlicksSetupSaveResult result=slicks_store_setup_pair(files,&ops);
        /* CFG then PLR, one write each; stop at the first failure. */
        if(result==SLICKS_SETUP_SAVED) {
            if(fail==1 || fail==2 || s.calls!=2) abort();
            for(unsigned i=0;i<2;++i) if(!equal(&s.files[i],files[i].bytes,(unsigned)files[i].size)) abort();
        } else if(result!=SLICKS_SETUP_SAVE_FAILED || s.calls!=fail) abort();
        if(fail==2 && !equal(&s.files[0],cfg,(unsigned)cfgsize)) abort();
        ++cases;
    }
    struct Store s={0}; struct SlicksSetupFileOps ops={write_file,&s};
    struct SlicksSetupFile empty[2]={files[0],{paths[1],plr,0}};
    if(slicks_store_setup_pair(empty,&ops)!=SLICKS_SETUP_SAVE_FAILED || s.calls) abort();
    printf("Setup storage: %u pair cases; one complete write per file in order, stop at first failure, empty stream rejected unwritten\n",cases);
    return 0;
}
