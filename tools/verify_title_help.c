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
    /* The title caller passes 2 to the keyboard-repeat reader at 36ce0.
     * Exercise every make/break/idle scan through its actual instructions.
     * No BIOS/mouse callback is stubbed: this reader only uses the keyboard
     * latch and BIOS tick count. Stop immediately before its far return. */
    for(unsigned scan=0;scan<256;++scan) {
        regs(u,0);
        uint16_t cs=0x2e0f,ax;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));
        word(u,0x8ef06,2);
        word(u,0x3cbf0+0x1714,scan);
        word(u,0x3cbf0+0x174e,0);
        word(u,0x46c,100); word(u,0x46e,0);
        check(uc_emu_start(u,0x36ce0,0x36d64,0,100));
        check(uc_reg_read(u,UC_X86_REG_AX,&ax));
        if((ax&255)!=(scan<128?scan:0)) abort();
    }
    check(uc_close(u)); puts("Original title Help topics and all 256 keyboard-reader scans match");
    return 0;
}
