#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/race_runtime.c"
#define DS 0x3cbf0U
#define BP 0x8f000U
static void ck(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e));exit(1); } }
static void word(uc_engine *u,unsigned a,unsigned v) { unsigned char b[2]={v,v>>8};ck(uc_mem_write(u,a,b,2)); }
static unsigned rd(uc_engine *u,unsigned a) { unsigned char b[2];ck(uc_mem_read(u,a,b,2));return b[0]|b[1]<<8; }
static void byte(uc_engine *u,unsigned a,unsigned v) { unsigned char b=v;ck(uc_mem_write(u,a,&b,1)); }
static unsigned rb(uc_engine *u,unsigned a) { unsigned char b;ck(uc_mem_read(u,a,&b,1));return b; }
static void dw(uc_engine *u,unsigned a,unsigned v) { word(u,a,v);word(u,a+2,v>>16); }
static int rlong(uc_engine *u,unsigned a) { return (int)(rd(u,a)|(rd(u,a+2)<<16)); }
static struct SlicksRaceRuntime race;
static unsigned allocations,sounds;
static struct SlicksSoundEvent sound_trace[64];
static void boundary(uc_engine *u,uint64_t address,uint32_t size,void *ctx)
{
    (void)size;(void)ctx;uint16_t sp,ss;
    ck(uc_reg_read(u,UC_X86_REG_SP,&sp));ck(uc_reg_read(u,UC_X86_REG_SS,&ss));
    unsigned stack=ss*16U+sp;uint16_t ip=rd(u,stack),cs=rd(u,stack+2),ax=0;
    if(address==0x3989b) {
        sound_trace[sounds++]=(struct SlicksSoundEvent){.sample_block=rd(u,stack+4),.flags=rd(u,stack+6),.priority=rd(u,stack+8)};
    } else if(address==0x332ad || address==0x334ef) {
        ++allocations;
        unsigned high=rd(u,DS+0x16c0),cap=rd(u,DS+0x16be);
        for(unsigned h=1;h<high;++h) if(!rb(u,0x90000+h*64+0x1a)) { ax=h;break; }
        if(!ax && high<cap) { ax=high;word(u,DS+0x16c0,high+1); }
        if(ax) {
            unsigned char zero[64]={0};ck(uc_mem_write(u,0x90000+ax*64,zero,64));
            byte(u,0x90000+ax*64+0x1a,1);
            byte(u,0x90000+ax*64+0x3a,address==0x334ef?rd(u,stack+8):1);
        }
    }
    /* 1c5a0 is the only map boundary: an all-zero unobstructed map.
     * The original moving probe, car probe, explosion, radial damage,
     * damage application, RNG and actor configuration execute unchanged. */
    sp+=4;ck(uc_reg_write(u,UC_X86_REG_SP,&sp));ck(uc_reg_write(u,UC_X86_REG_CS,&cs));
    ck(uc_reg_write(u,UC_X86_REG_IP,&ip));ck(uc_reg_write(u,UC_X86_REG_AX,&ax));
}
int main(void)
{
    unsigned char runtime[300000];FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f);fclose(f);
    uc_engine *u;ck(uc_open(UC_ARCH_X86,UC_MODE_16,&u));ck(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    ck(uc_mem_write(u,0x10100,runtime,bytes));
    struct SlicksWeaponRules rules={0};
    for(unsigned i=0;i<8;++i) {
        rules.radius[i]=(signed char)rb(u,DS+0x11e +i);rules.force[i]=(signed char)rb(u,DS+0x136+i);
        rules.damage[i]=(short)rd(u,DS+0x126+i*2);rules.effect[i]=(signed char)rb(u,DS+0x176+i);
        rules.hit_sound[i]=rb(u,DS+0x19e +i);
    }
    const unsigned hook_addresses[]={0x3989b,0x332ad,0x334ef,0x33303,0x1c5a0};uc_hook hooks[5];
    for(unsigned i=0;i<5;++i) ck(uc_hook_add(u,&hooks[i],UC_HOOK_CODE,boundary,0,hook_addresses[i],hook_addresses[i]));
    for(unsigned t=0;t<4096;++t) {
        memset(&race,0,sizeof race);race.weapons.ready=1;race.weapons.rules=rules;
        race.participation_ready=1;race.damage_scale=(t&128)?300:0;race.damage_enabled=1;
        race.random_state=0x12345678U+t;dw(u,DS+0x2aaa,race.random_state);
        race.collision_colour=37;race.weapons.impact_colour=17;
        word(u,BP-0xb,37);word(u,BP-0xc,17);
        word(u,DS+0x3026,race.damage_scale);byte(u,DS+0x36a6,1);
        unsigned char actor_bytes[201*64]={0};ck(uc_mem_write(u,0x90000,actor_bytes,sizeof actor_bytes));
        slicks_actor_slots_init(&race.weapons.slots);
        race.weapons.slots.capacity=(t&256)?14:200;race.weapons.slots.high_water=13;
        for(unsigned h=1;h<13;++h) { race.weapons.slots.state[h]=1;byte(u,0x90000+h*64+0x1a,1); }
        word(u,DS+0x16be,race.weapons.slots.capacity);word(u,DS+0x16c0,13);
        word(u,DS+0x16ce,0);word(u,DS+0x16d0,0x9000);
        for(unsigned i=0;i<26;++i) byte(u,DS+0x4c4a+i,i);
        for(unsigned d=0;d<4;++d) {
            struct SlicksRaceCar *car=&race.cars[d];
            car->vehicle=d;car->x=(100+(int)d*((t&32)?30:2))*100;car->y=8000;
            car->velocity_x=-100+(int)d*50;car->velocity_y=200-(int)d*50;
            car->actor_layer=(t&64)?d%2:0;car->drive_coefficients[5]=100;
            race.participation[d]=(t&512)&&d==2?0:1;
            race.properties[d].collision_weight=40+d*10;race.properties[d].impact_resistance=50+d*10;
            dw(u,DS+0x538c+d*4,car->x);dw(u,DS+0x539c+d*4,car->y);
            dw(u,DS+0x682e +d*4,car->velocity_x);dw(u,DS+0x683e +d*4,car->velocity_y);
            byte(u,DS+0x4bc6+d,race.participation[d]);byte(u,DS+0x5388+d,car->actor_layer);
            byte(u,DS+0x4edc+d,race.properties[d].collision_weight);
            byte(u,DS+0x4f00+d,race.properties[d].impact_resistance);
            byte(u,DS+0x3057+d*54,0);word(u,DS+0x3058+d*54,0);word(u,DS+0x6aec+d*14,100);
            for(unsigned channel=0;channel<4;++channel) word(u,DS+0x304f+d*54+channel*2,0);
            race.weapons.controls[d].cooldown=(short)(d*2);word(u,BP-0x20+d*2,d*2);
            for(unsigned p=0;p<30;++p) word(u,BP-0x16e +d*60+p*2,0);
        }
        word(u,DS+0x3130,0);
        unsigned driver=t%4,slot=1+(t/4)%29,type=(t/4)%8;
        struct SlicksWeaponProjectile *p=&race.weapons.projectiles[driver][slot];
        *p=(struct SlicksWeaponProjectile){.handle=12,.x=1603,.y=1285,
            .vx=type==5?(short)(t%112):(short)((int)(t%41)-20),.vy=type==5?(short)((driver+1)%4):7,
            .lifetime=(t&16)?2:1000,.type=(signed char)type,.layer=(t&64)?1:0};
        struct SlicksWeaponActor *a=&race.weapons.actors[12];
        a->kind=(type==3 || type>=5)?1:2;a->motion.frames=type==5?8:1;
        byte(u,0x90300+0x3a,a->motion.frames);
        const unsigned offsets[]={0x16e,0x25e,0x34e,0x43e,0x52e,0x70e};
        short values[]={p->handle,p->x,p->y,p->vx,p->vy,p->lifetime};
        for(unsigned j=0;j<6;++j) word(u,BP-offsets[j]+driver*60+slot*2,values[j]);
        byte(u,BP-0x5a6+driver*30+slot,type);byte(u,BP-0x61e +driver*30+slot,p->layer);
        unsigned ticks=1+(t/1024);word(u,BP-2,ticks);word(u,BP-0x68,0);
        allocations=sounds=0;
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xe000,bp=0xf000;
        ck(uc_reg_write(u,UC_X86_REG_CS,&cs));ck(uc_reg_write(u,UC_X86_REG_DS,&ds));
        ck(uc_reg_write(u,UC_X86_REG_SS,&ss));ck(uc_reg_write(u,UC_X86_REG_SP,&sp));ck(uc_reg_write(u,UC_X86_REG_BP,&bp));
        ck(uc_emu_start(u,0x21585,0x221a7,0,1000000));
        update_weapon_projectiles(&race,ticks);
        if(race.collision_error || race.random_state!=(unsigned)rlong(u,DS+0x2aaa) ||
           race.weapons.slots.high_water!=rd(u,DS+0x16c0) || sounds!=race.sound_event_count) {
            fprintf(stderr,"Projectile loop mismatch trial=%u type=%u rng=%lx/%x actors=%u/%u sounds=%u/%u error=%u\n",
                t,type,race.random_state,(unsigned)rlong(u,DS+0x2aaa),race.weapons.slots.high_water,rd(u,DS+0x16c0),race.sound_event_count,sounds,race.collision_error);return 1;
        }
        short result[]={p->handle,p->x,p->y,p->vx,p->vy,p->lifetime};
        for(unsigned j=0;j<6;++j) if(result[j]!=(short)rd(u,BP-offsets[j]+driver*60+slot*2)) {
            fprintf(stderr,"Projectile field mismatch trial=%u field=%u native=%d original=%d\n",t,j,result[j],(short)rd(u,BP-offsets[j]+driver*60+slot*2));return 1;
        }
        for(unsigned d=0;d<4;++d) {
            struct SlicksRaceCar *c=&race.cars[d];
            if(c->velocity_x!=rlong(u,DS+0x682e +d*4) || c->velocity_y!=rlong(u,DS+0x683e +d*4)) {
                fprintf(stderr,"Impact velocity mismatch trial=%u driver=%u\n",t,d);return 1;
            }
            for(unsigned j=0;j<4;++j) if(c->damage[j]!=(short)rd(u,DS+0x304f+d*54+j*2)) {
                fprintf(stderr,"Damage mismatch trial=%u driver=%u channel=%u native=%d original=%d\n",t,d,j,c->damage[j],(short)rd(u,DS+0x304f+d*54+j*2));return 1;
            }
        }
        for(unsigned h=12;h<race.weapons.slots.high_water;++h) {
            const struct SlicksActorMotion *m=&race.weapons.actors[h].motion;
            short fields[]={m->x,m->y,m->vx,m->vy,m->ax,m->ay,m->lifetime,m->age};
            const unsigned actor_offsets[]={0,2,0x1b,0x1d,0x1f,0x21,0x3e,0x3c};
            for(unsigned j=0;j<8;++j) if(fields[j]!=(short)rd(u,0x90000+h*64+actor_offsets[j])) {
                fprintf(stderr,"Effect actor mismatch trial=%u handle=%u field=%u native=%d original=%d\n",t,h,j,fields[j],(short)rd(u,0x90000+h*64+actor_offsets[j]));return 1;
            }
            unsigned char fields8[]={m->period,m->frame,m->frames,race.weapons.actors[h].priority,race.weapons.actors[h].occlusion};
            const unsigned actor_offsets8[]={0x23,0x24,0x3a,0x25,0x3b};
            for(unsigned j=0;j<5;++j) if(fields8[j]!=rb(u,0x90000+h*64+actor_offsets8[j])) {
                fprintf(stderr,"Effect byte mismatch trial=%u handle=%u field=%u native=%u original=%u\n",t,h,j,fields8[j],rb(u,0x90000+h*64+actor_offsets8[j]));return 1;
            }
        }
        for(unsigned i=0;i<sounds;++i) if(memcmp(&sound_trace[i],&race.sound_events[i],sizeof sound_trace[i])) return 1;
    }
    puts("Original projectile loop: 4096 expiry/homing/hit/explosion/damage/RNG/shared-capacity cases match");
    ck(uc_close(u));return 0;
}
