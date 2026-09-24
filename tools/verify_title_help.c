#define main options_verifier_main
#include "verify_options_menu.c"
#undef main
#include "../src/ui/title_help.h"
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,n));
    for(unsigned action=0;action<6;++action) for(unsigned selection=0;selection<7;++selection) {
        const unsigned char *topic=slicks_title_help_topic(action,selection);
        if(action!=3 && !(action==2 && selection==5)) { if(topic) abort(); continue; }
        regs(u,0);
        check(uc_emu_start(u,action==3?0x2a3cc:0x2a096,0x327dc,0,30));
        uint16_t cs,ip,sp; check(uc_reg_read(u,UC_X86_REG_CS,&cs)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        if((unsigned)cs*16+ip!=0x327dc) abort();
        unsigned pointer=16*readword(u,0x80000+sp+6)+readword(u,0x80000+sp+4);
        unsigned char original[21]; check(uc_mem_read(u,pointer,original,sizeof original));
        if(!memchr(original,0,sizeof original) || strcmp((const char *)original,(const char *)topic)) abort();
    }
    check(uc_close(u)); puts("Original title Help: F1 and Read This topic arguments match; unrelated actions excluded");
    return 0;
}
