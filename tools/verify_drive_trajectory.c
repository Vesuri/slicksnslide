/* Composed original motion caller. Game instructions and arithmetic helpers
 * execute unchanged; this is a test harness, never a production CPU context. */
#define main isolated_damage_main
#include "verify_dos_damage.c"
#undef main

#define DATA 0x3cbf0U
#define FRAME 0x80800U
static void octet(uc_engine *u,unsigned at,unsigned value)
{ unsigned char v=value;check(uc_mem_write(u,at,&v,1)); }
static unsigned octet_read(uc_engine *u,unsigned at)
{ unsigned char v;check(uc_mem_read(u,at,&v,1));return v; }
static void map_navigation(uc_engine *u,const struct SlicksTrackNavigation *n)
{
    word(u,DATA+0x5426,n->zone_count);
    octet(u,DATA+0x3641,n->pit_count);
    for(unsigned i=0;i<n->zone_count;++i) {
        for(unsigned j=0;j<3;++j) {
            word(u,DATA+0x5428+j*1200+i*2,n->zones[i].x[j]);
            word(u,DATA+0x5680+j*1200+i*2,n->zones[i].y[j]);
        }
        octet(u,DATA+0x6238+i,n->zones[i].speed);
    }
    word(u,DATA+0x6686,n->alternate_count);
    for(unsigned i=0;i<n->alternate_count;++i) {
        word(u,DATA+0x6688+2*i,n->alternate[i].x);
        word(u,DATA+0x6750+2*i,n->alternate[i].y);
    }
    octet(u,DATA+0x3640,n->pit_route_count);
    for(unsigned i=0;i<n->pit_route_count;++i) {
        octet(u,DATA+0x3642+i,n->pit_route_zone[i]);
        octet(u,DATA+0x3660+i,n->pit_route_destination[i]);
    }
    for(unsigned i=0;i<n->pit_count;++i) {
        word(u,DATA+0x367e +2*i,n->pits[i].x);
        word(u,DATA+0x3692+2*i,n->pits[i].y);
    }
    word(u,DATA+0x6364,n->checkpoint_count);
    for(unsigned i=0;i<n->checkpoint_count;++i) {
        word(u,DATA+0x6366+2*i,n->checkpoints[i].x[0]);
        word(u,DATA+0x642e +2*i,n->checkpoints[i].x[1]);
        word(u,DATA+0x64f6+2*i,n->checkpoints[i].y[0]);
        word(u,DATA+0x65be +2*i,n->checkpoints[i].y[1]);
    }
}
static void map_car(uc_engine *u,const struct SlicksRaceRuntime *r,unsigned d)
{
    const struct SlicksRaceCar *c=&r->cars[d];unsigned s=DATA+54*d;
#define W(at,value) word(u,DATA+(at)+2*d,value)
#define L(at,value) dword(u,DATA+(at)+4*d,value)
#define B(at,value) octet(u,DATA+(at)+d,value)
    L(0x538c,c->x);L(0x539c,c->y);L(0x682e,c->velocity_x);L(0x683e,c->velocity_y);
    L(0x684e,c->measured_speed);W(0x681c,c->heading);
    W(0x53b6,c->x/100-3);W(0x53be,c->y/100-3);
    W(0x68ea,c->ai_last_x/100-3);W(0x68f2,c->ai_last_y/100-3);
    W(0x68fa,c->ai_stuck_ticks);W(0x691a,c->ai_recovery_ticks);W(0x6922,c->ai_turn_ticks);
    W(0x690a,c->ai_target_x);W(0x6912,c->ai_target_y);W(0x6902,c->waypoint);
    W(0x53c6,c->ai_contact_ticks);W(0x2fa4,c->ai_contact_threshold);
    B(0x692a,c->ai_state);B(0x6936,c->ai_service_state);B(0x6932,c->ai_route_seen);
    B(0x692e,c->ai_recovery_right);B(0x4bc6,r->participation[d]);B(0x4bce,c->finished?1:-1);
    B(0x5374,c->forward_drive_latch);B(0x5388,c->actor_layer);B(0x4daa,c->collision_sampling);
    B(0x536c,c->actor_contact);B(0x5370,c->previous_actor_contact);
    B(0x4ee4,r->properties[c->vehicle].engine_sound);
    W(0x044c,d);octet(u,DATA+0x3f42+d,c->position_scale);
    W(0x4c3a,c->steering_scale);W(0x4c42,c->maximum_speed);W(0x4bfe,c->lap-1);
    W(0x53e6,0x7dd4-c->drive_bias);W(0x53ee,0x7dc2-c->drive_bias);
    W(0x53f6,0x7bd7-c->drive_bias);W(0x53fe,0x7bdd-c->drive_bias);
    W(0x540e,0x7ffc-c->drive_bias);W(0x5416,0x7db5-c->drive_bias);
    W(0x5406,0x7ee9-c->drive_bias);W(0x53de,0x8000-c->drive_bias);
    W(0x53d6,0x7c31-c->drive_bias);W(0x541e,0x8118-c->drive_bias);
    B(0x4dae,c->touching_car);B(0x537c,c->selected_surface);B(0x5378,c->effective_surface);
    B(0x4c70,c->oil_active);B(0x4c78,c->oil_turn_sign);
    W(0x53ce,c->collision_safe_x);B(0x6818,c->collision_safe_y);
    word(u,s+0x305c,c->checkpoint);
    const struct SlicksCarProperties *p=&r->properties[c->vehicle];
    B(0x4e84,p->body_radius_x);B(0x4e88,p->body_radius_y);B(0x4e8c,p->collision_radius);
    B(0x4ed8,p->model_class);B(0x4edc,p->collision_weight);B(0x4e90,p->property_4);
    B(0x4e98,p->property_5);B(0x4e94,p->property_6);B(0x4ee0,p->effect_profile);
    B(0x4ee4,p->engine_sound);B(0x4ee8,p->collision_sound);B(0x4eec,p->surface_sound);
    B(0x4ef0,p->smoke_profile);B(0x4ef4,p->property_29);B(0x4ef8,p->auxiliary_accumulator);
    B(0x4f00,p->impact_resistance);B(0x4efc,p->property_33);
    for(unsigned group=0;group<5;++group) for(unsigned ch=0;ch<3;++ch)
        octet(u,DATA+0x4e9c+group*12+ch*4+d,p->surface[group][ch]);
    for(unsigned i=0;i<7;++i) word(u,DATA+0x6ae2+14*d+2*i,c->drive_coefficients[i]);
    for(unsigned i=0;i<4;++i) word(u,s+0x304f+2*i,c->damage[i]);
    dword(u,s+0x303f,c->speed_fixed);dword(u,s+0x304b,c->pending_damage_impact);
    dword(u,s+0x305f,c->fuel);dword(u,s+0x3063,c->fuel_capacity);
    octet(u,s+0x305e,c->service_flags);octet(u,s+0x3057,c->damage_turn_sign);
    word(u,s+0x3058,c->special_drive_state);word(u,s+0x305a,c->special_drive_target);
    for(unsigned i=0;i<5;++i) octet(u,DATA+0x5344+5*d+i,(c->ai_control_latch>>i)&1);
    B(0x2fac,-1);B(0x2fb0,0);B(0x5e2,0);
    for(unsigned heading=0;heading<16;++heading) for(unsigned wheel=0;wheel<2;++wheel) {
        const struct SlicksCarSprite *sprite=&r->sprites[c->vehicle][heading&3];
        unsigned at=d*32+heading+wheel*16;
        octet(u,DATA+0x4ca4+at,sprite->wheel_x[heading>>2][wheel]);
        octet(u,DATA+0x4d24+at,sprite->wheel_y[heading>>2][wheel]);
    }
    word(u,FRAME-0x48+2*d,profile_steering_input(c->position_scale,r->participation[d]));
#undef W
#undef L
#undef B
}
static void compare_car(uc_engine *u,const struct SlicksRaceRuntime *r,unsigned d,unsigned step)
{
    const struct SlicksRaceCar *c=&r->cars[d];
#define SAME(value,expected) do { long v=(long)(value),e=(long)(expected); if(v!=e) {fprintf(stderr,"Motion step %u car %u %s native=%ld DOS=%ld xy=%ld,%ld surface=%u lap=%u/%u\n",step,d,#value,v,e,c->x,c->y,c->selected_surface,c->lap,readword(u,DATA+0x4bfe +2*d)+1);exit(1);} } while(0)
#define W(at,field) SAME(c->field,(short)readword(u,DATA+(at)+2*d))
#define L(at,field) SAME(c->field,readdword(u,DATA+(at)+4*d))
#define B(at,field) SAME(c->field,(signed char)octet_read(u,DATA+(at)+d))
    L(0x538c,x);L(0x539c,y);L(0x682e,velocity_x);L(0x683e,velocity_y);W(0x681c,heading);
    SAME(c->speed_fixed,readdword(u,DATA+0x303f+54*d));
    SAME((int)c->fuel,(int)readdword(u,DATA+0x305f+54*d));
    SAME(c->service_flags,octet_read(u,DATA+0x305e +54*d));
    SAME(c->pending_damage_impact,readdword(u,DATA+0x304b+54*d));
    W(0x68fa,ai_stuck_ticks);W(0x691a,ai_recovery_ticks);W(0x6922,ai_turn_ticks);
    W(0x690a,ai_target_x);W(0x6912,ai_target_y);W(0x6902,waypoint);
    B(0x692a,ai_state);B(0x6936,ai_service_state);B(0x6932,ai_route_seen);
    B(0x692e,ai_recovery_right);B(0x5374,forward_drive_latch);B(0x536c,actor_contact);
    L(0x684e,measured_speed);B(0x5370,previous_actor_contact);B(0x4daa,collision_sampling);
    B(0x5388,actor_layer);B(0x537c,selected_surface);B(0x5378,effective_surface);
    W(0x4c3a,steering_scale);W(0x4c42,maximum_speed);
    W(0x53c6,ai_contact_ticks);W(0x2fa4,ai_contact_threshold);
    B(0x4dae,touching_car);B(0x4c70,oil_active);B(0x4c78,oil_turn_sign);
    SAME(c->checkpoint,readword(u,DATA+0x305c+54*d));
    SAME(c->lap-1,readword(u,DATA+0x4bfe +2*d));
    SAME(c->special_drive_state,(short)readword(u,DATA+0x3058+54*d));
    SAME(c->special_drive_target,(short)readword(u,DATA+0x305a+54*d));
    for(unsigned i=0;i<4;++i) SAME(c->damage[i],(short)readword(u,DATA+0x304f+54*d+2*i));
    SAME(c->ai_last_x/100-3,(short)readword(u,DATA+0x68ea+2*d));
    SAME(c->ai_last_y/100-3,(short)readword(u,DATA+0x68f2+2*d));
    unsigned control=0;for(unsigned i=0;i<4;++i) control|=!!octet_read(u,DATA+0x5344+5*d+i)<<i;
    if(r->participation[d]>0) SAME(c->ai_control_latch,control);
    SAME(c->finished,(signed char)octet_read(u,DATA+0x4bce +d)>=0);
    SAME(r->pit_repair_ticks,(short)readword(u,FRAME-0x64));
    SAME(c->damage_smoke_ticks,(short)readword(u,FRAME-0x2c+2*d));
    SAME(c->collision_partner,octet_read(u,FRAME-0x4c+d));
    SAME(r->finish_deadline,(uint32_t)readdword(u,DATA+0x6862));
    SAME(r->random_state,(uint32_t)readdword(u,DATA+0x2aaa));
#undef B
#undef L
#undef W
#undef SAME
}
static void device_boundary(uc_engine *u,uint64_t address,uint32_t size,void *opaque)
{
    (void)address;(void)size;(void)opaque;
    uint16_t sp,ss,cs,ip,ax=0;
    check(uc_reg_read(u,UC_X86_REG_SP,&sp));check(uc_reg_read(u,UC_X86_REG_SS,&ss));
    ip=readword(u,ss*16U+sp);cs=readword(u,ss*16U+sp+2);sp+=4;
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));check(uc_reg_write(u,UC_X86_REG_CS,&cs));
    check(uc_reg_write(u,UC_X86_REG_IP,&ip));check(uc_reg_write(u,UC_X86_REG_AX,&ax));
}
int main(int argc,char **argv)
{
    unsigned scenario=argc>1?(unsigned)atoi(argv[1]):0;
    static unsigned char runtime[300000],raw[65536],packed[16384];
    FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    size_t size=fread(runtime,1,sizeof runtime,f);fclose(f);
    uc_engine *u;check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));check(uc_mem_write(u,0x10100,runtime,size));
    static struct SlicksRaceRuntime race;
    load_physics_track(race.material_map,race.surface_map,&race.navigation,
        argc>2?argv[2]:"ref/TRACKS/BASIC.SS");
    /* Both instruction ranges execute on independently evolving state. */
    race.participation_ready=1;race.participation[0]=1;race.fuel_option=10;
    race.damage_enabled=1;race.laps_to_run=4;race.boundary_level=5;race.random_state=123;
    race.race_mode=4;
    struct SlicksRaceCar *c=&race.cars[0];
    c->x=c->ai_last_x=13467;c->y=c->ai_last_y=7256;c->heading=7285;
    c->ai_target_x=114;c->ai_target_y=121;c->ai_state=1;c->ai_service_state=2;
    c->ai_stuck_ticks=100;c->ai_recovery_ticks=398;c->ai_contact_ticks=11;c->ai_contact_threshold=50;
    c->ai_route_seen=1;c->ai_control_latch=9;c->damage[0]=214;c->lap=3;
    c->fuel=0xfffffffcU;c->fuel_capacity=3312;c->position_scale=100;
    c->velocity_x=56;c->velocity_y=56;c->measured_speed=56;c->steering_scale=1000;c->steering_property=104;
    c->maximum_speed=200;c->actor_layer=1;c->collision_sampling=1;
    c->drive_coefficients[0]=103;c->drive_coefficients[1]=18;c->drive_coefficients[3]=99;
    c->drive_coefficients[2]=100;c->drive_coefficients[4]=100;c->drive_coefficients[5]=100;
    c->drive_coefficients[6]=104;
    c->service_flags=1;c->damage[0]=0;c->waypoint=4;
    if(scenario==1) {
        c->x=c->ai_last_x=21139;c->y=c->ai_last_y=6850;c->heading=894;
        c->ai_target_x=214;c->ai_target_y=64;c->waypoint=8;
    } else if(scenario>=2 && scenario!=8) {
        c->x=c->ai_last_x=25900;c->y=c->ai_last_y=5700;c->heading=4000;
        c->ai_target_x=-1;c->ai_target_y=0;c->ai_service_state=0;c->ai_state=0;
        c->waypoint=0;c->fuel=c->fuel_capacity=3600;c->lap=1;
        c->ai_stuck_ticks=0;c->ai_recovery_ticks=0;c->velocity_x=c->velocity_y=0;
    }
    unsigned char property_bytes[34];
    if(host_archive_load("ref/SLICKS.000","auto00.omi",property_bytes,sizeof property_bytes)!=34 ||
       slicks_race_add_car_properties(&race,0,property_bytes,34)) return 2;
    /* Cases 0..3 omit wheels to isolate motion. Later cases load real wheel
     * geometry. Audio/HUD/actor construction are external boundaries, but
     * their callers and RNG execute with a full particle pool on both sides. */
    for(unsigned dir=0;dir<4;++dir) for(unsigned rot=0;rot<4;++rot)
        for(unsigned wheel=0;wheel<2;++wheel) race.sprites[0][dir].wheel_x[rot][wheel]=-1;
    if(scenario>=4) for(unsigned vehicle=0;vehicle<10;++vehicle) {
        char name[20];unsigned char resource[128];
        snprintf(name,sizeof name,"auto%02u.omi",vehicle);
        long bytes=host_archive_load("ref/SLICKS.000",name,resource,sizeof resource);
        if(bytes!=34 || slicks_race_add_car_properties(&race,vehicle,resource,bytes)) return 2;
        for(unsigned dir=0;dir<4;++dir) {
            snprintf(name,sizeof name,"auto%02u.%03u",vehicle,dir);
            if(vehicle==9 && !dir) strcpy(name,"car9");
            bytes=host_archive_load("ref/SLICKS.000",name,resource,sizeof resource);
            if(bytes<=0 || slicks_race_add_car_sprite(&race,vehicle,dir,resource,bytes)) return 2;
        }
    }
    race.trail_particle_count=SLICKS_TRAIL_PARTICLE_MAX;
    uc_hook hooks[8];unsigned boundaries[]={0x1d9b6,0x1ddc0,0x39593,0x395b8,0x3989b,0x332ad,0x32ef2,0x33303};
    for(unsigned i=0;i<8;++i) check(uc_hook_add(u,&hooks[i],UC_HOOK_CODE,device_boundary,0,boundaries[i],boundaries[i]));
    for(unsigned i=0;i<128;++i) octet(u,DATA+0x4ca4+i,255);
    word(u,DATA+0x16ce,0);word(u,DATA+0x16d0,0xb000);
    word(u,DATA+0x6364,0);word(u,DATA+0x3124,0);word(u,DATA+0x3026,0);
    for(unsigned at=0;at<60800;++at) {raw[at]=(race.material_map[at]<<3)|(race.surface_map[at]&7);packed[at/4]|=(race.surface_map[at]>>3)<<((at&3)*2);}
    check(uc_mem_write(u,0x90000,raw,sizeof raw));check(uc_mem_write(u,0xa0000,packed,sizeof packed));
    word(u,DATA+0x05b8,0);word(u,DATA+0x05ba,0x9000);word(u,DATA+0x05bc,0);word(u,DATA+0x05be,0xa000);
    word(u,DATA+0x4c6c,5);word(u,DATA+0x3020,0);word(u,DATA+0x3024,10);
    octet(u,DATA+0x36a6,1);word(u,DATA+0x0092,4);word(u,DATA+0x4c18,4);
    dword(u,DATA+0x6862,0);dword(u,DATA+0x2aaa,race.random_state);
    map_navigation(u,&race.navigation);
    if(scenario==7) {race.damage_scale=300;word(u,DATA+0x3026,300);}
    for(unsigned d=0;d<4;++d) {
        if(d && scenario>=3) {race.cars[d]=*c;race.cars[d].y+=d*500;race.cars[d].ai_last_y=race.cars[d].y;race.participation[d]=1;}
        if(scenario>=4) {
            race.cars[d].vehicle=(scenario-4)*4+d;
            race.cars[d].vehicle%=10;
            race.cars[d].drive_bias=race.properties[race.cars[d].vehicle].drive_bias;
        }
        if(argc>2) {
            struct SlicksRaceCar *car=&race.cars[d];
            car->x=race.navigation.start_x*100L+d*100;
            car->y=race.navigation.start_y*100L;
            car->heading=race.navigation.start_heading*120;
            car->ai_last_x=car->x;car->ai_last_y=car->y;
            car->actor_layer=race.navigation.start_style;
        }
        if(scenario==5) race.participation[d]=d==2?0:d==0?-1:1;
        if(scenario==7) {
            race.cars[d].damage[0]=race.cars[d].damage[3]=d*100;
            race.cars[d].x=11400+d*30;race.cars[d].y=12100+d*30;
            race.cars[d].ai_last_x=race.cars[d].x;race.cars[d].ai_last_y=race.cars[d].y;
            race.cars[d].fuel=0;race.cars[d].ai_service_state=2;race.cars[d].ai_state=1;
            race.cars[d].ai_target_x=114;race.cars[d].ai_target_y=121;
        }
        if(scenario==8) {
            race.participation[d]=d==0;
            race.cars[d].vehicle=5;
            race.cars[d].drive_bias=race.properties[5].drive_bias;
            /* Read-only A1200 mixed-fleet observation at update 7200.
             * Resume the unfinished driver in isolation on both machines;
             * this is not a replay of the other three moving finishers. */
            if(!d) {
                c->x=13455;c->y=7251;c->velocity_x=107;c->velocity_y=26;
                c->speed_fixed=2898;c->measured_speed=66;c->heading=5908;
                c->ai_last_x=13495;c->ai_last_y=7254;c->lap=2;c->checkpoint=2;
                c->fuel=0xfffffffeU;c->fuel_capacity=3636;c->maximum_speed=80;
                c->ai_contact_ticks=1;c->forward_drive_latch=1;
                c->collision_safe_x=134;c->collision_safe_y=72;
                race.random_state=4098104851U;race.game_clock_ticks=13108;
                dword(u,DATA+0x2aaa,race.random_state);
            }
        }
        map_car(u,&race,d);
        octet(u,DATA+0x4bce +d,race.participation[d]?-1:0);
    }
    for(unsigned step=0;step<7200;++step) {
        unsigned ticks=step%2+1;
        race.game_clock_ticks+=ticks;
        dword(u,DATA+0x685e,race.game_clock_ticks);
        unsigned controls[4]={0};
        for(unsigned d=0;d<4;++d) if(race.participation[d]<0) {
            race.driver_controls[d]=step%240<180?1:step%240<210?5:10;
            for(unsigned bit=0;bit<5;++bit) octet(u,DATA+0x5344+5*d+bit,(race.driver_controls[d]>>bit)&1);
        }
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,bp=0x800,sp=0x700,ip;
        word(u,FRAME-2,ticks);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_BP,&bp));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        for(unsigned d=0;d<4;++d) if(race.participation[d]) {
        word(u,FRAME-0x68,d);
        check(uc_emu_start(u,0x202f0,0x214df,0,100000));check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        if(ip!=0x214df-0x19870) {fprintf(stderr,"Motion exit %x\n",ip);return 1;}
        controls[d]=prepare_car_motion(&race,d,ticks);compare_car(u,&race,d,step);
        }
        for(unsigned d=0;d<4;++d) if(race.participation[d]) {
        word(u,FRAME-0x68,d);
        check(uc_emu_start(u,0x221ac,0x23d8b,0,100000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        if(ip!=0x23d8b-0x19870) {fprintf(stderr,"Tail exit %x\n",ip);return 1;}
        finish_car_update(&race,d,ticks,(unsigned char)controls[d]);compare_car(u,&race,d,step);
        /* Drawing refreshes sprite origins between updates. Each side uses
         * its own position; never resynchronize divergent simulation state. */
        word(u,DATA+0x53b6+2*d,readdword(u,DATA+0x538c+4*d)/100-3);
        word(u,DATA+0x53be +2*d,readdword(u,DATA+0x539c+4*d)/100-3);
        }
    }
    printf("Composed DOS motion and per-car tail: scenario %u, 7200 updates match; final %ld,%ld service=%d fuel=%d\n",scenario,c->x,c->y,c->ai_service_state,(int)c->fuel);
    return 0;
}
