#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/ui/menu_background.h"
static unsigned char original[64000]; static unsigned mask,writes;
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned at,unsigned value)
{ unsigned char b[2]={value,value>>8}; check(uc_mem_write(u,at,b,2)); }
static void port(uc_engine *u,uint32_t address,int size,uint32_t value,void *context)
{
    (void)u; (void)context;
    if(address==0x3c4 && size==1 && value==2) return;
    if(address!=0x3c5 || size!=1) abort(); mask=value&15;
}
static void pixel(uc_engine *u,uc_mem_type type,uint64_t address,int size,int64_t value,void *context)
{
    (void)u; (void)type; (void)context;
    for(int byte=0;byte<size;++byte) {
        unsigned at=(unsigned)(address-0xa0000)+byte;
        for(unsigned plane=0;plane<4;++plane) if(mask&(1U<<plane)) {
            unsigned x=(at%100)*4+plane,y=at/100;
            if(x<320 && y<200) { original[y*320+x]=(unsigned char)((uint64_t)value>>(8*byte)); ++writes; }
        }
    }
}
struct Bounds { unsigned count; short rect[4]; };
static void dirty(void *context,short l,short t,short r,short b)
{ struct Bounds *d=context; ++d->count; d->rect[0]=l; d->rect[1]=t; d->rect[2]=r; d->rect[3]=b; }
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f); fclose(f); if(bytes<200000 || bytes==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,bytes));
    uc_hook output,memory;
    check(uc_hook_add(u,&output,UC_HOOK_INSN,port,0,1,0,UC_X86_INS_OUT));
    check(uc_hook_add(u,&memory,UC_HOOK_MEM_WRITE,pixel,0,0xa0000,0xaffff));
    const short crops[][4]={{40,28,190,72},{35,105,100,150},{0,0,320,200},
        {1,1,1,1},{2,2,2,2},{3,3,3,3},{4,4,5,7},{312,198,4,2}};
    unsigned cases=0;
    for(unsigned pattern=0;pattern<4;++pattern)
    for(unsigned crop=0;crop<sizeof crops/sizeof crops[0];++crop)
    for(unsigned phase=0;phase<4;++phase) {
        unsigned char saved[64000],native[64000],sprite[65536];
        for(unsigned i=0;i<64000;++i) {
            saved[i]=(unsigned char)(i*31+(i>>8)*13+pattern*73);
            native[i]=original[i]=(unsigned char)(i*17+pattern*67);
        }
        memset(sprite,0xdc,sizeof sprite); sprite[0]=80; sprite[1]=200;
        for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;++x)
            sprite[2+(x&3)*16000+y*80+x/4]=saved[y*320+x];
        check(uc_mem_write(u,0x50000,sprite,sizeof sprite));
        struct Bounds bounds={0,{0,0,0,0}};
        struct SlicksChunkyUi ui={native,0,dirty,&bounds};
        short dx=(short)phase,dy=0; const short *c=crops[crop];
        /* Avoid horizontal VGA wrapping: the live menu also stays in bounds. */
        if(dx+(c[0]&~3)+((c[2]+3)&~3)>320) continue;
        if(slicks_restore_menu_background(&ui,saved,dx,dy,c[0],c[1],c[2],c[3])) abort();
        uint16_t cs=0x3b4f,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
        word(u,0x3cbf0+0x1d7b,100);
        word(u,0x8f000,0); word(u,0x8f002,0x9000);
        word(u,0x8f004,dx); word(u,0x8f006,dy);
        for(unsigned i=0;i<4;++i) word(u,0x8f008+2*i,(unsigned short)c[i]);
        word(u,0x8f010,0); word(u,0x8f012,0x5000); word(u,0x8f014,0);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        mask=writes=0; check(uc_emu_start(u,0x3b9de,0x90000,0,2000000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        if(ip || sp!=0xf004 || memcmp(native,original,sizeof native) || bounds.count!=1 ||
            writes!=(unsigned)(bounds.rect[2]-bounds.rect[0])*(bounds.rect[3]-bounds.rect[1])) {
            fprintf(stderr,"Menu restore mismatch pattern=%u crop=%u phase=%u ip=%x sp=%x writes=%u\n",pattern,crop,phase,ip,sp,writes); return 1;
        }
        for(unsigned at=0;at<64000;++at) {
            unsigned x=at%320,y=at/320;
            if(x>=(unsigned)bounds.rect[0] && x<(unsigned)bounds.rect[2] &&
               y>=(unsigned)bounds.rect[1] && y<(unsigned)bounds.rect[3]) continue;
            if(native[at]!=(unsigned char)(at*17+pattern*67)) abort();
        }
        ++cases;
    }
    check(uc_close(u));
    printf("Original menu background restore: %u full visible-frame and dirty-coverage comparisons pass, including 150-row menu crop\n",cases);
    return 0;
}
