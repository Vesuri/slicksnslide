#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/ui/race_hud.h"
#include "../src/game/race_runtime.c"
#include "host_archive.h"
#include "../src/game/track_records.h"
#include "../src/game/weapon_state.h"

static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *uc,unsigned a,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(uc,a,b,2)); }
static unsigned readword(uc_engine *uc,unsigned a)
{ unsigned char b[2]; check(uc_mem_read(uc,a,b,2)); return b[0]|b[1]<<8; }
struct Capture { struct SlicksHudText text[6]; unsigned count,icons,status; };
static void capture(uc_engine *uc,uint64_t address,uint32_t size,void *user)
{
    (void)size;
    struct Capture *c=user;
    uint16_t ss,sp,cs,ip;
    check(uc_reg_read(uc,UC_X86_REG_SS,&ss)); check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    if(address==0x301ab || address==0x302b6) {
        if(c->count>=6) { fputs("Extra HUD text command\n",stderr); exit(1); }
        struct SlicksHudText *t=&c->text[c->count++];
        t->x=readword(uc,stack+4); t->y=readword(uc,stack+6);
        if(address==0x302b6) {
            t->numeric=1; t->number=readword(uc,stack+8); t->flags=readword(uc,stack+14);
        } else {
            unsigned source=readword(uc,stack+8)+16*readword(uc,stack+10);
            t->flags=readword(uc,stack+16);
            for(unsigned n=0;n<sizeof t->text;++n) {
                check(uc_mem_read(uc,source+n,&t->text[n],1));
                if(!t->text[n]) break;
                if(n==sizeof t->text-1) { fputs("HUD string exceeds command buffer\n",stderr); exit(1); }
            }
        }
    } else if(address==0x3aaf2) ++c->icons;
    else if(address==0x1d9b6) ++c->status;
    /* Only graphics/status boundaries are substituted; real ddc0 and the
     * complete aceb time formatter plus arithmetic helpers execute. */
    ip=readword(uc,stack); cs=readword(uc,stack+2); sp+=4;
    check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_IP,&ip));
    check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
}

