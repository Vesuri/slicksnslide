#define main track_info_verifier_main
#include "verify_track_info.c"
#undef main
#include "../src/ui/speed_dialog.h"
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); REQUIRE(f);
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f); REQUIRE(n && n<sizeof runtime);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n));
    const unsigned char keys[]={1,28,57,59,71,72,75,77,79,80,29,255};
    unsigned cases=0;
    for(unsigned value=0;value<65536;++value) for(unsigned k=0;k<sizeof keys;++k) {
        struct SlicksSpeedDialog dialog={0};
        word(u,0x3cbf0+0x5de,value); word(u,0x8effb,0);
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xe000,bp=0xf000,ax=keys[k],ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp)); check(uc_reg_write(u,UC_X86_REG_AX,&ax));
        check(uc_emu_start(u,0x1e124,0x1e185,0,1000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        short native=slicks_speed_dialog_key(&dialog,(short)value,keys[k]);
        unsigned char done; check(uc_mem_read(u,0x8effb,&done,1));
        REQUIRE(ip==0x1e185-0x19870 && get(u,0x3cbf0+0x5de)==(unsigned short)native && done==dialog.done);
        ++cases;
    }
    check(uc_close(u)); printf("Original speed dialog: %u full-word key/clamp/exit comparisons pass\n",cases); return 0;
}
