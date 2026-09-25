/* Run the original damage routine and its real multiply/divide/RNG helpers.
 * This establishes the state contract before connecting native damage. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/race_runtime.c"
#include "host_archive.h"

/* Existing isolated special-state fixture, not the production scheduler. */
static void update_car(struct SlicksRaceRuntime *race, unsigned short index,
                       unsigned short ticks)
{
    race->cars[index].position_scale=100; /* Fixture's original profile setting. */
    unsigned char controls=prepare_car_motion(race,index,ticks);
    finish_car_update(race,index,ticks,controls);
}

static void check(uc_err e)
{
    if (e) { fprintf(stderr, "%s\n", uc_strerror(e)); exit(1); }
}
static void word(uc_engine *uc, unsigned at, unsigned value)
{
    uint8_t b[2]={value,value>>8};
    check(uc_mem_write(uc,at,b,2));
}
static unsigned readword(uc_engine *uc, unsigned at)
{
    uint8_t b[2]; check(uc_mem_read(uc,at,b,2));
    return b[0] | b[1]<<8;
}
static void dword(uc_engine *uc, unsigned at, long value)
{
    word(uc,at,(uint32_t)value); word(uc,at+2,(uint32_t)value>>16);
}
static long readdword(uc_engine *uc, unsigned at)
{
    return (int32_t)(readword(uc,at)|(uint32_t)readword(uc,at+2)<<16);
}
struct JumpSoundCall { unsigned count, handle, flags, priority; };
struct ShadowCall { unsigned count, args[13]; };
static void verify_track_response(uc_engine *uc)
{
    static const int32_t velocities[]={0,1,-1,16,-16,861,-861,
                                       0x40000000,INT32_MIN,INT32_MAX};
    static unsigned char lower[60800],upper[60800],raw[65536],packed[16384];
    static const int offsets[]={-1,-320,1,320};
    unsigned cases=0;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    word(uc,0x605b8,0); word(uc,0x605ba,0x9000);
    word(uc,0x605bc,0); word(uc,0x605be,0xa000);
    for(unsigned index=0;index<4;++index)
    for(unsigned mask=0;mask<16;++mask)
    for(unsigned layer=0;layer<2;++layer)
    for(unsigned sx=0;sx<10;++sx)
    for(unsigned sy=0;sy<10;++sy) {
        static struct SlicksRaceRuntime race;
        memset(&race,0,sizeof race);
        struct SlicksRaceCar *car=&race.cars[index];
        short x=100+index,y=80+index;
        for(unsigned n=0;n<4;++n) {
            unsigned at=y*320+x+offsets[n];
            unsigned material=(mask&(1U<<n))?2:0;
            lower[at]=layer?0:material; upper[at]=layer?material:0;
            raw[at]=(lower[at]<<3)|upper[at];
        }
        check(uc_mem_write(uc,0x90000,raw,sizeof raw));
        check(uc_mem_write(uc,0xa0000,packed,sizeof packed));
        memcpy(race.material_map,lower,sizeof lower);
        memcpy(race.surface_map,upper,sizeof upper); race.boundary_level=5;
        car->actor_layer=layer; car->collision_sampling=1;
        car->velocity_x=velocities[sx]; car->velocity_y=velocities[sy];
        car->measured_speed=velocities[(sx+sy)%10];
        car->pending_damage_impact=1234;
        uint16_t cs=0x1987,ds=0x6000,ss=0x8000,sp=0xf000,ip;
        word(uc,0x8f000,0); word(uc,0x8f002,0x7000);
        word(uc,0x8f004,x); word(uc,0x8f006,y); word(uc,0x8f008,index);
        word(uc,0x64c6c,5); word(uc,0x63058+index*54,0);
        uint8_t enabled=1,selected=layer;
        check(uc_mem_write(uc,0x64daa+index,&enabled,1));
        check(uc_mem_write(uc,0x65388+index,&selected,1));
        dword(uc,0x6682e + index*4,car->velocity_x);
        dword(uc,0x6683e + index*4,car->velocity_y);
        dword(uc,0x6684e + index*4,car->measured_speed);
        dword(uc,0x6304b+index*0x36,1234);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x1c63e,0x70000,0,3000));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip)); check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
        check(uc_reg_read(uc,UC_X86_REG_CS,&cs));
        resolve_track_velocity(&race,car,x,y);
        if(cs*16U+ip!=0x70000 || sp!=0xf004 || race.collision_error ||
           car->velocity_x!=readdword(uc,0x6682e + index*4) ||
           car->velocity_y!=readdword(uc,0x6683e + index*4) ||
           car->x!=readdword(uc,0x6538c+index*4) ||
           car->y!=readdword(uc,0x6539c+index*4) ||
           car->pending_damage_impact!=readdword(uc,0x6304b+index*0x36)) {
            fprintf(stderr,"Track response mismatch car=%u mask=%u layer=%u velocity=%d/%d\n",
                    index,mask,layer,velocities[sx],velocities[sy]);
            exit(1);
        }
        long pending=car->pending_damage_impact;
        record_track_contact(&race,car,1);
        record_track_contact(&race,car,1);
        record_track_contact(&race,car,0);
        if(car->pending_damage_impact!=pending) exit(1);
        ++cases;
    }
    printf("DOS track response: %u complete original calls match velocity, position and branch-specific damage\n",cases);
}
static void track_map_read(uc_engine *uc, uc_mem_type type, uint64_t address,
                           int size, int64_t value, void *user)
{
    (void)uc; (void)type; (void)size; (void)value;
    if ((address >= 0x90000+60800 && address < 0xa0000) ||
        address >= 0xa0000+15200)
        *(unsigned *)user = 1;
}

static void load_physics_track(unsigned char *lower,unsigned char *upper,
                           struct SlicksTrackNavigation *result,const char *path)
{
    static unsigned char dat[65536],track[8192],arena[65536],logical[0x40000],masks[65536];
    struct SlicksTrackNavigation navigation;
    FILE *file=fopen("ref/SLICKS.DAT","rb");
    if(!file) { perror("pit fixture DAT"); exit(1); }
    size_t dat_size=fread(dat,1,sizeof dat,file); int error=ferror(file); fclose(file);
    if(error || !dat_size || dat_size==sizeof dat) exit(1);
    file=fopen(path,"rb");
    if(!file) { perror("pit fixture track"); exit(1); }
    size_t track_size=fread(track,1,sizeof track,file); error=ferror(file); fclose(file);
    if(error || track_size<368 || track_size==sizeof track) exit(1);
    long mask_size=host_archive_load("ref/SLICKS.000","masks",masks,sizeof masks);
    if(mask_size<=0 || slicks_build_track_scene_options(logical,lower,upper,
        dat,dat_size,track,track_size,arena,sizeof arena,&navigation,1,0)<=0 ||
       slicks_build_track_masks(lower,upper,masks,mask_size,track,track_size,
        arena,sizeof arena,1,track[6+0x165+4],&navigation)<0) {
        fputs("Could not decode original BASIC pit masks\n",stderr); exit(1);
    }
    if(result) *result=navigation;
}
static void load_pit_track(unsigned char *lower,unsigned char *upper,
                           struct SlicksTrackNavigation *result)
{ load_physics_track(lower,upper,result,"ref/TRACKS/BASIC.SS"); }
static void load_pit_masks(unsigned char *lower,unsigned char *upper)
{ load_pit_track(lower,upper,0); }

