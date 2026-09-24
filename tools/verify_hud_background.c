#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include <unicorn/m68k.h>
#include "host_archive.h"
#include "../src/ui/hud_background.h"

static unsigned char source[8192];
static unsigned cursor,length,allocated;
static unsigned char dos_pixels[64000];
static unsigned mask;
static unsigned bridge;
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *uc,unsigned a,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(uc,a,b,2)); }
static unsigned readword(uc_engine *uc,unsigned a)
{ unsigned char b[2]; check(uc_mem_read(uc,a,b,2)); return b[0]|b[1]<<8; }
static void icon_port(uc_engine *uc,uint32_t port,int size,uint32_t value,void *user)
{
    (void)uc; (void)user;
    if(port==0x3c4 && size==1 && value==2) return;
    if(port!=0x3c5 || size!=1) abort();
    mask=value&15;
}
static void icon_pixel(uc_engine *uc,uc_mem_type type,uint64_t address,int size,int64_t value,void *user)
{
    (void)uc; (void)type; (void)user;
    if(size!=1) abort();
    unsigned at=(unsigned)address-0xa0000;
    for(unsigned p=0;p<4;++p) if(mask&(1U<<p)) {
        unsigned x=at%100*4+p,y=at/100;
        if(x>=320 || y>=200) abort();
        dos_pixels[y*320+x]=(unsigned char)value;
    }
}
static int icon_pixels(uc_engine *x86,uc_engine *m68k,const unsigned char *pixels,
    unsigned width,unsigned height,const char *name)
{
    static unsigned char surface[64000],actual[64000];
    const unsigned locations[]={0,91,151,211,271,304};
    const int regs[]={UC_M68K_REG_D0,UC_M68K_REG_D1,UC_M68K_REG_D2,UC_M68K_REG_D3,
        UC_M68K_REG_D4,UC_M68K_REG_D5,UC_M68K_REG_D6,UC_M68K_REG_D7,
        UC_M68K_REG_A0,UC_M68K_REG_A1,UC_M68K_REG_A2,UC_M68K_REG_A3,
        UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6};
    for(unsigned background=0;background<3;++background)
    for(unsigned location=0;location<6;++location) {
        unsigned x=locations[location],y=192;
        for(unsigned i=0;i<sizeof surface;++i)
            surface[i]=background==0?0:background==1?255:(unsigned char)(i*73+19);
        memcpy(dos_pixels,surface,sizeof surface);
        word(x86,0x3cbf0+0x1d7b,100);
        uint16_t cs=0x3aa0,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
        word(x86,0x8f000,0); word(x86,0x8f002,0x7000);
        word(x86,0x8f004,x); word(x86,0x8f006,y);
        word(x86,0x8f008,0); word(x86,0x8f00a,0x5000); word(x86,0x8f00c,0);
        check(uc_reg_write(x86,UC_X86_REG_CS,&cs)); check(uc_reg_write(x86,UC_X86_REG_DS,&ds));
        check(uc_reg_write(x86,UC_X86_REG_SS,&ss)); check(uc_reg_write(x86,UC_X86_REG_SP,&sp));
        mask=0; check(uc_emu_start(x86,0x3aa7c,0x70000,0,100000));
        check(uc_reg_read(x86,UC_X86_REG_IP,&ip)); check(uc_reg_read(x86,UC_X86_REG_SP,&sp));
        if(ip || sp!=0xf004) return 1;
        check(uc_mem_write(m68k,0x50000,pixels,width*height));
        check(uc_mem_write(m68k,0x100000,surface,sizeof surface));
        uint32_t values[]={0xabcd0000|x,0xbcde0000|y,0xcdef0000|width,0xdefa0000|height,
            0x44445555,0x55556666,0x66667777,0x77778888,0x100000,0x50000,
            0x22223333,0x33334444,0x44445555,0x55556666,0x66667777};
        uint32_t stack=0x300000,sr=0,pc;
        unsigned char ret[]={0,0x38,0,0};
        check(uc_reg_write(m68k,UC_M68K_REG_SR,&sr)); check(uc_reg_write(m68k,UC_M68K_REG_A7,&stack));
        check(uc_mem_write(m68k,stack,ret,4));
        for(unsigned r=0;r<15;++r) check(uc_reg_write(m68k,regs[r],&values[r]));
        check(uc_emu_start(m68k,0,0x380000,0,100000));
        check(uc_reg_read(m68k,UC_M68K_REG_PC,&pc)); check(uc_reg_read(m68k,UC_M68K_REG_A7,&stack));
        if(pc!=0x380000 || stack!=0x300004) return 1;
        for(unsigned r=0;r<15;++r) { uint32_t value; check(uc_reg_read(m68k,regs[r],&value)); if(value!=values[r]) return 1; }
        check(uc_mem_read(m68k,0x100000,actual,sizeof actual));
        if(memcmp(actual,dos_pixels,sizeof actual)) {
            fprintf(stderr,"Transparent icon mismatch %s at x=%u background=%u\n",name,x,background); return 1;
        }
        check(uc_mem_write(m68k,0x100000,surface,sizeof surface));
        uint32_t arguments[]={0x380000,0x100000,0x50000,x,y,width,height};
        unsigned char argument_bytes[28];
        for(unsigned i=0;i<7;++i) for(unsigned b=0;b<4;++b)
            argument_bytes[i*4+b]=(unsigned char)(arguments[i]>>(24-b*8));
        stack=0x300000;
        check(uc_reg_write(m68k,UC_M68K_REG_A7,&stack));
        check(uc_mem_write(m68k,stack,argument_bytes,sizeof argument_bytes));
        for(unsigned r=0;r<15;++r) check(uc_reg_write(m68k,regs[r],&values[r]));
        check(uc_emu_start(m68k,bridge,0x380000,0,100000));
        check(uc_reg_read(m68k,UC_M68K_REG_A7,&stack));
        if(stack!=0x300004) return 1;
        for(unsigned r=2;r<15;++r) if(r!=8 && r!=9) {
            uint32_t value; check(uc_reg_read(m68k,regs[r],&value)); if(value!=values[r]) return 1;
        }
        check(uc_mem_read(m68k,0x100000,actual,sizeof actual));
        if(memcmp(actual,dos_pixels,sizeof actual)) { fputs("Icon GCC bridge mismatch\n",stderr); return 1; }
    }
    return 0;
}
static void boundary(uc_engine *uc,uint64_t address,uint32_t size,void *user)
{
    (void)size; (void)user;
    uint16_t ss,sp,cs,ip,ax=0,dx=0;
    check(uc_reg_read(uc,UC_X86_REG_SS,&ss)); check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    if(address==0x373a7) ax=1; /* Open the supplied archive member. */
    else if(address==0x12d8a) {
        if(cursor>=length) { fputs("Original decoder read past asset\n",stderr); exit(1); }
        ax=source[cursor++];
    } else if(address==0x13b07) {
        allocated=readword(uc,stack+4);
        if(allocated>8192) abort();
        dx=0x5000;
    } else if(address==0x1221d) ax=cursor;
    else if(address==0x36243) { fputs("Original decoder error\n",stderr); exit(1); }
    /* Allocation, file open/read/tell/close and scheduler boundaries only.
     * The complete original header parser/RLE/planar layout code executes. */
    check(uc_reg_write(uc,UC_X86_REG_AX,&ax)); check(uc_reg_write(uc,UC_X86_REG_DX,&dx));
    ip=readword(uc,stack); cs=readword(uc,stack+2); sp+=4;
    check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_IP,&ip));
    check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
}