static unsigned char vga[4*65536];
static unsigned write_mask;
static void fixed_hud_colour(uc_engine *uc,uint64_t address,uint32_t size,void *user)
{
    capture(uc,address,size,user);
    uint16_t ax=73;
    check(uc_reg_write(uc,UC_X86_REG_AX,&ax));
}
static int track_hud(const unsigned char *runtime,size_t bytes)
{
    uc_engine *uc; check(uc_open(UC_ARCH_X86,UC_MODE_16,&uc));
    check(uc_mem_map(uc,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(uc,0x10100,runtime,bytes));
    struct Capture c;
    const unsigned boundaries[]={0x301ab,0x2fe63,0x36fae};
    for(unsigned i=0;i<3;++i) {
        uc_hook h; check(uc_hook_add(uc,&h,UC_HOOK_CODE,capture,&c,boundaries[i],boundaries[i]));
    }
    const char names[][9]={"BASIC","HEIKKI30","ICE",""};
    check(uc_mem_write(uc,0x52000,names,sizeof names));
    word(uc,0x3cbf0+0x4da4,0); word(uc,0x3cbf0+0x4da6,0x5200);
    word(uc,0x3cbf0+0x62a,0); word(uc,0x3cbf0+0x62c,0x5300);
    /* Nonidentity selection verifies both indirections in the original. */
    for(unsigned i=0;i<4;++i) word(uc,0x53000+i*2,3-i);
    const unsigned times[]={0,1,2,9,822,17999,18000,32767,32768,65535};
    unsigned cases=0;
    for(unsigned selected=0;selected<4;++selected)
    for(unsigned t=0;t<sizeof times/sizeof times[0];++t) {
        memset(&c,0,sizeof c);
        word(uc,0x3cbf0+0x628,selected); word(uc,0x3cbf0+0x696b,times[t]);
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
        word(uc,0x8f000,0); word(uc,0x8f002,0x7000);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss)); check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x2adbe,0x70000,0,10000));
        check(uc_reg_read(uc,UC_X86_REG_CS,&cs)); check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
        struct SlicksHudText expected[2];
        unsigned n=slicks_hud_track_text(names[3-selected],times[t],expected);
        if(cs*16U+ip!=0x70000 || sp!=0xf004 || c.count!=1+2*(n-1)) return 1;
        for(unsigned i=0;i<c.count;++i)
            if(memcmp(&c.text[i],&expected[i?1:0],sizeof expected[0])) {
                fprintf(stderr,"Track HUD mismatch selection=%u time=%u command=%u\n",selected,times[t],i);
                return 1;
            }
        ++cases;
    }
    check(uc_close(uc));
    printf("DOS track HUD: %u original command sequences match name selection, coordinates and signed record-time gating\n",cases);
    return 0;
}
static void hud_port(uc_engine *uc,uint32_t port,int size,uint32_t value,void *user)
{
    (void)uc; (void)user;
    if(port==0x3c4 && size==1 && value==2) return;
    if(port==0x3c4 && size==2 && (value&255)==2) write_mask=(value>>8)&15;
    else if(port==0x3c5 && size==1) write_mask=value&15;
    else abort();
}
static void hud_pixel(uc_engine *uc,uc_mem_type type,uint64_t address,int size,int64_t value,void *user)
{
    (void)uc; (void)type; (void)user;
    unsigned offset=(unsigned)address-0xa0000;
    if(size<1 || size>2 || offset+(unsigned)size>65536) abort();
    for(unsigned plane=0;plane<4;++plane) if(write_mask&(1U<<plane))
        for(int i=0;i<size;++i) vga[plane*65536+offset+i]=(unsigned char)((uint64_t)value>>(i*8));
}
static int composed_hud(const unsigned char *runtime,size_t bytes,unsigned active_mask)
{
    static struct SlicksRaceRuntime race;
    static unsigned char resource[8192],surface[64000],saved[1200];
    memset(&race,0,sizeof race);
    race.participation_ready=1;
    for(unsigned car=0;car<4;++car)
        race.participation[car]=(active_mask&(1U<<car))?(car&1?1:-1):0;
    long size=host_archive_load("ref/SLICKS.000","alamenu.@I",resource,sizeof resource);
    if(size<0 || slicks_race_add_hud_background(&race,resource,(unsigned long)size)) return 1;
    size=host_archive_load("ref/SLICKS.000",SLICKS_RACE_FONT_NAME,resource,sizeof resource);
    if(size<0 || slicks_race_add_font(&race,resource,(unsigned long)size)) return 1;
    const char *icons[]={"vir5.@I","vir6.@I","vir7.@I","vir8.@I",
        "vir9.@I","vir10.@I","vir11.@I","vir12.@I"};
    for(unsigned i=0;i<8;++i) {
        size=host_archive_load("ref/SLICKS.000",icons[i],resource,sizeof resource);
        if(size<0 || slicks_race_add_weapon_icon(&race,i,resource,(unsigned long)size) ||
            !race.hud_weapon_icons[i].ready) return 1;
    }
    race.chunky=surface; race.hud_colours[0]=73; race.hud_colours[1]=91;
    race.hud_colours[2]=73;
    FILE *track=fopen("ref/TRACKS/BASIC.SS","rb"); if(!track) return 1;
    size_t track_size=fread(resource,1,sizeof resource,track); fclose(track);
    if(slicks_race_set_track_info(&race,"TRACKS/BASIC.SS",resource,track_size)) return 1;
    memset(surface,77,sizeof surface); memset(vga,77,sizeof vga);
    uc_engine *uc; check(uc_open(UC_ARCH_X86,UC_MODE_16,&uc));
    check(uc_mem_map(uc,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(uc,0x10100,runtime,bytes));
    check(uc_mem_write(uc,0x50000,race.font.runtime,sizeof race.font.runtime));
    word(uc,0x3cbf0+0x680,0); word(uc,0x3cbf0+0x682,0x5000);
    word(uc,0x3cbf0+0x1d7b,100);
    word(uc,0x3cbf0+0x1d87,0); word(uc,0x3cbf0+0x1d89,20000);
    unsigned char colours[2]={91,73}; check(uc_mem_write(uc,0x3cbf0+0x4c1a,colours,2));
    for(unsigned page=0;page<2;++page)
        for(unsigned y=184;y<200;++y) for(unsigned x=0;x<320;++x)
            vga[(x&3)*65536+page*20000+y*100+x/4]=race.hud_background[(y-184)*320+x];
    for(unsigned car=0;car<4;++car) {
        saved[0]=13; saved[1]=22;
        for(unsigned p=0;p<4;++p) for(unsigned y=0;y<22;++y) for(unsigned x=0;x<13;++x) {
            unsigned sx=90+60*car+x*4+p;
            saved[2+p*286+y*13+x]=(y<14 && sx<320)?race.hud_background[(y+2)*320+sx]:77;
        }
        unsigned addr=0x52000+car*0x500;
        check(uc_mem_write(uc,addr,saved,1146));
        word(uc,0x3cbf0+0x4bd2+4*car,0); word(uc,0x3cbf0+0x4bd4+4*car,addr/16);
        signed char active=race.participation[car]; check(uc_mem_write(uc,0x3cbf0+0x4bc6+car,&active,1));
    }
    uc_hook ports,pixels,status;
    struct Capture unused={0};
    check(uc_hook_add(uc,&ports,UC_HOOK_INSN,hud_port,NULL,1,0,UC_X86_INS_OUT));
    check(uc_hook_add(uc,&pixels,UC_HOOK_MEM_WRITE,hud_pixel,NULL,0xa0000,0xaffff));
    /* Status/weapon rendering has its own test; all background, palette,
     * number formatting, strings, glyphs and VGA plotters execute here. */
    check(uc_hook_add(uc,&status,UC_HOOK_CODE,capture,&unused,0x1d9b6,0x1d9b6));
    /* Execute the real track label/record renderer. Only palette search is
     * fixed to the same test colour used by the native side. */
    uc_hook nearest;
    check(uc_hook_add(uc,&nearest,UC_HOOK_CODE,fixed_hud_colour,&unused,0x36fae,0x36fae));
    check(uc_mem_write(uc,0x55000,"BASIC\0\0\0\0",9));
    word(uc,0x3cbf0+0x4da4,0); word(uc,0x3cbf0+0x4da6,0x5500);
    word(uc,0x3cbf0+0x628,0);
    word(uc,0x3cbf0+0x62a,0); word(uc,0x3cbf0+0x62c,0x5600); word(uc,0x56000,0);
    word(uc,0x3cbf0+0x696b,race.hud_record_time);
    {
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xf000;
        word(uc,0x8f000,0); word(uc,0x8f002,0x7000);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss)); check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x2adbe,0x70000,0,1000000));
    }
    /* The name is written only to the original current page. Our single
     * surface replaces both pages; copy that known page-0 region for comparison. */
    for(unsigned y=186;y<193;++y) for(unsigned x=0;x<90;++x)
        vga[(x&3)*65536+20000+y*100+x/4]=vga[(x&3)*65536+y*100+x/4];
    check(uc_hook_del(uc,status)); /* Now execute the original weapon HUD too. */
    word(uc,0x3cbf0+0x1d8d,0); word(uc,0x3cbf0+0x1d8f,200);
    word(uc,0x3cbf0+0x1d91,0); word(uc,0x3cbf0+0x1d93,79);
    race.weapons_enabled=1; race.status_colours[0]=31; race.weapon_hud_colour=71;
    word(uc,0x3cbf0+0x3020,1); word(uc,0x3cbf0+0x3024,0); word(uc,0x3cbf0+0x3026,0);
    unsigned char colour=71; check(uc_mem_write(uc,0x3cbf0+0x799,&colour,1));
    colour=31; check(uc_mem_write(uc,0x3cbf0+0x68e2,&colour,1));
    for(unsigned icon=0;icon<8;++icon) {
        const struct SlicksHudIcon *image=&race.hud_weapon_icons[icon];
        unsigned stride=(image->width+3)/4,plane_size=stride*image->height;
        unsigned char planar[132]={0}; planar[0]=stride; planar[1]=image->height;
        for(unsigned y=0;y<image->height;++y) for(unsigned x=0;x<image->width;++x)
            planar[2+(x&3)*plane_size+y*stride+x/4]=image->pixels[y*image->width+x];
        planar[2+4*plane_size]=stride*4-image->width;
        check(uc_mem_write(uc,0x58000+icon*256,planar,sizeof planar));
        word(uc,0x3cbf0+0x6b1a+(icon+5)*4,0);
        word(uc,0x3cbf0+0x6b1c+(icon+5)*4,0x5800+icon*16);
        race.weapon_capacity[icon+5]=20;
        unsigned char capacity=20; check(uc_mem_write(uc,0x3cbf0+0x10a4+icon+5,&capacity,1));
    }
    for(unsigned car=0;car<4;++car) {
        unsigned char saved_icon[130]={4,8};
        for(unsigned p=0;p<4;++p) for(unsigned y=0;y<8;++y) for(unsigned x=0;x<4;++x)
            saved_icon[2+p*32+y*4+x]=race.hud_background[(y+8)*320+90+60*car+x*4+p];
        check(uc_mem_write(uc,0x57000+car*256,saved_icon,sizeof saved_icon));
        word(uc,0x3cbf0+0x4be2+car*4,0); word(uc,0x3cbf0+0x4be4+car*4,0x5700+car*16);
    }
    static const unsigned laps[]={1,12,99,100,255,4};
    race.status_colours[1]=51; race.status_colours[2]=61;
    colour=51; check(uc_mem_write(uc,0x3cbf0+0x68e4,&colour,1));
    colour=61; check(uc_mem_write(uc,0x3cbf0+0x68e3,&colour,1));
    word(uc,0x3cbf0+0x1716,0); word(uc,0x3cbf0+0x1718,0x5900);
    for(unsigned step=0;step<32;++step) {
        race.weapons_enabled=!!(step&1); race.fuel_option=step&2; race.damage_scale=step&4;
        word(uc,0x3cbf0+0x3020,race.weapons_enabled);
        word(uc,0x3cbf0+0x3024,race.fuel_option); word(uc,0x3cbf0+0x3026,race.damage_scale);
        word(uc,0x59000,(step>>3)&1);
        for(unsigned car=0;car<4;++car) {
            race.cars[car].fuel=(step*7+car*21)%101; race.cars[car].fuel_capacity=100;
            race.cars[car].damage[0]=(short)((step*31+car*67)%1000);
            race.cars[car].service_flags=car&1;
            unsigned data=0x3cbf0+54*car;
            word(uc,data+0x305f,race.cars[car].fuel); word(uc,data+0x3061,0);
            word(uc,data+0x3063,100); word(uc,data+0x3065,0);
            word(uc,data+0x304f,race.cars[car].damage[0]);
            unsigned char flags=car&1; check(uc_mem_write(uc,data+0x305e,&flags,1));
            signed char weapon=step==8?-1:(signed char)((step+car)%8);
            race.selected_weapon[car]=weapon;
            check(uc_mem_write(uc,0x3cbf0+0x2fac+car,&weapon,1));
            for(unsigned slot=5;slot<13;++slot) {
                race.weapon_inventory[car][slot]=(short)((step*3+car+slot)%21);
                word(uc,0x3cbf0+0x6a7a+car*26+slot*2,race.weapon_inventory[car][slot]);
            }
            unsigned place=step<6+car?0:car+1;
            unsigned char rank=place?place:255;
            race.cars[car].lap=laps[step%6]; race.cars[car].finished=!!place;
            race.cars[car].finish_position=place;
            race.cars[car].last_lap_time_units=step*1568+car*180;
            race.cars[car].best_lap_time_units=17999-step*918-car*180;
            check(uc_mem_write(uc,0x3cbf0+0x4bce +car,&rank,1));
            word(uc,0x3cbf0+0x4bfe +2*car,race.cars[car].lap-1);
            word(uc,0x3cbf0+0x3037+54*car,race.cars[car].last_lap_time_units);
            word(uc,0x3cbf0+0x4c06+4*car,race.cars[car].best_lap_time_units);
            uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
            word(uc,0x8f000,0); word(uc,0x8f002,0x7000); word(uc,0x8f004,car);
            check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
            check(uc_reg_write(uc,UC_X86_REG_SS,&ss)); check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
            check(uc_emu_start(uc,0x1ddc0,0x70000,0,1000000));
            check(uc_reg_read(uc,UC_X86_REG_IP,&ip)); check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
            if(ip || sp!=0xf004) return 1;
        }
        draw_timers(&race,NULL);
        if(slicks_race_draw_status(&race,NULL,(step>>3)&1)) return 1;
        for(unsigned page=0;page<2;++page)
            for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;++x)
                if(surface[y*320+x]!=vga[(x&3)*65536+page*20000+y*100+x/4]) {
                    fprintf(stderr,"Composed HUD mismatch step=%u page=%u xy=%u,%u native=%u dos=%u\n",
                        step,page,x,y,surface[y*320+x],vga[(x&3)*65536+page*20000+y*100+x/4]); return 1;
                }
    }
    uc_close(uc);
    return 0;
}