static void verify_track_walk(uc_engine *uc,int real_pit)
{
    static const short paths[][4]={
        {100,80,100,80},{100,80,101,80},{100,80,99,80},
        {100,80,100,81},{100,80,100,79},{100,80,130,91},
        {130,91,100,80},{100,80,111,110},{111,110,100,80},
        {100,110,130,80},{130,80,100,110},{100,80,130,110},
        {10,10,309,179},{309,179,10,10}
    };
    static struct SlicksRaceRuntime race;
    static unsigned char raw[65536],packed[16384];
    static unsigned char pit_lower[60800],pit_upper[60800];
    unsigned cases=0,rejected=0,unretained_read=0;
    if(real_pit) load_pit_masks(pit_lower,pit_upper);
    uc_hook map_hook;
    check(uc_hook_add(uc,&map_hook,UC_HOOK_MEM_READ,track_map_read,
                     &unretained_read,0x90000,0xaffff));
    check(uc_ctl_remove_cache(uc,0,0x100000));
    word(uc,0x605b8,0); word(uc,0x605ba,0x9000);
    word(uc,0x605bc,0); word(uc,0x605be,0xa000);
    for(unsigned index=0;index<4;++index)
    for(unsigned path=0;path<(real_pit?990:sizeof paths/sizeof paths[0]);++path)
    for(unsigned pattern=0;pattern<(real_pit?1:5);++pattern)
    for(unsigned layer=0;layer<2;++layer)
    for(unsigned enabled=0;enabled<2;++enabled) {
        short x,y,nx,ny;
        if(real_pit) {
            x=209+(path/9)%10; y=61+path/90;
            nx=x+(short)(path%3)-1; ny=y+(short)((path/3)%3)-1;
        } else {
            x=paths[path][0]; y=paths[path][1]; nx=paths[path][2]; ny=paths[path][3];
        }
        memset(&race,0,sizeof race);
        memset(packed,0,sizeof packed);
        for(unsigned at=0;at<65536;++at) {
            unsigned px=at%320,py=at/320;
            unsigned blocked=pattern==1 || (pattern==2 && px==(unsigned)x && py==(unsigned)y) ||
                (pattern==3 && (px+py)%7==0) || (pattern==4 && px==115);
            unsigned material=blocked?2:0;
            raw[at]=layer?material:material<<3;
            if(at<60800) {
                race.material_map[at]=layer?0:material;
                race.surface_map[at]=layer?material:0;
                if(real_pit) {
                    race.material_map[at]=pit_lower[at];
                    race.surface_map[at]=pit_upper[at];
                    raw[at]=(pit_lower[at]<<3)|(pit_upper[at]&7);
                    packed[at/4]|=(pit_upper[at]>>3)<<((at&3)*2);
                }
            }
        }
        check(uc_mem_write(uc,0x90000,raw,sizeof raw));
        check(uc_mem_write(uc,0xa0000,packed,sizeof packed));
        struct SlicksRaceCar *car=&race.cars[index];
        race.boundary_level=5; car->actor_layer=layer; car->collision_sampling=enabled;
        car->velocity_x=real_pit?32:861; car->velocity_y=real_pit?0:-713;
        car->measured_speed=real_pit?16:1100;
        car->pending_damage_impact=1234; car->x=nx*100L+17; car->y=ny*100L+23;
        uint16_t cs=0x1987,ds=0x6000,ss=0x8000,sp=0xf000,ip,ax;
        word(uc,0x8f000,0); word(uc,0x8f002,0x7000);
        word(uc,0x8f004,x); word(uc,0x8f006,y); word(uc,0x8f008,nx); word(uc,0x8f00a,ny);
        word(uc,0x8f00c,0xee00); word(uc,0x8f00e,0x8000);
        word(uc,0x8f010,0xee00); word(uc,0x8f012,0x8000);
        word(uc,0x8f014,1); word(uc,0x8f016,0); /* scale=1, no actor probe */
        word(uc,0x8f018,index); word(uc,0x8f01a,0);
        word(uc,0x64c6c,5); word(uc,0x63058+index*54,0);
        uint8_t flag=enabled,selected=layer;
        check(uc_mem_write(uc,0x64daa+index,&flag,1));
        check(uc_mem_write(uc,0x65388+index,&selected,1));
        dword(uc,0x6682e + index*4,car->velocity_x);
        dword(uc,0x6683e + index*4,car->velocity_y);
        dword(uc,0x6684e + index*4,car->measured_speed);
        dword(uc,0x6304b+index*54,car->pending_damage_impact);
        dword(uc,0x6538c+index*4,x*100L+33); dword(uc,0x6539c+index*4,y*100L+77);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss)); check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        unretained_read=0;
        check(uc_emu_start(uc,0x1cb02,0x70000,0,100000));
        check(uc_reg_read(uc,UC_X86_REG_CS,&cs)); check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        check(uc_reg_read(uc,UC_X86_REG_SP,&sp)); check(uc_reg_read(uc,UC_X86_REG_AX,&ax));
        if(cs*16U+ip!=0x70000 || sp!=0xf004) {
            fputs("Original moving ray did not return cleanly\n",stderr); exit(1);
        }
        int actual=move_car_through_track(&race,car,x*100L+33,y*100L+77,0);
        /* Long overflowing rays may address unretained DOS map bytes;
         * keep the fail-loud contract instead of inventing their contents. */
        if(!!race.collision_error != !!unretained_read) {
            fprintf(stderr,"Retained-map mismatch path=%u pattern=%u layer=%u enabled=%u native=%u DOS=%u\n",
                    path,pattern,layer,enabled,race.collision_error,unretained_read); exit(1);
        }
        if(race.collision_error) {
            ++rejected;
            continue;
        }
        if(cs*16U+ip!=0x70000 || sp!=0xf004 || actual!=!!(ax&255) ||
           car->velocity_x!=readdword(uc,0x6682e + index*4) ||
           car->velocity_y!=readdword(uc,0x6683e + index*4) ||
           car->pending_damage_impact!=readdword(uc,0x6304b+index*54) ||
           car->x!=((ax&255)?readdword(uc,0x6538c+index*4):nx*100L+17) ||
           car->y!=((ax&255)?readdword(uc,0x6539c+index*4):ny*100L+23)) {
            fprintf(stderr,"Track walk mismatch car=%u path=%u pattern=%u layer=%u enabled=%u return=%d/%u\n",
                    index,path,pattern,layer,enabled,actual,ax&255); exit(1);
        }
        ++cases;
    }
    check(uc_hook_del(uc,map_hook));
    printf("DOS %s moving ray: %u complete original calls match hit, position, velocity and damage; %u original unretained-map reads rejected\n",real_pit?"BASIC upper-pit":"synthetic",cases,rejected);
}

static void verify_shadow_source(uc_engine *uc)
{
    /* Execute the real constructor and its real planar pixel writer. No
     * rendering stub: DS:4f44 is a generated image, not a car-sprite copy. */
    for (unsigned rx=1;rx<=15;++rx)
        for (unsigned ry=1;ry<=15;++ry) {
            uint16_t cs=0x1000,ds=0x6000,ss=0x8000,sp=0xf000,ip,final_sp;
            unsigned char actual[1100], initial[1100], radii[2]={rx,ry};
            unsigned n=rx+ry, stride=(n+3)/4, width=stride*4;
            memset(initial,0xa5,sizeof initial);
            check(uc_mem_write(uc,0x90000,initial,sizeof initial));
            check(uc_mem_write(uc,0x64e84+1,&radii[0],1));
            check(uc_mem_write(uc,0x64e88+1,&radii[1],1));
            word(uc,0x64f44+0x44,0); word(uc,0x64f46+0x44,0x9000);
            word(uc,0x8f000,0); word(uc,0x8f002,0xa000);
            word(uc,0x8f004,1);
            check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
            check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
            check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
            check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
            check(uc_emu_start(uc,0x1c3fc,0xa0000,0,100000));
            check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
            check(uc_reg_read(uc,UC_X86_REG_SP,&final_sp));
            check(uc_mem_read(uc,0x90000,actual,sizeof actual));
            if(ip || final_sp!=0xf004 || actual[0]!=stride || actual[1]!=n) {
                fprintf(stderr,"shadow source header/return mismatch %u,%u\n",rx,ry);
                exit(1);
            }
            for(unsigned y=0;y<n;++y)
                for(unsigned x=0;x<width;++x) {
                    unsigned at=2+(x&3)*stride*n+y*stride+x/4;
                    unsigned expected=x>=ry && x<n && y>=ry && ((x^y^ry)&1)==0 ? 37:0;
                    if(actual[at]!=expected) {
                        fprintf(stderr,"shadow pixel mismatch radii=%u,%u xy=%u,%u actual=%u expected=%u\n",
                                rx,ry,x,y,actual[at],expected); exit(1);
                    }
                }
            for(unsigned authoritative=0;authoritative<2;++authoritative) {
                static struct SlicksRaceRuntime race;
                static unsigned char chunky[64000], logical[0x40000];
                memset(&race,0,sizeof race);
                memset(chunky,91,sizeof chunky); memset(logical,91,sizeof logical);
                race.chunky=chunky; race.chunky_authoritative=authoritative;
                race.properties[0].body_radius_x=rx;
                race.properties[0].body_radius_y=ry;
                race.cars[0].x=race.cars[0].y=10000;
                race.cars[0].special_drive_state=1000;
                draw_shadows(&race,authoritative?NULL:logical);
                for(unsigned y=0;y<200;++y)
                    for(unsigned x=0;x<320;++x) {
                        unsigned expected=91;
                        if(x>=97 && x<97+width && y>=99 && y<99+n) {
                            unsigned sx=x-97,sy=y-99;
                            unsigned pixel=actual[2+(sx&3)*stride*n+sy*stride+sx/4];
                            if(pixel) expected=pixel;
                        }
                        if(chunky[y*320+x]!=expected ||
                           (!authoritative && read_pixel(logical,NULL,x,y)!=expected)) {
                            fprintf(stderr,"native shadow differs from DOS source %u,%u at %u,%u\n",rx,ry,x,y);
                            exit(1);
                        }
                    }
                restore_shadows(&race,authoritative?NULL:logical);
                for(unsigned at=0;at<sizeof chunky;++at)
                    if(chunky[at]!=91) { fputs("shadow restore mismatch\n",stderr); exit(1); }
                if(race.shadows[0].saved_valid || !race.dirty_row_count) {
                    fputs("shadow saved-under/dirty-state mismatch\n",stderr); exit(1);
                }
            }
        }
    puts("DOS shadow source: 225 constructors match padded checkerboard, transparent inset and palette index 37 (real pixel writer)");
    puts("Native shadow: 450 whole-screen comparisons match original source, both framebuffer paths restore background");
}

