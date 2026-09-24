#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/ui/saved_rectangle.h"
static unsigned char screen[64000]; static unsigned plane,mask,writes;
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned a,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(u,a,b,2)); }
static void port(uc_engine *u,uint32_t address,int size,uint32_t value,void *p)
{
    (void)u; (void)p;
    if(size!=1) abort();
    if(address==0x3ce && value==4) return;
    if(address==0x3c4 && value==2) return;
    if(address==0x3cf) { plane=value&3; return; }
    if(address==0x3c5) { mask=value&15; return; }
    abort();
}
static void memory(uc_engine *u,uc_mem_type type,uint64_t address,int size,int64_t value,void *p)
{
    (void)p; unsigned char bytes[8]; if(size>8) abort();
    for(int i=0;i<size;++i) {
        unsigned at=(unsigned)(address-0xa0000)+i,y=at/100,x=(at%100)*4;
        if(y>=200 || x>=320) abort();
        if(type==UC_MEM_READ) bytes[i]=screen[y*320+x+plane];
        else for(unsigned b=0;b<4;++b) if(mask&(1U<<b)) {
            screen[y*320+x+b]=(unsigned char)((uint64_t)value>>(8*i)); ++writes;
        }
    }
    if(type==UC_MEM_READ) check(uc_mem_write(u,address,bytes,(size_t)size));
}
struct Bounds { unsigned count; short v[4]; };
static void dirty(void *p,short l,short t,short r,short b)
{ struct Bounds *s=p; ++s->count; s->v[0]=l; s->v[1]=t; s->v[2]=r; s->v[3]=b; }
static void run(uc_engine *u,unsigned entry,const unsigned *args,unsigned count)
{
    uint16_t cs=(uint16_t)(entry==0x3b9de?0x3b4f:0x3aa0),ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
    word(u,0x8f000,0); word(u,0x8f002,0x9000);
    for(unsigned i=0;i<count;++i) word(u,0x8f004+2*i,args[i]);
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    check(uc_emu_start(u,entry,0x90000,0,2000000));
    check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    if(ip || sp!=0xf004) abort();
}
int main(void)
{
    /* The name field may reach VGA x=329. Check every right-edge alignment,
     * including an entirely invisible cursor and the last display row. */
    for(short x=310;x<=330;++x) {
        unsigned char guarded[64002],storage[34];
        memset(guarded,0x73,sizeof guarded); memset(storage,0xa5,sizeof storage);
        struct Bounds bounds={0,{0,0,0,0}};
        struct SlicksChunkyUi ui={guarded+1,0,dirty,&bounds};
        struct SlicksSavedRectangle saved={0};
        if(slicks_save_rectangle_visible(&saved,storage+1,32,&ui,x,199,17,1)) abort();
        if(saved.width!=20 || saved.height!=1) abort();
        for(unsigned c=0;c<20;++c) if(storage[c+1]!=(x+c<320?0x73:0)) abort();
        memset(ui.pixels,0x19,64000);
        if(slicks_restore_rectangle_visible(&ui,&saved,x,199)) abort();
        for(unsigned i=0;i<64000;++i)
            if(ui.pixels[i]!=(i>=199*320U+(unsigned)x && x<320?0x73:0x19)) abort();
        if(guarded[0]!=0x73 || guarded[64001]!=0x73 || storage[0]!=0xa5 || storage[21]!=0xa5) abort();
        if(bounds.count!=(unsigned)(x<320) || (x<320 &&
           (bounds.v[0]!=x || bounds.v[1]!=199 || bounds.v[2]!=320 || bounds.v[3]!=200))) abort();
        if(slicks_save_rectangle_visible(&saved,storage+1,19,&ui,x,199,17,1)!=-1 ||
           slicks_save_rectangle_visible(&saved,storage+1,32,&ui,x,200,17,1)!=-1) abort();
    }
    puts("Visible save-under: 21 edge alignments preserve surface/storage guards and clipped dirty bounds");
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); int error=ferror(f); fclose(f);
    if(error || n<200000 || n==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n)); word(u,0x3cbf0+0x1d7b,100);
    uc_hook output,mem;
    check(uc_hook_add(u,&output,UC_HOOK_INSN,port,0,1,0,UC_X86_INS_OUT));
    check(uc_hook_add(u,&mem,UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,memory,0,0xa0000,0xaffff));
    const short sizes[][2]={{1,1},{3,7},{7,9},{150,100},{52,25},{24,3}};
    unsigned cases=0;
    for(unsigned s=0;s<6;++s) for(unsigned source_phase=0;source_phase<4;++source_phase)
    for(unsigned dest_phase=0;dest_phase<4;++dest_phase) for(unsigned crop=0;crop<2;++crop) {
        unsigned char native[64000],storage[64000],packed[64003];
        for(unsigned i=0;i<64000;++i) screen[i]=native[i]=(unsigned char)(i*31+(i>>8)*13+s*73);
        struct Bounds bounds={0,{0,0,0,0}}; struct SlicksChunkyUi ui={native,0,dirty,&bounds};
        struct SlicksSavedRectangle saved;
        if(slicks_save_rectangle(&saved,storage,sizeof storage,&ui,120+source_phase,20,sizes[s][0],sizes[s][1])) abort();
        unsigned args[]={120+source_phase,20,(unsigned)sizes[s][0],(unsigned)sizes[s][1],0,0x5000,0};
        run(u,0x3abe5,args,7);
        unsigned total=saved.width*saved.height;
        check(uc_mem_read(u,0x50000,packed,total+3));
        if(packed[0]!=saved.width/4 || packed[1]!=saved.height || packed[total+2]!=((4-sizes[s][0])&3)) abort();
        for(unsigned y=0;y<saved.height;++y) for(unsigned x=0;x<saved.width;++x)
            if(storage[y*saved.width+x]!=packed[2+(x&3)*(total/4)+y*(saved.width/4)+x/4]) abort();
        for(unsigned i=0;i<64000;++i) screen[i]=native[i]=(unsigned char)(i*17+91);
        short sx=0,sy=0,w=saved.width,h=saved.height;
        if(crop) { sx=1; if(w>=8) { sx=5; w-=4; } if(h>1) { sy=1; --h; } }
        if(slicks_restore_rectangle(&ui,&saved,40+dest_phase,40,sx,sy,w,h)) abort();
        writes=0;
        if(crop) {
            unsigned a[]={40+dest_phase,40,(unsigned)sx,(unsigned)sy,(unsigned)w,(unsigned)h,0,0x5000,0};
            run(u,0x3b9de,a,9);
        } else { unsigned a[]={40+dest_phase,40,0,0x5000,0}; run(u,0x3aaf2,a,5); }
        if(memcmp(native,screen,64000) || bounds.count!=1 ||
           writes!=(unsigned)(bounds.v[2]-bounds.v[0])*(bounds.v[3]-bounds.v[1])) {
            fprintf(stderr,"Saved rectangle mismatch size=%u source=%u dest=%u crop=%u\n",s,source_phase,dest_phase,crop); return 1;
        }
        ++cases;
    }
    unsigned char storage[32],before[32],native[64000];
    memset(storage,0xa5,sizeof storage); memcpy(before,storage,sizeof before);
    memset(native,0x73,sizeof native);
    struct Bounds bounds={0,{0,0,0,0}}; struct SlicksChunkyUi ui={native,0,dirty,&bounds};
    struct SlicksSavedRectangle saved={storage,8,4};
    if(slicks_save_rectangle(&saved,storage,31,&ui,0,0,8,4)!=-1 ||
       saved.width!=8 || saved.height!=4 || memcmp(storage,before,32) || bounds.count) abort();
    if(slicks_save_rectangle(&saved,storage,32,&ui,319,0,1,1)!=-1 ||
       slicks_save_rectangle(&saved,storage,32,&ui,0,199,8,2)!=-1 ||
       slicks_restore_rectangle(&ui,&saved,0,0,5,0,8,1)!=-1 ||
       slicks_restore_rectangle(&ui,&saved,319,0,0,0,1,1)!=-1 || bounds.count) abort();
    for(unsigned i=0;i<64000;++i) if(native[i]!=0x73) abort();
    check(uc_close(u));
    printf("DOS saved rectangles: %u capture bytes/full restore/crop pixels and dirty-bounds comparisons pass\n",cases);
    return 0;
}
