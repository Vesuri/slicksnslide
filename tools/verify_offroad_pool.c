#define main damage_oracle_main
#include "verify_dos_damage.c"
#undef main
static unsigned available,allocated,next_handle;
static short tuples[200][13];
static void pool_service(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size;(void)context;uint16_t ss,sp,cs,ip,ax=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss));check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    if(address==0x332ad) {
        if(available) { --available;++allocated;ax=next_handle++; }
        check(uc_reg_write(u,UC_X86_REG_AX,&ax));
    } else {
        unsigned h=readword(u,stack+4);if(h>=200)abort();
        for(unsigned i=0;i<13;++i)tuples[h][i]=(short)readword(u,stack+4+i*2);
        /* Mirror the fields initialized by 32ef2 before its caller applies
         * permanent-mark overrides. Capturing arguments alone leaves the
         * dust actor's lifetime/state/priority uninitialized in this oracle. */
        word(u,0xb0000+h*64+0x3e,(unsigned short)tuples[h][9]);
        unsigned char mask=(unsigned char)tuples[h][10];
        unsigned char state=(unsigned char)tuples[h][11];
        unsigned char priority=(unsigned char)tuples[h][12];
        check(uc_mem_write(u,0xb0000+h*64+0x3b,&mask,1));
        check(uc_mem_write(u,0xb0000+h*64+0x1a,&state,1));
        check(uc_mem_write(u,0xb0000+h*64+0x25,&priority,1));
    }
    ip=readword(u,stack);cs=readword(u,stack+2);sp+=4;
    check(uc_reg_write(u,UC_X86_REG_IP,&ip));check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    static unsigned char runtime[300000],raw[65536],packed[65536];
    static struct SlicksRaceRuntime race;
    FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    size_t n=fread(runtime,1,sizeof runtime,f);fclose(f);
    uc_engine *u;check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));check(uc_mem_write(u,0x10100,runtime,n));
    uc_hook h;check(uc_hook_add(u,&h,UC_HOOK_CODE,pool_service,0,0x332ad,0x332ad));
    check(uc_hook_add(u,&h,UC_HOOK_CODE,pool_service,0,0x32ef2,0x32ef2));
    const unsigned ds=0x3cbf0;
    word(u,ds+0x5b8,0);word(u,ds+0x5ba,0x9000);word(u,ds+0x5bc,0);word(u,ds+0x5be,0xa000);
    word(u,ds+0x16ce,0);word(u,ds+0x16d0,0xb000);
    check(uc_mem_write(u,0xa0000,packed,sizeof packed));
    unsigned cases=0;
    for(unsigned full=0;full<3;++full)for(unsigned layer=0;layer<2;++layer)
    for(unsigned long_lived=0;long_lived<2;++long_lived)for(unsigned material=0;material<32;++material)
    for(unsigned speed=0;speed<5;++speed) {
        static const int speeds[]={200,201,250,251,4000};
        memset(&race,0,sizeof race);memset(tuples,0,sizeof tuples);
        race.track_actors_ready=1;race.random_state=cases*117U+3;
        slicks_actor_slots_init(&race.weapons.slots);race.weapons.slots.high_water=200;
        for(unsigned i=0;i<200;++i) { race.weapons.slots.state[i]=1;race.weapons.trail_index[i]=-1; }
        for(unsigned i=200-full;i<200;++i)race.weapons.slots.state[i]=0;
        available=full;allocated=0;next_handle=200-full;
        for(unsigned i=0;i<60800;++i) { race.material_map[i]=layer?0:material;race.surface_map[i]=layer?material:0;raw[i]=layer?material&7:material<<3; }
        memset(packed,layer?(material>>3)*85:0,sizeof packed);
        check(uc_mem_write(u,0x90000,raw,sizeof raw));check(uc_mem_write(u,0xa0000,packed,sizeof packed));
        unsigned char l=layer;check(uc_mem_write(u,ds+0x5388,&l,1));dword(u,ds+0x684e,speeds[speed]);dword(u,ds+0x2aaa,race.random_state);
        memset(raw,0,sizeof raw);check(uc_mem_write(u,0xb0000,raw,200*64));
        uint16_t cs=0x1987,dseg=0x3cbf,ss=0x8000,sp=0xf000;
        word(u,0x8f000,0);word(u,0x8f002,0x7000);
        word(u,0x8f004,100);word(u,0x8f006,80);word(u,0x8f008,65);word(u,0x8f00a,0);word(u,0x8f00c,long_lived);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&dseg));check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x1e7b1,0x70000,0,1000000));
        emit_offroad_wheel(&race,100,80,65,layer,speeds[speed],long_lived);
        if(race.trail_particle_count!=allocated || race.random_state!=(uint32_t)readdword(u,ds+0x2aaa)) {
            fprintf(stderr,"Offroad pool mismatch case %u free=%u material=%u speed=%d count=%u/%u seed=%lx/%lx\n",cases,full,material,speeds[speed],race.trail_particle_count,allocated,race.random_state,(unsigned long)(uint32_t)readdword(u,ds+0x2aaa));return 1;
        }
        for(unsigned i=0;i<allocated;++i) {
            unsigned handle=200-full+i;const struct SlicksTrailParticle *p=&race.trail_particles[i];
            unsigned char priority,mask,state;
            check(uc_mem_read(u,0xb0000+handle*64+0x25,&priority,1));check(uc_mem_read(u,0xb0000+handle*64+0x3b,&mask,1));check(uc_mem_read(u,0xb0000+handle*64+0x1a,&state,1));
            if(p->x!=(short)(tuples[handle][1]*64) || p->y!=(short)(tuples[handle][2]*64) ||
                p->velocity_x!=tuples[handle][3] || p->velocity_y!=tuples[handle][4] ||
                p->priority!=priority || p->permanent!=(state==5) ||
                p->lifetime!=readword(u,0xb0000+handle*64+0x3e) || mask!=layer*15) {
                fprintf(stderr,"Offroad actor tuple mismatch case %u actor %u xy=%ld,%ld/%d,%d v=%d,%d/%d,%d priority=%u/%u permanent=%u/%u life=%u/%u mask=%u/%u\n",cases,i,p->x,p->y,(short)(tuples[handle][1]*64),(short)(tuples[handle][2]*64),p->velocity_x,p->velocity_y,tuples[handle][3],tuples[handle][4],p->priority,priority,p->permanent,state==5,p->lifetime,readword(u,0xb0000+handle*64+0x3e),mask,layer*15);return 1;
            }
        }
        ++cases;
    }
    printf("Offroad pool: %u original material/threshold/allocation/RNG/actor tuple cases match\n",cases);
    check(uc_close(u));return 0;
}
