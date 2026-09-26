#define main menu_icon_verifier_main
#include "verify_menu_icon.c"
#undef main
#include "../src/game/race_runtime.c"
static struct SlicksRaceRuntime race;
static unsigned char native[64000],before[64000];
#if defined(SLICKS_NATIVE_SPRITE_TEST)
#include "native_sprite_oracle.h"
#endif
static void actor_pixel(uc_engine *u,uc_mem_type type,uint64_t address,int size,int64_t value,void *context)
{
    (void)u;(void)type;(void)context;
    unsigned at=(unsigned)(address-0xa0000);
    for(int b=0;b<size;++b)for(unsigned p=0;p<4;++p)if(mask&(1U<<p)) {
        unsigned x=((at+b)%100)*4+p,y=(at+b)/100;
        if(x<320&&y<200)dos[y*320+x]=(unsigned char)((uint64_t)value>>(8*b));
    }
}
int main(void)
{
    static unsigned char runtime[300000],arena[65536];
    FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f);fclose(f);
    uc_engine *u;check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));check(uc_mem_write(u,0x10100,runtime,bytes));
    const unsigned addresses[]={0x373a7,0x317af,0x12d8a,0x1221d,0x13b07,0x119c2,0x36243};uc_hook h;
    for(unsigned i=0;i<sizeof addresses/sizeof *addresses;++i)check(uc_hook_add(u,&h,UC_HOOK_CODE,services,0,addresses[i],addresses[i]));
    check(uc_hook_add(u,&h,UC_HOOK_INSN,port,0,1,0,UC_X86_INS_OUT));
    check(uc_hook_add(u,&h,UC_HOOK_MEM_WRITE,actor_pixel,0,0xa0000,0xaffff));
    f=fopen("ref/SLICKS.DAT","rb");if(!f)return 2;length=fread(source,1,sizeof source,f);fclose(f);
    if(slicks_decode_track_actor_assets(source,length,arena,sizeof arena,race.track_actor_assets))return 1;
    race.track_actors_ready=1;
    for(unsigned image=0;image<119;++image) {
        if(image==110) { while(cursor+3<=length && memcmp(source+cursor,"\x12\x34\0",3))++cursor;cursor+=3; }
        allocations=0;
        word(u,0x8f004,0);word(u,0x8f006,0x9000);word(u,0x8f008,0);word(u,0x8f00a,0x6000);word(u,0x8f00c,2);start(u,0x2e51a);
        int index=-1;const unsigned base[5]={79,80,81,82,89};
        for(unsigned i=0;i<5;++i)if(image==base[i])index=i;
        if(image>=110)index=image-105;
        if(index<0)continue;
        unsigned char original[256];check(uc_mem_read(u,0x50000,original,sizeof original));
        check(uc_mem_write(u,0x51000+index*256,original,sizeof original));
    }
    unsigned cases=0;
    for(unsigned asset=0;asset<14;++asset)for(unsigned trial=0;trial<128;++trial) {
        /* Each oracle case replaces the foreground maps (a new race). */
        for(unsigned i=0;i<64;++i)race.track_sprite_visibility[i].valid=0;
        unsigned page=trial&1,limit=(trial&2)?15:0;
        short x=(short)(100+trial%4),y=(short)(50+trial%8);
        if(trial&16)x=-1;
        if(trial&32)y=-1;
        if(trial&64) { x=316;y=196; }
        static unsigned char raw[64000];
        for(unsigned i=0;i<64000;++i) {
            before[i]=native[i]=dos[i]=(unsigned char)(i*19+trial);
            raw[i]=i>=60800?0:(unsigned char)((i+trial*17)%256);
            if(i<60800){race.material_map[i]=raw[i]>>3;race.surface_map[i]=raw[i]&7;}
        }
        check(uc_mem_write(u,0x70000,raw,sizeof raw));
        word(u,0x616ce,0);word(u,0x616d0,0x9000);word(u,0x616ca,0);word(u,0x616cc,0x7000);
        word(u,0x616c6,page);word(u,0x616c7,1);word(u,0x616c4,184);
        word(u,0x3cbf0+0x1d7d,320);word(u,0x3cbf0+0x1d89,0);word(u,0x61d7b,100);
        word(u,0x3cbf0+0x1d7b,100);word(u,0x3cbf0+0x1d8d,0);word(u,0x3cbf0+0x1d8f,200);word(u,0x3cbf0+0x1d91,0);word(u,0x3cbf0+0x1d93,79);
        unsigned char original[64]={0};original[0x18]=1;original[0x3b]=limit;
        check(uc_mem_write(u,0x90040,original,sizeof original));
        word(u,0x90044+page*2,x);word(u,0x90048+page*2,y);word(u,0x9004c,asset*256);word(u,0x9004e,0x5100);
        uint16_t cs=0x2e0f,ds=0x6000,ss=0x8000,sp=0xf000;
        word(u,0x8f000,0);word(u,0x8f002,0x5500);word(u,0x8f004,1);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&ds));check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x33673,0x55000,0,100000));
        unsigned kind=asset<5?asset:asset<8?0:asset<11?2:4;
        unsigned frame=asset<5?0:asset<8?asset-4:asset<11?asset-7:asset-10;
        race.chunky=native;race.weapons.slots.state[1]=1;
        race.weapons.actors[1]=(struct SlicksWeaponActor){.asset=kind,.kind=3,.occlusion=limit};
        race.weapons.actors[1].motion.frame=frame;
        race.weapons.actors[1].motion.x=x*64;race.weapons.actors[1].motion.y=y*64;draw_weapon_actor(&race,1);
        if(memcmp(native,dos,64000)) {
            for(unsigned i=0;i<64000;++i)if(native[i]!=dos[i]){fprintf(stderr,"Track actor %u trial=%u xy=%d,%d mask=%u pixel=%u,%u native=%u DOS=%u\n",asset,trial,x,y,limit,i%320,i/320,native[i],dos[i]);break;}return 1;
        }
        race.sprite_dirty_deferred=1;race.sprite_dirty_count=0;
        restore_weapon_actor(&race,1);if(memcmp(native,before,64000))abort();
        /* Same key must hit the visibility cache without changing pixels. */
        draw_weapon_actor(&race,1);if(memcmp(native,dos,64000))abort();
        finish_sprite_dirty_batch(&race);
        race.sprite_dirty_deferred=1;
        restore_weapon_actor(&race,1);if(memcmp(native,before,64000))abort();
        /* Handles 1 and 65 share a cache slot. A different position must
         * evict, and drawing 1 again must revalidate rather than reuse it. */
        race.weapons.actors[65]=race.weapons.actors[1];race.weapons.slots.state[65]=1;
        race.weapons.actors[65].motion.x+=64;
        draw_weapon_actor(&race,65);restore_weapon_actor(&race,65);
        if(memcmp(native,before,64000))abort();
        draw_weapon_actor(&race,1);if(memcmp(native,dos,64000))abort();
        restore_weapon_actor(&race,1);if(memcmp(native,before,64000))abort();
        finish_sprite_dirty_batch(&race);++cases;
    }
    printf("Track actor rendering: %u original full-screen/foreground/edge comparisons and restoration checks pass\n",cases);
    for(unsigned trial=0;trial<64;++trial) {
        for(unsigned i=0;i<64;++i)race.track_sprite_visibility[i].valid=0;
        race.participation_ready=1;memset(race.participation,0,sizeof race.participation);
        race.navigation.actor_count=0;race.trail_particle_count=0;
        memset(race.trail_priority_counts,0,sizeof race.trail_priority_counts);
        initialize_weapon_actors(&race);
        for(unsigned p=0;p<64000;++p)before[p]=native[p]=dos[p]=(unsigned char)(p*7+trial);
        memset(race.material_map,0,sizeof race.material_map);memset(race.surface_map,0,sizeof race.surface_map);
        static const unsigned priorities[]={0,1,2,3,4,5,6,7,10,17,126,128,255};
        for(unsigned i=0;i<210;++i) {
            if(i%3==0) add_trail_component(&race,100,100,(unsigned char)(40+i),3,0,0,30);
            else {
                short h=allocate_weapon_actor(&race,0,1,0);if(!h)continue;
                struct SlicksWeaponActor *a=&race.weapons.actors[h];
                a->kind=3;a->asset=(i+trial)%5;a->motion.frame=0;
                configure_weapon_actor(&race,h,100,100,0,0,30,3,0);
            }
        }
        for(unsigned h=1;h<200;++h) {
            int t=race.weapons.trail_index[h];unsigned p=priorities[(h+trial)%13];
            if(t>=0)race.trail_particles[t].priority=p;
            else race.weapons.actors[h].priority=p;
        }
        for(unsigned priority=0;priority<128;++priority)for(unsigned h=1;h<200;++h) {
            int t=race.weapons.trail_index[h];struct SlicksWeaponActor *a=&race.weapons.actors[h];
            if(t<0 && !a->kind)continue;
            if((t>=0?race.trail_particles[t].priority:a->priority)!=priority)continue;
            unsigned char original[64]={0};original[0x18]=t<0;original[0x26]=t>=0?race.trail_particles[t].colour:0;
            check(uc_mem_write(u,0x90000+h*64,original,sizeof original));
            word(u,0x90000+h*64+4,100);word(u,0x90000+h*64+8,100);
            word(u,0x90000+h*64+0xc,a->asset*256);word(u,0x90000+h*64+0xe,0x5100);
            word(u,0x616c6,0);word(u,0x616c7,1);
            uint16_t cs=0x2e0f,ds=0x6000,ss=0x8000,sp=0xf000;
            word(u,0x8f000,0);word(u,0x8f002,0x5500);word(u,0x8f004,h);
            check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&ds));check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
            check(uc_emu_start(u,0x33673,0x55000,0,100000));
        }
        draw_race_actors(&race,0);
        if(memcmp(native,dos,64000)) { fprintf(stderr,"Mixed track priority mismatch %u\n",trial);return 1; }
        restore_race_actors(&race,0);
        if(memcmp(native,before,64000)) { fprintf(stderr,"Mixed track restoration mismatch %u\n",trial);return 1; }
    }
    puts("Shared track/point renderer: 64 saturated-pool signed-priority and reverse-restoration cases match DOS");
    for(unsigned kind=0;kind<5;++kind)for(unsigned priority=0;priority<8;++priority) {
        race.trail_particle_count=0;memset(race.trail_priority_counts,0,sizeof race.trail_priority_counts);
        initialize_weapon_actors(&race);memset(native,40,sizeof native);
        short h=allocate_weapon_actor(&race,0,1,0);
        race.weapons.actors[h].kind=3;race.weapons.actors[h].asset=kind;
        configure_weapon_actor(&race,h,100,100,0,0,0,priority,0);
        const struct SlicksTrackActorAsset *image=&race.track_actor_assets[kind];
        unsigned pixel=0;while(pixel<(unsigned)image->width*image->height && !image->pixels[pixel])++pixel;
        if(pixel==(unsigned)image->width*image->height)abort();
        short x=100+pixel%image->width,y=100+pixel/image->width;
        add_trail_component(&race,x,y,71,0,0,0,3);
        draw_race_actors(&race,0);restore_race_actors(&race,0);
        race.actor_page=1;race.trail_particles[0].lifetime=1;
        commit_expiring_trails(&race);
        unsigned th=race.weapons.trail_handle[0];
        race.weapons.trail_index[th]=-1;race.weapons.slots.state[th]=0;
        race.trail_particle_count=0;memset(race.trail_priority_counts,0,sizeof race.trail_priority_counts);
        draw_race_actors(&race,0);restore_race_actors(&race,0);
        if(native[y*320+x]!=71) { fprintf(stderr,"Track actor erased permanent mark kind=%u priority=%u\n",kind,priority);return 1; }
    }
    puts("Permanent marks survive expiration beneath all five track actor kinds at eight priorities");
    check(uc_close(u));return 0;
}