static void verify_native_shadow_lifetime(uc_engine *uc)
{
    for(unsigned initial_page=0;initial_page<2;++initial_page) {
        struct SlicksCarShadow shadow={0};
        unsigned page=initial_page;
        for(unsigned frame=0;frame<12;++frame) {
            uint16_t cs=0x3000,ds=0x6000,es=0x9000,ss=0x8000;
            uint16_t bp=0x800,sp=0x700,bx=64,ip;
            unsigned char state,phase=page;
            if(frame==0 || frame==7) { shadow.state=3; shadow.lifetime=3; }
            state=(unsigned char)shadow.state;
            check(uc_mem_write(uc,0x9005a,&state,1));
            word(uc,0x9007e,shadow.lifetime);
            word(uc,0x616ce,0); word(uc,0x616d0,es);
            word(uc,0x807fe,1);
            check(uc_mem_write(uc,0x616c6,&phase,1));
            check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
            check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
            check(uc_reg_write(uc,UC_X86_REG_ES,&es));
            check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
            check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
            check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
            check(uc_reg_write(uc,UC_X86_REG_BX,&bx));
            if(shadow.state>0) {
                check(uc_emu_start(uc,0x33918,0x3394f,0,100));
                check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
                if(ip!=0x394f) { fputs("shadow expiry stop mismatch\n",stderr); exit(1); }
            }
            advance_shadow(&shadow,page);
            check(uc_mem_read(uc,0x9005a,&state,1));
            if((signed char)state!=shadow.state || readword(uc,0x9007e)!=shadow.lifetime) {
                fputs("native shadow lifetime differs from DOS\n",stderr); exit(1);
            }
            page^=1;
        }
    }
    puts("Native shadow lifetime: both 12-pass original-code sequences match, including reactivation");
}
static void inspect_shadow(uc_engine *uc, uint64_t address,
                            uint32_t size, void *opaque)
{
    struct ShadowCall *call=opaque;
    uint16_t sp,ss;
    (void)address; (void)size;
    check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
    check(uc_reg_read(uc,UC_X86_REG_SS,&ss));
    ++call->count;
    for(unsigned i=0;i<13;++i)
        call->args[i]=readword(uc,((unsigned)ss<<4)+sp+4+i*2);
}
static void verify_shadow_dispatch(uc_engine *uc)
{
    static const short states[]={-1,0,500,501,1000,32767};
    struct ShadowCall call={0};
    uc_hook hook;
    uint8_t retf=0xcb;
    check(uc_mem_write(uc,0x32ef2,&retf,1)); /* Actor update/rendering stubbed. */
    check(uc_hook_add(uc,&hook,UC_HOOK_CODE,inspect_shadow,&call,0x32ef2,0x32ef2));
    for(unsigned s=0;s<6;++s) {
        uint16_t cs=0x2000,ds=0x6000,ss=0x8000,bp=0x800,sp=0x700,ip,final_sp;
        uint8_t priority=77;
        memset(&call,0,sizeof call);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
        check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
        check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        word(uc,0x80800-0x68,1);
        word(uc,0x80800-0x18+2,6);
        word(uc,0x63058+0x36,states[s]);
        word(uc,0x653b6+2,263); word(uc,0x653be + 2,70);
        word(uc,0x3cbf0+0x16ce,0); word(uc,0x3cbf0+0x16d0,0x9000);
        check(uc_mem_write(uc,0x90000+6*64+0x25,&priority,1));
        check(uc_emu_start(uc,0x23ebf,0x23f45,0,200));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        check(uc_reg_read(uc,UC_X86_REG_SP,&final_sp));
        check(uc_mem_read(uc,0x90000+6*64+0x25,&priority,1));
        unsigned expected[13]={6,263,70+(unsigned)(states[s]/500),0,0,0,0,33,0,3,0,3,3};
        unsigned active=states[s]>500;
        if(ip!=0x3f45 || final_sp!=sp || call.count!=active ||
           priority!=(active?0:77) ||
           (active && memcmp(call.args,expected,sizeof expected))) {
            fprintf(stderr,"shadow dispatch mismatch state=%d calls=%u priority=%u ip=%x\n",
                    states[s],call.count,priority,ip); exit(1);
        }
    }
    check(uc_hook_del(uc,hook));
    puts("DOS airborne shadow: six actor-call/threshold/priority cases matched (actor renderer stubbed)");
}
static void inspect_jump_sound(uc_engine *uc, uint64_t address,
                               uint32_t size, void *opaque)
{
    struct JumpSoundCall *call=opaque;
    uint16_t sp,ss;
    (void)address; (void)size;
    check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
    check(uc_reg_read(uc,UC_X86_REG_SS,&ss));
    unsigned at=((unsigned)ss<<4)+sp;
    ++call->count;
    call->handle=readword(uc,at+4);
    call->flags=readword(uc,at+6);
    call->priority=readword(uc,at+8);
}
static void stop_contact_sound(uc_engine *uc,uint64_t address,uint32_t size,void *opaque)
{
    (void)address; (void)size; (void)opaque;
    check(uc_emu_stop(uc));
}

static void verify_contact_sound(uc_engine *uc)
{
    uc_hook mixer_hook,end_hook;
    struct JumpSoundCall call={0};
    uint8_t saved,retf=0xcb;
    unsigned cases=0;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    check(uc_mem_read(uc,0x3989b,&saved,1));
    check(uc_mem_write(uc,0x3989b,&retf,1));
    check(uc_hook_add(uc,&mixer_hook,UC_HOOK_CODE,inspect_jump_sound,&call,0x3989b,0x3989b));
    check(uc_hook_add(uc,&end_hook,UC_HOOK_CODE,stop_contact_sound,0,0x23b0f,0x23b0f));
    for(unsigned index=0;index<4;++index)
    for(unsigned current=0;current<2;++current)
    for(unsigned previous=0;previous<2;++previous)
    for(unsigned other=0;other<2;++other)
    for(unsigned last_wall=0;last_wall<2;++last_wall)
    for(unsigned heading=0;heading<16;++heading)
    for(unsigned handle=5;handle<=200;handle+=195) {
        static struct SlicksRaceRuntime race;
        memset(&race,0,sizeof race); memset(&call,0,sizeof call);
        struct SlicksRaceCar *car=&race.cars[index];
        car->actor_contact=current; car->previous_actor_contact=previous;
        car->touching_car=other; car->touching_solid=last_wall;
        car->heading=heading*1200;
        uint16_t cs=0x1987,ds=0x6000,ss=0x8000,bp=0xf000,sp=0xef00,ip;
        uint8_t c=current,p=previous,o=other,h=handle;
        check(uc_mem_write(uc,0x6536c+index,&c,1));
        check(uc_mem_write(uc,0x65370+index,&p,1));
        check(uc_mem_write(uc,0x64dae + index,&o,1));
        check(uc_mem_write(uc,0x64c4f+other,&h,1));
        word(uc,0x8ef98,index);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss)); check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x23ada,0x23c94,0,300));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip)); check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
        emit_contact_sound(&race,car);
        unsigned expected=current&&!previous;
        if(sp!=0xef00 || ip+0x19870!=(expected?0x23b0f:0x23c94) ||
           call.count!=expected || race.sound_event_count!=expected ||
           (expected && (call.handle!=(uint16_t)(int16_t)(int8_t)handle ||
             call.flags!=2 || call.priority!=14 || race.sound_events[0].sample_block!=5+other ||
             race.sound_events[0].flags!=call.flags || race.sound_events[0].priority!=call.priority))) {
            fprintf(stderr,"Contact sound mismatch car=%u current=%u previous=%u other=%u heading=%u\n",
                    index,current,previous,other,heading); exit(1);
        }
        ++cases;
    }
    check(uc_hook_del(uc,mixer_hook)); check(uc_hook_del(uc,end_hook));
    check(uc_mem_write(uc,0x3989b,&saved,1));
    check(uc_ctl_remove_cache(uc,0,0x100000));
    printf("DOS contact sounds: %u original transition/sample/flag/priority cases match (mixer stubbed)\n",cases);
}