int main(void)
{
    static unsigned char runtime[300000],native[5120],planar[8192];
    FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t runtime_size=fread(runtime,1,sizeof runtime,f);
    if(ferror(f) || !feof(f)) return 2; fclose(f);
    uc_engine *uc; check(uc_open(UC_ARCH_X86,UC_MODE_16,&uc));
    check(uc_mem_map(uc,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(uc,0x10100,runtime,runtime_size));
    uc_engine *m68k;
    check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&m68k));
    check(uc_ctl_set_cpu_model(m68k,UC_CPU_M68K_M68020));
    check(uc_mem_map(m68k,0,0x400000,UC_PROT_ALL));
    unsigned char code[4096];
    f=fopen("build/hud_icon_test.bin","rb"); if(!f) return 2;
    size_t code_size=fread(code,1,sizeof code,f); if(ferror(f) || !feof(f)) return 2; fclose(f);
    if(code_size<4) return 2;
    for(unsigned i=0;i<4;++i) bridge=(bridge<<8)|code[code_size-4+i];
    check(uc_mem_write(m68k,0,code,code_size));
    uc_hook ports,writes;
    check(uc_hook_add(uc,&ports,UC_HOOK_INSN,icon_port,NULL,1,0,UC_X86_INS_OUT));
    check(uc_hook_add(uc,&writes,UC_HOOK_MEM_WRITE,icon_pixel,NULL,0xa0000,0xaffff));
    const unsigned addresses[]={0x373a7,0x12d8a,0x13b07,0x1221d,0x119c2,0x317af,0x36243};
    for(unsigned i=0;i<sizeof addresses/sizeof *addresses;++i) {
        uc_hook h; check(uc_hook_add(uc,&h,UC_HOOK_CODE,boundary,NULL,addresses[i],addresses[i]));
    }
    const char *names[]={"alamenu.@I","vir1.@I","vir2.@I","vir3.@I","vir4.@I",
        "vir_fuel.@I","vir5.@I","vir6.@I","vir7.@I","vir8.@I","vir9.@I",
        "vir10.@I","vir11.@I","vir12.@I"};
    for(unsigned asset=0;asset<sizeof names/sizeof names[0];++asset) {
    long bytes=host_archive_load("ref/SLICKS.000",names[asset],source,sizeof source);
    if(bytes<0) return 2; length=(unsigned)bytes; cursor=allocated=0;
    unsigned short width,height;
    if(slicks_decode_hud_image(source,length,native,sizeof native,&width,&height)) return 1;
    unsigned stride=(width+3)/4,plane_size=stride*height;
    uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,ip,ax;
    word(uc,0x8f000,0); word(uc,0x8f002,0x7000);
    word(uc,0x8f004,0); word(uc,0x8f006,0x6000); /* filename */
    word(uc,0x8f008,0x20); word(uc,0x8f00a,0x6000); /* output pointer */
    word(uc,0x8f00c,3); /* same allocation flags as startup */
    check(uc_reg_write(uc,UC_X86_REG_CS,&cs)); check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
    check(uc_reg_write(uc,UC_X86_REG_SS,&ss)); check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
    check(uc_emu_start(uc,0x2e51a,0x70000,0,1000000));
    check(uc_reg_read(uc,UC_X86_REG_IP,&ip)); check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
    check(uc_reg_read(uc,UC_X86_REG_AX,&ax));
    if(ip || sp!=0xf004 || ax || cursor!=length || allocated!=plane_size*4+3 ||
       readword(uc,0x60020)!=0 || readword(uc,0x60022)!=0x5000) {
        fprintf(stderr,"Original decode status ip=%x sp=%x ax=%x read=%u/%u allocation=%u\n",ip,sp,ax,cursor,length,allocated);
        return 1;
    }
    check(uc_mem_read(uc,0x50000,planar,allocated));
    if(planar[0]!=stride || planar[1]!=height || planar[2+plane_size*4]!=stride*4-width) {
        fprintf(stderr,"HUD %s metadata width=%u/%u height=%u/%u tail=%u\n",names[asset],planar[0],stride,planar[1],height,planar[2+plane_size*4]); return 1;
    }
    for(unsigned y=0;y<height;++y) for(unsigned x=0;x<width;++x)
        if(native[y*width+x]!=planar[2+(x&3)*plane_size+y*stride+x/4]) {
            fprintf(stderr,"HUD asset %s mismatch at %u,%u\n",names[asset],x,y); return 1;
        }
    if(asset && icon_pixels(uc,m68k,native,width,height,names[asset])) return 1;
    for(unsigned cut=0;cut<length;++cut)
        if(!slicks_decode_hud_image(source,cut,native,sizeof native,&width,&height)) return 1;
    printf("Original %s decoder: all %u pixels match; %u bytes consumed; every truncated input rejected\n",names[asset],width*height,length);
    }
    uc_close(uc);
    uc_close(m68k);
    puts("Original transparent blitter vs 68020: 234 register-entry plus 234 GCC-bridge full-surface comparisons pass, including background and preserved registers");
    return 0;
}
