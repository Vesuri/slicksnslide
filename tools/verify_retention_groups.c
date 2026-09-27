#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SLICKS_RETENTION_CHECK
#include "../src/game/race_runtime.c"
static struct SlicksRaceRuntime race;
static unsigned char pixels[64000],base[64000],expected[64000];
static unsigned order[12],handles[12];
static void require(int ok,const char *what,unsigned t){if(!ok){fprintf(stderr,"Group case %u: %s\n",t,what);exit(1);}}
static int before(unsigned a,unsigned b){
    unsigned pa=race.weapons.actors[a].priority,pb=race.weapons.actors[b].priority;
    return pa<pb || (pa==pb && a<b);
}
static void setup(unsigned t){
    memset(&race,0,sizeof race);memset(&slicks_retention,0,sizeof slicks_retention);
    race.chunky=pixels;race.navigation.actor_count=12;race.sprite_dirty_deferred=1;
    for(unsigned i=0;i<64000;++i)base[i]=pixels[i]=(unsigned char)(i*13+t);
    for(unsigned a=0;a<14;++a){race.track_actor_assets[a].width=5;race.track_actor_assets[a].height=5;}
    for(unsigned i=0;i<12;++i){
        unsigned h=20+(i*5+t)%12;handles[i]=order[i]=h;race.track_actor_handles[i]=h;
        unsigned x=i<3?20+i*3:i<5?80+(i-3)*3:130+(i-5)*18,y=i<3?20:i<5?30:50;
        struct SlicksWeaponActor *a=&race.weapons.actors[h];
        a->kind=3;a->asset=i%5;a->priority=(i*3+t)%8;a->motion.x=x*64;a->motion.y=y*64;
        race.weapons.slots.state[h]=1;
        struct SlicksRetentionEntry *e=&slicks_retention.entries[i];
        e->handle=h;e->asset=a->asset;e->priority=a->priority;e->key=(x*64<<16)|(y*64);
    }
    slicks_retention_rebuild(&race,12);
    require(slicks_retention.candidates==12 && slicks_retention.group_count==2,"component count",t);
    unsigned g0=slicks_retention.group_of_handle[handles[0]],g1=slicks_retention.group_of_handle[handles[3]];
    require(g0 && g1 && g0!=g1,"separate components",t);
    for(unsigned i=0;i<12;++i)require(slicks_retention.group_of_handle[handles[i]]==(i<3?g0:i<5?g1:0),"transitive membership",t);
    for(unsigned g=1;g<=2;++g){unsigned previous=0,n=0;
        for(unsigned h=slicks_retention.group_head[g];h;h=slicks_retention.group_next[h]){
            require(++n<=12 && (!previous || before(h,previous)),"reverse draw order",t);previous=h;
        }
        require(n==(g==g0?3:2),"complete member chain",t);
    }
    for(unsigned i=1;i<12;++i){unsigned h=order[i],j=i;while(j && before(h,order[j-1])){order[j]=order[j-1];--j;}order[j]=h;}
    /* Independent full-background save and transparent checkerboard paint. */
    for(unsigned i=0;i<12;++i){unsigned h=order[i];struct SlicksWeaponActor *a=&race.weapons.actors[h];
        a->old_x=a->motion.x/64;a->old_y=a->motion.y/64;a->old_width=a->old_height=5;a->saved=1;
        for(unsigned y=0;y<5;++y)for(unsigned x=0;x<5;++x){unsigned at=(a->old_y+y)*320+a->old_x+x;
            a->saved_under[y*5+x]=pixels[at];if((x+y+h)&1)pixels[at]=(unsigned char)(80+h);}
        a->retain=RETAIN_KEPT;
    }
    memset(race.sprite_dirty_previous,0x5a,sizeof race.sprite_dirty_previous);
    memcpy(expected,pixels,64000);
}
static void restore_expected(unsigned h){
    const struct SlicksWeaponActor *a=&race.weapons.actors[h];
    for(unsigned y=0;y<5;++y)for(unsigned x=0;x<5;++x)
        expected[(a->old_y+y)*320+a->old_x+x]=a->saved_under[y*5+x];
}
int main(void){
    /* Deliberately omit invalidation after every source-key mutation. The
     * diagnostic scan must expose it independently of native producers. */
    for(unsigned t=0;t<13;++t){
        setup(0);slicks_retention.geometry_dirty=0;
        slicks_geometry_cache_checks=slicks_geometry_cache_mismatches=0;
        audit_retention_geometry(&race);
        require(slicks_geometry_cache_checks==1 && !slicks_geometry_cache_mismatches,"clean geometry audit",t);
        struct SlicksWeaponActor *a=&race.weapons.actors[handles[0]];
        switch(t){
        case 0:a->motion.x+=64;break;
        case 1:a->motion.y+=64;break;
        case 2:a->asset=4;break;
        case 3:++a->priority;break;
        case 4:race.weapons.slots.state[handles[0]]=0;break;
        case 5:a->kind=0;break;
        case 6:a->motion.frame=4;break;
        case 7:race.track_actor_handles[0]=0;break;
        case 8:--race.navigation.actor_count;break;
        case 9:++a->motion.x;break; /* fraction does not alter geometry */
        case 10:a->motion.frame=1;break; /* same animation union */
        case 11:++a->colour;break; /* validated separately before drawing */
        case 12:++a->occlusion;break;
        }
        audit_retention_geometry(&race);
        require(slicks_geometry_cache_mismatches==(t<9),"detect missing geometry invalidation",t);
        slicks_retention.geometry_dirty=1;
        audit_retention_geometry(&race);
        require(slicks_geometry_cache_checks==2,"dirty geometry bypasses cache audit",t);
    }
    setup(0);slicks_retention.geometry_dirty=0;
    configure_weapon_actor(&race,handles[0],42,43,0,0,0,6,0);
    require(slicks_retention.geometry_dirty,"configuration invalidates geometry",0);
    for(unsigned t=0;t<1024;++t){
        setup(t);unsigned selected=t%5,group=slicks_retention.group_of_handle[handles[selected]],restored[12],n=0;
        for(unsigned i=12;i;--i){unsigned h=order[i-1];if(slicks_retention.group_of_handle[h]==group){restore_expected(h);restored[n++]=h;}}
        slicks_retention_late(&race,handles[selected],t&1);
        require(!memcmp(pixels,expected,64000),"late-release pixels",t);
        require(race.sprite_dirty_count==n,"dirty count",t);
        for(unsigned i=0;i<n;++i)require(race.sprite_dirty_handles[i]==restored[i],"late-release sequence",t);
        for(unsigned i=0;i<12;++i){unsigned h=handles[i],changed=slicks_retention.group_of_handle[h]==group;
            require(race.weapons.actors[h].saved==!changed,"saved flags",t);
            require(race.weapons.actors[h].retain==(changed?0:RETAIN_KEPT),"keep flags",t);
        }
        const unsigned char *p=(const unsigned char *)race.sprite_dirty_previous;
        for(unsigned i=0;i<sizeof race.sprite_dirty_previous;++i)require(p[i]==0x5a,"previous description preserved",t);

        setup(t);for(unsigned i=12;i;--i)restore_expected(order[i-1]);
        slicks_retention_release(&race);
        require(!memcmp(pixels,expected,64000) && !memcmp(pixels,base,64000),"global release",t);
        require(race.sprite_dirty_count==12,"global release once",t);

        setup(t);for(unsigned i=12;i;--i)if(slicks_retention.group_of_handle[order[i-1]])restore_expected(order[i-1]);
        /* Rebuild after motion: old groups must be unwound before new lists. */
        slicks_retention.entries[1].key=(310U*64<<16)|(20U*64);
        slicks_retention_rebuild(&race,12);
        require(!memcmp(pixels,expected,64000) && race.sprite_dirty_count==5,"rebuild release",t);
        for(unsigned i=5;i<12;++i)require(race.weapons.actors[handles[i]].retain==RETAIN_KEPT,"unrelated isolated sprite kept",t);

        setup(t);
        for(unsigned i=12;i;--i){unsigned h=order[i-1];
            if(slicks_retention.group_of_handle[h] || h==handles[5] || h==handles[6])restore_expected(h);}
        /* Two formerly disjoint sprites now overlap: both old backgrounds
         * must be restored, even though unrelated isolated sprites stay. */
        slicks_retention.entries[6].key=(133U*64<<16)|(50U*64);
        slicks_retention_rebuild(&race,12);
        require(!memcmp(pixels,expected,64000) && race.sprite_dirty_count==7,"new group releases old singletons",t);
        require(!race.weapons.actors[handles[5]].retain && !race.weapons.actors[handles[6]].retain,"new group not partially kept",t);

        setup(t);unsigned bad=handles[t%3];
        for(unsigned i=0;i<12;++i)race.weapons.actors[handles[i]].retain=RETAIN_NEXT|RETAIN_ELIGIBLE;
        race.weapons.actors[bad].retain=RETAIN_ELIGIBLE;
        retention_finish_groups(&race);
        for(unsigned i=0;i<12;++i)require(race.weapons.actors[handles[i]].retain==
            (i<3?RETAIN_ELIGIBLE:RETAIN_NEXT|RETAIN_ELIGIBLE),"atomic next-frame gate",t);
        require(!memcmp(pixels,expected,64000),"finalization does not paint",t);

        setup(t);
        /* One out-of-bounds member excludes its complete connected component. */
        slicks_retention.entries[0].key=((unsigned)(unsigned short)-64<<16)|(20U*64);
        slicks_retention.entries[1].key=(2U*64<<16)|(20U*64);
        slicks_retention.entries[2].key=(5U*64<<16)|(20U*64);
        slicks_retention_rebuild(&race,12);
        require(slicks_retention.candidates==9 && slicks_retention.group_count==1,"clipped component exclusion",t);
        for(unsigned i=0;i<3;++i)require(!slicks_retention.entries[i].flags && !slicks_retention.group_of_handle[handles[i]],"clipped membership",t);
        slicks_race_invalidate_retention(&race);
        require(!slicks_retention.valid && !slicks_retention.group_count,"handoff invalidation",t);
        for(unsigned i=0;i<12;++i)require(!race.weapons.actors[handles[i]].retain,"handoff keep bits",t);
    }
    puts("Sprite groups: 1024 priority/handle permutations; transitive groups, exact reverse saved-under restoration, atomic eligibility, rebuild/release/clipping/invalidation and metadata preservation pass; geometry audit catches 9 missing invalidations and accepts 4 non-geometric changes");
    return 0;
}
