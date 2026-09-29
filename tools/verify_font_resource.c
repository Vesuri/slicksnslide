#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "host_archive.h"
#include "../src/ui/font_resource.h"
#include "../src/game/race_runtime.c"
static unsigned char source[8192]; static unsigned length,cursor,allocated;
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned a,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(u,a,b,2)); }
static unsigned get(uc_engine *u,unsigned a)
{ unsigned char b[2]; check(uc_mem_read(u,a,b,2)); return b[0]|b[1]<<8; }
static unsigned startup_fonts;
/* Whole-image encoded candidates, including segment aliases and wrapped near
 * calls. This does not rule out computed indirect calls or runtime patches. */
static void verify_font_loader_references(const unsigned char *image,size_t size)
{
    const unsigned calls[]={0x19de0,0x19e07,0x19e1c};
    unsigned mask=0,near=0;
    for(size_t i=0;i+3<size;++i) {
        unsigned offset=image[i]|image[i+1]<<8,segment=image[i+2]|image[i+3]<<8;
        if(offset+16U*segment==0x2fc0c) {
            unsigned address=0x10100U+(unsigned)i,found=0;
            for(unsigned j=0;j<3;++j)if(address==calls[j]+1 && i && image[i-1]==0x9a){
                mask|=1U<<j;found=1;
            }
            if(!found){fprintf(stderr,"Unexpected font-loader far reference %x\n",address);abort();}
        }
    }
    for(size_t i=0;i+2<size;++i)if(image[i]==0xe8){
        int displacement=(int16_t)(image[i+1]|image[i+2]<<8);
        if(((0x10100U+(unsigned)i+3+displacement-0x2fc0c)&65535U)==0){
            fprintf(stderr,"Font-loader near candidate %x\n",0x10100U+(unsigned)i);++near;
        }
    }
    if(mask!=7 || near)abort();
    puts("Font loader encoded-reference audit: exactly three startup far calls, no near candidates");
}
static void startup_font(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)address; (void)size; (void)context;
    uint16_t ss,sp,cs,ip,ax=0,dx=(uint16_t)(0x5000+startup_fonts*0x100);
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp,source=get(u,stack+4)+16U*get(u,stack+6);
    char name[16]; check(uc_mem_read(u,source,name,sizeof name));
    for(unsigned i=0;i<sizeof name;++i) if(name[i]>='A' && name[i]<='Z') name[i]+='a'-'A';
    if((!startup_fonts && strcmp(name+(name[0]=='/'),SLICKS_RACE_FONT_NAME)) ||
       (startup_fonts && name[0])) abort();
    ++startup_fonts;
    ip=get(u,stack); cs=get(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void verify_startup_font(uc_engine *u)
{
    uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000;
    uc_hook hook; check(uc_hook_add(u,&hook,UC_HOOK_CODE,startup_font,0,0x2fc0c,0x2fc0c));
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    check(uc_emu_start(u,0x19dd8,0x19e2b,0,1000));
    if(startup_fonts!=3 || get(u,0x3cbf0+0x680) || get(u,0x3cbf0+0x682)!=0x5000 ||
       get(u,0x3cbf0+0x686)!=0x5100 || get(u,0x3cbf0+0x688) || get(u,0x3cbf0+0x68a)!=0x5200) abort();
    check(uc_hook_del(u,hook));
    puts("Original startup loads kirj/pieni/iso in order into DS:0680/0684/0688");
}
static void io(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size; (void)context; uint16_t ss,sp,cs,ip,ax=0,dx=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp)); unsigned stack=ss*16U+sp;
    if(address==0x373a7) dx=0x7000;
    else if(address==0x12d8a) { if(cursor>=length) abort(); ax=source[cursor++]; }
    else if(address==0x1221d) ax=cursor;
    else if(address==0x13b07) { allocated=get(u,stack+4); if(allocated>16384) abort(); dx=0x5000; }
    ip=get(u,stack); cs=get(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f); fclose(f); if(bytes<200000 || bytes==sizeof runtime) return 2;
    verify_font_loader_references(runtime,bytes);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,bytes));
    verify_startup_font(u);
    const unsigned addresses[]={0x373a7,0x12d8a,0x1221d,0x13b07,0x119c2};
    for(unsigned i=0;i<5;++i) { uc_hook h; check(uc_hook_add(u,&h,UC_HOOK_CODE,io,0,addresses[i],addresses[i])); }
    const char *names[]={"kirj.@f","pieni.@f","iso.@f"};
    for(unsigned test=0;test<3;++test) {
        long loaded=host_archive_load("ref/SLICKS.000",names[test],source,sizeof source); if(loaded<0) return 2;
        length=(unsigned)loaded; cursor=allocated=0;
        unsigned char native[16384],original[16384]; memset(native,0xa5,sizeof native);
        check(uc_mem_write(u,0x50000,native,sizeof native));
        long output=slicks_decode_font_resource(source,length,native,sizeof native); if(output<0) return 1;
        if(slicks_font_resource_size(source,length)!=output) abort();
        memset(native,0xa5,sizeof native);
        if(slicks_decode_font_resource(source,length,native,(unsigned long)output)!=output) abort();
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,ip,ax,dx;
        word(u,0x8f000,0); word(u,0x8f002,0x9000); word(u,0x8f004,0); word(u,0x8f006,0x6000); word(u,0x8f008,0);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x2fc0c,0x90000,0,1000000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        check(uc_reg_read(u,UC_X86_REG_AX,&ax)); check(uc_reg_read(u,UC_X86_REG_DX,&dx));
        check(uc_mem_read(u,0x50000,original,sizeof original));
        if(ip || sp!=0xf004 || ax || dx!=0x5000 || get(u,0x3cbf0+0x6bd4)!=ax ||
           get(u,0x3cbf0+0x6bd6)!=dx || cursor!=length || output>(long)allocated || memcmp(native,original,sizeof native)) {
            fprintf(stderr,"Font resource mismatch %s output=%ld allocation=%u consumed=%u\n",names[test],output,allocated,cursor); return 1;
        }
        if(test<2) {
            static struct SlicksRaceRuntime race;
            memset(race.font.runtime,0xa5,sizeof race.font.runtime);
            if(slicks_race_add_font(&race,source,length) || !race.font.ready ||
               memcmp(race.font.runtime,original,sizeof race.font.runtime)) {
                fprintf(stderr,"Production HUD font differs from original loader\n"); return 1;
            }
            puts("Production HUD loader: full runtime buffer matches original, including padding and untouched tail");
        }
        for(unsigned cut=0;cut<length;++cut) {
            memset(native,0xa5,sizeof native);
            if(slicks_font_resource_size(source,cut)!=-1 ||
               slicks_decode_font_resource(source,cut,native,sizeof native)!=-1) abort();
            for(unsigned i=0;i<sizeof native;++i) if(native[i]!=0xa5) abort();
        }
        for(unsigned capacity=0;capacity<(unsigned)output;++capacity) {
            if(slicks_decode_font_resource(source,length,native,capacity)!=-1) abort();
            for(unsigned i=0;i<sizeof native;++i) if(native[i]!=0xa5) abort();
        }
        printf("Original %s: %ld runtime bytes/padding match; all truncated inputs and short output capacities rejected atomically\n",names[test],output);
    }
    check(uc_close(u)); return 0;
}
