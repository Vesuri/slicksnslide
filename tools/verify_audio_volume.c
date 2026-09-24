#define main configuration_verifier_main
#include "verify_configuration.c"
#undef main
#include "../src/game/audio_volume.h"

int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,n));
    /* No driver/mixer callback: compare the original master state and
     * background sample gain before the independent Paula boundary. */
    unsigned char zero=0; check(uc_mem_write(u,0x3cbf0+0x17bd,&zero,1));
    for(unsigned value=0;value<65536;++value) {
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,bp=0xf100;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        word(u,0x8f000,0); word(u,0x8f002,0x9000); word(u,0x8f004,value);
        check(uc_emu_start(u,0x392ef,0x90000,0,100));
        unsigned char master; check(uc_mem_read(u,0x3cbf0+0x17c0,&master,1));
        struct SlicksConfiguration config={0}; config.options[1]=(short)value;
        if(master!=slicks_configuration_volume(&config)) abort();
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        word(u,0x8f10a,value);
        check(uc_emu_start(u,0x39bf0,0x39c00,0,100));
        unsigned char gain; check(uc_mem_read(u,0x3cbf0+0x1862,&gain,1));
        if(gain!=slicks_background_gain((short)value)) abort();
    }
    for(unsigned master=0;master<=100;++master) for(unsigned gain=0;gain<=255;++gain) {
        unsigned volume=slicks_paula_volume(master,gain);
        if(volume>64 || volume!=(64U*master*gain)/25500U) abort();
        if(master && volume<slicks_paula_volume(master-1,gain)) abort();
        if(gain && volume<slicks_paula_volume(master,gain-1)) abort();
    }
    check(uc_close(u));
    puts("Audio volume: 65536 original master/background argument pairs match; 25856 Paula gain mappings bounded and monotonic");
    return 0;
}
