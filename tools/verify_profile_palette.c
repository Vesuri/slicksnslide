#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/profile_palette.h"
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t size=fread(runtime,1,sizeof runtime,f); int error=ferror(f); fclose(f);
    if(error || size<200000 || size==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,size));
    const unsigned char target[]={0,0,0,0x60},ret[]={0,0,0,0x70};
    check(uc_mem_write(u,0x3cbf0+0x68ae,target,4));
    for(unsigned first=0;first<256;++first) for(unsigned last=0;last<256;++last) {
        unsigned char colours[4][6],expected[768],actual[768];
        for(unsigned car=0;car<4;++car) for(unsigned ch=0;ch<3;++ch) {
            colours[car][ch]=(unsigned char)(first+car*31+ch*17);
            colours[car][ch+3]=(unsigned char)(last+car*11+ch*7);
        }
        memset(expected,0xa5,sizeof expected);
        check(uc_mem_write(u,0x60000,expected,sizeof expected));
        check(uc_mem_write(u,0x3cbf0+0x310c,colours,sizeof colours));
        slicks_profile_palette(expected,colours);
        check(uc_mem_write(u,0x8f000,ret,4));
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x19d04,0x70000,0,10000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        check(uc_mem_read(u,0x60000,actual,sizeof actual));
        if(ip || sp!=0xf004 || memcmp(actual,expected,sizeof actual)) {
            fprintf(stderr,"profile palette mismatch first=%u last=%u\n",first,last); return 1;
        }
    }
    check(uc_close(u));
    puts("DOS profile palette: 65536 complete-routine comparisons pass, all signed endpoint pairs and untouched palette entries");
    return 0;
}