struct BurstCalls {
    unsigned count,colour; unsigned args[7][13],colours[7];
    unsigned use_budget,available,next_handle;
};
static void inspect_burst(uc_engine *uc,uint64_t address,uint32_t size,void *opaque)
{
    struct BurstCalls *calls=opaque;
    uint16_t ss,sp,ax=1;
    (void)size;
    check(uc_reg_read(uc,UC_X86_REG_SS,&ss)); check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
    unsigned at=ss*16U+sp;
    if(address==0x332ad) {
        calls->colour=readword(uc,at+4)&255;
        if(calls->use_budget) {
            ax=calls->available?(--calls->available,calls->next_handle++):0;
        }
        check(uc_reg_write(uc,UC_X86_REG_AX,&ax));
    } else {
        if(calls->count>=7) { fputs("Extra collision actor\n",stderr); exit(1); }
        calls->colours[calls->count]=calls->colour;
        for(unsigned i=0;i<13;++i) calls->args[calls->count][i]=readword(uc,at+4+i*2);
        ++calls->count;
    }
}
static void verify_contact_particles(uc_engine *uc)
{
    static const int32_t velocities[]={0,861,-861,INT32_MIN,INT32_MAX};
    static const uint32_t seeds[]={0,1,0x1fadec20};
    struct BurstCalls calls;
    uc_hook constructor,actor;
    uint8_t saved_constructor,saved_actor,retf=0xcb;
    unsigned cases=0;
    check(uc_mem_read(uc,0x332ad,&saved_constructor,1));
    check(uc_mem_read(uc,0x32ef2,&saved_actor,1));
    check(uc_mem_write(uc,0x332ad,&retf,1)); check(uc_mem_write(uc,0x32ef2,&retf,1));
    check(uc_ctl_remove_cache(uc,0,0x100000));
    check(uc_hook_add(uc,&constructor,UC_HOOK_CODE,inspect_burst,&calls,0x332ad,0x332ad));
    check(uc_hook_add(uc,&actor,UC_HOOK_CODE,inspect_burst,&calls,0x32ef2,0x32ef2));
    for(unsigned index=0;index<4;++index)
    for(unsigned partner=0;partner<4;++partner)
    for(unsigned layer=0;layer<2;++layer)
    for(unsigned velocity=0;velocity<5;++velocity)
    for(unsigned seed=0;seed<3;++seed)
    for(unsigned full=0;full<10;++full) {
        static struct SlicksRaceRuntime race;
        memset(&race,0,sizeof race); memset(&calls,0,sizeof calls);
        struct SlicksRaceCar *car=&race.cars[index];
        race.random_state=seeds[seed]; race.collision_colour=200;
        car->actor_contact=1; car->collision_partner=partner; car->actor_layer=layer;
        car->x=10333; car->y=8377;
        car->velocity_x=velocities[velocity]; car->velocity_y=velocities[4-velocity];
        unsigned budget=full>=2?full-2:0;
        if(full==1) race.trail_particle_count=SLICKS_TRAIL_PARTICLE_MAX;
        if(full>=2) {
            race.track_actors_ready=1;
            slicks_actor_slots_init(&race.weapons.slots);
            race.weapons.slots.high_water=200;
            for(unsigned h=0;h<200;++h) {
                race.weapons.slots.state[h]=h<200-budget;
                race.weapons.trail_index[h]=-1;
            }
            calls.use_budget=1;calls.available=budget;calls.next_handle=200-budget;
        }
        uint16_t cs=0x1987,ds=0x6000,ss=0x8000,bp=0xf000,sp=0xef00,ip;
        uint8_t p=partner,l=layer,c=200;
        word(uc,0x8ef98,index); check(uc_mem_write(uc,0x8efb4+index,&p,1));
        check(uc_mem_write(uc,0x8eff5,&c,1)); check(uc_mem_write(uc,0x65388+index,&l,1));
        dword(uc,0x6538c+index*4,car->x); dword(uc,0x6539c+index*4,car->y);
        dword(uc,0x6682e + index*4,car->velocity_x); dword(uc,0x6683e + index*4,car->velocity_y);
        dword(uc,0x62aaa,seeds[seed]);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss)); check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x23b0f,0x23c94,0,100000));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip)); check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
        check(uc_mem_read(uc,0x8efb4+index,&p,1));
        emit_contact_particles(&race,car,index);
        if(full>=2 && race.track_actor_scratch!=(short)readword(uc,0x8efc4)) {
            fputs("Collision burst shared scratch mismatch\n",stderr);exit(1);
        }
        if(ip+0x19870!=0x23c94 || sp!=0xef00 || calls.count!=(partner?3:7) ||
           car->collision_partner!=p || race.random_state!=(uint32_t)readdword(uc,0x62aaa) ||
           race.trail_particle_count!=(full==1?SLICKS_TRAIL_PARTICLE_MAX:full>=2?(budget<calls.count?budget:calls.count):calls.count)) {
            fprintf(stderr,"Collision burst state mismatch car=%u partner=%u velocity=%u seed=%u full=%u\n",
                    index,partner,velocity,seed,full);
            fprintf(stderr,"IP=%x SP=%x calls=%u particles=%lu partner=%u/%u rng=%08x/%08x\n",
                    ip,sp,calls.count,(unsigned long)race.trail_particle_count,car->collision_partner,p,
                    (unsigned)race.random_state,(unsigned)readdword(uc,0x62aaa)); exit(1);
        }
        if(full!=1) for(unsigned i=0;i<race.trail_particle_count;++i) {
            struct SlicksTrailParticle *p=&race.trail_particles[i];
            unsigned *a=calls.args[i];
            if(a[0]!=(full>=2?200-budget+i:1) || p->colour!=calls.colours[i] || p->x!=(short)a[1]*64L ||
               p->y!=(short)a[2]*64L || p->velocity_x!=(short)a[3] || p->velocity_y!=(short)a[4] ||
               a[5] || a[6] || a[7] || a[8] || p->lifetime!=a[9] ||
               p->occlusion_limit!=a[10] || a[11]!=1 || p->priority!=a[12] || p->permanent) {
                fprintf(stderr,"Collision burst tuple mismatch case=%u particle=%u\n",cases,i); exit(1);
            }
        }
        ++cases;
    }
    check(uc_hook_del(uc,constructor)); check(uc_hook_del(uc,actor));
    check(uc_mem_write(uc,0x332ad,&saved_constructor,1));
    check(uc_mem_write(uc,0x32ef2,&saved_actor,1));
    check(uc_ctl_remove_cache(uc,0,0x100000));
    printf("DOS collision bursts: %u original emission/RNG/tuple cases match, including shared-pool zero/seven-slot budgets\n",cases);
}

static void verify_damage_smoke(uc_engine *uc)
{
    static const short damage[]={0,400,401,402,405,430,431,999,32767};
    static const short counters[]={-1,0,1,29,30,32767};
    uint8_t saved_constructor,saved_actor,retf=0xcb;
    struct BurstCalls calls;uc_hook constructor,actor;unsigned cases=0;
    check(uc_mem_read(uc,0x332ad,&saved_constructor,1));
    check(uc_mem_read(uc,0x32ef2,&saved_actor,1));
    check(uc_mem_write(uc,0x332ad,&retf,1));check(uc_mem_write(uc,0x32ef2,&retf,1));
    check(uc_ctl_remove_cache(uc,0,0x100000));
    check(uc_hook_add(uc,&constructor,UC_HOOK_CODE,inspect_burst,&calls,0x332ad,0x332ad));
    check(uc_hook_add(uc,&actor,UC_HOOK_CODE,inspect_burst,&calls,0x32ef2,0x32ef2));
    for(unsigned d=0;d<4;++d) for(unsigned layer=0;layer<2;++layer)
    for(unsigned a=0;a<9;++a) for(unsigned b=0;b<6;++b) for(unsigned full=0;full<4;++full) {
        static struct SlicksRaceRuntime race;memset(&race,0,sizeof race);memset(&calls,0,sizeof calls);
        struct SlicksRaceCar *car=&race.cars[d];
        car->damage[0]=damage[a];car->damage_smoke_ticks=counters[b];
        car->x=10333;car->y=8377;car->actor_layer=layer;
        race.damage_smoke_colour=71;race.random_state=cases*713U+1;
        if(full==1)race.trail_particle_count=SLICKS_TRAIL_PARTICLE_MAX;
        race.track_actor_scratch=123;word(uc,0x8efc4,123);
        if(full>=2) {
            race.track_actors_ready=1;
            slicks_actor_slots_init(&race.weapons.slots);
            race.weapons.slots.high_water=200;
            for(unsigned h=0;h<200;++h) {
                race.weapons.slots.state[h]=(full==2 || h!=199);
                race.weapons.trail_index[h]=-1;
            }
            calls.use_budget=1;calls.available=full-2;calls.next_handle=199;
        }
        uint16_t cs=0x1987,ds=0x6000,ss=0x8000,bp=0xf000,sp=0xef00,ip;
        word(uc,0x8ef98,d);word(uc,0x8efd4+2*d,counters[b]);word(uc,0x6304f+54*d,damage[a]);
        word(uc,0x8eff3,71);word(uc,0x653b6+2*d,100);word(uc,0x653be +2*d,80);
        uint8_t l=layer;check(uc_mem_write(uc,0x65388+d,&l,1));
        word(uc,0x616ce,0);word(uc,0x616d0,0xb000);dword(uc,0x62aaa,race.random_state);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss));check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x239dc,0x23ada,0,100000));check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        emit_damage_smoke(&race,car);
        if(full>=2 && race.track_actor_scratch!=(short)readword(uc,0x8efc4)) {
            fputs("Damage smoke shared scratch mismatch\n",stderr);exit(1);
        }
        if(ip+0x19870!=0x23ada || car->damage_smoke_ticks!=(short)readword(uc,0x8efd4+2*d) ||
           race.random_state!=(uint32_t)readdword(uc,0x62aaa) ||
           race.trail_particle_count!=(full==1?SLICKS_TRAIL_PARTICLE_MAX:full==2?0:calls.count)) exit(1);
        if(full!=1 && race.trail_particle_count) {
            const struct SlicksTrailParticle *p=&race.trail_particles[0];unsigned *v=calls.args[0];
            if(p->colour!=calls.colours[0] || p->x!=(short)v[1]*64L || p->y!=(short)v[2]*64L ||
               p->velocity_x!=(short)v[3] || p->velocity_y!=(short)v[4] || p->lifetime!=v[9] ||
               p->occlusion_limit!=v[10] || p->priority!=7 || p->permanent) exit(1);
        }
        ++cases;
    }
    check(uc_hook_del(uc,constructor));check(uc_hook_del(uc,actor));
    check(uc_mem_write(uc,0x332ad,&saved_constructor,1));check(uc_mem_write(uc,0x32ef2,&saved_actor,1));
    check(uc_ctl_remove_cache(uc,0,0x100000));
    printf("DOS damage smoke: %u threshold/counter/wrap/RNG/actor-tuple cases match\n",cases);
}