struct RecordStream { unsigned char data[363]; unsigned at; int fatal; };
struct WeaponCapture { unsigned count, fault, address[8], words[8][6]; };
static void weapon_fault(uc_engine *uc,uint32_t interrupt,void *user)
{
    struct WeaponCapture *c=user;
    if(interrupt!=0) { fprintf(stderr,"Unexpected weapon interrupt %u\n",interrupt); abort(); }
    c->fault=1;
    check(uc_emu_stop(uc));
}
static void weapon_draw(uc_engine *uc,uint64_t address,uint32_t size,void *user)
{
    (void)size;
    struct WeaponCapture *c=user;
    uint16_t ss,sp,cs,ip;
    check(uc_reg_read(uc,UC_X86_REG_SS,&ss)); check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    if(c->count>=8) abort();
    c->address[c->count]=(unsigned)address;
    for(unsigned i=0;i<(address==0x39ed8?6U:5U);++i)
        c->words[c->count][i]=readword(uc,stack+4+i*2);
    ++c->count;
    ip=readword(uc,stack); cs=readword(uc,stack+2); sp+=4;
    check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_IP,&ip));
    check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
}
static int inventory_oracle(const unsigned char *runtime,size_t bytes)
{
    uc_engine *uc; check(uc_open(UC_ARCH_X86,UC_MODE_16,&uc));
    check(uc_mem_map(uc,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(uc,0x10100,runtime,bytes));
    const short modes[]={0,1,-32768};
    unsigned cases=0;
    for(unsigned mask=0;mask<8192;++mask)
    for(unsigned mode=0;mode<3;++mode) {
        unsigned char flags[13];
        short inventory[4][13],cash[4];
        short starting_cash=(short)(unsigned short)(mask*17U+mode*32767U);
        for(unsigned slot=0;slot<13;++slot)
            flags[slot]=(unsigned char)(((mask+slot*38U)&254U)|((mask>>slot)&1));
        memset(inventory,0xa5,sizeof inventory); memset(cash,0xa5,sizeof cash);
        check(uc_mem_write(uc,0x3cbf0+0x106f,flags,sizeof flags));
        check(uc_mem_write(uc,0x3cbf0+0x6a7a,inventory,sizeof inventory));
        check(uc_mem_write(uc,0x3cbf0+0x4bf6,cash,sizeof cash));
        word(uc,0x3cbf0+0x3022,(unsigned short)modes[mode]);
        word(uc,0x3cbf0+0x302a,(unsigned short)starting_cash);
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,bp=0xf000,sp=0xef00,ip;
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
        check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
        check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x26372,0x263e6,0,10000));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        if(ip+16U*cs!=0x263e6) {
            fprintf(stderr,"Inventory loop stopped at %04x:%04x\n",cs,ip); return 1;
        }
        slicks_new_game_inventory(inventory,cash,flags,modes[mode],starting_cash);
        for(unsigned driver=0;driver<4;++driver) {
            if(readword(uc,0x3cbf0+0x4bf6+2*driver)!=(unsigned short)cash[driver]) return 1;
            for(unsigned slot=0;slot<13;++slot)
                if(readword(uc,0x3cbf0+0x6a7a+26*driver+2*slot)!=(unsigned short)inventory[driver][slot]) {
                    fprintf(stderr,"Inventory mismatch mask=%u mode=%d driver=%u slot=%u\n",
                        mask,modes[mode],driver,slot); return 1;
                }
        }
        ++cases;
    }
    check(uc_close(uc));
    printf("DOS new-game inventory: %u complete four-player initializations match all 13-slot flag masks, mode gating and cash\n",cases);
    return 0;
}

