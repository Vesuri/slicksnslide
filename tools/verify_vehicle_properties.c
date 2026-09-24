#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/race_runtime.c"
#include "host_archive.h"
#include "../src/game/driver_input.h"
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned a,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(u,a,b,2)); }
static unsigned readword(uc_engine *u,unsigned a)
{ unsigned char b[2]; check(uc_mem_read(u,a,b,2)); return b[0]|b[1]<<8; }
struct Stream { unsigned char data[34]; unsigned at; };
static void project(unsigned char *ds,unsigned driver,const struct SlicksCarProperties *p)
{
    ds[0x4e84+driver]=p->body_radius_x; ds[0x4e88+driver]=p->body_radius_y;
    ds[0x4e8c+driver]=p->collision_radius; ds[0x4ed8+driver]=p->model_class;
    ds[0x4e90+driver]=p->property_4; ds[0x4e98+driver]=p->property_5;
    ds[0x4e94+driver]=p->property_6;
    for(unsigned group=0;group<5;++group)
        for(unsigned channel=0;channel<3;++channel)
            ds[0x4e9c+group*12+channel*4+driver]=(unsigned char)p->surface[group][channel];
    ds[0x4edc+driver]=p->collision_weight;
    ds[0x4dba+2*driver]=(unsigned short)p->drive_bias;
    ds[0x4dbb+2*driver]=(unsigned short)p->drive_bias>>8;
    ds[0x4ee0+driver]=p->effect_profile; ds[0x4ee4+driver]=(unsigned char)p->engine_sound;
    ds[0x4ee8+driver]=p->collision_sound; ds[0x4eec+driver]=p->surface_sound;
    ds[0x4ef0+driver]=p->smoke_profile; ds[0x4ef4+driver]=p->property_29;
    ds[0x4ef8+driver]=(unsigned char)p->auxiliary_accumulator;
    ds[0x4f00+driver]=p->impact_resistance; ds[0x4efc+driver]=p->property_33;
}
static void io(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)size; struct Stream *s=p; uint16_t ss,sp,cs,ip,ax=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=16U*ss+sp;
    if(address==0x12d8a) { if(s->at>=34) abort(); ax=s->data[s->at++]; }
    ip=readword(u,stack); cs=readword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_CS,&cs));
    check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void keyboard_byte(uc_engine *u,uint64_t address,uint32_t size,void *opaque)
{
    (void)address; (void)size;
    uint16_t ax=*(unsigned char *)opaque,ss,sp,cs,ip;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    ip=readword(u,16U*ss+sp); cs=readword(u,16U*ss+sp+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_CS,&cs));
    check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}

static void verify_driver_keys(uc_engine *u,const unsigned char *runtime)
{
    unsigned char scan=0; uc_hook hook;
    check(uc_hook_add(u,&hook,UC_HOOK_CODE,keyboard_byte,&scan,0x36c97,0x36c97));
    unsigned cases=0;
    for(unsigned seed=0;seed<=256;++seed)
    for(unsigned event=0;event<256;++event) {
        unsigned char keys[20],order[4],controls[4],raw[20];
        for(unsigned i=0;i<20;++i)
            keys[i]=seed==256?runtime[0x3cbf0-0x10100+0x6ac+i]:
                (unsigned char)(seed+(seed&1?i*17:0));
        unsigned permutation=seed%24,available[4]={0,1,2,3};
        for(unsigned i=0;i<4;++i) {
            unsigned pick=permutation%(4-i); permutation/=4-i;
            order[i]=(unsigned char)available[pick];
            for(unsigned j=pick;j+1<4-i;++j) available[j]=available[j+1];
            if(seed==255) order[i]=2; /* Original spare groups can alias. */
            controls[i]=(unsigned char)((seed*7+i*13+event)&31);
            for(unsigned action=0;action<5;++action)
                raw[i*5+action]=(controls[i]&(1U<<action))?128:0;
        }
        check(uc_mem_write(u,0x3cbf0+0x5358,keys,sizeof keys));
        check(uc_mem_write(u,0x3cbf0+0x4bf2,order,sizeof order));
        check(uc_mem_write(u,0x3cbf0+0x5344,raw,sizeof raw));
        scan=(unsigned char)event;
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xee00,ip,final_sp;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x1aa3c,0x1aabb,0,2000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&final_sp));
        check(uc_mem_read(u,0x3cbf0+0x5344,raw,sizeof raw));
        slicks_driver_key(controls,keys,order,scan);
        if(cs*16U+ip!=0x1aabb || final_sp!=sp) abort();
        for(unsigned i=0;i<4;++i) for(unsigned action=0;action<5;++action)
            if(!!(controls[i]&(1U<<action))!=!!raw[i*5+action]) {
                fprintf(stderr,"Driver key mismatch seed=%u event=%u driver=%u action=%u\n",seed,event,i,action); exit(1);
            }
        ++cases;
    }
    check(uc_hook_del(u,hook));
    printf("DOS keyboard: %u full callback cases match press/release, group order, duplicate bindings and all five actions\n",cases);
}