static void verify_jump_sound(uc_engine *uc)
{
    uc_hook hook;
    struct JumpSoundCall call={0};
    uint8_t retf=0xcb;
    /* The earlier lifetime oracle translated the next block past this
     * test's stop address. Rebuild that translation for the tighter stop. */
    check(uc_ctl_remove_cache(uc,0x23c94,0x23d3e));
    check(uc_mem_write(uc,0x3989b,&retf,1)); /* Mixer itself is outside scope. */
    check(uc_hook_add(uc,&hook,UC_HOOK_CODE,inspect_jump_sound,&call,0x3989b,0x3989b));
    for(unsigned pending=0;pending<2;++pending)
    for(unsigned raw=7;raw<=200;raw+=193) {
        uint16_t cs=0x2000,ds=0x6000,ss=0x8000,sp=0x700,final_sp,ip;
        uint8_t flag=pending,handle=raw,cleared;
        memset(&call,0,sizeof call);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
        check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_mem_write(uc,0x606c1,&flag,1));
        check(uc_mem_write(uc,0x64c51,&handle,1));
        check(uc_emu_start(uc,0x23c94,0x23cb1,0,100));
        check(uc_reg_read(uc,UC_X86_REG_SP,&final_sp));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        check(uc_mem_read(uc,0x606c1,&cleared,1));
        if(ip!=0x3cb1 || final_sp!=sp || cleared || call.count!=pending ||
           (pending && (call.handle!=(uint16_t)(int16_t)(int8_t)raw ||
                        call.flags!=2 || call.priority!=12))) {
            fprintf(stderr,"jump sound dispatch mismatch pending=%u raw=%u ip=%x sp=%x clear=%u calls=%u handle=%x flags=%u priority=%u\n",
                    pending,raw,ip,final_sp,cleared,call.count,call.handle,call.flags,call.priority); exit(1);
        }
    }
    check(uc_hook_del(uc,hook));
    puts("DOS jump sound: four request/handle cases matched, mixer stubbed");
}
static void verify_surface_contact(uc_engine *uc)
{
    static const int32_t velocities[]={0,1,-1,861,-861,INT32_MIN,INT32_MAX};
    unsigned cases=0;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    for(unsigned index=0;index<4;++index)
    for(unsigned sampling=0;sampling<2;++sampling)
    for(unsigned v=0;v<7;++v) {
        struct SlicksRaceCar car={0};
        uint16_t cs=0x1987,ds=0x6000,ss=0x8000,bp=0xf000,sp=0xef00,ip;
        unsigned char surface=27,latch=sampling,previous=0,actual;
        car.effective_surface=27; car.collision_sampling=sampling;
        car.velocity_x=velocities[v]; car.velocity_y=velocities[6-v];
        word(uc,0x8ef98,index);
        check(uc_mem_write(uc,0x65378+index,&surface,1));
        check(uc_mem_write(uc,0x64daa+index,&latch,1));
        check(uc_mem_write(uc,0x65370+index,&previous,1));
        dword(uc,0x6682e + index*4,car.velocity_x); dword(uc,0x6683e + index*4,car.velocity_y);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss)); check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x231de,0x238eb,0,1000));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip)); check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
        apply_surface_contact(&car);
        check(uc_mem_read(uc,0x64daa+index,&actual,1));
        check(uc_mem_read(uc,0x65370+index,&previous,1));
        if(ip+0x19870!=0x238eb || sp!=0xef00 || car.collision_sampling!=actual ||
           car.previous_actor_contact!=previous || car.velocity_x!=readdword(uc,0x6682e + index*4) ||
           car.velocity_y!=readdword(uc,0x6683e + index*4)) {
            fputs("Surface 27 collision sampling mismatch\n",stderr); exit(1);
        }
        ++cases;
    }
    printf("DOS surface 27: %u signed-velocity/sampling cases match real dispatch\n",cases);
}