static int weapon_oracle(const unsigned char *runtime,size_t bytes)
{
    uc_engine *uc; check(uc_open(UC_ARCH_X86,UC_MODE_16,&uc));
    check(uc_mem_map(uc,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(uc,0x10100,runtime,bytes));
    struct WeaponCapture c;
    uc_hook faults;
    check(uc_hook_add(uc,&faults,UC_HOOK_INTR,weapon_fault,&c,1,0));
    const unsigned hooks[]={0x39ed8,0x3aaf2,0x3aa7c};
    for(unsigned i=0;i<3;++i) { uc_hook h; check(uc_hook_add(uc,&h,UC_HOOK_CODE,weapon_draw,&c,hooks[i],hooks[i])); }
    const short amounts[]={0,1,20,127,1638,1639,8192,-1,-32768,32767};
    const signed char capacities[]={0,1,20,127,-1,-128};
    const unsigned base=0x3cbf0;
    unsigned char colour=71; check(uc_mem_write(uc,base+0x799,&colour,1));
    colour=31; check(uc_mem_write(uc,base+0x68e2,&colour,1));
    word(uc,base+0x3024,0); word(uc,base+0x3026,0);
    word(uc,base+0x1d87,0x1234); word(uc,base+0x1d89,0x5678);
    uc_context *clean;
    check(uc_context_alloc(uc,&clean)); check(uc_context_save(uc,clean));
    unsigned cases=0;
    for(unsigned driver=0;driver<4;++driver)
    for(int selected=-1;selected<8;++selected)
    for(unsigned mode=0;mode<4;++mode)
    for(unsigned amount=0;amount<10;++amount)
    for(unsigned cap=0;cap<6;++cap) {
        /* Reset the emulator's pending exception state between fault cases. */
        check(uc_context_restore(uc,clean));
        short inventory[13]={0}; signed char capacity[13]={0};
        unsigned icon=selected<0?0:(unsigned)selected+5;
        inventory[icon]=amounts[amount]; capacity[icon]=capacities[cap];
        unsigned char active=mode&1,selection=(unsigned char)selected;
        check(uc_mem_write(uc,base+0x4bc6+driver,&active,1));
        check(uc_mem_write(uc,base+0x2fac+driver,&selection,1));
        word(uc,base+0x3020,mode&2);
        word(uc,base+0x6a7a+driver*26+icon*2,(unsigned short)inventory[icon]);
        check(uc_mem_write(uc,base+0x10a4+icon,&capacity[icon],1));
        word(uc,base+0x4be2+driver*4,0x1010); word(uc,base+0x4be4+driver*4,0x5050);
        word(uc,base+0x6b1a+icon*4,0x2020+icon); word(uc,base+0x6b1c+icon*4,0x6060);
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
        word(uc,0x8f000,0); word(uc,0x8f002,0x7000); word(uc,0x8f004,driver);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss)); check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        c.count=0; c.fault=0;
        struct SlicksHudWeapon native={0};
        int result=slicks_hud_weapon(driver,active,mode&2,(signed char)selected,inventory,capacity,&native);
        uc_err error=uc_emu_start(uc,0x1d9b6,0x70000,0,10000);
        if(result==-1) {
            if(error!=UC_ERR_OK || !c.fault || c.count!=2) goto mismatch;
        } else {
            check(error);
            check(uc_reg_read(uc,UC_X86_REG_CS,&cs)); check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
            check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
            if(c.fault || cs*16U+ip!=0x70000 || sp!=0xf004 || c.count!=(active && (mode&2)?2U:0U)+(result==1?6U:0U)) goto mismatch;
            for(unsigned i=0;i<c.count;++i) {
                unsigned expected[6]={0}, address=0x39ed8;
                if(i<4) {
                    expected[0]=106+driver*60; expected[1]=187;
                    expected[2]=i<2?126+driver*60:(unsigned short)native.bar_right;
                    expected[3]=i<2?190:188; expected[4]=i<2?31:71;
                    expected[5]=(i&1)?0x5678:0x1234;
                } else {
                    address=i<6?0x3aaf2:0x3aa7c;
                    expected[0]=(unsigned short)(i<6?native.restore_x:native.icon_x); expected[1]=192;
                    expected[2]=i<6?0x1010:0x2020+native.icon;
                    expected[3]=i<6?0x5050:0x6060;
                    expected[4]=i<6?((i&1)?0x1234:0x5678):((i&1)?0x5678:0x1234);
                }
                if(c.address[i]!=address || memcmp(c.words[i],expected,(i<4?6:5)*sizeof(unsigned))) goto mismatch;
            }
        }
        ++cases; continue;
    mismatch:
        fprintf(stderr,"Weapon HUD mismatch driver=%u selected=%d mode=%u amount=%d capacity=%d result=%d calls=%u error=%s\n",driver,selected,mode,amounts[amount],capacities[cap],result,c.count,uc_strerror(error));
        return 1;
    }
    unsigned selections=0;
    const signed char currents[]={-1,0,1,2,3,4,5,6,7,8,127};
    const short unavailable[]={-32768,-1,0,1};
    for(unsigned driver=0;driver<4;++driver)
    for(unsigned bits=0;bits<256;++bits)
    for(unsigned current=0;current<sizeof currents/sizeof currents[0];++current) {
        check(uc_context_restore(uc,clean));
        short inventory[13]={0};
        for(unsigned slot=0;slot<8;++slot) {
            inventory[slot+5]=(bits&(1U<<slot))?(slot&1?32767:2):unavailable[(slot+driver)%4];
            word(uc,base+0x6a84+26*driver+2*slot,(unsigned short)inventory[slot+5]);
        }
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000,ip,ax;
        word(uc,0x8f000,0); word(uc,0x8f002,0x7000);
        word(uc,0x8f004,driver); word(uc,0x8f006,(unsigned char)currents[current]);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss)); check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x1eb49,0x70000,0,10000));
        check(uc_reg_read(uc,UC_X86_REG_AX,&ax)); check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
        signed char expected=slicks_next_weapon(inventory,currents[current]);
        if(ip || sp!=0xf004 || (unsigned char)ax!=(unsigned char)expected) {
            fprintf(stderr,"Weapon selection mismatch driver=%u mask=%u current=%d native=%d dos=%d\n",
                    driver,bits,currents[current],expected,(signed char)ax); return 1;
        }
        ++selections;
    }
    printf("DOS weapon selection: %u cases match all inventory-availability masks, wraparound and signed counts\n",selections);
    unsigned initializations=0;
    const signed char roles[]={-128,-1,0,1,127};
    const unsigned long seeds[]={0,1,0x1234,0x7fffffff,0x80000000,0xffffffff};
    for(unsigned driver=0;driver<4;++driver)
    for(unsigned bits=0;bits<256;++bits)
    for(unsigned role=0;role<sizeof roles;++role)
    for(unsigned seed=0;seed<sizeof seeds/sizeof seeds[0];++seed) {
        check(uc_context_restore(uc,clean));
        short inventory[13]={0};
        for(unsigned slot=0;slot<13;++slot) {
            inventory[slot]=slot>=5 && (bits&(1U<<(slot-5)))?2:0;
            word(uc,base+0x6a7a+26*driver+2*slot,(unsigned short)inventory[slot]);
        }
        unsigned long random=seeds[seed];
        word(uc,base+0x2aaa,(unsigned)random); word(uc,base+0x2aac,(unsigned)(random>>16));
        check(uc_mem_write(uc,base+0x4bc6+driver,&roles[role],1));
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xe000,bp=0xf000,ip;
        word(uc,0x8f000-0x68,driver);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss)); check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_emu_start(uc,0x1fd72,0x1fdc6,0,10000));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip)); check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
        signed char selected; check(uc_mem_read(uc,base+0x2fac+driver,&selected,1));
        signed char expected=slicks_initial_weapon(inventory,roles[role],&random);
        unsigned long actual=readword(uc,base+0x2aaa)|((unsigned long)readword(uc,base+0x2aac)<<16);
        if(ip!=0x6556 || sp!=0xe000 || selected!=expected || random!=actual) {
            fprintf(stderr,"Initial weapon mismatch driver=%u mask=%u role=%d seed=%lx selected=%d/%d RNG=%lx/%lx ip=%x sp=%x\n",
                driver,bits,roles[role],seeds[seed],selected,expected,actual,random,ip,sp); return 1;
        }
        ++initializations;
    }
    printf("Original initial weapons: %u complete selection/RNG cases pass\n",initializations);
    check(uc_context_free(clean));
    check(uc_close(uc));
    printf("DOS weapon HUD: %u complete command/fault cases match gating, signed bar arithmetic, restore and icon draws\n",cases);
    return 0;
}
static void record_io(uc_engine *uc,uint64_t address,uint32_t size,void *user)
{
    (void)size;
    struct RecordStream *s=user;
    if(address==0x36243) { s->fatal=1; check(uc_emu_stop(uc)); return; }
    uint16_t ss,sp,cs,ip,ax=0,dx=0;
    check(uc_reg_read(uc,UC_X86_REG_SS,&ss)); check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    if(address==0x11eaf) ax=1;
    else if(address==0x1219b) s->at=readword(uc,stack+8);
    else if(address==0x12d8a) ax=s->at<sizeof s->data?s->data[s->at++]:65535;
    ip=readword(uc,stack); cs=readword(uc,stack+2); sp+=4;
    check(uc_reg_write(uc,UC_X86_REG_AX,&ax)); check(uc_reg_write(uc,UC_X86_REG_DX,&dx));
    check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_IP,&ip));
    check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
}
static int records_oracle(const unsigned char *runtime,size_t bytes)
{
    unsigned char original[363];
    FILE *f=fopen("ref/TRACKS/BASIC.SS","rb"); if(!f) return 1;
    size_t got=fread(original,1,sizeof original,f); fclose(f); if(got!=sizeof original) return 1;
    for(unsigned length=0;length<sizeof original;++length) {
        struct SlicksTrackRecords out={0};
        if(slicks_track_records(original,length,&out)!=-1) {
            fprintf(stderr,"Truncated track records accepted at %u bytes\n",length); return 1;
        }
    }
    uc_engine *uc; check(uc_open(UC_ARCH_X86,UC_MODE_16,&uc));
    check(uc_mem_map(uc,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(uc,0x10100,runtime,bytes));
    struct RecordStream s;
    const unsigned hooks[]={0x11eaf,0x1219b,0x12d8a,0x119c2,0x36243};
    for(unsigned i=0;i<5;++i) { uc_hook h; check(uc_hook_add(uc,&h,UC_HOOK_CODE,record_io,&s,hooks[i],hooks[i])); }
    for(unsigned test=0;test<=363;++test) {
        memcpy(s.data,original,sizeof original); s.at=0; s.fatal=0;
        if(test) s.data[test-1]^=0x80;
        struct SlicksTrackRecords native;
        memset(&native,0x55,sizeof native);
        int result=slicks_track_records(s.data,sizeof s.data,&native);
        unsigned char seed[319]; memset(seed,0x55,sizeof seed);
        check(uc_mem_write(uc,0x3cbf0+0x693a,seed,sizeof seed));
        uint16_t cs=0x1010,ds=0x3cbf,ss=0x8000,sp=0xf000,ax,ip;
        word(uc,0x8f000,0); word(uc,0x8f002,0x7000);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss)); check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x1a62e,0x70000,0,100000));
        check(uc_reg_read(uc,UC_X86_REG_AX,&ax)); check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        check(uc_reg_read(uc,UC_X86_REG_CS,&cs));
        if(s.fatal!=(result<0)) { fprintf(stderr,"Record fatal mismatch case=%u native=%d dos=%d\n",test,result,s.fatal); return 1; }
        if(!s.fatal) {
            check(uc_mem_read(uc,0x3cbf0+0x693a,seed,sizeof seed));
            if(cs*16U+ip!=0x70000 || (ax&255)!=(unsigned)result ||
                memcmp(seed,native.entries,sizeof seed) || native.trailer!=readword(uc,0x3cbf0+0x4db2)) {
                fprintf(stderr,"Record data/return mismatch case=%u\n",test); return 1;
            }
        }
    }
    check(uc_close(uc));
    puts("DOS track records: 364 original-loader comparisons pass (all header-byte mutations, data, checksums and fatal paths)");
    return 0;
}

