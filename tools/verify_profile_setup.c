#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/profile_setup.h"
#include "../src/ui/player_menu.h"
#include "../src/ui/player_menu_draw.h"
#include "../src/ui/profile_editor_draw.h"

static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned a,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(u,a,b,2)); }
static unsigned readword(uc_engine *u,unsigned a)
{ unsigned char b[2]; check(uc_mem_read(u,a,b,2)); return b[0]|b[1]<<8; }
struct Calls { unsigned override,random,applied; unsigned char mode[4],driver[4],vehicle[4]; };
struct MenuDrawCall { unsigned kind; short args[8]; unsigned char text[64]; };
#ifndef SLICKS_MENU_TRACE_CAPACITY
#define SLICKS_MENU_TRACE_CAPACITY 48
#endif
struct MenuDrawTrace { struct MenuDrawCall calls[SLICKS_MENU_TRACE_CAPACITY]; unsigned count; };
static struct MenuDrawCall *menu_call(void *p,unsigned kind)
{ struct MenuDrawTrace *t=p; if(t->count>=SLICKS_MENU_TRACE_CAPACITY) abort(); struct MenuDrawCall *c=&t->calls[t->count++]; c->kind=kind; return c; }
static void menu_restore(void *p,short x,short y,short sx,short sy,short w,short h)
{ struct MenuDrawCall *c=menu_call(p,0); short a[]={x,y,sx,sy,w,h}; memcpy(c->args,a,sizeof a); }
static void menu_bevel(void *p,short x,short y,short w,short h,unsigned char r,unsigned char g,unsigned char b)
{ struct MenuDrawCall *c=menu_call(p,1); short a[]={x,y,w,h,r,g,b}; memcpy(c->args,a,sizeof a); }
static void menu_sprite(void *p,short sprite,short x,short y)
{ struct MenuDrawCall *c=menu_call(p,2); c->args[0]=sprite; c->args[1]=x; c->args[2]=y; }
static void menu_text(void *p,unsigned font,const unsigned char *text,short x,short y,unsigned char flags)
{ struct MenuDrawCall *c=menu_call(p,3); c->args[0]=(short)font; c->args[1]=x; c->args[2]=y; c->args[3]=flags;
  unsigned i=0; do { if(i>=64) abort(); c->text[i]=text[i]; } while(text[i++]); }
