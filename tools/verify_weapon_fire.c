#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/weapon_fire.h"
#define DS 0x3cbf0U
#define BP 0x8f000U
static void ck(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e));exit(1); } }
static void word(uc_engine *u,unsigned a,unsigned v) { unsigned char b[2]={v,v>>8};ck(uc_mem_write(u,a,b,2)); }
static unsigned rd(uc_engine *u,unsigned a) { unsigned char b[2];ck(uc_mem_read(u,a,b,2));return b[0]|b[1]<<8; }
static void byte(uc_engine *u,unsigned a,unsigned v) { unsigned char b=v;ck(uc_mem_write(u,a,&b,1)); }
static unsigned rb(uc_engine *u,unsigned a) { unsigned char b;ck(uc_mem_read(u,a,&b,1));return b; }
struct Trace { int events[256];unsigned count,allocated,fail;short flash; };
static short alloc_event(struct Trace *t,int type)
{ t->events[t->count++]=type;return (++t->allocated%t->fail)?(short)t->allocated:0; }
static void flash(void *ctx,unsigned driver,signed char scale)
{ (void)driver;(void)scale;struct Trace *t=ctx;t->flash=alloc_event(t,-1); }
static void sound(void *ctx,unsigned char sample,unsigned char flags,unsigned char priority)
{ struct Trace *t=ctx;t->events[t->count++]=0x100000|sample|(flags<<8)|(priority<<12); }
static short allocate(void *ctx,signed char type) { return alloc_event(ctx,type); }
struct Hook { struct Trace *trace;signed char type; };
static void boundary(uc_engine *u,uint64_t a,uint32_t size,void *ctx)
{
    (void)size;struct Hook *h=ctx;uint16_t sp,ss;
    ck(uc_reg_read(u,UC_X86_REG_SP,&sp));ck(uc_reg_read(u,UC_X86_REG_SS,&ss));
    unsigned stack=ss*16U+sp;uint16_t ip=rd(u,stack),cs=rd(u,stack+2),ax=0;
    if(a==0x3989b) sound(h->trace,rd(u,stack+4),rd(u,stack+6),rd(u,stack+8));
    else if(a!=0x33303) {
        int type=(cs*16U+ip==0x2071a)?-1:h->type;
        ax=(uint16_t)alloc_event(h->trace,type);
        if(type==-1) h->trace->flash=(short)ax;
    }
    sp+=4;ck(uc_reg_write(u,UC_X86_REG_SP,&sp));ck(uc_reg_write(u,UC_X86_REG_CS,&cs));
    ck(uc_reg_write(u,UC_X86_REG_IP,&ip));ck(uc_reg_write(u,UC_X86_REG_AX,&ax));
}
int main(void)
{
    unsigned char runtime[300000];FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    size_t n=fread(runtime,1,sizeof runtime,f);fclose(f);
    uc_engine *u;ck(uc_open(UC_ARCH_X86,UC_MODE_16,&u));ck(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    ck(uc_mem_write(u,0x10100,runtime,n));
    struct SlicksWeaponRules rules={0};
    for(unsigned i=0;i<8;++i) {
        rules.delay[i]=(short)rd(u,DS+0x10e +i*2);rules.lifetime[i]=(short)rd(u,DS+0x156+i*2);
        rules.shots[i]=(signed char)rb(u,DS+0x146+i);rules.speed[i]=(signed char)rb(u,DS+0x16e +i);
        rules.spread[i]=(signed char)rb(u,DS+0x14e +i);rules.muzzle[i]=(signed char)rb(u,DS+0x17e +i);
        rules.fire_sound[i]=rb(u,DS+0x196+i);
    }
    signed char dx[16],dy[16];ck(uc_mem_read(u,DS+0x6c3,dx,16));ck(uc_mem_read(u,DS+0x6d3,dy,16));
    struct Hook h;uc_hook hooks[5];const unsigned addresses[]={0x330ab,0x332ad,0x334ef,0x33303,0x3989b};
    for(unsigned i=0;i<5;++i) ck(uc_hook_add(u,&hooks[i],UC_HOOK_CODE,boundary,&h,addresses[i],addresses[i]));
    unsigned random=123;
    for(unsigned t=0;t<8192;++t) {
        unsigned driver=t%4;signed char selected=(t/4)%8;
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xe000,bp=0xf000;
        ck(uc_reg_write(u,UC_X86_REG_CS,&cs));ck(uc_reg_write(u,UC_X86_REG_DS,&ds));
        ck(uc_reg_write(u,UC_X86_REG_SS,&ss));ck(uc_reg_write(u,UC_X86_REG_SP,&sp));ck(uc_reg_write(u,UC_X86_REG_BP,&bp));
        word(u,BP-0x68,driver);word(u,DS+0x16ce,0);word(u,DS+0x16d0,0x9000);
        unsigned char actors[4096]={0};ck(uc_mem_write(u,0x90000,actors,sizeof actors));
        int x[4],y[4];signed char roles[4];short inventory[13]={0};
        for(unsigned i=0;i<4;++i) {
            x[i]=1000+(int)i*1500;y[i]=9000+(int)i*100;roles[i]=(signed char)((t>>(i+5))%3-1);
            word(u,DS+0x538c+i*4,x[i]);word(u,DS+0x538e +i*4,0);
            word(u,DS+0x539c+i*4,y[i]);word(u,DS+0x539e +i*4,0);
            byte(u,DS+0x4bc6+i,roles[i]);word(u,DS+0x53b6+i*2,x[i]/100);word(u,DS+0x53be +i*2,y[i]/100);
        }
        for(unsigned i=0;i<8;++i) {
            inventory[i+5]=((t>>(i+3))&1)?1:100;
            word(u,DS+0x6a84+driver*26+i*2,inventory[i+5]);
        }
        inventory[selected+5]=(short)((t/32)%4);word(u,DS+0x6a84+driver*26+selected*2,inventory[selected+5]);
        struct SlicksWeaponControl control={.request=(t&1024)?2:1,.cooldown=(t&2048)?2:0,.repeat_ticks=17};
        word(u,BP-0x20+driver*2,control.cooldown);word(u,BP-0xa+driver*2,17);
        byte(u,DS+0x2fac+driver,selected);byte(u,DS+0x2fb0+driver,control.request);
        short heading=(t%16)*1200;word(u,DS+0x681c+driver*2,heading);byte(u,DS+0x5388+driver,t%2);
        unsigned clock=(t&4096)?250:1000;word(u,DS+0x685e,clock);word(u,DS+0x6860,0);
        unsigned char controls=31;byte(u,DS+0x5345+5*driver,1);
        rules.unlimited=(t>>8)&1;word(u,DS+0x1a8,rules.unlimited);
        for(unsigned i=0;i<26;++i) byte(u,DS+0x4c4a+i,i);
        struct SlicksWeaponProjectile pool[30];
        const unsigned offsets[]={0x16e,0x25e,0x34e,0x43e,0x52e,0x70e};
        for(unsigned i=0;i<30;++i) {
            random=random*1664525U+1013904223U;
            pool[i]=(struct SlicksWeaponProjectile){.handle=(t&128)?1:(short)(random%3-1),.x=101,.y=202,.vx=303,.vy=404,.lifetime=505,.type=6,.layer=1};
            short fields[]={pool[i].handle,101,202,303,404,505};
            for(unsigned j=0;j<6;++j) word(u,BP-offsets[j]+driver*60+i*2,fields[j]);
            byte(u,BP-0x5a6+driver*30+i,6);byte(u,BP-0x61e +driver*30+i,1);
        }
        unsigned long seed=random;word(u,DS+0x2aaa,seed);word(u,DS+0x2aac,seed>>16);
        struct Trace original={.fail=(t&512)?2:255},native=original;
        h.trace=&original;h.type=selected;
        struct SlicksWeaponFireOps ops={flash,sound,allocate,&native};
        short last_slot=-777;word(u,BP-0x3c,last_slot);
        /* The preparation bridge skips coordinate construction only when
         * the original transaction cannot enter its shot body. Prove that
         * finish/cycle alone has the same state and leaves shot data alone. */
        int no_shot=!slicks_weapon_can_fire(&control,selected,clock);
        struct SlicksWeaponControl shortcut=control;
        unsigned char shortcut_controls=controls;
        signed char shortcut_selected=selected;
        short inventory_before[13];memcpy(inventory_before,inventory,sizeof inventory);
        struct SlicksWeaponProjectile pool_before[30];memcpy(pool_before,pool,sizeof pool);
        unsigned long seed_before=seed;
        if(no_shot)shortcut_selected=slicks_weapon_finish_request(&shortcut,
            inventory,selected,rules.delay,&shortcut_controls);
        signed char out=slicks_weapon_fire(&control,inventory,selected,&controls,driver,clock,
            pool,x,y,roles,heading,t%2,dx,dy,&seed,&rules,&ops,&last_slot);
        if(no_shot && (shortcut_selected!=out || shortcut_controls!=controls ||
           memcmp(&shortcut,&control,sizeof control) ||
           memcmp(inventory_before,inventory,sizeof inventory) ||
           memcmp(pool_before,pool,sizeof pool) || seed!=seed_before ||
           last_slot!=-777 || native.count)) {
            fprintf(stderr,"No-shot shortcut mismatch trial=%u\n",t);return 1;
        }
        ck(uc_emu_start(u,0x206a5,0x20c6f,0,100000));
        if(last_slot!=(short)rd(u,BP-0x3c) || original.count!=native.count || memcmp(original.events,native.events,original.count*sizeof(int)) ||
           out!=(signed char)rb(u,DS+0x2fac+driver) || control.request!=rb(u,DS+0x2fb0+driver) ||
           control.cooldown!=(short)rd(u,BP-0x20+driver*2) || control.repeat_ticks!=(short)rd(u,BP-0xa+driver*2) ||
           !!(controls&2)!=rb(u,DS+0x5345+driver*5) ||
           seed!=(rd(u,DS+0x2aaa)|((unsigned long)rd(u,DS+0x2aac)<<16))) {
            fprintf(stderr,"Fire transaction mismatch trial=%u type=%d events=%u/%u\n",t,selected,original.count,native.count);return 1;
        }
        for(unsigned i=0;i<8;++i) if(inventory[i+5]!=(short)rd(u,DS+0x6a84+driver*26+i*2)) return 1;
        for(unsigned i=0;i<30;++i) {
            short fields[]={pool[i].handle,pool[i].x,pool[i].y,pool[i].vx,pool[i].vy,pool[i].lifetime};
            for(unsigned j=0;j<6;++j) if(fields[j]!=(short)rd(u,BP-offsets[j]+driver*60+i*2)) {
                fprintf(stderr,"Projectile mismatch trial=%u slot=%u field=%u\n",t,i,j);return 1;
            }
            if(pool[i].type!=(signed char)rb(u,BP-0x5a6+driver*30+i) || pool[i].layer!=(signed char)rb(u,BP-0x61e +driver*30+i)) return 1;
        }
    }
    puts("Original full firing transaction: 8192 sound/allocation/order/projectile/RNG/depletion/cycling cases match");
    ck(uc_close(u));return 0;
}
