#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/weapon_shop.h"
static uc_engine *trace_engine;
static unsigned trace_trial,trace_phase;
static void ck(uc_err e) { if(e) {
    uint16_t cs=0,ip=0;
    if(trace_engine) { uc_reg_read(trace_engine,UC_X86_REG_CS,&cs); uc_reg_read(trace_engine,UC_X86_REG_IP,&ip); }
    fprintf(stderr,"%s trial=%u phase=%u at %04x:%04x\n",uc_strerror(e),trace_trial,trace_phase,cs,ip); exit(1);
} }
static void word(uc_engine *u,unsigned a,unsigned v) { unsigned char b[2]={v,v>>8}; ck(uc_mem_write(u,a,b,2)); }
static unsigned rd(uc_engine *u,unsigned a) { unsigned char b[2]; ck(uc_mem_read(u,a,b,2)); return b[0]|b[1]<<8; }
static void byte(uc_engine *u,unsigned a,unsigned v) { unsigned char b=v; ck(uc_mem_write(u,a,&b,1)); }
static void stop(uc_engine *u,uint64_t a,uint32_t n,void *p) { (void)a;(void)n;(void)p; ck(uc_emu_stop(u)); }
static void regs(uc_engine *u)
{
    uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xe000,bp=0xf000;
    ck(uc_reg_write(u,UC_X86_REG_CS,&cs)); ck(uc_reg_write(u,UC_X86_REG_DS,&ds));
    ck(uc_reg_write(u,UC_X86_REG_SS,&ss)); ck(uc_reg_write(u,UC_X86_REG_SP,&sp)); ck(uc_reg_write(u,UC_X86_REG_BP,&bp));
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t size=fread(runtime,1,sizeof runtime,f); fclose(f);
    uc_engine *u; ck(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); ck(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    trace_engine=u;
    ck(uc_mem_write(u,0x10100,runtime,size));
    struct SlicksShopRules r;
    ck(uc_mem_read(u,0x3cbf0+0x106f,r.flags,13)); ck(uc_mem_read(u,0x3cbf0+0x10a4,r.capacity,13));
    ck(uc_mem_read(u,0x3cbf0+0x107c,r.batch,13)); ck(uc_mem_read(u,0x3cbf0+0x1aa,r.vehicle_capacity,10));
    ck(uc_mem_read(u,0x3cbf0+0x13e,r.weapon_weight,8));
    for(unsigned i=0;i<13;++i) { r.base_price[i]=(short)rd(u,0x3cbf0+0x108a+2*i); r.ammunition_price[i]=(short)rd(u,0x3cbf0+0x17c+2*i); }
    uc_hook hook; ck(uc_hook_add(u,&hook,UC_HOOK_CODE,stop,0,0x2d229,0x2d229));
    const short levels[]={-1,0,1,2,3,4,9,19,20,49,50,99,100,32767};
    const short balances[]={-32768,-1,0,1,10,100,200,1000,32767};
    unsigned cases=0;
    for(unsigned trial=0;trial<32768;++trial) {
        trace_trial=trial; trace_phase=0;
        unsigned driver=trial&3,item=(trial/4)%13,vehicle=(trial/52)%10;
        unsigned enabled=trial/520,extra=(trial>>7)&1;
        signed char role=(trial%11)?((trial&1)?1:-1):0;
        short inventory[13],original[13];
        for(unsigned i=0;i<13;++i) inventory[i]=original[i]=levels[(trial/3+i)%14];
        inventory[item]=original[item]=levels[(trial/7)%14];
        struct SlicksRaceOptions options={.weapons_enabled=enabled&1,.inventory_mode=enabled&2,
            .fuel=enabled&4,.damage=enabled&8};
        word(u,0x3cbf0+0x3020,options.weapons_enabled); word(u,0x3cbf0+0x3022,options.inventory_mode);
        word(u,0x3cbf0+0x3024,options.fuel); word(u,0x3cbf0+0x3026,options.damage); byte(u,0x3cbf0+0x62f,extra);
        byte(u,0x3cbf0+0x4bc6+driver,role); byte(u,0x3cbf0+0x4bc2+driver,vehicle);
        regs(u); word(u,0x8e004,driver); word(u,0x8e006,item);
        word(u,0x8e000,0); word(u,0x8e002,0x7000);
        for(unsigned i=0;i<13;++i) word(u,0x3cbf0+0x6a7a+26*driver+2*i,inventory[i]);
        short expected=slicks_shop_price(&r,&options,inventory,role,vehicle,item,extra);
        ck(uc_emu_start(u,0x2aad6,0x70000,0,100000)); uint16_t ax; ck(uc_reg_read(u,UC_X86_REG_AX,&ax));
        if((short)ax!=expected) { fprintf(stderr,"price trial=%u item=%u original=%d native=%d\n",trial,item,(short)ax,expected); return 1; }
        for(unsigned sell=0;sell<2;++sell) {
            trace_phase=sell+1;
            regs(u); memcpy(inventory,original,sizeof inventory);
            for(unsigned i=0;i<13;++i) word(u,0x3cbf0+0x6a7a+26*driver+2*i,inventory[i]);
            short cash=balances[(trial/13)%9]; word(u,0x3cbf0+0x4bf6+2*driver,cash);
            byte(u,0x8f000-0x1a,driver); byte(u,0x8f000-0x19,item);
            byte(u,0x8f000-2,0); byte(u,0x8f000-3,13); byte(u,0x8f000-0xb,driver);
            if(sell) slicks_shop_sell(&r,&options,inventory,&cash,role,vehicle,item,extra);
            else slicks_shop_buy(&r,&options,inventory,&cash,role,vehicle,item,extra);
            ck(uc_emu_start(u,sell?0x2d184:0x2d0b1,0x2d229,0,100000));
            if(cash!=(short)rd(u,0x3cbf0+0x4bf6+2*driver) || inventory[item]!=(short)rd(u,0x3cbf0+0x6a7a+26*driver+2*item)) {
                fprintf(stderr,"transaction trial=%u sell=%u item=%u native count/cash=%d/%d original=%d/%d\n",trial,sell,item,inventory[item],cash,(short)rd(u,0x3cbf0+0x6a7a+26*driver+2*item),(short)rd(u,0x3cbf0+0x4bf6+2*driver)); return 1;
            }
        }
        ++cases;
    }
    printf("Original shop: %u prices and %u buy/sell transactions match\n",cases,cases*2);
    cases=0;
    for(unsigned trial=0;trial<4096;++trial) {
        regs(u); word(u,0x8e000,0); word(u,0x8e002,0x7000);
        unsigned long seed=0x91ae32UL+trial*32771UL;
        word(u,0x3cbf0+0x2aaa,seed); word(u,0x3cbf0+0x2aac,seed>>16);
        short inventory[4][13],cash[4]; signed char roles[4],vehicles[4];
        unsigned flags=trial/256;
        struct SlicksRaceOptions options={.weapons_enabled=flags&1,.inventory_mode=flags&2,.fuel=flags&4,.damage=flags&8};
        word(u,0x3cbf0+0x3020,options.weapons_enabled); word(u,0x3cbf0+0x3022,options.inventory_mode);
        word(u,0x3cbf0+0x3024,options.fuel); word(u,0x3cbf0+0x3026,options.damage); byte(u,0x3cbf0+0x62f,1);
        for(unsigned d=0;d<4;++d) {
            roles[d]=(signed char)((trial>>(2*d))%3)-1; vehicles[d]=(trial+d)%10; cash[d]=(short)((trial*17+d*23)%2000);
            byte(u,0x3cbf0+0x4bc6+d,roles[d]); byte(u,0x3cbf0+0x4bc2+d,vehicles[d]); word(u,0x3cbf0+0x4bf6+2*d,cash[d]);
            for(unsigned i=0;i<13;++i) { inventory[d][i]=(trial+d+i)%5; word(u,0x3cbf0+0x6a7a+26*d+2*i,inventory[d][i]); }
        }
        slicks_shop_computers(&r,&options,inventory,cash,roles,vehicles,1,&seed);
        ck(uc_emu_start(u,0x2c45c,0x70000,0,1000000));
        for(unsigned d=0;d<4;++d) {
            if(cash[d]!=(short)rd(u,0x3cbf0+0x4bf6+2*d)) return 1;
            for(unsigned i=0;i<13;++i) if(inventory[d][i]!=(short)rd(u,0x3cbf0+0x6a7a+26*d+2*i)) return 1;
        }
        if(seed!=(rd(u,0x3cbf0+0x2aaa)|((unsigned long)rd(u,0x3cbf0+0x2aac)<<16))) return 1;
        ++cases;
    }
    printf("Original computer shop: %u whole four-driver calls and shared RNG states match\n",cases);
    ck(uc_close(u)); return 0;
}