static void verify_participant_gates(uc_engine *u)
{
    static struct SlicksRaceRuntime race;
    unsigned cases=0;
    for(unsigned driver=0;driver<4;++driver)
    for(int role=-128;role<=127;++role) {
        race.participation_ready=1; race.participation[driver]=(signed char)role;
        unsigned char byte=(unsigned char)role;
        check(uc_mem_write(u,0x3cbf0+0x4bc6+driver,&byte,1));
        const unsigned starts[]={0x202f3,0x221af,0x2035f};
        const unsigned ends[]={driver_role(&race,driver)?0x202fd:0x214df,
            driver_role(&race,driver)?0x221b9:0x23d8b,
            driver_role(&race,driver)>0?0x20368:0x20373};
        for(unsigned gate=0;gate<3;++gate) {
            uint16_t cs=0x1987,ds=0x3cbf,bx=driver,ip;
            check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
            check(uc_reg_write(u,UC_X86_REG_BX,&bx));
            check(uc_emu_start(u,starts[gate],ends[gate],0,10));
            check(uc_reg_read(u,UC_X86_REG_IP,&ip));
            if(cs*16U+ip!=ends[gate]) {
                fprintf(stderr,"Participant gate mismatch driver=%u role=%d gate=%u\n",driver,role,gate); exit(1);
            }
            ++cases;
        }
    }
    printf("DOS participants: %u motion/tail/AI signed-role branch cases match\n",cases);
}

static void verify_profile_steering(uc_engine *u)
{
    unsigned cases=0;
    for(unsigned scale=0;scale<256;++scale)
    for(int participation=-128;participation<=127;++participation) {
        unsigned driver=scale%4,profile=7+driver*23;
        unsigned char value=(unsigned char)scale,role=(unsigned char)participation;
        word(u,0x3cbf0+0x44c+driver*2,profile);
        check(uc_mem_write(u,0x3cbf0+0x3f42+profile,&value,1));
        check(uc_mem_write(u,0x3cbf0+0x4bc6+driver,&role,1));
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,bp=0xf000,sp=0xee00,ip;
        word(u,0x8f000-0x68,driver);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_emu_start(u,0x1fcf5,0x1fd3d,0,100));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        if(cs*16U+ip!=0x1fd3d || profile_steering_input(scale,participation)!=
           (short)readword(u,0x8f000-0x48+2*driver)) {
            fprintf(stderr,"Profile steering mismatch scale=%u role=%d\n",scale,participation); exit(1);
        }
        ++cases;
    }
    printf("DOS profile steering: %u scale/role combinations match word input, including AI values above 255\n",cases);
}

