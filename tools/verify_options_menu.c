#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/ui/options_menu.h"
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned a,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(u,a,b,2)); }
static unsigned readword(uc_engine *u,unsigned a)
{ unsigned char b[2]; check(uc_mem_read(u,a,b,2)); return b[0]|b[1]<<8; }
static void regs(uc_engine *u,unsigned scan)
{
    uint16_t cs=0x266c,ss=0x8000,ds=0x3cbf,bp=0xf000,sp=0xef00,ax=scan;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_AX,&ax));
}
int main(void)
{
    FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    unsigned char runtime[300000]; size_t n=fread(runtime,1,sizeof runtime,f);
    if(ferror(f) || n<200000 || n==sizeof runtime) return 2;
    fclose(f);
    const unsigned char *ds=runtime+0x3cbf0-0x10100;
    struct SlicksOptionSpec specs[15];
    if(slicks_option_specs(specs,ds,0x2fa4)) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,n));
    static const unsigned scans[]={0x4b,0x4d,0x47,0x49,0x4f,0x51,0x1c,0x1d,0x39};
    unsigned cases=0;
    for(unsigned row=0;row<15;++row) for(unsigned key=0;key<9;++key)
    for(unsigned pattern=0;pattern<1032;++pattern) {
        static const short edges[]={-32768,32767,-1,0,1,4,5,300};
        short value=pattern<8?edges[pattern]:(short)(unsigned short)(pattern*193U+row*37U);
        regs(u,scans[key]);
        word(u,0x8effb,row); word(u,0x8efee,scans[key]);
        word(u,0x3cbf0+0x92+8*row,(unsigned short)value);
        check(uc_emu_start(u,0x294d1,0x29659,0,150));
        if((unsigned short)slicks_option_value(value,&specs[row],scans[key])!=readword(u,0x3cbf0+0x92+8*row)) {
            fprintf(stderr,"Option value mismatch row=%u scan=%x value=%d\n",row,scans[key],value); return 1;
        }
        ++cases;
    }
    /* Full original dispatch for navigation and all real option edits. */
    check(uc_ctl_remove_cache(u,0,0x100000));
    static const unsigned keys[]={0,1,0x44,0x48,0x50,0x4b,0x4d,0x47,0x49,0x4f,0x51,0x1c,0x1d,0x39};
    unsigned dispatches=0;
    for(unsigned mode=0;mode<6;++mode) for(unsigned row=0;row<18;++row)
    for(unsigned key=0;key<sizeof keys/sizeof keys[0];++key) {
        if(row>=15 && key>=5) continue; /* Separate action callbacks, not numeric records. */
        struct SlicksConfiguration c;
        memset(&c,0,sizeof c);
        slicks_configuration_defaults(&c,ds,0x2fa4); c.options[0]=(short)mode;
        check(uc_mem_write(u,0x3cbf0+0x92,ds+0x92,15*8)); word(u,0x3cbf0+0x92,mode);
        unsigned char locals[32]={0}; locals[27]=(unsigned char)row; locals[26]=123;
        check(uc_mem_write(u,0x8efe0,locals,sizeof locals));
        word(u,0x8efee,keys[key]); word(u,0x3cbf0+0x6c0,0);
        regs(u,keys[key]);
        check(uc_emu_start(u,0x293fb,0x29659,0,300));
        check(uc_mem_read(u,0x8efe0,locals,sizeof locals));
        struct SlicksOptionsMenu menu={(unsigned char)row,0,0,123};
        if(slicks_options_menu_key(&menu,&c,specs,(unsigned char)keys[key]) ||
           menu.row!=locals[27] || menu.done!=locals[25] ||
           (unsigned char)menu.redraw!=locals[26] || menu.dirty!=(readword(u,0x3cbf0+0x6c0)&255)) {
            fprintf(stderr,"Option dispatch mismatch mode=%u row=%u key=%x native=%u,%u,%d,%u DOS=%u,%u,%d,%u\n",
                mode,row,keys[key],menu.row,menu.done,menu.redraw,menu.dirty,
                locals[27],locals[25],(signed char)locals[26],readword(u,0x3cbf0+0x6c0)&255); return 1;
        }
        for(unsigned i=0;i<15;++i) if((unsigned short)c.options[i]!=readword(u,0x3cbf0+0x92+8*i)) return 1;
        ++dispatches;
    }
    check(uc_close(u));
    printf("Original options: %u value and %u full-dispatch comparisons pass across all 15 settings and 6 modes\n",cases,dispatches);
    return 0;
}
