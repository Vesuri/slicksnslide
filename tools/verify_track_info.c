#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/ui/track_info.h"
static unsigned char bytes[65536];
static unsigned length,position,draws,old_message,rectangle;
static short origin_x,origin_y;
static struct SlicksTrackPreview view;
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
#define REQUIRE(c) do { if(!(c)) { fprintf(stderr,"Track info assertion line %d\n",__LINE__); exit(1); } } while(0)
static void word(uc_engine *u,unsigned at,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(u,at,b,2)); }
static unsigned get(uc_engine *u,unsigned at)
{ unsigned char b[2]; check(uc_mem_read(u,at,b,2)); return b[0]|b[1]<<8; }
static void boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)size; (void)p; uint16_t ss,sp,cs,ip,ax=0,dx=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    if(address==0x11eaf) { ax=1; position=0; }
    else if(address==0x1219b) {
        unsigned whence=get(u,stack+12);
        int32_t offset=(int32_t)((uint32_t)get(u,stack+8)|((uint32_t)get(u,stack+10)<<16));
        REQUIRE(whence<=1); position=(unsigned)((whence?position:0)+offset);
        REQUIRE(position<=length);
    } else if(address==0x12d8a) { REQUIRE(position<length); ax=bytes[position++]; }
    else if(address==0x36c97) ax=100; /* unchanged key, no preview interruption */
    else if(address==0x301ab) ++old_message;
    else if(address==0x39ed8) {
        REQUIRE((short)get(u,stack+4)==origin_x && (short)get(u,stack+6)==origin_y);
        REQUIRE((short)get(u,stack+8)==(short)(origin_x+65));
        REQUIRE((short)get(u,stack+10)==(short)(origin_y+40));
        REQUIRE((get(u,stack+12)&255)==21); ++rectangle;
    } else if(address==0x1a1d4) {
        short x,y; unsigned char type,rotation;
        REQUIRE(draws<view.count);
        slicks_track_preview_object(&view,draws++,origin_x,origin_y,&x,&y,&type,&rotation);
        REQUIRE((short)get(u,stack+4)==x && (short)get(u,stack+6)==y);
        REQUIRE(get(u,stack+8)==type && (get(u,stack+10)&255)==rotation);
    }
    ip=get(u,stack); cs=get(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void run(uc_engine *u,unsigned address)
{
    word(u,0x8f000,0); word(u,0x8f002,0x9000);
    word(u,0x8f004,0); word(u,0x8f006,0x6500);
    word(u,0x8f008,origin_x); word(u,0x8f00a,origin_y);
    uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    check(uc_emu_start(u,address,0x90000,0,10000000));
    check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    REQUIRE(!ip && sp==0xf004);
}
static void compare(uc_engine *u)
{
    unsigned char native[512],dos[512]; memset(native,0xa5,sizeof native);
    check(uc_mem_write(u,0x3cbf0+0x4c7c,native,sizeof native));
    REQUIRE(!slicks_track_description(bytes,length,native,sizeof native));
    run(u,0x1a4ef); check(uc_mem_read(u,0x3cbf0+0x4c7c,dos,sizeof dos));
    REQUIRE(!memcmp(native,dos,sizeof native));
    draws=old_message=rectangle=0;
    int result=slicks_track_preview_open(&view,bytes,length); REQUIRE(result>=0);
    run(u,0x1a32d);
    REQUIRE(result?old_message==1 && !draws && !rectangle:
        !old_message && rectangle==1 && draws==view.count);
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); REQUIRE(f);
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f); REQUIRE(n && n<sizeof runtime);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n));
    check(uc_mem_write(u,0x65000,"BASIC",6));
    const unsigned addresses[]={0x11eaf,0x1219b,0x12d8a,0x119c2,0x36c97,0x301ab,0x39ed8,0x1a1d4};
    for(unsigned i=0;i<sizeof addresses/sizeof addresses[0];++i) {
        uc_hook h; check(uc_hook_add(u,&h,UC_HOOK_CODE,boundary,0,addresses[i],addresses[i]));
    }
    DIR *dir=opendir("ref/TRACKS"); REQUIRE(dir); struct dirent *entry; unsigned tracks=0;
    origin_x=245; origin_y=20;
    while((entry=readdir(dir))) {
        size_t len=strlen(entry->d_name); if(len<3 || strcmp(entry->d_name+len-3,".SS")) continue;
        char path[1024]; snprintf(path,sizeof path,"ref/TRACKS/%s",entry->d_name);
        f=fopen(path,"rb"); REQUIRE(f); length=(unsigned)fread(bytes,1,sizeof bytes,f); fclose(f);
        REQUIRE(length<sizeof bytes); compare(u); ++tracks;
    }
    closedir(dir); REQUIRE(tracks==195);
    unsigned synthetic=0;
    for(unsigned variant=0;variant<16;++variant) {
        memset(bytes,0,sizeof bytes); length=2048; bytes[5]=variant&1?1:2;
        bytes[6]=4; /* object stream at 1024 */
        bytes[1025]=16;
        for(unsigned i=1;i<256;++i) bytes[377+i]=(unsigned char)i;
        for(unsigned i=0;i<16;++i) {
            unsigned x=(i*4097+variant)&65535,at=1026+5*i;
            bytes[at]=x>>8; bytes[at+1]=x; bytes[at+2]=(unsigned char)(i*17);
            bytes[at+3]=(unsigned char)(i*17); bytes[at+4]=(unsigned char)(i*17);
        }
        origin_x=variant&2?32760:-13; origin_y=variant&4?32760:-7;
        if(variant&8) { bytes[1024]=0x80; bytes[1025]=0; }
        compare(u); ++synthetic;
    }
    unsigned char guard[300]; memset(guard,0xa5,sizeof guard);
    for(unsigned cut=0;cut<=633;++cut) REQUIRE(slicks_track_description(bytes,cut,guard,sizeof guard)==-1);
    REQUIRE(slicks_track_description(bytes,length,guard,255)==-1);
    for(unsigned i=0;i<sizeof guard;++i) REQUIRE(guard[i]==0xa5);
    struct SlicksTrackPreview sentinel={bytes+1,999}; bytes[5]=2;
    for(unsigned cut=0;cut<1026;++cut) {
        REQUIRE(slicks_track_preview_open(&sentinel,bytes,cut)==-1);
        REQUIRE(sentinel.objects==bytes+1 && sentinel.count==999);
    }
    bytes[1024]=0; bytes[1025]=16;
    for(unsigned cut=1026;cut<1106;++cut) {
        REQUIRE(slicks_track_preview_open(&sentinel,bytes,cut)==-1);
        REQUIRE(sentinel.objects==bytes+1 && sentinel.count==999);
    }
    bytes[6]=0x80; REQUIRE(slicks_track_preview_open(&sentinel,bytes,length)==-1);
    check(uc_close(u));
    printf("Original track description/preview: %u real tracks, %u signed-coordinate/character/old-format cases and bounded-input guards pass\n",tracks,synthetic);
    return 0;
}
