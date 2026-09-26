#define main damage_oracle_main
#include "verify_dos_damage.c"
#undef main
static short configured[200][13];
static void actor_config(uc_engine *u,uint64_t at,uint32_t size,void *context)
{
    (void)at;(void)size;(void)context;
    uint16_t ss,sp,cs,ip;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss));check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;ip=readword(u,stack);cs=readword(u,stack+2);sp+=4;
    unsigned handle=readword(u,stack+4);
    if(handle>=200)abort();
    for(unsigned i=0;i<13;++i)configured[handle][i]=(short)readword(u,stack+4+i*2);
    check(uc_reg_write(u,UC_X86_REG_IP,&ip));check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    static unsigned char runtime[300000],raw[65536],packed[16384];
    static struct SlicksRaceRuntime race;
    FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    size_t n=fread(runtime,1,sizeof runtime,f);fclose(f);
    uc_engine *u;check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n));uc_hook hook;
    check(uc_hook_add(u,&hook,UC_HOOK_CODE,actor_config,0,0x32ef2,0x32ef2));
    unsigned char tables[25];check(uc_mem_read(u,0x3cbf0 + 0x1be,tables,sizeof tables));
    check(uc_mem_write(u,0x601be,tables,sizeof tables));
    word(u,0x605b8,0);word(u,0x605ba,0x9000);word(u,0x605bc,0);word(u,0x605be,0xa000);
    word(u,0x3cbf0 + 0x16ce,0);word(u,0x3cbf0 + 0x16d0,0xb000);
    check(uc_mem_write(u,0xa0000,packed,sizeof packed));
    unsigned cases=0;
    for(unsigned trial=0;trial<4000;++trial) {
        for(unsigned p=0;p<60800;++p) {
            unsigned v=(p+trial*13)%97;
            race.material_map[p]=v<2?2:v<4?19:0;
            race.surface_map[p]=v==5?2:0;
            raw[p]=(race.material_map[p]<<3)|race.surface_map[p];
        }
        check(uc_mem_write(u,0x90000,raw,sizeof raw));
        race.navigation.actor_count=10;word(u,0x63124,10);
        race.boundary_level=trial%6;word(u,0x64c6c,race.boundary_level);
        for(unsigned i=0;i<10;++i) {
            struct SlicksTrackActor *a=&race.navigation.actors[i];
            a->kind=(i+trial)%5;a->layer=(i+trial)&1;
            a->x=(short)(16*(100+(int)(trial%7))+i%3-1);a->y=(short)(16*80+i%3-1);
            a->velocity_x=(short)((trial*11+i*13)%321-160);
            a->velocity_y=(short)((trial*17+i*7)%321-160);
            /* Stationary objects must still sample layers and accept car
             * impulses, including the original negative-coordinate cases. */
            if(trial>=2000)a->velocity_x=a->velocity_y=0;
            if(trial%11==0) { a->x=-17;a->y=320; }
            word(u,0x63126 + i*2,a->x);word(u,0x631ee + i*2,a->y);
            word(u,0x632b6 + i*2,a->velocity_x);word(u,0x6337e + i*2,a->velocity_y);
            unsigned char b=a->kind;check(uc_mem_write(u,0x63446 + i,&b,1));
            b=a->layer;check(uc_mem_write(u,0x634aa + i,&b,1));
            b=i+5;check(uc_mem_write(u,0x6350e + i,&b,1));
        }
        for(unsigned d=0;d<4;++d) {
            struct SlicksRaceCar *c=&race.cars[d];
            c->x=100*(100+(int)(trial%7))+(int)((trial+d)%9-4)*100;
            c->y=8000+(int)((trial/9+d)%9-4)*100;
            c->velocity_x=(int)((trial*31+d*227)%12001)-6000;
            c->velocity_y=(int)((trial*7+d*331)%14001)-7000;
            c->measured_speed=trial%6==0?300:trial%6==1?-100:1000;
            c->actor_layer=(trial+d)&1;
            dword(u,0x6538c + d*4,c->x);dword(u,0x6539c + d*4,c->y);
            dword(u,0x6682e + d*4,c->velocity_x);dword(u,0x6683e + d*4,c->velocity_y);
            dword(u,0x6684e + d*4,c->measured_speed);
            check(uc_mem_write(u,0x65388 + d,&c->actor_layer,1));
        }
        uint16_t cs=0x1987,ds=0x6000,ss=0x8000,bp=0xf000,sp=0xef00,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_BP,&bp));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x1ff97,0x202eb,0,1000000));check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        if(ip+0x19870!=0x202eb)abort();
        update_track_actor_motion(&race);
        for(unsigned i=0;i<10;++i) {
            struct SlicksTrackActor *a=&race.navigation.actors[i];unsigned char layer;
            check(uc_mem_read(u,0x634aa + i,&layer,1));
            if(a->x!=(short)readword(u,0x63126 + i*2) || a->y!=(short)readword(u,0x631ee + i*2) ||
               a->velocity_x!=(short)readword(u,0x632b6 + i*2) || a->velocity_y!=(short)readword(u,0x6337e + i*2) || a->layer!=layer) {
                fprintf(stderr,"Track actor mismatch trial %u actor %u: xy %d,%d/%d,%d velocity %d,%d/%d,%d layer %u/%u\n",trial,i,a->x,a->y,(short)readword(u,0x63126 + i*2),(short)readword(u,0x631ee + i*2),a->velocity_x,a->velocity_y,(short)readword(u,0x632b6 + i*2),(short)readword(u,0x6337e + i*2),a->layer,layer);return 1;
            }
        }
        for(unsigned d=0;d<4;++d)if(race.cars[d].velocity_x!=readdword(u,0x6682e + d*4) || race.cars[d].velocity_y!=readdword(u,0x6683e + d*4)) {
            fprintf(stderr,"Actor car impulse mismatch trial %u driver %u\n",trial,d);return 1;
        }
        cases+=10;
    }
    printf("Track actors: %u original motion/layer/wall/contact updates match\n",cases);
    check(uc_mem_read(u,0x3cbf0+0x1c3,race.track_flag_styles,205));
    check(uc_mem_write(u,0x601c3,race.track_flag_styles,205));
    race.track_actors_ready=1;
    for(unsigned style=0;style<200;++style) {
        memset(configured,0,sizeof configured);
        race.track_actor_scratch=style;word(u,0x8efc4,style);
        race.random_state=0x12345678;dword(u,0x62aaa,race.random_state);
        for(unsigned i=0;i<10;++i) {
            struct SlicksTrackActor *a=&race.navigation.actors[i];
            a->kind=i%5;a->x=(short)(-19+i*401);a->y=(short)(-17+i*73);a->layer=i&1;
            race.track_actor_handles[i]=i+5;
            word(u,0x63126+i*2,a->x);word(u,0x631ee +i*2,a->y);
            check(uc_mem_write(u,0x63446+i,&a->kind,1));check(uc_mem_write(u,0x634aa+i,&a->layer,1));
        }
        uint16_t cs=0x1987,ds=0x6000,ss=0x8000,bp=0xf000,sp=0xef00;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_BP,&bp));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x22879,0x22981,0,1000000));
        activate_track_flags(&race);
        for(unsigned i=0;i<10;++i) {
            unsigned h=i+5;struct SlicksWeaponActor *a=&race.weapons.actors[h];
            if(race.navigation.actors[i].kind!=2) { if(configured[h][0])abort();continue; }
            unsigned char period,priority,occlusion;
            check(uc_mem_read(u,0xb0000+h*64+0x23,&period,1));
            check(uc_mem_read(u,0xb0000+h*64+0x25,&priority,1));
            check(uc_mem_read(u,0xb0000+h*64+0x3b,&occlusion,1));
            if(a->motion.x!=(short)(configured[h][1]*64) || a->motion.y!=(short)(configured[h][2]*64) ||
                a->motion.period!=(signed char)period || a->priority!=priority || a->occlusion!=occlusion) {
                fprintf(stderr,"Flag activation mismatch style %u actor %u\n",style,i);return 1;
            }
        }
        if(race.random_state!=(uint32_t)readdword(u,0x62aaa))abort();
    }
    puts("Finish flags: all 200 aliased style indices match original coordinates, priority, masking, period and RNG");
    check(uc_close(u));return 0;
}
