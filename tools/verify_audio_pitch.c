#define main configuration_verifier_main
#include "verify_configuration.c"
#undef main
#include "../src/game/audio_pitch.h"
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,n));
    unsigned cases=0;
    for(unsigned vehicle=0;vehicle<10;++vehicle) for(unsigned speed=0;speed<65536;++speed) {
        uint16_t cs=0x2000,ds=0x3cbf,ss=0x8000,sp=0xf000,bp=0xf100,frequency;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        word(u,0x8f100-0x3a,vehicle); word(u,0x8f100-0x68,0);
        word(u,0x3cbf0+0x684e,speed); word(u,0x3cbf0+0x6850,0);
        check(uc_emu_start(u,0x22776,0x227a5,0,200));
        check(uc_reg_read(u,UC_X86_REG_DX,&frequency));
        if(frequency!=slicks_engine_frequency(vehicle,speed)) {
            fprintf(stderr,"Pitch mismatch vehicle=%u speed=%u DOS=%u native=%u\n",
                vehicle,speed,frequency,slicks_engine_frequency(vehicle,speed)); return 1;
        }
        ++cases;
    }
    check(uc_close(u));
    printf("Engine pitch: %u original x86 comparisons pass (all vehicles and 16-bit speeds)\n",cases);
    return 0;
}
