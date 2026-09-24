#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/ui/change_cars_dialog.h"
static void check(uc_err e)
{ if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned address,unsigned value)
{ unsigned char b[2]={value,value>>8}; check(uc_mem_write(u,address,b,2)); }
static unsigned long readseed(uc_engine *u)
{
    unsigned char b[4]; check(uc_mem_read(u,0x3cbf0+0x2aaa,b,4));
    return b[0]|((unsigned long)b[1]<<8)|((unsigned long)b[2]<<16)|((unsigned long)b[3]<<24);
}
static void verify_prepare(uc_engine *u)
{
    unsigned cases=0;
    for(unsigned roles=0;roles<81;++roles)
    for(unsigned selector=0;selector<256;++selector)
    for(int force=-1;force<=1;++force) {
        struct SlicksSetupProfile profiles[4]={{0}};
        struct SlicksProfileSelection players={0};
        unsigned role_digits=roles;
        unsigned char weights[10];
        for(unsigned i=0;i<10;++i) weights[i]=(unsigned char)(selector*7+i*23);
        check(uc_mem_write(u,0x3cbf0+0x1b4,weights,10));
        for(unsigned i=0;i<4;++i) {
            players.selected[i]=(short)(i^1);
            players.participation[i]=(signed char)(role_digits%3)-1; role_digits/=3;
            players.vehicle[i]=(signed char)(i+3);
            profiles[i^1].vehicle=(unsigned char)(selector+i);
            word(u,0x3cbf0+0x44c+2*i,players.selected[i]);
        }
        for(unsigned i=0;i<4;++i)
            check(uc_mem_write(u,0x3cbf0+0x3fa6+i,&profiles[i].vehicle,1));
        check(uc_mem_write(u,0x3cbf0+0x4bc6,players.participation,4));
        check(uc_mem_write(u,0x3cbf0+0x4bc2,players.vehicle,4));
        word(u,0x3cbf0+0x4c6e,10);
        unsigned long seed=(0x9e3779b9UL*(selector+roles)+force)&0xffffffffUL;
        word(u,0x3cbf0+0x2aaa,seed); word(u,0x3cbf0+0x2aac,seed>>16);
        unsigned char show=(unsigned char)force;
        check(uc_mem_write(u,0x8efff,&show,1));
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,bp=0xf000,sp=0xe000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x2480d,0x24859,0,5000));
        signed char actual[4]; check(uc_mem_read(u,0x3cbf0+0x4bc2,actual,4));
        check(uc_mem_read(u,0x8efff,&show,1));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        signed char expected=slicks_change_cars_prepare(&players,profiles,10,weights,&seed,(signed char)force);
        if(ip!=0xafe9 || sp!=0xe000 || expected!=(signed char)show ||
           memcmp(actual,players.vehicle,4) || seed!=readseed(u)) {
            fprintf(stderr,"Change Cars preparation mismatch roles=%u selector=%u force=%d\n",roles,selector,force);
            exit(1);
        }
        ++cases;
    }
    printf("Original Change Cars opening: %u profile/role/visibility/RNG cases pass\n",cases);
}
struct DrawCall { short kind,args[7]; };
struct DrawTrace { struct DrawCall calls[12]; unsigned count; };
static struct DrawCall *record(void *p,short kind)
{
    struct DrawTrace *t=p; if(t->count>=12) abort();
    struct DrawCall *c=&t->calls[t->count++]; c->kind=kind; return c;
}
static void restore(void *p,short x,short y,short sx,short sy,short w,short h)
{ short a[]={x,y,sx,sy,w,h}; memcpy(record(p,0)->args,a,sizeof a); }
static void bevel(void *p,short x,short y,short w,short h,unsigned char r,unsigned char g,unsigned char b)
{ short a[]={x,y,w,h,r,g,b}; memcpy(record(p,1)->args,a,sizeof a); }
static void sprite(void *p,signed char vehicle,short x,short y)
{ short a[]={vehicle,x,y}; memcpy(record(p,2)->args,a,sizeof a); }
/* Observe known rendering calls before execution; their caller pops args.
 * Pixel rendering is tested separately by the shared graphics primitives. */
