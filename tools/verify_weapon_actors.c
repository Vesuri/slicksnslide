#define main menu_icon_verifier_main
#include "verify_menu_icon.c"
#undef main
#include "../src/game/race_runtime.c"
static struct SlicksRaceRuntime race;
static unsigned char native[64000],before[64000];
#if defined(SLICKS_NATIVE_SPRITE_TEST)
#include "native_sprite_oracle.h"
#endif
/* DOS can write the 400-pixel virtual margin; compare the 320-pixel display. */
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
    unsigned char runtime[300000];FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f);fclose(f);
    uc_engine *u;check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));check(uc_mem_write(u,0x10100,runtime,bytes));
    const unsigned addresses[]={0x373a7,0x317af,0x12d8a,0x1221d,0x13b07,0x119c2,0x36243};uc_hook h;
    for(unsigned i=0;i<sizeof addresses/sizeof *addresses;++i)check(uc_hook_add(u,&h,UC_HOOK_CODE,services,0,addresses[i],addresses[i]));
    check(uc_hook_add(u,&h,UC_HOOK_INSN,port,0,1,0,UC_X86_INS_OUT));
    check(uc_hook_add(u,&h,UC_HOOK_MEM_WRITE,actor_pixel,0,0xa0000,0xaffff));
    const char *names[]={"miina.ase","aikabomb.ase","flam_raj.@I","ohjus.1","ohjus.2","ohjus.3","ohjus.4","ohjus.5","ohjus.6","ohjus.7","ohjus.8",
        "savu.1","savu.2","savu.3","rajahdys.1","rajahdys.2","rajahdys.3","rajahdys.4","flash.@I"};
    unsigned cases=0;
    for(unsigned asset=0;asset<19;++asset) {
        long loaded=host_archive_load("ref/SLICKS.000",names[asset],source,sizeof source);if(loaded<0)return 2;length=loaded;cursor=allocations=0;
        word(u,0x8f004,0);word(u,0x8f006,0x9000);word(u,0x8f008,0);word(u,0x8f00a,0x6000);word(u,0x8f00c,2);start(u,0x2e51a);
        struct SlicksWeaponAsset *image=&race.weapons.assets[asset];unsigned short w,hgt;
        if(slicks_decode_indexed_menu_icon(source,length,image->pixels,sizeof image->pixels,&w,&hgt))abort();
        image->width=w;image->height=hgt;image->ready=1;
        unsigned char original_image[256];
        unsigned original_size=3+((w+3)/4)*4*hgt;
        check(uc_mem_read(u,0x50000,original_image,original_size));
        check(uc_mem_write(u,0x51000+asset*256,original_image,original_size));
        for(unsigned trial=0;trial<128;++trial) {
            unsigned page=trial&1,limit=(trial&2)?15:0,pattern=(trial/4)%8;
            short x=(short)(100+trial%4),y=(short)(50+(trial/32)*45);
            if(y+hgt>200)y=(short)(200-hgt);
            if(trial&16)x=(short)(320-w);
            unsigned char raw[64000];
            for(unsigned i=0;i<64000;++i) {
                before[i]=native[i]=dos[i]=(unsigned char)(i*19+trial);
                raw[i]=i>=60800?0:(unsigned char)((i+pattern*17)%256);
                if(i<60800){race.material_map[i]=raw[i]>>3;race.surface_map[i]=raw[i]&7;}
            }
            check(uc_mem_write(u,0x70000,raw,sizeof raw));
            word(u,0x616ce,0);word(u,0x616d0,0x9000);word(u,0x616ca,0);word(u,0x616cc,0x7000);
            word(u,0x616c6,page);word(u,0x616c7,1);word(u,0x616c4,184);
            word(u,0x3cbf0+0x1d7d,320);word(u,0x3cbf0+0x1d89,0);word(u,0x61d7b,100);
            word(u,0x3cbf0+0x1d7b,100);word(u,0x3cbf0+0x1d8d,0);word(u,0x3cbf0+0x1d8f,200);word(u,0x3cbf0+0x1d91,0);word(u,0x3cbf0+0x1d93,79);
            unsigned char actor[64]={0};actor[0x18]=1;actor[0x3b]=limit;check(uc_mem_write(u,0x90040,actor,sizeof actor));
            word(u,0x90044+page*2,x);word(u,0x90048+page*2,y);word(u,0x9004c,0);word(u,0x9004e,0x5000);
            uint16_t cs=0x2e0f,ds=0x6000,ss=0x8000,sp=0xf000;
            word(u,0x8f000,0);word(u,0x8f002,0x5500);word(u,0x8f004,1);
            check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&ds));check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
            check(uc_emu_start(u,0x33673,0x55000,0,100000));
            race.chunky=native;race.weapons.slots.state[1]=1;race.weapons.actors[1]=(struct SlicksWeaponActor){.asset=asset,.kind=1,.occlusion=limit};
            race.weapons.actors[1].motion.x=x*64;race.weapons.actors[1].motion.y=y*64;draw_weapon_actor(&race,1);
            if(memcmp(native,dos,64000)) {
                for(unsigned i=0;i<64000;++i)if(native[i]!=dos[i]){fprintf(stderr,"Weapon actor %s trial=%u mask=%u pixel=%u,%u native=%u DOS=%u\n",names[asset],trial,limit,i%320,i/320,native[i],dos[i]);break;}return 1;
            }
            restore_weapon_actor(&race,1);if(memcmp(native,before,64000)){fputs("Weapon actor restore failed\n",stderr);return 1;}++cases;
        }
    }
    printf("Original weapon actors: %u full-screen sprite/mask/page comparisons and exact restoration checks pass\n",cases);
    for(unsigned trial=0;trial<128;++trial) {
        race.weapons.ready=1;race.participation_ready=1;race.trail_particle_count=0;
        memset(race.trail_priority_counts,0,sizeof race.trail_priority_counts);
        initialize_weapon_actors(&race);
        /* Exercise holes in a saturated common pool, not append-only order. */
        for(unsigned i=0;i<210;++i) {
            if(i&1) add_trail_component(&race,100,100,(unsigned char)(30+i),3,0,0,20);
            else {
                short h=allocate_weapon_actor(&race,i%19,1,0);
                configure_weapon_actor(&race,h,100,100,0,0,20,3,0);
            }
        }
        if(race.weapons.slots.high_water!=200)abort();
        for(unsigned h=9;h<200;h+=7)if(race.weapons.trail_index[h]<0) {
            race.weapons.slots.state[h]=0;race.weapons.actors[h].kind=0;
        }
        for(unsigned i=0;i<35;++i) {
            short expected=0;
            for(unsigned h=1;h<200;++h)if(!race.weapons.slots.state[h]){expected=(short)h;break;}
            short actual=allocate_weapon_actor(&race,(i+trial)%19,1,0);
            if(actual!=expected)abort();
            configure_weapon_actor(&race,actual,100,100,0,0,20,3,0);
        }
        for(unsigned i=0;i<64000;++i)before[i]=native[i]=dos[i]=(unsigned char)(i*37+trial);
        for(unsigned priority=0;priority<=6;++priority) for(unsigned h=1;h<200;++h) {
            if(race.weapons.slots.state[h]<=0)continue;
            int trail=race.weapons.trail_index[h];
            if(trail<0&&!race.weapons.actors[h].kind)continue;
            unsigned char colour=0,limit=0;unsigned asset=0;
            unsigned original_priority=(h+trial)%4;
            original_priority=original_priority==0?0:original_priority==1?3:original_priority==2?5:6;
            if(original_priority!=priority)continue;
            short x=(short)(98+(h+trial)%7),y=(short)(98+(h*3+trial)%7);
            if(trail>=0) {
                struct SlicksTrailParticle *t=&race.trail_particles[trail];t->x=x*64;t->y=y*64;t->priority=priority;colour=t->colour;
            } else {
                struct SlicksWeaponActor *a=&race.weapons.actors[h];a->motion.x=x*64;a->motion.y=y*64;a->priority=priority;asset=a->asset;
            }
            unsigned char actor[64]={0};actor[0x18]=trail<0;actor[0x26]=colour;actor[0x3b]=limit;
            check(uc_mem_write(u,0x90000+h*64,actor,sizeof actor));word(u,0x90000+h*64+4,x);word(u,0x90000+h*64+8,y);
            word(u,0x90000+h*64+0xc,asset*256);word(u,0x90000+h*64+0xe,0x5100);word(u,0x616c6,0);word(u,0x616c7,1);
            uint16_t cs=0x2e0f,ds=0x6000,ss=0x8000,sp=0xf000;
            word(u,0x8f000,0);word(u,0x8f002,0x5500);word(u,0x8f004,h);
            check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&ds));check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
            check(uc_emu_start(u,0x33673,0x55000,0,100000));
        }
        for(unsigned bucket=0;bucket<4;++bucket)draw_trail_particles(&race,bucket);
        if(memcmp(native,dos,64000)){fprintf(stderr,"Mixed actor ordering failed trial=%u\n",trial);return 1;}
        for(unsigned bucket=4;bucket;--bucket)restore_trail_particles(&race,bucket-1);
        if(memcmp(native,before,64000)){fprintf(stderr,"Mixed actor restoration failed trial=%u\n",trial);return 1;}
    }
    check(uc_close(u));puts("Shared weapon/trail pool: 128 saturated/reused mixed-priority full-screen and restoration cases pass");return 0;
}