static void verify_surface_jump(uc_engine *uc)
{
    static const long speeds[]={-1,0,350,351,1000,65536};
    static const unsigned char properties[]={0,1,255};
    unsigned cases=0;
    /* Use the actual relocated CS and jump table, not just the case body. */
    for(unsigned surface=13;surface<=14;++surface)
    for(unsigned s=0;s<6;++s)
    for(unsigned p=0;p<3;++p) {
        static struct SlicksRaceRuntime race;
        memset(&race,0,sizeof race);
        struct SlicksRaceCar *car=&race.cars[1];
        car->effective_surface=surface; car->measured_speed=speeds[s];
        car->special_drive_target=123;
        car->velocity_x=960; car->velocity_y=-861;
        race.properties[0].model_class=properties[p];
        uint16_t cs=0x1987,ds=0x6000,ss=0x8000,bp=0x800,sp=0x700,ip;
        uint8_t value=surface,zero=0,requested;
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
        check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
        check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        word(uc,0x80800-0x68,1);
        word(uc,0x6305a+0x36,123);
        dword(uc,0x6684e + 4,speeds[s]);
        dword(uc,0x6682e + 4,960); dword(uc,0x6683e + 4,-861);
        check(uc_mem_write(uc,0x65378+1,&value,1));
        check(uc_mem_write(uc,0x64ed8+1,&properties[p],1));
        check(uc_mem_write(uc,0x606c1,&zero,1));
        check(uc_emu_start(uc,0x231de,0x238eb,0,1000));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        check(uc_mem_read(uc,0x606c1,&requested,1));
        unsigned char native_requested=apply_surface_jump(&race,car);
        if(ip!=0xa07b || requested!=native_requested ||
           (int16_t)readword(uc,0x6305a+0x36)!=car->special_drive_target ||
           readdword(uc,0x6682e + 4)!=car->velocity_x ||
           readdword(uc,0x6683e + 4)!=car->velocity_y) {
            fprintf(stderr,"surface jump mismatch surface=%u speed=%ld property=%u ip=%x\n",
                    surface,speeds[s],properties[p],ip); exit(1);
        }
        ++cases;
    }
    printf("DOS surface jumps: %u real jump-table dispatch cases matched\n",cases);
}
static void verify_special_lifetime(uc_engine *uc)
{
    static const short states[]={-32768,-267,-1,0,1,265,266,267,1000,32767};
    static const short targets[]={-1,0,1,700,32767};
    static const unsigned ticks[]={1,2,45};
    unsigned cases=0;
    for(unsigned s=0;s<10;++s)
    for(unsigned t=0;t<5;++t)
    for(unsigned q=0;q<3;++q) {
        struct SlicksRaceCar car={0};
        car.special_drive_state=states[s]; car.special_drive_target=targets[t];
        uint16_t cs=0x2000,ds=0x6000,ss=0x8000,bp=0x800,sp=0x700,ip;
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
        check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
        check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        word(uc,0x80800-0x68,2); word(uc,0x80800-2,ticks[q]);
        word(uc,0x63058+2*0x36,states[s]);
        word(uc,0x6305a+2*0x36,targets[t]);
        check(uc_emu_start(uc,0x23cb1,0x23d3e,0,1000));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        advance_special_state(&car,ticks[q]);
        if(ip!=0x3d3e ||
           (int16_t)readword(uc,0x63058+2*0x36)!=car.special_drive_state ||
           (int16_t)readword(uc,0x6305a+2*0x36)!=car.special_drive_target) {
            fprintf(stderr,"special lifetime mismatch state=%d target=%d ticks=%u\n",
                    states[s],targets[t],ticks[q]); exit(1);
        }
        ++cases;
    }
    printf("DOS special lifetime: %u original-instruction cases matched\n",cases);
}
static void verify_finished_drive(uc_engine *uc)
{
    static const long scalars[]={-3000,0,6999,7000,9999,INT32_MIN,INT32_MAX};
    static const unsigned ticks[]={1,2,205};
    static const short maxima[]={100,400};
    unsigned cases=0;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    for(unsigned index=0;index<4;++index)
    for(unsigned finished=0;finished<2;++finished)
    for(unsigned service=0;service<2;++service)
    for(unsigned scalar=0;scalar<7;++scalar)
    for(unsigned t=0;t<3;++t)
    for(unsigned maximum=0;maximum<2;++maximum) {
        struct SlicksRaceCar car={0};
        car.speed_fixed=scalars[scalar]; car.fuel=0xfffffffcU;
        car.finished=finished; car.service_flags=service; car.maximum_speed=maxima[maximum];
        uint16_t cs=0x2000,ds=0x6000,ss=0x8000,bp=0x800,sp=0x700,ip,final_sp;
        uint8_t rank=finished?1:255,flag=service,one=1,zero=0,actual_latch;
        word(uc,0x80800-0x68,index); word(uc,0x80800-2,ticks[t]);
        check(uc_mem_write(uc,0x64bce +index,&rank,1));
        check(uc_mem_write(uc,0x6305e +54*index,&flag,1));
        check(uc_mem_write(uc,0x65374+index,&zero,1));
        check(uc_mem_write(uc,0x65344+5*index,&one,1));
        word(uc,0x64c42+2*index,maxima[maximum]); word(uc,0x63058+54*index,0);
        dword(uc,0x66862,0); dword(uc,0x6303f+54*index,car.speed_fixed);
        dword(uc,0x6305f+54*index,-4);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss)); check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        /* Include the no-deadline caller gate: finished cars must reach throttle. */
        check(uc_emu_start(uc,0x20373,0x20509,0,1000));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip)); check(uc_reg_read(uc,UC_X86_REG_SP,&final_sp));
        check(uc_mem_read(uc,0x65374+index,&actual_latch,1));
        apply_throttle(&car,ticks[t]);
        if(ip!=0x0509 || final_sp!=sp || car.speed_fixed!=readdword(uc,0x6303f+54*index) ||
           car.fuel!=(uint32_t)readdword(uc,0x6305f+54*index) ||
           car.forward_drive_latch!=actual_latch) {
            fprintf(stderr,"Finished throttle mismatch car=%u finished=%u service=%u scalar=%ld ticks=%u max=%d\n",
                index,finished,service,scalars[scalar],ticks[t],maxima[maximum]); exit(1);
        }
        ++cases;
    }
    printf("DOS throttle: %u caller-gate/fuel/wrap/limit/finished-drive cases match\n",cases);
    unsigned gates=0;
    for(unsigned index=0;index<4;++index)
    for(unsigned other=0;other<4;++other) if(index!=other)
    for(unsigned ranks=0;ranks<4;++ranks) {
        uint16_t cs=0x2000,ds=0x6000,ss=0x8000,bp=0x800,sp=0x700,ip;
        uint8_t active=1,rank_a=(ranks&1)?1:255,rank_b=(ranks&2)?2:255;
        word(uc,0x80800-0x68,index); word(uc,0x80800-0x3a,other);
        check(uc_mem_write(uc,0x64bc6+index,&active,1));
        check(uc_mem_write(uc,0x64bc6+other,&active,1));
        check(uc_mem_write(uc,0x65388+index,&active,1));
        check(uc_mem_write(uc,0x65388+other,&active,1));
        check(uc_mem_write(uc,0x64bce +index,&rank_a,1));
        check(uc_mem_write(uc,0x64bce +other,&rank_b,1));
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss)); check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x22d36,0x22d63,0,100));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        if(ip!=0x2d63) { fputs("Finished car excluded by original collision gate\n",stderr); exit(1); }
        ++gates;
    }
    check(uc_ctl_remove_cache(uc,0,0x100000));
    printf("DOS finished collisions: %u active same-layer pair gates retain finished entrants\n",gates);
}

static void verify_driver_phase_edges(uc_engine *uc)
{
    check(uc_ctl_remove_cache(uc,0,0x100000));
    for(unsigned phase=0;phase<2;++phase)
    for(unsigned index=0;index<4;++index) {
        unsigned start=phase?0x23d8b:0x214df;
        unsigned end=index<3?(phase?0x221ac:0x202f0):(phase?0x23d97:0x214eb);
        uint16_t cs=0x1987,ss=0x8000,bp=0x800,sp=0x700,ip,final_sp;
        word(uc,0x80800-0x68,index);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
        check(uc_reg_write(uc,UC_X86_REG_BP,&bp)); check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,start,end,0,20));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip)); check(uc_reg_read(uc,UC_X86_REG_SP,&final_sp));
        if(cs*16U+ip!=end || final_sp!=sp || readword(uc,0x80800-0x68)!=index+1) {
            fprintf(stderr,"Original driver phase edge mismatch phase=%u car=%u\n",phase,index);
            exit(1);
        }
    }
    check(uc_ctl_remove_cache(uc,0,0x100000));
    puts("DOS driver phases: all four motion-loop and all four tail-loop edges verified");
}

