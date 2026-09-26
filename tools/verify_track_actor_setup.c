/* Execute the original track-actor constructor loop. Resource allocation is
 * intercepted; all selection, positioning, hiding, priorities and RNG execute. */
#define main damage_oracle_main
#include "verify_dos_damage.c"
#undef main
static unsigned next_handle,frames[200];
static short configured[200][13];
/* The startup/countdown fixture must never reach racing particle assembly. */
unsigned short slicks_advance_particles(struct SlicksTrailParticle *p,unsigned long n,
    unsigned char indices[SLICKS_TRAIL_PRIORITY_COUNT][SLICKS_TRAIL_PARTICLE_MAX],
    unsigned short counts[SLICKS_TRAIL_PRIORITY_COUNT],struct SlicksDirtyPixel *dirty,
    unsigned short *dirty_count,unsigned char *chunky,unsigned long page)
{
    (void)p;(void)n;(void)indices;(void)counts;(void)dirty;(void)dirty_count;(void)chunky;(void)page;
    abort();
}
static void setup_service(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size;(void)context;
    uint16_t ss,sp,cs,ip,ax;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss));check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    if(address==0x330ab) {
        ax=next_handle++;frames[ax]=1;check(uc_reg_write(u,UC_X86_REG_AX,&ax));
    } else {
        unsigned h=readword(u,stack+4);if(h>=200)abort();
        if(address==0x33303) ++frames[h];
        else for(unsigned i=0;i<13;++i)configured[h][i]=(short)readword(u,stack+4+i*2);
    }
    ip=readword(u,stack);cs=readword(u,stack+2);sp+=4;
    check(uc_reg_write(u,UC_X86_REG_IP,&ip));check(uc_reg_write(u,UC_X86_REG_CS,&cs));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    static unsigned char runtime[300000];static struct SlicksRaceRuntime race;
    FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    size_t n=fread(runtime,1,sizeof runtime,f);fclose(f);
    uc_engine *u;check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));check(uc_mem_write(u,0x10100,runtime,n));
    uc_hook hook;
    check(uc_hook_add(u,&hook,UC_HOOK_CODE,setup_service,0,0x330ab,0x330ab));
    check(uc_hook_add(u,&hook,UC_HOOK_CODE,setup_service,0,0x33303,0x33303));
    check(uc_hook_add(u,&hook,UC_HOOK_CODE,setup_service,0,0x32ef2,0x32ef2));
    const unsigned ds=0x3cbf0;
    word(u,ds+0x16ce,0);word(u,ds+0x16d0,0xb000);
    unsigned cases=0;
    for(unsigned count=0;count<=100;++count)for(unsigned trial=0;trial<8;++trial) {
        memset(&race,0,sizeof race);memset(frames,0,sizeof frames);memset(configured,0,sizeof configured);
        race.track_actors_ready=1;race.participation_ready=1;race.navigation.actor_count=count;
        race.random_state=0x12345678U+count*117U+trial;
        dword(u,ds+0x2aaa,race.random_state);word(u,ds+0x3124,count);next_handle=5;
        for(unsigned i=0;i<count;++i) {
            struct SlicksTrackActor *a=&race.navigation.actors[i];
            a->kind=(i+trial)%5;a->x=(short)(i*331+trial-33);a->y=(short)(i*51+trial-19);
            word(u,ds+0x3126+i*2,a->x);word(u,ds+0x31ee +i*2,a->y);
            check(uc_mem_write(u,ds+0x3446+i,&a->kind,1));
        }
        uint16_t cs=0x1987,dseg=0x3cbf,ss=0x8000,bp=0xf000,sp=0xef00;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&dseg));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_BP,&bp));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x24ee4,0x25044,0,10000000));
        static unsigned char logical[0x40000],chunky[64000];
        race.navigation.zone_count=1;race.font.ready=1;
        for(unsigned v=0;v<SLICKS_VEHICLE_COUNT;++v) {
            race.properties[v].ready=1;
            for(unsigned d=0;d<SLICKS_CAR_BASE_DIRECTIONS;++d)race.sprites[v][d].ready=1;
        }
        for(unsigned i=0;i<SLICKS_START_LIGHT_COUNT;++i)race.start_lights[i].ready=1;
        for(unsigned i=0;i<14;++i) {
            race.track_actor_assets[i].width=race.track_actor_assets[i].height=1;
            race.track_actor_assets[i].pixels[0]=i+1;
        }
        memset(logical,40,sizeof logical);memset(chunky,77,sizeof chunky);
        /* A new race must not reuse pointers or masks from a previous track. */
        memset(race.track_draw_packets,0xa5,sizeof race.track_draw_packets);
        if(slicks_race_start(&race,logical,chunky))abort();
        for(unsigned i=0;i<64;++i)if(race.track_draw_packets[i].valid)abort();
        if(race.random_state!=(uint32_t)readdword(u,ds+0x2aaa) || next_handle!=count+5)abort();
        for(unsigned i=0;i<count;++i) {
            unsigned h=i+5;struct SlicksWeaponActor *a=&race.weapons.actors[h];
            unsigned char priority,period;
            check(uc_mem_read(u,0xb0000+h*64+0x25,&priority,1));
            check(uc_mem_read(u,0xb0000+h*64+0x23,&period,1));
            if(race.track_actor_handles[i]!=h || a->kind!=3 || a->asset!=race.navigation.actors[i].kind ||
               a->motion.x!=(short)(configured[h][1]*64) || a->motion.y!=(short)(configured[h][2]*64) ||
               a->motion.frames!=frames[h] || a->motion.period!=(signed char)period ||
               a->priority!=priority || a->occlusion!=configured[h][10] || race.weapons.slots.state[h]!=configured[h][11]) {
                fprintf(stderr,"Track setup mismatch count=%u trial=%u actor=%u\n",count,trial,i);return 1;
            }
        }
        /* Initial-frame objects already exist, and saved-under unwinds to
         * source scenery, not the deliberately poisoned previous buffer. */
        restore_start_light(&race,0);restore_race_actors(&race,0);
        for(unsigned i=0;i<64000;++i)if(chunky[i]!=40)abort();
        if(count==5) {
            unsigned long seed=race.random_state;
            /* Keep the movable fixture within retained map bounds. */
            for(unsigned i=0;i<count;++i) {
                race.navigation.actors[i].x=1600+i*160;
                race.navigation.actors[i].y=1280;
            }
            for(unsigned frame=0;frame<20;++frame)slicks_race_step(&race,logical);
            if(race.racing || race.frame_count!=20 || race.trail_particle_count || race.random_state!=seed || race.collision_error)abort();
            for(unsigned i=0;i<count;++i) {
                unsigned h=race.track_actor_handles[i];
                if(race.weapons.actors[h].motion.frames==4 && !race.weapons.actors[h].motion.frame)abort();
            }
            for(unsigned d=0;d<4;++d)if(race.cars[d].x || race.cars[d].y)abort();
        }
        ++cases;
    }
    printf("Track actor setup: %u original count/order/position/hiding/frame/priority/RNG cases match\n",cases);
    puts("First-frame background restoration and countdown actor animation/car gating pass");
    check(uc_close(u));return 0;
}