static void verify_position_scale(uc_engine *u)
{
    const int32_t values[]={0,1,-1,1999,-2001,123456,-654321,INT32_MAX,INT32_MIN};
    unsigned cases=0;
    for(unsigned driver=0;driver<4;++driver)
    for(unsigned scale=0;scale<256;++scale)
    for(unsigned v=0;v<9;++v)
    for(unsigned p=0;p<9;++p) {
        /* Distinct selected profile slots catch confusing driver/profile
         * indices; execute both axes and real multiply/divide helpers. */
        unsigned profile=7+driver*23;
        unsigned char byte=(unsigned char)scale;
        word(u,0x3cbf0+0x44c+driver*2,profile);
        check(uc_mem_write(u,0x3cbf0+0x3f42+profile,&byte,1));
        const unsigned slots[]={0x682e,0x683e,0x538c,0x539c};
        const int32_t input[]={values[v],values[(v+4)%9],values[p],values[(p+3)%9]};
        for(unsigned i=0;i<4;++i) {
            word(u,0x3cbf0+slots[i]+4*driver,(uint32_t)input[i]);
            word(u,0x3cbf0+slots[i]+4*driver+2,(uint32_t)input[i]>>16);
        }
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,bp=0xf000,sp=0xee00,ip;
        word(u,0x8f000-0x68,driver);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_emu_start(u,0x21230,0x212bc,0,1000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        if(16U*cs+ip!=0x212bc) abort();
        for(unsigned axis=0;axis<2;++axis) {
            unsigned a=0x8f000-(axis?0x74:0x70);
            uint32_t expected=readword(u,a)|((uint32_t)readword(u,a+2)<<16);
            if((uint32_t)scaled_position(input[axis+2],input[axis],scale)!=expected) {
                fprintf(stderr,"Position scale mismatch driver=%u scale=%u velocity=%u position=%u axis=%u\n",driver,scale,v,p,axis);
                exit(1);
            }
        }
        ++cases;
    }
    printf("DOS position scaling: %u paired-axis cases match every byte scale and signed multiply/add overflow\n",cases);
}

static void verify_drive_setup(uc_engine *u,const unsigned char *runtime)
{
    /* Execute original table copies and all seven interpolations, including
     * the seventh output consumed by the steering instruction at 20ccb. */
    check(uc_mem_write(u,0x3cbf0+0x1175,runtime+0x3cbf0-0x10100+0x1175,91));
    unsigned cases=0;
    for(unsigned driver=0;driver<4;++driver)
    for(unsigned a=0;a<=20;++a)
    for(unsigned b=0;b<=20;++b)
    for(unsigned c=0;c<=20;++c) {
        struct SlicksRaceCar car={0};
        car.drive_setup[0]=a; car.drive_setup[1]=b; car.drive_setup[3]=c;
        for(unsigned slot=0;slot<13;++slot)
            word(u,0x3cbf0+0x6a7a+26*driver+2*slot,car.drive_setup[slot]);
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xee00,ip;
        word(u,0x8ee04,driver);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x2e032,0x2e0f7,0,2000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        if(16U*cs+ip!=0x2e0f7) abort();
        interpolate_drive_coefficients(&car);
        for(unsigned k=0;k<7;++k)
            if((unsigned short)car.drive_coefficients[k]!=readword(u,0x3cbf0+0x6ae2+14*driver+2*k)) {
                fprintf(stderr,"Drive setup mismatch car=%u upgrades=%u/%u/%u coefficient=%u\n",driver,a,b,c,k);
                exit(1);
            }
        ++cases;
    }
    printf("DOS drive setup: %u complete seven-coefficient interpolations match all supported upgrade combinations\n",cases);
}

static void verify_oil_spin(uc_engine *u)
{
    static struct SlicksRaceRuntime race;
    const int32_t speeds[]={0,1,9,10,12345,-12345,INT32_MIN,INT32_MAX};
    const short headings[]={-32768,-1,0,19199,19200,19201,32767};
    const short ticks[]={-32768,-1,0,1,2,5};
    unsigned count=0;
    for(unsigned seed=0;seed<128;++seed)
    for(unsigned mode=0;mode<8;++mode)
    for(unsigned t=0;t<6;++t) {
        unsigned driver=seed%4;
        unsigned char data[65536]={0};
        memset(&race,0,sizeof race);
        struct SlicksRaceCar *car=&race.cars[driver];
        car->effective_surface=(mode&4)?0:18;
        car->oil_active=mode&1;
        car->oil_turn_sign=(signed char)(seed*37);
        car->heading=headings[seed%7];
        car->measured_speed=speeds[seed%8];
        race.properties[0].collision_sound=(mode&2)?0:1;
        race.random_state=(uint32_t)(seed*0x9e3779b9U);
        project(data,driver,&race.properties[0]);
        data[0x5378+driver]=car->effective_surface;
        data[0x4c70+driver]=car->oil_active;
        data[0x4c78+driver]=(unsigned char)car->oil_turn_sign;
        check(uc_mem_write(u,0x3cbf0,data,sizeof data));
        word(u,0x3cbf0+0x681c+2*driver,(unsigned short)car->heading);
        word(u,0x3cbf0+0x684e +4*driver,(uint32_t)car->measured_speed);
        word(u,0x3cbf0+0x6850+4*driver,(uint32_t)car->measured_speed>>16);
        word(u,0x3cbf0+0x2aaa,race.random_state);
        word(u,0x3cbf0+0x2aac,race.random_state>>16);
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,bp=0xf000,sp=0xee00,ip;
        word(u,0x8f000-0x68,driver); word(u,0x8f000-2,(unsigned short)ticks[t]);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_emu_start(u,0x231bd,0x238eb,0,10000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        apply_oil_spin(&race,car,(unsigned short)ticks[t]);
        unsigned char latch,sign;
        check(uc_mem_read(u,0x3cbf0+0x4c70+driver,&latch,1));
        check(uc_mem_read(u,0x3cbf0+0x4c78+driver,&sign,1));
        uint32_t rng=readword(u,0x3cbf0+0x2aaa)|((uint32_t)readword(u,0x3cbf0+0x2aac)<<16);
        if(16U*cs+ip!=0x238eb || (unsigned short)car->heading!=readword(u,0x3cbf0+0x681c+2*driver) ||
           race.random_state!=rng || car->oil_active!=latch || (unsigned char)car->oil_turn_sign!=sign) {
            fprintf(stderr,"Oil spin mismatch seed=%u mode=%u ticks=%d heading=%d/%d rng=%08lx/%08x latch=%u/%u sign=%d/%d\n",
                seed,mode,ticks[t],car->heading,(short)readword(u,0x3cbf0+0x681c+2*driver),race.random_state,rng,
                car->oil_active,latch,car->oil_turn_sign,(signed char)sign); exit(1);
        }
        ++count;
    }
    printf("DOS oil spin: %u heading/RNG/entry-latch/sign comparisons pass, including signed ticks and overflow\n",count);
}

static void verify_surface_velocity(uc_engine *u)
{
    const unsigned char surfaces[]={0,3,4,5,6,7,8,11,12,18,19};
    const short biases[]={-32768,-200,0,310,32767};
    const short ticks[]={-32768,-1,0,1,2,7};
    const int32_t velocities[]={0,1,-1,12345,-23456,65537,INT32_MAX,INT32_MIN};
    const unsigned addresses[]={0x53e6,0x5406,0x541e,0x53d6,0x53de};
    const unsigned bases[]={0x7dd4,0x7ee9,0x8118,0x7c31,0x8000};
    unsigned cases=0;
    for(unsigned driver=0;driver<4;++driver)
    for(unsigned s=0;s<sizeof surfaces;++s)
    for(unsigned b=0;b<sizeof biases/sizeof *biases;++b)
    for(unsigned t=0;t<sizeof ticks/sizeof *ticks;++t)
    for(unsigned v=0;v<sizeof velocities/sizeof *velocities;++v)
    for(unsigned enabled=0;enabled<2;++enabled) {
        unsigned char data[65536]={0};
        struct SlicksCarProperties properties={0};
        struct SlicksRaceCar car={0};
        properties.collision_sound=enabled;
        project(data,driver,&properties);
        data[0x5378+driver]=surfaces[s];
        /* Isolate the velocity outputs; verify_oil_spin separately checks
         * oil RNG, heading and entry state against the original dispatch. */
        data[0x4c70+driver]=1;
        check(uc_mem_write(u,0x3cbf0,data,sizeof data));
        for(unsigned i=0;i<5;++i)
            word(u,0x3cbf0+addresses[i]+2*driver,(unsigned short)(bases[i]-biases[b]));
        car.velocity_x=velocities[v];
        car.velocity_y=velocities[(v+3)%8];
        car.drive_bias=biases[b]; car.effective_surface=surfaces[s];
        const unsigned at[]={0x682e,0x683e};
        const int32_t input[]={car.velocity_x,car.velocity_y};
        for(unsigned axis=0;axis<2;++axis) {
            word(u,0x3cbf0+at[axis]+4*driver,(uint32_t)input[axis]);
            word(u,0x3cbf0+at[axis]+4*driver+2,(uint32_t)input[axis]>>16);
        }
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,bp=0xf000,sp=0xee00,ip;
        word(u,0x8f000-0x68,driver); word(u,0x8f000-2,(unsigned short)ticks[t]);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_emu_start(u,0x231bd,0x238eb,0,10000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        if(16U*cs+ip!=0x238eb) abort();
        apply_surface_velocity(&car,&properties,(unsigned short)ticks[t]);
        const int32_t actual[]={car.velocity_x,car.velocity_y};
        for(unsigned axis=0;axis<2;++axis) {
            unsigned a=0x3cbf0+at[axis]+4*driver;
            uint32_t expected=readword(u,a)|((uint32_t)readword(u,a+2)<<16);
            if((uint32_t)actual[axis]!=expected) {
                fprintf(stderr,"Surface velocity mismatch driver=%u surface=%u bias=%d ticks=%d velocity=%u gate=%u axis=%u got=%d expected=%d\n",
                    driver,surfaces[s],biases[b],ticks[t],v,enabled,axis,actual[axis],(int32_t)expected);
                exit(1);
            }
        }
        ++cases;
    }
    printf("DOS surface velocity: %u paired-component cases match signed ticks, bias, property gate and low-dword overflow\n",cases);
}

static void verify_limits(uc_engine *u)
{
    static unsigned char zero[65536];
    check(uc_mem_write(u,0x3cbf0,zero,sizeof zero));
    const short ticks[]={-1,0,1,2};
    unsigned cases=0;
    for(unsigned value=0;value<256;++value)
    for(unsigned surface=0;surface<32;++surface)
    for(unsigned t=0;t<4;++t) {
        unsigned driver=value%4;
        struct SlicksCarProperties properties={0};
        struct SlicksRaceCar car={0};
        properties.property_4=(unsigned char)(value+7);
        properties.property_6=(unsigned char)(value+19);
        properties.collision_sound=value&1;
        for(unsigned group=0;group<5;++group)
            for(unsigned channel=0;channel<3;++channel)
                properties.surface[group][channel]=(signed char)(value+group*31+channel*7);
        unsigned char data[65536]; memset(data,0,sizeof data);
        project(data,driver,&properties);
        data[0x5378+driver]=surface; data[0x4c70+driver]=1;
        check(uc_mem_write(u,0x3cbf0,data,sizeof data));
        const unsigned damping[]={0x53d6,0x53de,0x53e6,0x5406,0x541e};
        for(unsigned i=0;i<5;++i) word(u,0x3cbf0+damping[i]+2*driver,32768);
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,bp=0xf000,sp=0xee00,ip;
        word(u,0x8f000-0x68,driver); word(u,0x8f000-2,(unsigned short)ticks[t]);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_emu_start(u,0x231bd,0x238eb,0,10000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        car.effective_surface=surface;
        update_surface_limits(&car,&properties,(unsigned short)ticks[t]);
        if(16U*cs+ip!=0x238eb ||
            (unsigned short)car.steering_scale!=readword(u,0x3cbf0+0x4c3a+2*driver) ||
            car.maximum_speed!=readword(u,0x3cbf0+0x4c42+2*driver)) {
            fprintf(stderr,"Surface limit mismatch value=%u surface=%u ticks=%d\n",value,surface,ticks[t]); exit(1);
        }
        ++cases;
    }
    printf("DOS surface limits: %u dispatch cases match steering/speed words, unsigned properties, signed product wrap and zero-tick gates\n",cases);
}

int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f); int error=ferror(f); fclose(f);
    if(error || bytes<200000 || bytes==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,bytes));
    struct Stream stream; uc_hook h;
    check(uc_hook_add(u,&h,UC_HOOK_CODE,io,&stream,0x12d8a,0x12d8a));
    check(uc_hook_add(u,&h,UC_HOOK_CODE,io,&stream,0x119c2,0x119c2));
    static struct SlicksRaceRuntime race;
    static unsigned char initial[65536],expected[65536],actual[65536];
    memset(initial,0xa5,sizeof initial);
    unsigned cases=0;
    for(unsigned vehicle=0;vehicle<10;++vehicle)
    for(unsigned variant=0;variant<513;++variant)
    for(unsigned driver=0;driver<4;++driver) {
        char name[16]; snprintf(name,sizeof name,"auto%02u.omi",vehicle);
        if(host_archive_load("ref/SLICKS.000",name,stream.data,34)!=34) return 2;
        if(variant<256) { stream.data[23]=variant; stream.data[24]=255-variant; }
        else if(variant<512) {
            for(unsigned field=0;field<34;++field)
                stream.data[field]=(unsigned char)(variant+field*37+vehicle*11);
            /* Native safety guards reject zero divisors. The original read
             * block does not validate them; its later consumers divide. */
            const unsigned divisors[]={4,6,22,32};
            for(unsigned i=0;i<4;++i) if(!stream.data[divisors[i]]) stream.data[divisors[i]]=1;
        }
        stream.at=0;
        if(slicks_race_add_car_properties(&race,vehicle,stream.data,34)) return 1;
        if(memcmp(race.properties[vehicle].raw,stream.data,34)) return 1;
        memcpy(expected,initial,sizeof expected);
        project(expected,driver,&race.properties[vehicle]);
        check(uc_mem_write(u,0x3cbf0,initial,sizeof initial));
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,bp=0xf000,sp=0xef00,ip;
        word(u,0x8f006,driver); word(u,0x8eff6,0x1234); word(u,0x8eff8,0x5678);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_emu_start(u,0x1cdba,0x1d142,0,10000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        unsigned char threshold; check(uc_mem_read(u,0x3cbf0+0x4ee0+driver,&threshold,1));
        check(uc_mem_read(u,0x3cbf0,actual,sizeof actual));
        if(16U*cs+ip!=0x1d142 || stream.at!=34 ||
            (short)readword(u,0x3cbf0+0x4dba+2*driver)!=race.properties[vehicle].drive_bias ||
            threshold!=race.properties[vehicle].effect_profile || memcmp(actual,expected,sizeof actual)) {
            fprintf(stderr,"Property mismatch vehicle=%u variant=%u driver=%u\n",vehicle,variant,driver); return 1;
        }
        ++cases;
    }
    verify_limits(u);
    verify_surface_velocity(u);
    verify_oil_spin(u);
    verify_drive_setup(u,runtime);
    verify_position_scale(u);
    verify_profile_steering(u);
    verify_participant_gates(u);
    verify_driver_keys(u,runtime);
    check(uc_close(u));
    printf("DOS vehicle properties: %u full property-read block/DS comparisons pass (10 original assets, signed fields, byte-30 discard and all destinations)\n",cases);
    return 0;
}
