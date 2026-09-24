#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/track_lists.h"
#define abort() do { fprintf(stderr,"Track-list assertion at line %d\n",__LINE__); exit(1); } while(0)
static unsigned char file_bytes[8192]; static unsigned file_size,file_at;
static unsigned char written[8192]; static unsigned written_size,opens;
static const unsigned char names[][9]={"BASIC","1WAY","ABCDEFGH","a","DUP","DUP"};
static const unsigned char *name(void *p,unsigned i) { (void)p; return names[i]; }
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned at,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(u,at,b,2)); }
static unsigned get(uc_engine *u,unsigned at)
{ unsigned char b[2]; check(uc_mem_read(u,at,b,2)); return b[0]|b[1]<<8; }
static void boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)size; (void)p; uint16_t ss,sp,cs,ip,ax=0,dx=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    if(address==0x11eaf) { dx=(uint16_t)(0x7000+0x100*opens++); if(opens==1) file_at=0; }
    else if(address==0x12d8a) { if(file_at>=file_size) abort(); ax=file_bytes[file_at++]; }
    else if(address==0x123ce) { if(written_size==sizeof written) abort(); written[written_size++]=(unsigned char)get(u,stack+4); }
    else if(address==0x11f08) {
        unsigned at=get(u,stack+4)+16*get(u,stack+6); unsigned char c;
        for(;;++at) { check(uc_mem_read(u,at,&c,1)); if(!c) break; if(written_size==sizeof written) abort(); written[written_size++]=c; }
    } else if(address==0x35e63) {
        unsigned i=get(u,stack+8); if(i>=6) abort();
        check(uc_mem_write(u,0x62000,names[i],9)); dx=0x6200;
    }
    ip=get(u,stack); cs=get(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void beword(unsigned v) { file_bytes[file_size++]=v>>8; file_bytes[file_size++]=v; }
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f); fclose(f);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,bytes));
    const unsigned hooks[]={0x11eaf,0x12d8a,0x119c2,0x35e63,0x123ce,0x11f08,0x1319e,0x11483};
    for(unsigned i=0;i<sizeof hooks/sizeof hooks[0];++i) { uc_hook h; check(uc_hook_add(u,&h,UC_HOOK_CODE,boundary,0,hooks[i],hooks[i])); }
    unsigned cases=0,write_cases=0;
    for(unsigned variant=0;variant<20;++variant) {
        memcpy(file_bytes,"SSTrk\032",6); file_size=6; beword(3);
        for(unsigned list=0;list<3;++list) {
            unsigned count=(variant+list*3)%19; beword(count);
            memset(file_bytes+file_size,0,21); snprintf((char *)file_bytes+file_size,21,"List %u",list); file_size+=21;
            for(unsigned i=0;i<count;++i) {
                const unsigned char *s=(i+variant)%8<6?names[(i+variant)%8]:(const unsigned char *)((i&1)?"MISSING":"basic");
                memset(file_bytes+file_size,0,8); memcpy(file_bytes+file_size,s,strlen((const char *)s)); file_size+=8;
            }
        }
        if(variant&1) { file_bytes[4]=99; file_bytes[5]=88; } /* skipped original bytes */
        struct SlicksTrackLists lists;
        if(slicks_track_lists_open(&lists,file_bytes,file_size) || lists.count!=3) abort();
        for(unsigned selected=0;selected<3;++selected) {
            opens=0;
            short native[64]; for(unsigned i=0;i<64;++i) { native[i]=(short)(900+i); word(u,0x60000+2*i,native[i]); }
            struct SlicksTrackPlaylist playlist={native,7,64};
            if(slicks_track_lists_select(&lists,selected,&playlist,6,name,0)) abort();
            word(u,0x3cbf0+0x62a,0); word(u,0x3cbf0+0x62c,0x6000);
            word(u,0x3cbf0+0x90,7); word(u,0x3cbf0+0x4da8,6);
            word(u,0x8f000,0); word(u,0x8f002,0x9000); word(u,0x8f004,selected);
            word(u,0x8f006,0); word(u,0x8f008,0x6500);
            uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
            check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
            check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
            check(uc_emu_start(u,0x2d28a,0x90000,0,10000000)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
            if(ip || get(u,0x3cbf0+0x90)!=playlist.count) {
                fprintf(stderr,"State mismatch variant=%u list=%u ip=%x count=%u/%u read=%u/%u\n",
                    variant,selected,ip,get(u,0x3cbf0+0x90),playlist.count,file_at,file_size); return 1;
            }
            for(unsigned i=0;i<64;++i) if(get(u,0x60000+2*i)!=(unsigned short)native[i]) {
                fprintf(stderr,"List mismatch variant=%u selected=%u word=%u\n",variant,selected,i); return 1;
            }
            ++cases;
        }
        for(int remove=-1;remove<3;++remove) {
            short tracks[16]; unsigned count=variant%17;
            for(unsigned i=0;i<count;++i) { tracks[i]=(short)((i*3+variant)%6); word(u,0x60000+2*i,tracks[i]); }
            struct SlicksTrackPlaylist playlist={tracks,(unsigned short)count,16};
            unsigned char title[21]={0},output[8192];
            snprintf((char *)title,sizeof title,"My list %u",variant);
            memset(output,0xa5,sizeof output); memset(written,0xa5,sizeof written); written_size=opens=0;
            long size=slicks_track_lists_write(&lists,remove,remove<0?title:0,&playlist,6,name,0,output,sizeof output);
            if(size<0) abort();
            check(uc_mem_write(u,0x65000,title,sizeof title));
            word(u,0x3cbf0+0x90,count);
            word(u,0x8f000,0); word(u,0x8f002,0x9000); word(u,0x8f004,(unsigned short)remove);
            word(u,0x8f006,0); word(u,0x8f008,remove<0?0x6500:0);
            uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
            check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
            check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
            check(uc_emu_start(u,0x2d4db,0x90000,0,10000000)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
            if(ip || written_size!=(unsigned long)size || memcmp(output,written,sizeof output)) {
                fprintf(stderr,"Writer mismatch variant=%u remove=%d bytes=%ld/%u ip=%x\n",variant,remove,size,written_size,ip); return 1;
            }
            for(unsigned capacity=0;capacity<(unsigned)size;++capacity) {
                memset(output,0xa5,sizeof output);
                if(slicks_track_lists_write(&lists,remove,remove<0?title:0,&playlist,6,name,0,output,capacity)!=-1) abort();
                for(unsigned i=0;i<sizeof output;++i) if(output[i]!=0xa5) abort();
            }
            ++write_cases;
        }
        for(unsigned cut=0;cut<file_size;++cut) {
            struct SlicksTrackLists guard={0,123,456};
            if(slicks_track_lists_open(&guard,file_bytes,cut)!=-1 || guard.bytes || guard.size!=123 || guard.count!=456) abort();
        }
        short one[1]={123}; struct SlicksTrackPlaylist small={one,1,1};
        if(variant==10 && (slicks_track_lists_select(&lists,0,&small,6,name,0)!=-1 || one[0]!=123 || small.count!=1)) abort();
    }
    check(uc_close(u));
    printf("Original saved-track-list reader: %u complete playlist/tail comparisons; missing names, duplicates, exact eight-byte names, ASCII case folding, truncation and atomic capacity failure pass\n",cases);
    printf("Original saved-track-list writer: %u entire-file append/delete comparisons and all short capacities rejected without writes pass\n",write_cases);
    return 0;
}