static void verify_pit_integration(uc_engine *uc)
{
    const unsigned data=0x3cbf0,index=2;
    static struct SlicksRaceRuntime race;
    static unsigned char saved[65536],raw[65536],packed[16384];
    static const long positions[][2]={{21139,6850},{21290,6850},{21650,6650},
                                     {21450,6950},{21750,6350}};
    static const long scalars[]={3000,0,-3000};
    static const short headings[]={894,1200,9600,18000};
    static const short biases[]={0,-8,-600,32767};
    static const long velocities[][2]={{32,0},{861,-713},{-861,713}};
    static const unsigned char scales[]={0,10,50,100,255};
    load_pit_masks(race.material_map,race.surface_map);
    for(unsigned at=0;at<60800;++at) {
        raw[at]=(race.material_map[at]<<3)|(race.surface_map[at]&7);
        packed[at/4]|=(race.surface_map[at]>>3)<<((at&3)*2);
    }
    check(uc_mem_write(uc,0x90000,raw,sizeof raw));
    check(uc_mem_write(uc,0xa0000,packed,sizeof packed));
    check(uc_mem_read(uc,data,saved,sizeof saved));
    check(uc_ctl_remove_cache(uc,0,0x100000));
    unsigned cases=0;
    for(unsigned p=0;p<5;++p)
    for(unsigned scalar=0;scalar<3;++scalar)
    for(unsigned heading=0;heading<4;++heading)
    for(unsigned v=0;v<3;++v)
    for(unsigned ticks=1;ticks<=2;++ticks)
    for(unsigned bias=0;bias<4;++bias)
    for(unsigned scale_index=0;scale_index<sizeof scales;++scale_index)
    for(unsigned active=0;active<2;++active) {
        struct SlicksRaceCar *car=&race.cars[index];
        memset(car,0,sizeof *car); race.collision_error=0; race.boundary_level=5;
        car->x=positions[p][0]; car->y=positions[p][1];
        car->speed_fixed=scalars[scalar]; car->heading=headings[heading];
        car->drive_bias=biases[bias];
        car->velocity_x=velocities[v][0]; car->velocity_y=velocities[v][1];
        car->measured_speed=16; car->damage[0]=214;
        car->drive_coefficients[0]=103; car->drive_coefficients[1]=18;
        car->drive_coefficients[3]=99; car->actor_layer=1; car->collision_sampling=1;
        car->pending_damage_impact=1234;
        check(uc_mem_write(uc,data,saved,sizeof saved));
        word(uc,data+0x05b8,0); word(uc,data+0x05ba,0x9000);
        word(uc,data+0x05bc,0); word(uc,data+0x05be,0xa000);
        word(uc,data+0x4c6c,5); word(uc,data+0x3058+54*index,0);
        uint8_t one=1,zero=0,coast=!active,scale=scales[scale_index];
        car->position_scale=scale;
        check(uc_mem_write(uc,data+0x4daa+index,&one,1));
        check(uc_mem_write(uc,data+0x5388+index,&one,1));
        check(uc_mem_write(uc,data+0x536c+index,&zero,1));
        word(uc,data+0x044c+2*index,0); check(uc_mem_write(uc,data+0x3f42,&scale,1));
        word(uc,data+0x681c+2*index,car->heading);
        word(uc,data+0x304f+54*index,214);
        word(uc,data+0x6ae2+14*index,103); word(uc,data+0x6ae4+14*index,18);
        word(uc,data+0x6ae8+14*index,99);
        word(uc,data+0x53ee +2*index,0x7dc2-car->drive_bias);
        word(uc,data+0x53f6+2*index,0x7bd7-car->drive_bias);
        word(uc,data+0x53fe +2*index,0x7bdd-car->drive_bias);
        word(uc,data+0x53e6+2*index,0x7dd4-car->drive_bias);
        dword(uc,data+0x303f+54*index,car->speed_fixed);
        dword(uc,data+0x538c+4*index,car->x); dword(uc,data+0x539c+4*index,car->y);
        dword(uc,data+0x682e +4*index,car->velocity_x);
        dword(uc,data+0x683e +4*index,car->velocity_y);
        dword(uc,data+0x684e +4*index,car->measured_speed);
        dword(uc,data+0x304b+54*index,car->pending_damage_impact);
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,bp=0x800,sp=0x700,ip,final_sp;
        word(uc,0x80800-0x68,index); word(uc,0x80800-2,ticks);
        check(uc_mem_write(uc,0x80800-0x5e +index,&coast,1));
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss)); check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x20dd1,0x214df,0,30000));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip)); check(uc_reg_read(uc,UC_X86_REG_SP,&final_sp));
        integrate_car_motion(&race,car,ticks,active);
        uint8_t contact; check(uc_mem_read(uc,data+0x536c+index,&contact,1));
        if(cs*16U+ip!=0x214df || final_sp!=sp || race.collision_error ||
           car->x!=readdword(uc,data+0x538c+4*index) || car->y!=readdword(uc,data+0x539c+4*index) ||
           car->velocity_x!=readdword(uc,data+0x682e +4*index) ||
           car->velocity_y!=readdword(uc,data+0x683e +4*index) ||
           car->speed_fixed!=readdword(uc,data+0x303f+54*index) ||
           car->pending_damage_impact!=readdword(uc,data+0x304b+54*index) ||
           car->actor_contact!=contact) {
            fprintf(stderr,"Pit integration mismatch p=%u scalar=%ld heading=%d v=%u ticks=%u bias=%d active=%u pc=%x sp=%x speed=%ld/%ld xy=%ld,%ld/%ld,%ld velocity=%ld,%ld/%ld,%ld\n",
                p,scalars[scalar],headings[heading],v,ticks,car->drive_bias,active,cs*16U+ip,final_sp,
                car->speed_fixed,readdword(uc,data+0x303f+54*index),car->x,car->y,
                readdword(uc,data+0x538c+4*index),readdword(uc,data+0x539c+4*index),
                car->velocity_x,car->velocity_y,readdword(uc,data+0x682e +4*index),readdword(uc,data+0x683e +4*index));
            exit(1);
        }
        ++cases;
    }
    check(uc_mem_write(uc,data,saved,sizeof saved));
    check(uc_ctl_remove_cache(uc,0,0x100000));
    printf("DOS pit integration: %u composed force/movement/collision/tick-loop cases match\n",cases);
}

static void verify_world_clamp(uc_engine *uc)
{
    static const long xs[]={INT32_MIN,299,300,301,31699,31700,31701,INT32_MAX};
    static const long ys[]={INT32_MIN,299,300,301,17899,17900,17901,INT32_MAX};
    static const long velocities[]={0,1,-1,861,-713,INT32_MIN,INT32_MAX};
    static const short biases[]={0,-8,-600,32767};
    unsigned cases=0;
    check(uc_ctl_remove_cache(uc,0,0x100000));
    for(unsigned index=0;index<4;++index)
    for(unsigned x=0;x<8;++x)
    for(unsigned y=0;y<8;++y)
    for(unsigned v=0;v<7;++v)
    for(unsigned b=0;b<4;++b) {
        struct SlicksRaceCar car={0};
        car.x=xs[x]; car.y=ys[y]; car.drive_bias=biases[b];
        car.velocity_x=velocities[v]; car.velocity_y=velocities[(v+3)%7];
        uint16_t cs=0x2000,ds=0x6000,ss=0x8000,bp=0x800,sp=0x700,ip,final_sp;
        word(uc,0x80800-0x68,index);
        word(uc,0x653e6+index*2,(unsigned short)(0x7dd4-biases[b]));
        dword(uc,0x6538c+index*4,car.x); dword(uc,0x6539c+index*4,car.y);
        dword(uc,0x6682e +index*4,car.velocity_x);
        dword(uc,0x6683e +index*4,car.velocity_y);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss)); check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x21357,0x214d1,0,2000));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip)); check(uc_reg_read(uc,UC_X86_REG_SP,&final_sp));
        clamp_car_to_track(&car);
        if(ip!=0x14d1 || final_sp!=sp ||
           car.x!=readdword(uc,0x6538c+index*4) || car.y!=readdword(uc,0x6539c+index*4) ||
           car.velocity_x!=readdword(uc,0x6682e +index*4) ||
           car.velocity_y!=readdword(uc,0x6683e +index*4)) {
            fprintf(stderr,"World clamp mismatch car=%u x=%ld y=%ld velocity=%u bias=%d\n",
                    index,xs[x],ys[y],v,biases[b]); exit(1);
        }
        ++cases;
    }
    printf("DOS screen bounds: %u original-instruction position/both-velocity/corner cases match\n",cases);
}