static void menu_draw_boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)size; uint16_t ss,sp,cs,ip;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp; short a[9]; for(unsigned i=0;i<9;++i) a[i]=(short)readword(u,stack+4+2*i);
    if(address==0x3b9de) menu_restore(p,a[0],a[1],a[2],a[3],a[4],a[5]);
    else if(address==0x309cf) menu_bevel(p,a[0],a[1],a[2],a[3],(unsigned char)a[4],(unsigned char)a[5],(unsigned char)a[6]);
    else if(address==0x2e2d2) menu_sprite(p,a[2]==0x300?-1:(short)((a[2]-0x400)/16),a[0],a[1]);
    else {
        unsigned char text[64]; check(uc_mem_read(u,(unsigned short)a[2]+16U*(unsigned short)a[3],text,sizeof text));
        menu_text(p,a[4]==0x100?0:1,text,a[0],a[1],(unsigned char)a[6]);
    }
    ip=readword(u,stack); cs=readword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void verify_menu_draw(uc_engine *u)
{
    const unsigned addresses[]={0x3b9de,0x309cf,0x2e2d2,0x301ab}; uc_hook hooks[4]; struct MenuDrawTrace dos;
    for(unsigned i=0;i<4;++i) check(uc_hook_add(u,&hooks[i],UC_HOOK_CODE,menu_draw_boundary,&dos,addresses[i],addresses[i]));
    unsigned char action_bytes[44],random[4];
    check(uc_mem_read(u,0x3cbf0+0x1110,action_bytes,44)); check(uc_mem_read(u,0x3cbf0+0x1393,random,4));
    struct SlicksPlayerMenuLabels labels={random,random+2,{action_bytes,action_bytes+11,action_bytes+22,action_bytes+33}};
    unsigned cases=0;
    for(unsigned variant=0;variant<64;++variant) for(unsigned row=0;row<8;++row) {
        struct SlicksPlayerProfiles p; memset(&p,0,sizeof p); p.count=(short)(1+variant%7);
        for(unsigned i=0;i<7;++i) { p.names[i][0]='A'+i; p.names[i][1]='0'+variant%10; p.setup[i].vehicle=(unsigned char)((variant+i)%7);
            check(uc_mem_write(u,0x3cbf0+0x36aa+21*i,p.names[i],21));
            check(uc_mem_write(u,0x3cbf0+0x3fa6+i,&p.setup[i].vehicle,1)); }
        short selected[4]; signed char roles[4];
        for(unsigned i=0;i<4;++i) { selected[i]=(short)((variant+i)%7); roles[i]=(signed char)((variant+i)%3-1); word(u,0x3cbf0+0x44c+2*i,selected[i]); }
        check(uc_mem_write(u,0x3cbf0+0x4bc6,roles,4)); word(u,0x3cbf0+0x4c6e,4);
        word(u,0x3cbf0+0x680,0x100); word(u,0x3cbf0+0x682,0x6000);
        word(u,0x3cbf0+0x688,0x200); word(u,0x3cbf0+0x68a,0x6000);
        for(unsigned i=0;i<4;++i) { word(u,0x3cbf0+0x4e44+4*i,0x400+16*i); word(u,0x3cbf0+0x4e46+4*i,0x6000); }
        word(u,0x8effc,0x300); word(u,0x8effe,0x6000); unsigned char r=(unsigned char)row; check(uc_mem_write(u,0x8eff9,&r,1));
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        memset(&dos,0,sizeof dos); check(uc_emu_start(u,0x286ab,0x28981,0,100000));
        struct MenuDrawTrace native={0}; struct SlicksPlayerMenuDrawOps ops={menu_restore,menu_bevel,menu_sprite,menu_text,&native};
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        if(slicks_draw_player_menu(row,selected,roles,&p,4,&labels,&ops) || ip!=0x28981-0x266c0 || sp!=0xeb00 || memcmp(&dos,&native,sizeof dos)) {
            fprintf(stderr,"Player menu draw mismatch variant=%u row=%u DOS calls=%u native=%u\n",variant,row,dos.count,native.count); exit(1);
        }
        ++cases;
    }
    for(unsigned i=0;i<4;++i) check(uc_hook_del(u,hooks[i]));
    printf("DOS player-menu drawing: %u complete command/text/order comparisons pass\n",cases);
}
static void editor_colour(void *p,unsigned char colour)
{ menu_call(p,4)->args[0]=colour; }
static void editor_number(void *p,unsigned short number,short x,short y,unsigned char flags)
{ struct MenuDrawCall *c=menu_call(p,5); short a[]={(short)number,x,y,flags}; memcpy(c->args,a,sizeof a); }
static unsigned char editor_nearest(void *p,unsigned char r,unsigned char g,unsigned char b)
{ struct MenuDrawCall *c=menu_call(p,6); c->args[0]=r;c->args[1]=g;c->args[2]=b; return (unsigned char)(r+g*3+b*7); }
static void editor_rectangle(void *p,short l,short t,short r,short b,unsigned char colour)
{ struct MenuDrawCall *c=menu_call(p,7); short a[]={l,t,r,b,colour}; memcpy(c->args,a,sizeof a); }
static void editor_draw_boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    if(address==0x3b9de || address==0x309cf || address==0x2e2d2 || address==0x301ab) {
        menu_draw_boundary(u,address,size,p); return;
    }
    uint16_t ss,sp,cs,ip,ax=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp; short a[7]; for(unsigned i=0;i<7;++i) a[i]=(short)readword(u,stack+4+2*i);
    if(address==0x2fe63) editor_colour(p,(unsigned char)a[0]);
    else if(address==0x302b6) editor_number(p,(unsigned short)a[2],a[0],a[1],(unsigned char)a[5]);
    else if(address==0x36fae) { ax=editor_nearest(p,(unsigned char)a[0],(unsigned char)a[1],(unsigned char)a[2]); check(uc_reg_write(u,UC_X86_REG_AX,&ax)); }
    else if(address==0x39ed8) editor_rectangle(p,a[0],a[1],a[2],a[3],(unsigned char)a[4]);
    else abort();
    ip=readword(u,stack); cs=readword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void verify_editor_draw(uc_engine *u)
{
    const unsigned addresses[]={0x3b9de,0x309cf,0x2e2d2,0x301ab,0x2fe63,0x302b6,0x36fae,0x39ed8};
    uc_hook hooks[8]; struct MenuDrawTrace dos;
    for(unsigned i=0;i<8;++i) check(uc_hook_add(u,&hooks[i],UC_HOOK_CODE,editor_draw_boundary,&dos,addresses[i],addresses[i]));
    unsigned char rows[48],roles[18],percent[64],random[64],each[64],unavailable[64];
    check(uc_mem_read(u,0x3cbf0+0x10ce,rows,sizeof rows));
    check(uc_mem_read(u,0x3cbf0+0x10fe,roles,sizeof roles));
    check(uc_mem_read(u,0x3cbf0+0x1280,percent,sizeof percent));
    check(uc_mem_read(u,0x3cbf0+0x1300,random,sizeof random));
    check(uc_mem_read(u,0x3cbf0+0x1314,each,sizeof each));
    check(uc_mem_read(u,0x3cbf0+0x132a,unavailable,sizeof unavailable));
    struct SlicksProfileEditorLabels labels={{rows,rows+8,rows+16,rows+24,rows+32,rows+40},
        {roles,roles+9},percent,random,each,unavailable};
    unsigned cases=0; const unsigned char redraws[]={0,1,2,255},vehicles[]={0,1,9,10,11,12,127,128,255};
    for(unsigned variant=0;variant<36;++variant) for(unsigned row=0;row<6;++row) for(unsigned rd=0;rd<4;++rd) {
        struct SlicksPlayerProfiles p={0}; unsigned index=variant%2?99:3;
        p.setup[index].vehicle=vehicles[variant%9]; p.setup[index].flags=(unsigned char)variant;
        p.setting[index]=(unsigned char)(variant*9);
        for(unsigned j=0;j<6;++j) p.setup[index].colours[j]=(unsigned char)(variant*17+j*31);
        unsigned char name[21]={0}; for(unsigned j=0;j<variant%21;++j) name[j]='A'+j;
        struct SlicksProfileEditor e={(unsigned char)row,redraws[rd],0};
        check(uc_mem_write(u,0x8efa8,rows,sizeof rows)); check(uc_mem_write(u,0x8ef96,roles,sizeof roles));
        check(uc_mem_write(u,0x8efd8,name,sizeof name));
        check(uc_mem_write(u,0x8effc,&e.row,1)); check(uc_mem_write(u,0x8effb,&e.redraw,1));
        unsigned char colours[]={23,47}; check(uc_mem_write(u,0x8effe,colours,1)); check(uc_mem_write(u,0x8effd,colours+1,1));
        word(u,0x8f006,index); word(u,0x3cbf0+0x4c6e,10);
        check(uc_mem_write(u,0x3cbf0+0x3ede +index,&p.setup[index].flags,1));
        check(uc_mem_write(u,0x3cbf0+0x3fa6+index,&p.setup[index].vehicle,1));
        check(uc_mem_write(u,0x3cbf0+0x3f42+index,&p.setting[index],1));
        check(uc_mem_write(u,0x3cbf0+0x1db+6*index,p.setup[index].colours,6));
        word(u,0x3cbf0+0x680,0x100); word(u,0x3cbf0+0x682,0x6000);
        for(unsigned i=0;i<10;++i) { word(u,0x3cbf0+0x4e44+4*i,0x400+16*i); word(u,0x3cbf0+0x4e46+4*i,0x6000); }
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        memset(&dos,0,sizeof dos); check(uc_emu_start(u,0x27bea,0x27f53,0,100000));
        struct MenuDrawTrace native={0}; struct SlicksProfileEditorDrawOps ops={
            {menu_restore,menu_bevel,menu_sprite,menu_text,&native},editor_colour,editor_number,editor_nearest,editor_rectangle};
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        unsigned char redraw; check(uc_mem_read(u,0x8effb,&redraw,1));
        if(slicks_draw_profile_editor(&e,&p,index,name,10,23,47,&labels,&ops) ||
           ip!=0x27f53-0x266c0 || sp!=0xeb00 || e.redraw!=redraw || memcmp(&dos,&native,sizeof dos)) {
            fprintf(stderr,"Editor draw mismatch variant=%u row=%u redraw=%u DOS=%u native=%u\n",variant,row,redraws[rd],dos.count,native.count);
            for(unsigned i=0;i<dos.count || i<native.count;++i) if(memcmp(&dos.calls[i],&native.calls[i],sizeof dos.calls[i]))
                fprintf(stderr,"call %u kinds %u/%u args %d,%d,%d,%d / %d,%d,%d,%d\n",i,dos.calls[i].kind,native.calls[i].kind,
                    dos.calls[i].args[0],dos.calls[i].args[1],dos.calls[i].args[2],dos.calls[i].args[3],
                    native.calls[i].args[0],native.calls[i].args[1],native.calls[i].args[2],native.calls[i].args[3]);
            exit(1);
        }
        ++cases;
    }
    for(unsigned i=0;i<8;++i) check(uc_hook_del(u,hooks[i]));
    printf("DOS profile-editor drawing: %u complete command/text/order/redraw comparisons pass\n",cases);
}
static void menu_modal_boundary(uc_engine *u,uint64_t address,uint32_t size,void *opaque)
{
    (void)size;
    if(address==0x28a1b) {
        /* Keyboard-drain call is a platform boundary, not menu state. */
        uint16_t ip=0x28a20-0x266c0;
        check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    } else {
        *(unsigned *)opaque=(unsigned)address;
        check(uc_emu_stop(u));
    }
}
static void verify_menu_modals(uc_engine *u)
{
    const unsigned addresses[]={0x28a1b,0x28a36,0x28a82,0x28ac1,0x28a0c,0x28ce4};
    uc_hook hooks[6]; unsigned stopped=0,cases=0;
    for(unsigned i=0;i<6;++i)
        check(uc_hook_add(u,&hooks[i],UC_HOOK_CODE,menu_modal_boundary,&stopped,addresses[i],addresses[i]));
    const unsigned char keys[]={0x1c,0x1d,0x39,0x3b};
    const short counts[]={3,99,100};
    for(unsigned c=0;c<3;++c) for(unsigned row=0;row<8;++row) for(unsigned k=0;k<4;++k) {
        struct SlicksPlayerMenu menu={(unsigned char)row,0,0,0};
        struct SlicksSetupProfile p[100]={0}; short selected[4]={0,1,2,1};
        check(uc_mem_write(u,0x8eff9,&menu.row,1));
        check(uc_mem_write(u,0x8eff8,&menu.redraw,1));
        check(uc_mem_write(u,0x8eff7,&menu.done,1));
        check(uc_mem_write(u,0x3cbf0+0x6c0,&menu.dirty,1));
        word(u,0x3cbf0+0x36a8,counts[c]);
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000,ax=keys[k];
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp)); check(uc_reg_write(u,UC_X86_REG_AX,&ax));
        stopped=0; check(uc_emu_start(u,0x28999,0x28dab,0,10000));
        enum SlicksPlayerMenuAction action=slicks_player_menu_key(&menu,selected,p,counts[c],10,keys[k]);
        unsigned target=action==SLICKS_PLAYER_MENU_PICK?0x28a36:
            action==SLICKS_PLAYER_MENU_ADD?0x28a82:
            action==SLICKS_PLAYER_MENU_EDIT || action==SLICKS_PLAYER_MENU_DELETE?0x28ac1:
            action==SLICKS_PLAYER_MENU_HELP?0x28a0c:0x28ce4;
        unsigned char dirty,done;
        check(uc_mem_read(u,0x3cbf0+0x6c0,&dirty,1)); check(uc_mem_read(u,0x8eff7,&done,1));
        if(stopped!=target || dirty!=menu.dirty || done!=menu.done) abort();
        ++cases;
    }
    for(unsigned i=0;i<6;++i) check(uc_hook_del(u,hooks[i]));
    printf("DOS player-menu dialogs: %u dispatch-boundary/dirty/exit comparisons pass; dialog contents remain separate\n",cases);
}
static void verify_menu_keys(uc_engine *u)
{
    unsigned cases=0;
    for(unsigned pattern=0;pattern<4;++pattern) for(unsigned row=0;row<8;++row)
    for(unsigned scan=0;scan<256;++scan) {
        if(scan==0x1c || scan==0x1d || scan==0x39 || scan==0x3b ||
            (row>=4 && (scan==0x1e || scan==0x2e))) continue;
        struct SlicksSetupProfile p[4]={0}; short selected[4];
        struct SlicksPlayerMenu menu={(unsigned char)row,0,0,(unsigned char)(pattern&1)};
        for(unsigned i=0;i<4;++i) {
            selected[i]=(short)((i+pattern)%4);
            p[i].flags=(unsigned char)(pattern*2+i);
            p[i].vehicle=(unsigned char)(pattern==0?10:pattern==1?11:pattern==2?127:255);
            word(u,0x3cbf0+0x44c+2*i,selected[i]);
            check(uc_mem_write(u,0x3cbf0+0x3ede +i,&p[i].flags,1));
            check(uc_mem_write(u,0x3cbf0+0x3fa6+i,&p[i].vehicle,1));
        }
        word(u,0x3cbf0+0x36a8,4); word(u,0x3cbf0+0x4c6e,10);
        check(uc_mem_write(u,0x3cbf0+0x6c0,&menu.dirty,1));
        check(uc_mem_write(u,0x8eff9,&menu.row,1));
        check(uc_mem_write(u,0x8eff8,&menu.redraw,1));
        check(uc_mem_write(u,0x8eff7,&menu.done,1));
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000,ax=scan,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp)); check(uc_reg_write(u,UC_X86_REG_AX,&ax));
        check(uc_emu_start(u,0x28999,0x28dab,0,10000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        if(ip!=0x28dab-0x266c0 || sp!=0xeb00 ||
            slicks_player_menu_key(&menu,selected,p,4,10,(unsigned char)scan)!=SLICKS_PLAYER_MENU_NONE) abort();
        unsigned char actual[4];
        check(uc_mem_read(u,0x8eff9,actual,1));
        check(uc_mem_read(u,0x8eff8,actual+1,1));
        check(uc_mem_read(u,0x8eff7,actual+2,1));
        check(uc_mem_read(u,0x3cbf0+0x6c0,actual+3,1));
        if(memcmp(actual,&menu,4)) { fprintf(stderr,"menu state row=%u scan=%u pattern=%u\n",row,scan,pattern); abort(); }
        for(unsigned i=0;i<4;++i) {
            unsigned char vehicle;
            check(uc_mem_read(u,0x3cbf0+0x3fa6+i,&vehicle,1));
            if((short)readword(u,0x3cbf0+0x44c+2*i)!=selected[i] || vehicle!=p[i].vehicle) abort();
        }
        ++cases;
    }
    printf("DOS player-menu keys: %u non-modal dispatch/state comparisons pass, including signed vehicle bytes\n",cases);
}
static void verify_menu(uc_engine *u)
{
    unsigned cases=0;
    for(unsigned flags=0;flags<16;++flags) for(unsigned layout=0;layout<256;++layout)
    for(unsigned choice=0;choice<4;++choice) {
        struct SlicksSetupProfile profiles[4]={0}; short initial[4],expected[4];
        for(unsigned i=0;i<4;++i) {
            profiles[i].flags=(flags&(1U<<i))?2:0;
            initial[i]=expected[i]=(short)((layout>>(2*i))&3);
            check(uc_mem_write(u,0x3cbf0+0x3ede +i,&profiles[i].flags,1));
        }
        for(unsigned operation=0;operation<6;++operation) {
            memcpy(expected,initial,sizeof expected);
            for(unsigned i=0;i<4;++i) word(u,0x3cbf0+0x44c+2*i,(unsigned short)initial[i]);
            word(u,0x3cbf0+0x36a8,4);
            uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xf000,ip,ax;
            word(u,0x8f000,0); word(u,0x8f002,0x7000);
            word(u,0x8f004,operation<4?operation:choice);
            word(u,0x8f006,operation<4?choice:operation==4?255:1);
            check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
            check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
            check(uc_emu_start(u,operation<4?0x279b6:0x28344,0x70000,0,10000));
            check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
            check(uc_reg_read(u,UC_X86_REG_AX,&ax));
            if(ip || sp!=0xf004) abort();
            if(operation<4) slicks_menu_assign_profile(expected,profiles,operation,(short)choice);
            else if((short)ax!=slicks_menu_step_profile(initial,profiles,4,(short)choice,operation==4?-1:1)) abort();
            for(unsigned i=0;i<4;++i)
                if((short)readword(u,0x3cbf0+0x44c+2*i)!=expected[i]) abort();
            ++cases;
        }
    }
    printf("DOS profile menu: %u original assignment/navigation comparisons pass, including sharing, duplicate selections and endpoints\n",cases);
}
static int verify_choices(uc_engine *u)
{
    uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp,ax,ip;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss));
    for(unsigned mode=0;mode<65536;++mode) {
        word(u,0x3cbf0+0x92,mode); sp=0xf000;
        word(u,0x8f000,0); word(u,0x8f002,0x7000);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x198b6,0x70000,0,100));
        check(uc_reg_read(u,UC_X86_REG_AX,&ax));
        if(ax!=slicks_profiles_override((short)mode)) return 1;
    }
    const short counts[]={-32768,-1,0,1,4,127,128,129,255,256,257,300};
    unsigned cases=0;
    for(unsigned pattern=0;pattern<5;++pattern)
    for(unsigned c=0;c<sizeof counts/sizeof counts[0];++c)
    for(unsigned sample=0;sample<128;++sample) {
        unsigned char weights[300];
        for(unsigned i=0;i<300;++i)
            weights[i]=pattern==0?0:pattern==1?255:pattern==2?1:
                pattern==3?(i==sample?255:0):(unsigned char)(i*37+sample);
        unsigned long seed=(0x9e3779b9UL*sample+pattern)&0xffffffffUL;
        word(u,0x3cbf0+0x2aaa,seed); word(u,0x3cbf0+0x2aac,seed>>16);
        word(u,0x3cbf0+0x4c6e,(unsigned short)counts[c]);
        check(uc_mem_write(u,0x3cbf0+0x1b4,weights,sizeof weights));
        sp=0xf000; word(u,0x8f000,0); word(u,0x8f002,0x7000);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x19967,0x70000,0,10000));
        check(uc_reg_read(u,UC_X86_REG_AX,&ax)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        unsigned char expected=slicks_choose_profile_vehicle(weights,counts[c],&seed);
        unsigned long actual_seed=readword(u,0x3cbf0+0x2aaa)|
            ((unsigned long)readword(u,0x3cbf0+0x2aac)<<16);
        if(ip || sp!=0xf004 || (unsigned char)ax!=expected || seed!=actual_seed) {
            fprintf(stderr,"Vehicle chooser mismatch pattern=%u count=%d sample=%u native=%u DOS=%u\n",
                pattern,counts[c],sample,expected,(unsigned char)ax); return 1;
        }
        ++cases;
    }
    printf("DOS setup choices: all 65536 game-type values and %u weighted choices/RNG states match\n",cases);
    return 0;
}
static unsigned char override(void *p)
{ struct Calls *c=p; return c->mode[c->override++]; }
static unsigned char choose(void *p)
{ struct Calls *c=p; return (unsigned char)(2+c->random++); }
static void apply(void *p,unsigned driver,signed char vehicle)
{ struct Calls *c=p; c->driver[c->applied]=driver; c->vehicle[c->applied++]=(unsigned char)vehicle; }
static void boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)size;
    uint16_t ss,sp,cs,ip,ax=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=16U*ss+sp;
    if(address==0x198b6) ax=override(p);
    else if(address==0x19967) ax=choose(p);
    else apply(p,readword(u,stack+4),(signed char)readword(u,stack+6));
    ip=readword(u,stack); cs=readword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_CS,&cs));
    check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    unsigned char runtime[300000];
    FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t size=fread(runtime,1,sizeof runtime,f); int error=ferror(f); fclose(f);
    if(error || size<200000 || size==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,size));
    if(verify_choices(u)) return 1;
    /* The composed test substitutes calls that just executed for real.
     * Use a fresh engine so cached translated blocks cannot bypass hooks. */
    check(uc_close(u)); check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,size));
    struct Calls dos;
    const unsigned boundaries[]={0x198b6,0x19967,0x1ccfc};
    for(unsigned i=0;i<3;++i) { uc_hook h; check(uc_hook_add(u,&h,UC_HOOK_CODE,boundary,&dos,boundaries[i],boundaries[i])); }
    const short selections[]={-32768,-2,-1,0,1,2,3,4,32767};
    const signed char choose_modes[]={-128,-1,0,1,127};
    const short vehicle_counts[]={4,0,-1,32767};
    const unsigned char vehicles[]={0,3,4,5,127,128,255};
    unsigned cases=0;
    for(short count=1;count<=4;++count)
    for(unsigned flags=0;flags<256;++flags)
    for(unsigned variant=0;variant<27;++variant)
    for(unsigned mode=0;mode<5;++mode)
    for(unsigned suppress_mode=0;suppress_mode<2;++suppress_mode) {
        struct SlicksSetupProfile profiles[4];
        unsigned char fallback[4][6];
        struct SlicksProfileSelection state,actual;
        memset(&state,0,sizeof state); memset(&actual,0,sizeof actual);
        memset(&dos,0,sizeof dos);
        for(unsigned i=0;i<4;++i) {
            profiles[i].flags=(unsigned char)(flags+i*2);
            profiles[i].vehicle=vehicles[(variant+i)%7];
            for(unsigned k=0;k<6;++k) {
                profiles[i].colours[k]=(unsigned char)(i*6+k+31);
                fallback[i][k]=(unsigned char)(i*6+k+131);
            }
            state.selected[i]=selections[(variant+i)%9];
            state.vehicle[i]=(signed char)(variant%3==0?255:variant+i);
            memset(state.colours[i],0xa5,6);
            state.order[i]=99; state.participation[i]=99;
            dos.mode[i]=(variant>=18 || (variant>=9 && ((variant+i)&1))) ? 255 : 0;
            word(u,0x3cbf0+0x44c+2*i,(unsigned short)state.selected[i]);
            check(uc_mem_write(u,0x3cbf0+0x3ede +i,&profiles[i].flags,1));
            check(uc_mem_write(u,0x3cbf0+0x3fa6+i,&profiles[i].vehicle,1));
            check(uc_mem_write(u,0x3cbf0+0x1db+6*i,profiles[i].colours,6));
        }
        short override_count=(short)(variant%7)-1;
        short vehicle_count=vehicle_counts[variant%4];
        unsigned char suppress=suppress_mode?255:0;
        word(u,0x3cbf0+0x36a8,count); word(u,0x3cbf0+0xf1a,(unsigned short)override_count);
        word(u,0x3cbf0+0x4c6e,(unsigned short)vehicle_count);
        check(uc_mem_write(u,0x3cbf0+0x433,fallback,sizeof fallback));
        check(uc_mem_write(u,0x3cbf0+0x4bc2,state.vehicle,4));
        check(uc_mem_write(u,0x3cbf0+0x4bc6,state.participation,4));
        check(uc_mem_write(u,0x3cbf0+0x310c,state.colours,24));
        check(uc_mem_write(u,0x3cbf0+0x4bf2,state.order,4));
        struct Calls native=dos;
        unsigned char appearance[4][6];
        memcpy(appearance,state.colours,sizeof appearance);
        if(variant<9 || variant>=18)
            slicks_select_profile_colours(appearance,state.selected,profiles,count,
                fallback,variant>=18?5:0,override_count);
        struct SlicksProfileSetupOps ops={override,choose,apply,&native};
        slicks_select_profiles(&state,profiles,count,fallback,override_count,
            vehicle_count,suppress,choose_modes[mode],&ops);
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
        word(u,0x8f000,0); word(u,0x8f002,0x7000);
        word(u,0x8f004,suppress); word(u,0x8f006,(unsigned char)choose_modes[mode]);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x2bb70,0x70000,0,10000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        for(unsigned i=0;i<4;++i) actual.selected[i]=(short)readword(u,0x3cbf0+0x44c+2*i);
        check(uc_mem_read(u,0x3cbf0+0x4bc2,actual.vehicle,4));
        check(uc_mem_read(u,0x3cbf0+0x4bc6,actual.participation,4));
        check(uc_mem_read(u,0x3cbf0+0x310c,actual.colours,24));
        if((variant<9 || variant>=18) && memcmp(appearance,actual.colours,sizeof appearance)) {
            fputs("Rendering-only profile colour selection differs from original\n",stderr); return 1;
        }
        check(uc_mem_read(u,0x3cbf0+0x4bf2,actual.order,4));
        check(uc_mem_read(u,0x3cbf0+0x4c16,&actual.count,1));
        if(ip || sp!=0xf004 || memcmp(&state,&actual,sizeof state) || memcmp(&dos,&native,sizeof dos)) {
            fprintf(stderr,"Profile setup mismatch flags=%u variant=%u mode=%d suppress=%u\n",flags,variant,choose_modes[mode],suppress); return 1;
        }
        ++cases;
    }
    verify_menu(u);
    verify_menu_keys(u);
    verify_menu_modals(u);
    verify_menu_draw(u);
    verify_editor_draw(u);
    check(uc_close(u));
    printf("DOS profile setup: %u complete four-player state/call-order comparisons pass\n",cases);
    return 0;
}
