#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/ui/profile_actions.h"
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static unsigned rd(uc_engine *u,unsigned a)
{ unsigned char b[2]; check(uc_mem_read(u,a,b,2)); return b[0]|b[1]<<8; }
static void stop(uc_engine *u,uint64_t a,uint32_t size,void *p)
{ (void)size; *(unsigned *)p=(unsigned)a; check(uc_emu_stop(u)); }
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); int error=ferror(f); fclose(f);
    if(error || n<200000 || n==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n));
    unsigned stopped=0; uc_hook hooks[5]; unsigned points[]={0x28b25,0x28b48,0x28ce4,0x28c3d,0x28cbd};
    for(unsigned i=0;i<5;++i) check(uc_hook_add(u,&hooks[i],UC_HOOK_CODE,stop,&stopped,points[i],points[i]));
    unsigned cases=0;
    for(unsigned flags=0;flags<2;++flags) {
        unsigned char table[4096]; memset(table,flags?255:0,sizeof table);
        check(uc_mem_write(u,0x3cbf0+0x3ede,table,sizeof table));
        for(unsigned value=0;value<65536;++value) {
            uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xe000,bp=0xf000,ax=(uint16_t)value;
            check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
            check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
            check(uc_reg_write(u,UC_X86_REG_BP,&bp)); check(uc_reg_write(u,UC_X86_REG_AX,&ax));
            stopped=0; check(uc_emu_start(u,0x28b00,0x90000,0,1000));
            short index=-123; enum SlicksProfileListAction action=slicks_profile_list_action((short)value,flags?255:0,&index);
            unsigned expected=action==SLICKS_PROFILE_LIST_EDIT?0x28b25:
                action==SLICKS_PROFILE_LIST_CONFIRM_DELETE?0x28b48:0x28ce4;
            if(stopped!=expected || (action && index!=(short)rd(u,0x8eff0))) {
                fprintf(stderr,"Profile action mismatch result=%04x flags=%u stopped=%x\n",value,flags,stopped); return 1;
            }
            ax=(uint16_t)value; check(uc_reg_write(u,UC_X86_REG_AX,&ax)); stopped=0;
            check(uc_emu_start(u,0x28c29,0x90000,0,100));
            if(stopped!=(slicks_profile_delete_confirmed((unsigned short)value)?0x28c3d:0x28cbd)) abort();
            ++cases;
        }
    }
    check(uc_close(u));
    printf("DOS profile actions: %u full-word result/protection/confirmation comparisons pass\n",cases);
    return 0;
}
