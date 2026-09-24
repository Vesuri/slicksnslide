#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include <unicorn/m68k.h>
#include "host_archive.h"
#include "../src/ui/menu_icon.h"
static unsigned char source[65536],dos[64000]; static unsigned length,cursor,allocations,mask;
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned at,unsigned value)
{ unsigned char b[2]={value,value>>8}; check(uc_mem_write(u,at,b,2)); }
static unsigned getword(uc_engine *u,unsigned at)
{ unsigned char b[2]; check(uc_mem_read(u,at,b,2)); return b[0]|b[1]<<8; }
static void services(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size; (void)context; uint16_t ss,sp,ip,cs,ax=0,dx=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp)); unsigned stack=ss*16U+sp;
    if(address==0x373a7) dx=0x9000;
    else if(address==0x12d8a) { if(cursor>=length) abort(); ax=source[cursor++]; }
    else if(address==0x1221d) ax=cursor;
    else if(address==0x13b07) { if(++allocations>2 || getword(u,stack+4)>32768) abort(); dx=allocations==1?0x5000:0x7000; }
    else if(address==0x36243) { fputs("Original icon error\n",stderr); exit(1); }
    ip=getword(u,stack); cs=getword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void port(uc_engine *u,uint32_t address,int size,uint32_t value,void *context)
{
    (void)u; (void)context;
    if(address==0x3c4 && size==1 && value==2) return;
    if(address==0x3c4 && size==2 && (value&255)==2) { mask=(value>>8)&15; return; }
    if(address==0x3c5 && size==1) { mask=value&15; return; } abort();
}
static void pixel(uc_engine *u,uc_mem_type type,uint64_t address,int size,int64_t value,void *context)
{
    (void)u; (void)type; (void)context;
    if(size!=1) abort(); unsigned at=(unsigned)(address-0xa0000);
    for(unsigned p=0;p<4;++p) if(mask&(1U<<p)) {
        unsigned x=(at%100)*4+p,y=at/100; if(x>=320 || y>=200) abort();
        dos[y*320+x]=(unsigned char)value;
    }
}
static void start(uc_engine *u,unsigned address)
{
    uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
    word(u,0x8f000,0); word(u,0x8f002,0x9000);
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    check(uc_emu_start(u,address,0x90000,0,10000000));
    check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp)); if(ip || sp!=0xf004) abort();
}
int main(void)
{
    unsigned char runtime[300000],code[4096]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f); fclose(f); if(bytes<200000 || bytes==sizeof runtime) return 2;
    uc_engine *u,*m; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,bytes));
    const unsigned addresses[]={0x373a7,0x12d8a,0x1221d,0x13b07,0x119c2,0x139fd,0x36243};
    for(unsigned i=0;i<7;++i) { uc_hook h; check(uc_hook_add(u,&h,UC_HOOK_CODE,services,0,addresses[i],addresses[i])); }
    uc_hook h; check(uc_hook_add(u,&h,UC_HOOK_INSN,port,0,1,0,UC_X86_INS_OUT));
    check(uc_hook_add(u,&h,UC_HOOK_MEM_WRITE,pixel,0,0xa0000,0xaffff));
    f=fopen("build/hud_icon_test.bin","rb"); if(!f) return 2;
    bytes=fread(code,1,sizeof code,f); fclose(f); if(!bytes || bytes==sizeof code) return 2;
    check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&m)); check(uc_ctl_set_cpu_model(m,UC_CPU_M68K_M68020));
    check(uc_mem_map(m,0,0x400000,UC_PROT_ALL)); check(uc_mem_write(m,0,code,bytes));
    const char *names[]={"computer.@16","carimage16","auto01.@16","auto02.@16","auto03.@16","auto04.@16",
        "auto05.@16","auto06.@16","auto07.@16","auto08.@16","auto09.@16",
        "keys_m1.@16","keys_m2.@16","keys_m3.@16","keys_m4.@16","keys_m5.@16"};
    unsigned cases=0;
    for(unsigned asset=0;asset<sizeof names/sizeof names[0];++asset) for(unsigned pattern=0;pattern<3;++pattern) {
        long loaded=host_archive_load("ref/SLICKS.000",names[asset],source,sizeof source); if(loaded<0) return 2;
        length=(unsigned)loaded; cursor=allocations=0;
        unsigned char palette[768],pixels[16384],native[64000]; unsigned short width,height;
        for(unsigned i=0;i<768;++i) palette[i]=pattern==0?0:(unsigned char)((i*43+(i>>3)*17+pattern*19)&63);
        if(slicks_decode_menu_icon(source,length,palette,pixels,sizeof pixels,&width,&height)) { fprintf(stderr,"Decode %s failed\n",names[asset]); return 1; }
        check(uc_mem_write(u,0x60000,palette,sizeof palette)); word(u,0x3cbf0+0x71b8,0); word(u,0x3cbf0+0x71ba,0x6000); word(u,0x3cbf0+0x1d7b,100);
        word(u,0x8f004,0); word(u,0x8f006,0x9000); word(u,0x8f008,3); start(u,0x2e0f8);
        if(cursor!=length || allocations!=1) abort();
        for(unsigned draw=0;draw<3;++draw) {
            unsigned x=draw==0?49:draw==1?215:320-width,y=draw==2?200-height:29+16*draw;
            if(x+width>320 || y+height>200) abort();
            for(unsigned i=0;i<64000;++i) dos[i]=(unsigned char)(i*13+pattern+draw*73);
            check(uc_mem_write(m,0x100000,dos,sizeof dos)); check(uc_mem_write(m,0x50000,pixels,width*height));
            /* After first draw change the palette: cached icons keep indices. */
            if(draw==1) { memset(palette,63,sizeof palette); check(uc_mem_write(u,0x60000,palette,sizeof palette)); }
            word(u,0x8f004,x); word(u,0x8f006,y); word(u,0x8f008,0); word(u,0x8f00a,0x5000); word(u,0x8f00c,0); start(u,0x2e2d2);
            if(allocations!=2 || getword(u,0x50000)!=0xb200) abort();
            uint32_t values[]={x,y,width,height,0x100000,0x50000},stack=0x300000,sr=0,pc;
            const int regs[]={UC_M68K_REG_D0,UC_M68K_REG_D1,UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_A0,UC_M68K_REG_A1};
            for(unsigned i=0;i<6;++i) check(uc_reg_write(m,regs[i],&values[i]));
            check(uc_reg_write(m,UC_M68K_REG_SR,&sr)); check(uc_reg_write(m,UC_M68K_REG_A7,&stack)); unsigned char ret[]={0,0x38,0,0}; check(uc_mem_write(m,stack,ret,4));
            check(uc_emu_start(m,0,0x380000,0,1000000)); check(uc_reg_read(m,UC_M68K_REG_PC,&pc));
            check(uc_mem_read(m,0x100000,native,sizeof native));
            if(pc!=0x380000 || memcmp(native,dos,sizeof native)) { fprintf(stderr,"Icon mismatch %s palette=%u draw=%u\n",names[asset],pattern,draw); return 1; }
            ++cases;
        }
        for(unsigned cut=0;cut<length;++cut) {
            memset(pixels,0xa5,sizeof pixels);
            if(slicks_decode_menu_icon(source,cut,palette,pixels,sizeof pixels,&width,&height)!=-1) abort();
            for(unsigned i=0;i<sizeof pixels;++i) if(pixels[i]!=0xa5) abort();
        }
    }
    check(uc_close(u)); check(uc_close(m));
    printf("Original 15-bit menu icons: %u full-frame first/cached-draw comparisons against 68020, all 16 assets, transparency and palette-cache persistence pass; truncated inputs rejected atomically\n",cases);
    return 0;
}