int main(void)
{
    static unsigned char runtime[300000];
    FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f); int error=ferror(f); fclose(f);
    if(error || bytes<200000 || bytes==sizeof runtime) return 2;
    uc_engine *uc; check(uc_open(UC_ARCH_X86,UC_MODE_16,&uc));
    check(uc_mem_map(uc,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(uc,0x10100,runtime,bytes));
    static const unsigned boundaries[]={0x301ab,0x302b6,0x3aaf2,0x2fe63,0x1d9b6};
    struct Capture c;
    for(unsigned i=0;i<5;++i) { uc_hook h; check(uc_hook_add(uc,&h,UC_HOOK_CODE,capture,&c,boundaries[i],boundaries[i])); }
    static const unsigned laps[]={1,4,99,100,255};
    static const unsigned times[]={0,1,9,1568,5634,17999,18000,65535};
    unsigned cases=0;
    for(unsigned driver=0;driver<4;++driver)
    for(unsigned active=0;active<3;++active)
    for(unsigned place=0;place<=4;++place)
    for(unsigned lap=0;lap<5;++lap)
    for(unsigned time=0;time<8;++time) {
        memset(&c,0,sizeof c);
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
        unsigned data=ds*16U;
        unsigned char enabled=active==2?255:active,rank=place?place:255;
        check(uc_mem_write(uc,data+0x4bc6+driver,&enabled,1));
        check(uc_mem_write(uc,data+0x4bce +driver,&rank,1));
        word(uc,data+0x4bfe +2*driver,laps[lap]-1);
        word(uc,data+0x3037+54*driver,times[time]);
        word(uc,data+0x4c06+4*driver,times[7-time]);
        word(uc,0x8f000,0); word(uc,0x8f002,0x7000); word(uc,0x8f004,driver);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss)); check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        check(uc_emu_start(uc,0x1ddc0,0x70000,0,10000));
        check(uc_reg_read(uc,UC_X86_REG_CS,&cs)); check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
        struct SlicksHudText expected[3];
        unsigned n=active?slicks_hud_driver_text(driver,laps[lap],place,times[time],times[7-time],expected):0;
        if(cs*16U+ip!=0x70000 || sp!=0xf004 || c.count!=2*n || c.icons!=2*(active!=0) || c.status!=(active!=0)) {
            fprintf(stderr,"HUD command count/return mismatch driver=%u active=%u place=%u count=%u expected=%u pc=%x\n",driver,active,place,c.count,2*n,cs*16U+ip); return 1;
        }
        for(unsigned i=0;i<c.count;++i) if(memcmp(&c.text[i],&expected[i/2],sizeof expected[0])) {
            fprintf(stderr,"HUD text mismatch driver=%u place=%u command=%u xy=%d,%d flags=%u number=%d text=%s\n",
                driver,place,i,c.text[i].x,c.text[i].y,c.text[i].flags,c.text[i].number,c.text[i].text); return 1;
        }
        ++cases;
    }
    check(uc_close(uc));
    printf("DOS driver HUD: %u complete text-command sequences match both pages (icon/palette/status graphics stubbed)\n",cases);
    if(inventory_oracle(runtime,bytes) || weapon_oracle(runtime,bytes) || records_oracle(runtime,bytes) || track_hud(runtime,bytes)) return 1;
    for(unsigned mask=0;mask<16;++mask) if(composed_hud(runtime,bytes,mask)) return 1;
    puts("Composed original HUD: 512 full-screen transitions, original kirj font, all 16 participation masks, mixed human/computer and racing/finished drivers, every weapon/fuel/damage option combination and both refuelling blink phases (page-0 name mirrored)");
    return 0;
}