static void drawing(uc_engine *u,uint64_t address,uint32_t size,void *unused)
{
    (void)size;
    if(address==0x249c7 || address==0x249fc || address==0x24a3a) {
        uint16_t sp; check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        unsigned char bytes[18]; short args[9];
        check(uc_mem_read(u,0x80000+sp,bytes,sizeof bytes));
        for(unsigned i=0;i<9;++i) args[i]=(short)(bytes[i*2]|bytes[i*2+1]<<8);
        if(address==0x249c7) restore(unused,args[0],args[1],args[2],args[3],args[4],args[5]);
        else if(address==0x249fc) bevel(unused,args[0],args[1],args[2],args[3],args[4],args[5],args[6]);
        else sprite(unused,(signed char)((args[2]-0x400)/16),args[0],args[1]);
        uint16_t ip=(uint16_t)(address+5-0x19870);
        check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    }
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb");
    if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f);
    if(n<200000 || n==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n));
    verify_prepare(u);
    struct DrawTrace dos;
    uc_hook hook; check(uc_hook_add(u,&hook,UC_HOOK_CODE,drawing,&dos,1,0));
    unsigned cases=0,mappings=0;
    for(unsigned mask=1;mask<16;++mask) {
        signed char roles[4]; unsigned char count=0;
        for(unsigned i=0;i<4;++i) { roles[i]=(mask&(1U<<i))?(i&1?-1:1):0; if(roles[i]) ++count; }
        check(uc_mem_write(u,0x3cbf0+0x4bc6,roles,4));
        check(uc_mem_write(u,0x3cbf0+0x4c16,&count,1));
        unsigned char vehicle_count[2]={10,0};
        check(uc_mem_write(u,0x3cbf0+0x4c6e,vehicle_count,2));
        for(unsigned row=0;row<count;++row) {
            uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,bp=0xf000,sp=0xe000;
            check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
            check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
            check(uc_reg_write(u,UC_X86_REG_SP,&sp));
            uint16_t x=210,y=71;
            check(uc_reg_write(u,UC_X86_REG_SI,&x)); check(uc_reg_write(u,UC_X86_REG_DI,&y));
            signed char pictured[4]={2,4,6,8};
            check(uc_mem_write(u,0x3cbf0+0x4bc2,pictured,4));
            for(unsigned i=0;i<10;++i) {
                word(u,0x3cbf0+0x4e44+4*i,0x400+16*i);
                word(u,0x3cbf0+0x4e46+4*i,0x6000);
            }
            unsigned char locals[32]={0}; locals[0x12]=row;
            check(uc_mem_write(u,0x8efe0,locals,32));
            memset(&dos,0,sizeof dos);
            check(uc_emu_start(u,0x24992,0x24a51,0,1000));
            unsigned char driver; check(uc_mem_read(u,0x8eff1,&driver,1));
            if(driver!=slicks_change_cars_driver(roles,row)) return 1;
            struct DrawTrace native={0};
            struct SlicksChangeCarsDrawOps ops={restore,bevel,sprite,&native};
            struct SlicksChangeCarsDialog dialog={(signed char)row,0};
            if(slicks_change_cars_draw(&dialog,roles,pictured,x,y,&ops)!=driver ||
               memcmp(&dos,&native,sizeof dos)) {
                fprintf(stderr,"Change Cars draw commands mismatch mask=%u row=%u\n",mask,row); return 1;
            }
            ++mappings;
            for(unsigned vehicle=0;vehicle<10;++vehicle)
            for(unsigned key=0;key<256;++key) {
                signed char cars[4]={2,4,6,8},actual[4]; cars[driver]=(signed char)vehicle;
                struct SlicksChangeCarsDialog d={(signed char)row,0};
                check(uc_mem_write(u,0x3cbf0+0x4bc2,cars,4));
                memset(locals,0,32); locals[0x12]=row; locals[0x11]=driver;
                check(uc_mem_write(u,0x8efe0,locals,32));
                uint16_t ax=(uint16_t)(int16_t)(int8_t)key,ip;
                check(uc_reg_write(u,UC_X86_REG_AX,&ax));
                check(uc_emu_start(u,0x24a69,0x24af0,0,150));
                check(uc_reg_read(u,UC_X86_REG_IP,&ip));
                check(uc_mem_read(u,0x8efe0,locals,32));
                check(uc_mem_read(u,0x3cbf0+0x4bc2,actual,4));
                if(slicks_change_cars_key(&d,roles,cars,10,(unsigned char)key) ||
                   ip!=0xb280 || locals[0x12]!=(unsigned char)d.row || locals[0x13]!=d.done ||
                   memcmp(cars,actual,4)) {
                    fprintf(stderr,"Change Cars mismatch mask=%u row=%u vehicle=%u key=%u ip=%x\n",
                        mask,row,vehicle,key,ip); return 1;
                }
                ++cases;
            }
        }
    }
    check(uc_close(u));
    printf("Original Change Cars: %u complete draw-command/active-row cases and %u key/vehicle cases pass\n",mappings,cases);
    return 0;
}
