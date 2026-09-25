#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/weapon_actions.h"
#include "../src/game/weapon_projectile.h"
static void ck(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void wr(uc_engine *u,unsigned a,unsigned v) { unsigned char b[2]={v,v>>8}; ck(uc_mem_write(u,a,b,2)); }
static unsigned rd(uc_engine *u,unsigned a) { unsigned char b[2]; ck(uc_mem_read(u,a,b,2)); return b[0]|b[1]<<8; }
static void byte(uc_engine *u,unsigned a,unsigned v) { unsigned char b=v; ck(uc_mem_write(u,a,&b,1)); }
static unsigned rb(uc_engine *u,unsigned a) { unsigned char b; ck(uc_mem_read(u,a,&b,1)); return b; }
static void regs(uc_engine *u,unsigned driver)
{
    uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xe000,bp=0xf000;
    ck(uc_reg_write(u,UC_X86_REG_CS,&cs)); ck(uc_reg_write(u,UC_X86_REG_DS,&ds));
    ck(uc_reg_write(u,UC_X86_REG_SS,&ss)); ck(uc_reg_write(u,UC_X86_REG_SP,&sp));
    ck(uc_reg_write(u,UC_X86_REG_BP,&bp)); wr(u,0x8f000-0x68,driver);
}
static void stop(uc_engine *u,uint64_t a,uint32_t n,void *p)
{ (void)a;(void)n;(void)p; ck(uc_emu_stop(u)); }
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f); fclose(f);
    uc_engine *u; ck(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); ck(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    ck(uc_mem_write(u,0x10100,runtime,bytes));
    const signed char roles[]={-128,-1,0,1,127};
    unsigned cases=0;
    for(unsigned driver=0;driver<4;++driver) for(unsigned role=0;role<5;++role)
    for(unsigned enabled=0;enabled<2;++enabled) for(int selected=-1;selected<8;++selected)
    for(unsigned controls=0;controls<32;++controls) for(unsigned held=0;held<2;++held)
    for(unsigned request=0;request<3;++request) {
        regs(u,driver); wr(u,0x3cbf0+0x3020,enabled?0x8000:0);
        byte(u,0x3cbf0+0x4bc6+driver,roles[role]); byte(u,0x3cbf0+0x2fac+driver,selected);
        byte(u,0x3cbf0+0x2fb0+driver,request); byte(u,0x8f000-0x24+driver,held);
        for(unsigned i=0;i<5;++i) byte(u,0x3cbf0+0x5344+driver*5+i,!!(controls&(1U<<i)));
        struct SlicksWeaponControl s={.request=request,.cycle_held=held};
        slicks_weapon_human_request(&s,enabled?(-32767-1):0,roles[role],selected,controls);
        ck(uc_emu_start(u,0x20509,0x20595,0,1000));
        if(rb(u,0x3cbf0+0x2fb0+driver)!=s.request || rb(u,0x8f000-0x24+driver)!=s.cycle_held) abort();
        ++cases;
    }
    printf("Original weapon input: %u human/AI/gate/held-control/request cases pass\n",cases);
    uc_hook gate; ck(uc_hook_add(u,&gate,UC_HOOK_CODE,stop,0,0x20c09,0x20c09));
    const unsigned clocks[]={0,249,250,251,65535,65536,0x7fffffff,0x80000000,0xffffffff};
    const short timers[]={-32768,-1,0,1,32767}; cases=0;
    for(unsigned driver=0;driver<4;++driver) for(unsigned request=0;request<3;++request)
    for(int selected=-1;selected<8;++selected) for(unsigned t=0;t<5;++t) for(unsigned c=0;c<9;++c) {
        regs(u,driver); byte(u,0x3cbf0+0x2fb0+driver,request); byte(u,0x3cbf0+0x2fac+driver,selected);
        wr(u,0x8f000-0x20+2*driver,timers[t]); wr(u,0x3cbf0+0x685e,clocks[c]); wr(u,0x3cbf0+0x6860,clocks[c]>>16);
        struct SlicksWeaponControl s={.request=request,.cooldown=timers[t]};
        ck(uc_emu_start(u,0x206a5,0x206e8,0,1000)); uint16_t ip; ck(uc_reg_read(u,UC_X86_REG_IP,&ip));
        if((ip+0x19870==0x206e8)!=slicks_weapon_can_fire(&s,selected,clocks[c])) abort();
        ++cases;
    }
    ck(uc_hook_del(u,gate)); printf("Original fire gate: %u clock/cooldown/selection boundaries pass\n",cases);
    short delays[8]; for(unsigned i=0;i<8;++i) delays[i]=(short)rd(u,0x3cbf0+0x10e +2*i);
    cases=0;
    for(unsigned driver=0;driver<4;++driver) for(unsigned mask=0;mask<256;++mask)
    for(int selected=-1;selected<8;++selected) for(unsigned request=0;request<3;++request) {
        regs(u,driver); short inventory[13]={0};
        for(unsigned i=0;i<8;++i) { inventory[i+5]=(mask&(1U<<i))?2:1; wr(u,0x3cbf0+0x6a84+26*driver+2*i,inventory[i+5]); }
        byte(u,0x3cbf0+0x2fb0+driver,request); byte(u,0x3cbf0+0x2fac+driver,selected);
        byte(u,0x3cbf0+0x5345+5*driver,1); wr(u,0x8f000-0x20+2*driver,123); wr(u,0x8f000-0xa+2*driver,456);
        struct SlicksWeaponControl s={.request=request,.cooldown=123,.repeat_ticks=456}; unsigned char controls=31;
        signed char out=slicks_weapon_finish_request(&s,inventory,selected,delays,&controls);
        ck(uc_emu_start(u,0x20c09,0x20c6f,0,10000));
        if((signed char)rb(u,0x3cbf0+0x2fac+driver)!=out || rb(u,0x3cbf0+0x2fb0+driver)!=s.request ||
            rb(u,0x3cbf0+0x5345+5*driver)!=!!(controls&2) ||
            (short)rd(u,0x8f000-0x20+2*driver)!=s.cooldown || (short)rd(u,0x8f000-0xa+2*driver)!=s.repeat_ticks) abort();
        ++cases;
    }
    printf("Original weapon cycling: %u inventory/selection/request cases pass\n",cases);
    ck(uc_hook_add(u,&gate,UC_HOOK_CODE,stop,0,0x20c09,0x20c09));
    ck(uc_ctl_remove_cache(u,0x10100,0x50000));
    cases=0;
    const short counts[]={-32768,-1,0,1,2,3,100,32767};
    for(unsigned driver=0;driver<4;++driver) for(int selected=0;selected<8;++selected)
    for(unsigned n=0;n<8;++n) for(unsigned unlimited=0;unlimited<2;++unlimited) {
        regs(u,driver); short inventory[13]={0}; inventory[selected+5]=counts[n];
        wr(u,0x3cbf0+0x6a84+26*driver+2*selected,counts[n]); wr(u,0x3cbf0+0x1a8,unlimited);
        byte(u,0x3cbf0+0x2fac+driver,selected); byte(u,0x3cbf0+0x2fb0+driver,1);
        struct SlicksWeaponControl s={.request=1}; slicks_weapon_consume(&s,inventory,selected,unlimited);
        ck(uc_emu_start(u,0x20bbf,0x20c09,0,1000));
        if((short)rd(u,0x3cbf0+0x6a84+26*driver+2*selected)!=inventory[selected+5] || rb(u,0x3cbf0+0x2fb0+driver)!=s.request) {
            fprintf(stderr,"depletion driver=%u weapon=%d count=%d unlimited=%u: original=%d/%u native=%d/%u\n",driver,selected,counts[n],unlimited,(short)rd(u,0x3cbf0+0x6a84+26*driver+2*selected),rb(u,0x3cbf0+0x2fb0+driver),inventory[selected+5],s.request);
            return 1;
        }
        ++cases;
    }
    printf("Original weapon depletion: %u signed/wrapping/unlimited cases pass\n",cases);
    ck(uc_hook_del(u,gate));
    for(unsigned n=0;n<65536;++n) {
        regs(u,0); short ticks=(short)(n*71U); wr(u,0x8f000-0x20,n); wr(u,0x8f000-2,ticks);
        struct SlicksWeaponControl s={.cooldown=(short)n}; slicks_weapon_cooldown(&s,ticks);
        ck(uc_emu_start(u,0x21585,0x215a2,0,1000)); if((short)rd(u,0x8f000-0x20)!=s.cooldown) abort();
    }
    puts("Original weapon cooldown: 65536 signed timer/subtraction cases pass");
    /* Restore the original data after the preceding mutable-state tests. */
    ck(uc_mem_write(u,0x10100,runtime,bytes));
    signed char dirx[16],diry[16],ranges[9];
    ck(uc_mem_read(u,0x3cbf0+0x6c3,dirx,sizeof dirx));
    ck(uc_mem_read(u,0x3cbf0+0x6d3,diry,sizeof diry));
    ck(uc_mem_read(u,0x3cbf0+0x165,ranges,sizeof ranges));
    unsigned random=1234; cases=0;
    for(unsigned trial=0;trial<32768;++trial) {
        unsigned driver=trial&3;
        regs(u,driver);
        /* ENTER pushes BP; the original far-call argument is SP+4. Stop
         * before RETF so no fabricated caller is needed. */
        wr(u,0x8e004,driver);
        int x[4],y[4]; signed char role[4]; short inventory[13]={0};
        for(unsigned i=0;i<4;++i) {
            random=random*1664525U+1013904223U;
            x[i]=(trial&16)?(int)random:(int)(random%32000);
            random=random*1664525U+1013904223U;
            y[i]=(trial&16)?(int)random:(int)(random%20000);
            role[i]=(signed char)((random%3)-1);
            if(trial&32) { x[i]=16000+(int)i*100; y[i]=10000+(int)i*100; }
            wr(u,0x3cbf0+0x538c+4*i,(unsigned)x[i]); wr(u,0x3cbf0+0x538e +4*i,(unsigned)x[i]>>16);
            wr(u,0x3cbf0+0x539c+4*i,(unsigned)y[i]); wr(u,0x3cbf0+0x539e +4*i,(unsigned)y[i]>>16);
            byte(u,0x3cbf0+0x4bc6+i,role[i]);
        }
        signed char selected=(trial/4)%9-1;
        short heading=((trial/36)%16)*1200;
        unsigned char counter=(trial&64)?11:(unsigned char)(trial/128);
        unsigned clock=trial/256;
        inventory[selected+5]=(trial&128)?1:100;
        wr(u,0x3cbf0+0x6a7a+26*driver+2*(selected+5),inventory[selected+5]);
        byte(u,0x3cbf0+0x2fac+driver,selected); byte(u,0x3cbf0+0x68e5+driver,counter);
        wr(u,0x3cbf0+0x681c+2*driver,heading); wr(u,0x3cbf0+0x685e,clock);
        unsigned expected=slicks_weapon_ai_request(&counter,driver,x,y,heading,role,selected,inventory,clock,dirx,diry,ranges);
        ck(uc_emu_start(u,0x1ebbb,0x1ed63,0,100000));
        uint16_t ax; ck(uc_reg_read(u,UC_X86_REG_AX,&ax));
        if((ax&255)!=expected || rb(u,0x3cbf0+0x68e5+driver)!=counter) {
            fprintf(stderr,"AI trial %u original=%u/%u native=%u/%u\n",trial,ax&255,rb(u,0x3cbf0+0x68e5+driver),expected,counter); return 1;
        }
        ++cases;
    }
    printf("Original weapon AI: %u counter/heading/range/role/clock/wrapping-coordinate cases pass\n",cases);
    unsigned nearest_cases=0,init_cases=0;
    for(unsigned trial=0;trial<8192;++trial) {
        unsigned driver=trial&3,slot=1+(trial%29); int x[4],y[4]; signed char role[4];
        regs(u,driver); wr(u,0x8e004,driver);
        for(unsigned i=0;i<4;++i) {
            random=random*1664525U+1013904223U;
            x[i]=(trial&16)?(int)random:(int)(random%32000);
            random=random*1664525U+1013904223U;
            y[i]=(trial&16)?(int)random:(int)(random%20000);
            role[i]=(signed char)((random%3)-1);
            if(trial&32) { x[i]=16000; y[i]=10000; }
            wr(u,0x3cbf0+0x538c+4*i,(unsigned)x[i]); wr(u,0x3cbf0+0x538e +4*i,(unsigned)x[i]>>16);
            wr(u,0x3cbf0+0x539c+4*i,(unsigned)y[i]); wr(u,0x3cbf0+0x539e +4*i,(unsigned)y[i]>>16);
            byte(u,0x3cbf0+0x4bc6+i,role[i]);
        }
        unsigned nearest=slicks_weapon_nearest(driver,x,y,role);
        ck(uc_emu_start(u,0x1ea80,0x1eb48,0,10000)); uint16_t ax; ck(uc_reg_read(u,UC_X86_REG_AX,&ax));
        if((ax&255)!=nearest) { fprintf(stderr,"nearest trial %u: %u/%u\n",trial,ax&255,nearest); return 1; }
        ++nearest_cases;
        regs(u,driver); wr(u,0x8f000-0x3c,slot);
        signed char type=(trial/4)%8,layer=(trial/32)%3;
        short heading=((trial/96)%16)*1200;
        short lifetime=(short)rd(u,0x3cbf0+0x156+type*2);
        signed char speed=(signed char)rb(u,0x3cbf0+0x16e +type),spread=(signed char)rb(u,0x3cbf0+0x14e +type);
        unsigned long seed=random;
        wr(u,0x3cbf0+0x2aaa,random); wr(u,0x3cbf0+0x2aac,random>>16);
        byte(u,0x3cbf0+0x2fac+driver,type); byte(u,0x3cbf0+0x5388+driver,layer);
        wr(u,0x3cbf0+0x681c+2*driver,heading);
        struct SlicksWeaponProjectile p={0};
        slicks_weapon_projectile_init(&p,driver,x,y,role,heading,type,layer,lifetime,speed,spread,dirx,diry,&seed);
        ck(uc_emu_start(u,0x20951,0x20b88,0,100000));
        unsigned offset=60*driver+2*slot;
        if(p.x!=(short)rd(u,0x8f000 - 0x25e + offset) || p.y!=(short)rd(u,0x8f000 - 0x34e + offset) ||
           p.vx!=(short)rd(u,0x8f000 - 0x43e + offset) || p.vy!=(short)rd(u,0x8f000 - 0x52e + offset) ||
           p.lifetime!=(short)rd(u,0x8f000 - 0x70e + offset) ||
           p.type!=(signed char)rb(u,0x8f000-0x5a6+30*driver+slot) ||
           p.layer!=(signed char)rb(u,0x8f000 - 0x61e + 30*driver+slot) ||
           seed!=(rd(u,0x3cbf0+0x2aaa)|((unsigned long)rd(u,0x3cbf0+0x2aac)<<16))) {
            fprintf(stderr,"projectile init trial %u type=%d native velocity=%d,%d original=%d,%d\n",trial,type,p.vx,p.vy,(short)rd(u,0x8f000 - 0x43e + offset),(short)rd(u,0x8f000 - 0x52e + offset)); return 1;
        }
        ++init_cases;
    }
    printf("Original nearest target: %u role/tie/wrapping-distance cases pass\n",nearest_cases);
    printf("Original projectile initialization: %u weapon/heading/position/layer/RNG cases pass\n",init_cases);
    ck(uc_hook_add(u,&gate,UC_HOOK_CODE,stop,0,0x20ba5,0x20ba5));
    for(unsigned trial=0;trial<1024;++trial) {
        unsigned driver=trial&3; regs(u,driver);
        struct SlicksWeaponProjectile pool[30]={{0}};
        for(unsigned i=0;i<30;++i) {
            random=random*1664525U+1013904223U;
            pool[i].handle=(trial&16)?(short)(random%3-1):1;
            if(i==trial%30) pool[i].handle=0;
            wr(u,0x8f000 - 0x16e + 60*driver+2*i,pool[i].handle);
        }
        ck(uc_emu_start(u,0x20855,0x20890,0,10000));
        if(rd(u,0x8f000-0x3c)!=slicks_weapon_free_slot(pool)) return 1;
    }
    ck(uc_hook_del(u,gate));
    puts("Original projectile free-slot search: 1024 mixed/full/reserved-slot cases pass");
    cases=0;
    for(unsigned heading=0;heading<112;++heading) for(unsigned target=0;target<16;++target) {
        unsigned driver=heading&3,slot=1+heading%29,offset=60*driver+2*slot;
        regs(u,driver); wr(u,0x8f000-0x3a,slot);
        wr(u,0x8f000 - 0x43e + offset,heading); byte(u,0x8f000-0x77,target);
        struct SlicksWeaponProjectile p={.vx=heading};
        unsigned sector=slicks_weapon_home(&p,target);
        ck(uc_emu_start(u,0x21753,0x2184b,0,10000));
        if(p.vx!=(short)rd(u,0x8f000 - 0x43e + offset) || sector!=rb(u,0x8f000-0x77)) return 1;
        ++cases;
    }
    printf("Original homing turn: %u complete heading/target combinations pass\n",cases);
    ck(uc_ctl_remove_cache(u,0x10100,0x50000));
    cases=0;
    for(unsigned trial=0;trial<16384;++trial) {
        unsigned driver=trial&3,slot=1+trial%29,offset=60*driver+2*slot;
        regs(u,driver); wr(u,0x8f000-0x3a,slot);
        random=random*1664525U+1013904223U;
        struct SlicksWeaponProjectile p={.x=(short)random,.y=(short)(random>>16),.type=(trial/4)%8};
        short ticks=(short)trial;
        p.vx=p.type==5?(short)(trial%112):(short)(random>>8); p.vy=(short)(random>>12);
        wr(u,0x8f000 - 0x25e + offset,p.x); wr(u,0x8f000 - 0x34e + offset,p.y);
        wr(u,0x8f000 - 0x43e + offset,p.vx); wr(u,0x8f000 - 0x52e + offset,p.vy);
        wr(u,0x8f000-2,ticks); byte(u,0x8f000-0x77,p.vx/7);
        short nx,ny; slicks_weapon_move(&p,ticks,dirx,diry,&nx,&ny);
        ck(uc_emu_start(u,p.type==5?0x2184b:0x218f9,p.type==5?0x218ad:0x21965,0,10000));
        if(nx!=(short)rd(u,0x8f000-0x70) || ny!=(short)rd(u,0x8f000-0x72)) {
            fprintf(stderr,"movement trial %u\n",trial); return 1;
        }
        ++cases;
    }
    printf("Original projectile movement: %u signed velocity/position/tick cases pass\n",cases);
    uc_hook retire,detonate;
    ck(uc_hook_add(u,&retire,UC_HOOK_CODE,stop,0,0x21a2a,0x21a2a));
    ck(uc_hook_add(u,&detonate,UC_HOOK_CODE,stop,0,0x21a77,0x21a77));
    cases=0;
    const short lives[]={-32768,-1,0,1,799,800,801,32767};
    for(unsigned driver=0;driver<4;++driver) for(unsigned type=0;type<8;++type)
    for(unsigned l=0;l<8;++l) for(unsigned t=0;t<8;++t) {
        unsigned slot=1+l,offset=60*driver+2*slot; regs(u,driver);
        wr(u,0x8f000-0x3a,slot); wr(u,0x8f000-2,lives[t]);
        wr(u,0x8f000 - 0x70e + offset,lives[l]); byte(u,0x8f000-0x5a6+30*driver+slot,type);
        struct SlicksWeaponProjectile p={.type=type,.lifetime=lives[l]}; unsigned char excluded;
        enum SlicksWeaponExpiry action=slicks_weapon_age(&p,driver,lives[t],&excluded);
        ck(uc_emu_start(u,0x21965,0x21b47,0,10000)); uint16_t ip; ck(uc_reg_read(u,UC_X86_REG_IP,&ip));
        unsigned end=ip+0x19870;
        if(p.lifetime!=(short)rd(u,0x8f000 - 0x70e + offset) || excluded!=rb(u,0x8f000-0x79) ||
           end!=(action==SLICKS_WEAPON_RETIRE?0x21a2a:action==SLICKS_WEAPON_DETONATE?0x21a77:0x21b47)) {
            fprintf(stderr,"expiry driver=%u type=%u life=%d ticks=%d end=%x action=%d\n",driver,type,lives[l],lives[t],end,action); return 1;
        }
        ++cases;
    }
    printf("Original projectile expiry: %u type/lifetime/tick/owner-mask cases pass\n",cases);
    ck(uc_close(u)); return 0;
}