static void verify_special(uc_engine *uc)
{
    unsigned cases=0;
    for(int state=-1;state<=1;state+=2)
    for(int scalar_sign=-1;scalar_sign<=1;scalar_sign+=2)
    for(int bias=0;bias>=-30;bias-=30) {
        static struct SlicksRaceRuntime race;
        memset(&race,0,sizeof race);
        struct SlicksRaceCar *car=&race.cars[0];
        race.human_control=1;
        race.controls=SLICKS_CONTROL_ACCELERATE|SLICKS_CONTROL_BRAKE|
                      SLICKS_CONTROL_LEFT|SLICKS_CONTROL_RIGHT;
        car->special_drive_state=state; car->drive_bias=bias;
        car->x=car->y=10000;
        car->speed_fixed=scalar_sign*10000;
        car->velocity_x=960; car->velocity_y=-860;
        uint16_t cs=0x2000,ds=0x6000,ss=0x8000,bp=0x800,sp=0x700,ip;
        uint8_t coast=1;
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
        check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
        check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        word(uc,0x80800-0x68,0);
        check(uc_mem_write(uc,0x80800-0x5e,&coast,1));
        word(uc,0x63058,state);
        word(uc,0x6540e,0x7ffc-bias); word(uc,0x653fe,0x7bdd-bias);
        dword(uc,0x6303f,car->speed_fixed);
        dword(uc,0x6682e,960); dword(uc,0x6683e,-860);
        check(uc_emu_start(uc,0x20dd9,0x21230,0,1000));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        update_car(&race,0,1);
        if(ip!=0x1230 || readdword(uc,0x6303f)!=car->speed_fixed ||
           readdword(uc,0x6682e)!=car->velocity_x ||
           readdword(uc,0x6683e)!=car->velocity_y) {
            fprintf(stderr,"special-state mismatch state=%d scalar_sign=%d bias=%d\n",
                    state,scalar_sign,bias); exit(1);
        }
        ++cases;
    }
    printf("DOS signed special states: %u original-instruction integration cases matched\n",cases);
}
static void verify_brake(uc_engine *uc)
{
    static const long speeds[]={-1,99,100,65536};
    static const signed char properties[]={0,1,-1};
    static const short biases[]={0,-8,-600};
    static const unsigned ticks[]={1,2,45};
    unsigned cases=0;
    for(unsigned s=0;s<4;++s)
    for(unsigned p=0;p<3;++p)
    for(unsigned b=0;b<3;++b)
    for(unsigned t=0;t<3;++t)
    for(unsigned latch=0;latch<2;++latch)
    for(unsigned damage=998;damage<=999;++damage) {
        static struct SlicksRaceRuntime race;
        memset(&race,0,sizeof race);
        struct SlicksRaceCar *car=&race.cars[1];
        car->measured_speed=speeds[s]; car->drive_bias=biases[b];
        car->damage[0]=damage; car->forward_drive_latch=latch;
        car->speed_fixed=1234; car->maximum_speed=100;
        car->velocity_x=960; car->velocity_y=-860;
        race.properties[0].engine_sound=properties[p];
        uint16_t cs=0x2000,ds=0x6000,ss=0x8000,bp=0x800,sp=0x700,ip,final_sp;
        uint8_t flag=latch,property=(uint8_t)properties[p],actual_flag;
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
        check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
        check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        word(uc,0x80800-0x68,1); word(uc,0x80800-2,ticks[t]);
        word(uc,0x65416+2,(unsigned short)(0x7db5-biases[b]));
        word(uc,0x64c42+2,100);
        word(uc,0x6304f+0x36,damage);
        dword(uc,0x6303f+0x36,1234);
        dword(uc,0x6682e + 4,960); dword(uc,0x6683e + 4,-860);
        dword(uc,0x6684e + 4,speeds[s]);
        check(uc_mem_write(uc,0x65374+1,&flag,1));
        check(uc_mem_write(uc,0x64ee4+1,&property,1));
        check(uc_emu_start(uc,0x205bc,0x206a5,0,20000));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        check(uc_reg_read(uc,UC_X86_REG_SP,&final_sp));
        check(uc_mem_read(uc,0x65374+1,&actual_flag,1));
        apply_brake(&race,car,ticks[t]);
        if(ip!=0x06a5 || final_sp!=sp || actual_flag!=car->forward_drive_latch ||
           readdword(uc,0x6303f+0x36)!=car->speed_fixed ||
           readdword(uc,0x6682e + 4)!=car->velocity_x ||
           readdword(uc,0x6683e + 4)!=car->velocity_y) {
            fprintf(stderr,"brake mismatch case=%u ip=%x sp=%x\n",cases,ip,final_sp);
            exit(1);
        }
        ++cases;
    }
    printf("DOS brake/reverse: %u original-instruction cases matched\n",cases);
}
int main(int argc, char **argv)
{
    static unsigned char runtime[300000];
    static const int32_t impulses[]={-1,0,1,30,300,1000,100000,0x7fffffff};
    static const int16_t scales[]={0,1,100};
    static const int16_t coefficients[]={30,100};
    static const uint8_t resistances[]={1,100};
    static const int8_t signs[]={-1,0,1};
    static const int16_t initial[]={0,990,999,32760};
    FILE *file=fopen(argc>1?argv[1]:"disasm/runtime.bin","rb");
    if (!file) { perror("runtime"); return 1; }
    size_t size=fread(runtime,1,sizeof runtime,file); fclose(file);
    if (size<0xee00 || memcmp(runtime+0xec64,"\xc8\x06\x00\x00\x56\x57",6) ||
        memcmp(runtime+0x3175,"\x56\x96\x92\x85\xc0",5)) {
        fprintf(stderr,"unsupported original image layout\n"); return 1;
    }
    uc_engine *uc;
    check(uc_open(UC_ARCH_X86,UC_MODE_16,&uc));
    check(uc_mem_map(uc,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(uc,0x10100,runtime,size));
    unsigned cases=0;
    for(unsigned i=0;i<sizeof impulses/sizeof *impulses;++i)
    for(unsigned s=0;s<sizeof scales/sizeof *scales;++s)
    for(unsigned c=0;c<sizeof coefficients/sizeof *coefficients;++c)
    for(unsigned r=0;r<sizeof resistances/sizeof *resistances;++r)
    for(unsigned d=0;d<sizeof signs/sizeof *signs;++d)
    for(unsigned enabled=0;enabled<2;++enabled) {
        uint16_t cs=0x1010,ds=0x6000,ss=0x8000,sp=0xf000,final_sp,ip;
        uint8_t flag=enabled,sign=(uint8_t)signs[d],actual_sign;
        uint32_t seed=0x1fadec20u,expected_seed=seed;
        static struct SlicksRaceRuntime race;
        memset(&race,0,sizeof race);
        race.damage_scale=scales[s]; race.damage_enabled=enabled;
        race.random_state=seed;
        race.properties[0].impact_resistance=resistances[r];
        race.cars[0].drive_coefficients[5]=coefficients[c];
        race.cars[0].damage_turn_sign=signs[d];
        for(unsigned channel=0;channel<4;++channel)
            race.cars[0].damage[channel]=initial[channel];
        apply_damage(&race,0,impulses[i]);
        int16_t delta=0;
        int8_t expected_sign=signs[d];
        if(impulses[i]>0 && scales[s] && enabled) {
            int32_t value=(int32_t)((uint32_t)impulses[i]*(uint32_t)scales[s]);
            value/=resistances[r];
            value=(int32_t)((uint32_t)value*(uint32_t)coefficients[c]);
            value/=100;
            value/=2;
            delta=(int16_t)value;
            if(!expected_sign) {
                expected_seed=seed*0x015a4e35u+1;
                expected_sign=(int8_t)(2*((expected_seed>>16 & 0x7fff)*2/32768)-1);
            }
        }
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
        check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        word(uc,0x8f000,0); word(uc,0x8f002,0x9000);
        word(uc,0x8f004,0); /* Driver index. */
        word(uc,0x8f006,(uint32_t)impulses[i]);
        word(uc,0x8f008,(uint32_t)impulses[i]>>16);
        word(uc,0x63026,scales[s]);
        check(uc_mem_write(uc,0x636a6,&flag,1));
        check(uc_mem_write(uc,0x64f00,&resistances[r],1));
        /* Original relocated ES immediate is 3cbfh. */
        word(uc,0x3cbf0+0x6aec,coefficients[c]);
        word(uc,0x62aaa,seed); word(uc,0x62aac,seed>>16);
        check(uc_mem_write(uc,0x63057,&sign,1));
        for(unsigned channel=0;channel<4;++channel)
            word(uc,0x6304f+channel*2,initial[channel]);
        check(uc_emu_start(uc,0x1ed64,0x90000,0,10000));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        check(uc_reg_read(uc,UC_X86_REG_SP,&final_sp));
        check(uc_mem_read(uc,0x63057,&actual_sign,1));
        uint32_t actual_seed=readword(uc,0x62aaa)|(uint32_t)readword(uc,0x62aac)<<16;
        if(ip || final_sp!=0xf004 || actual_seed!=expected_seed ||
           (int8_t)actual_sign!=expected_sign ||
           race.random_state!=actual_seed ||
           race.cars[0].damage_turn_sign!=(int8_t)actual_sign) {
            fprintf(stderr,"damage control mismatch case=%u ip=%x sp=%x sign=%d/%d seed=%x/%x\n",
                    cases,ip,final_sp,(int8_t)actual_sign,expected_sign,actual_seed,expected_seed);
            return 1;
        }
        for(unsigned channel=0;channel<4;++channel) {
            int16_t expected=initial[channel];
            if(impulses[i]>0 && scales[s] && enabled) {
                expected=(int16_t)(expected+delta);
                if(expected>999) expected=999;
            }
            int16_t actual=(int16_t)readword(uc,0x6304f+channel*2);
            if(actual!=expected || race.cars[0].damage[channel]!=actual) {
                fprintf(stderr,"damage mismatch case=%u channel=%u actual=%d expected=%d\n",
                        cases,channel,actual,expected); return 1;
            }
        }
        ++cases;
    }
    verify_brake(uc);
    verify_finished_drive(uc);
    verify_driver_phase_edges(uc);
    verify_pit_integration(uc);
    verify_world_clamp(uc);
    verify_special(uc);
    verify_special_lifetime(uc);
    verify_surface_jump(uc);
    verify_surface_contact(uc);
    verify_contact_sound(uc);
    verify_contact_particles(uc);
    verify_damage_smoke(uc);
    verify_jump_sound(uc);
    verify_shadow_dispatch(uc);
    verify_shadow_source(uc);
    verify_native_shadow_lifetime(uc);
    verify_track_response(uc);
    verify_track_walk(uc,0);
    verify_track_walk(uc,1);
    check(uc_close(uc));
    for (long impact=299; impact<=308; ++impact) {
        static struct SlicksRaceRuntime race;
        memset(&race,0,sizeof race);
        race.damage_scale=100; race.damage_enabled=1;
        race.random_state=0x1fadec20u;
        race.properties[0].impact_resistance=100;
        race.cars[0].drive_coefficients[5]=100;
        race.cars[0].pending_damage_impact=impact;
        race.cars[1].pending_damage_impact=1000;
        consume_car_damage(&race,0);
        unsigned long seed_after=race.random_state;
        consume_car_damage(&race,0);
        if (race.cars[0].damage[0]!=(impact==308) ||
            race.cars[0].pending_damage_impact ||
            race.cars[1].pending_damage_impact!=1000 ||
            race.random_state!=seed_after ||
            ((seed_after!=0x1fadec20u)!=(impact>=304))) {
            fprintf(stderr,"pending impact consumption mismatch at %ld\n",impact);
            return 1;
        }
    }
    printf("DOS damage: %u full-routine cases match native arithmetic, four channels, gating, sign/RNG and stack\n",cases);
    puts("Native damage tail: 10 threshold/one-time consumption fixtures passed");
    return 0;
}
