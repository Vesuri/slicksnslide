#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "host_archive.h"
#include "../src/ui/menu_bitmap.h"
static unsigned char source[70000]; static unsigned long cursor,length;
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned a,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(u,a,b,2)); }
static unsigned get(uc_engine *u,unsigned a)
{ unsigned char b[2]; check(uc_mem_read(u,a,b,2)); return b[0]|b[1]<<8; }
static void io(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size; (void)context; uint16_t ss,sp,cs,ip,ax=0,dx=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp)); unsigned stack=ss*16U+sp;
    if(address==0x12d8a || address==0x12d6e) { if(cursor>=length) abort(); ax=source[cursor++]; }
    else if(address==0x1221d) { ax=cursor; dx=cursor>>16; }
    else if(address==0x1219b) {
        unsigned long offset=get(u,stack+8)|((unsigned long)get(u,stack+10)<<16);
        unsigned origin=get(u,stack+12);
        if(origin>1) abort(); cursor=(origin?cursor:0)+offset; if(cursor>length) abort(); word(u,0x70000,0);
    } else if(address==0x12078) {
        unsigned n=get(u,stack+8)*get(u,stack+10),dest=get(u,stack+4)+16U*get(u,stack+6);
        if(n>length-cursor) abort(); check(uc_mem_write(u,dest,source+cursor,n)); cursor+=n; ax=get(u,stack+10);
    } else if(address==0x13b07) { if(get(u,stack+4)>65000) abort(); dx=0x5000; }
    else if(address==0x36243) { fputs("Original BMP loader rejected fixture\n",stderr); exit(1); }
    ip=get(u,stack); cs=get(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f); fclose(f); if(bytes<200000 || bytes==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,bytes));
    const unsigned addresses[]={0x12d8a,0x12d6e,0x1221d,0x1219b,0x12078,0x13b07,0x119c2,0x36243};
    for(unsigned i=0;i<8;++i) { uc_hook hook; check(uc_hook_add(u,&hook,UC_HOOK_CODE,io,0,addresses[i],addresses[i])); }
    const char *names[]={"players.bmp","loading.bmp","end1.bmp","end2.bmp"};
    for(unsigned test=0;test<4;++test) {
        long loaded=host_archive_load("ref/SLICKS.000",names[test],source,sizeof source); if(loaded<0) return 2;
        length=(unsigned long)loaded; cursor=0;
        unsigned char pixels[64000],palette[768],original[65000]; unsigned width,height; unsigned long consumed;
        if(slicks_decode_menu_bitmap(source,length,pixels,palette,&width,&height,&consumed)) return 1;
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,ip,ax,dx;
        word(u,0x70000,0); word(u,0x8f000,0); word(u,0x8f002,0x9000);
        word(u,0x8f004,0); word(u,0x8f006,0x7000); word(u,0x8f008,1);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x2ec59,0x90000,0,10000000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        check(uc_reg_read(u,UC_X86_REG_AX,&ax)); check(uc_reg_read(u,UC_X86_REG_DX,&dx));
        check(uc_mem_read(u,0x50000,original,sizeof original));
        if(ip || sp!=0xf004 || ax || dx!=0x5000 || original[0]*4U!=width || original[1]!=height || cursor!=consumed ||
            memcmp(original+width*height+6,palette,768)) { fprintf(stderr,"BMP header/palette/stream mismatch %s DOS=%lu native=%lu\n",names[test],cursor,consumed); return 1; }
        for(unsigned y=0;y<height;++y) for(unsigned x=0;x<width;++x)
            if(pixels[y*width+x]!=original[2+(x&3)*(width/4)*height+y*(width/4)+x/4]) { fprintf(stderr,"BMP pixel mismatch %s %u,%u\n",names[test],x,y); return 1; }
        printf("Original %s: %ux%u pixels and all 768 palette bytes match; %lu bytes consumed\n",names[test],width,height,consumed);
        /* Every header/palette prefix, plus sampled pixel-stream truncations
         * and the exact final-byte boundary. Failed output is disposable. */
        unsigned long offset=slicks_bmp_word32(source+10);
        for(unsigned long cut=0;cut<offset;++cut)
            if(!slicks_decode_menu_bitmap(source,cut,pixels,palette,&width,&height,0)) abort();
        for(unsigned long cut=offset;cut<consumed;cut+=257)
            if(!slicks_decode_menu_bitmap(source,cut,pixels,palette,&width,&height,0)) abort();
        if(!slicks_decode_menu_bitmap(source,consumed-1,pixels,palette,&width,&height,0)) abort();
    }
    puts("Menu BMP truncation guards: all header/palette prefixes, sampled payload cuts and final-byte cuts rejected");
    check(uc_close(u)); return 0;
}
