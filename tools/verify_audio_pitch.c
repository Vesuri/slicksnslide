#define main configuration_verifier_main
#include "verify_configuration.c"
#undef main
#include "../src/game/audio_pitch.h"
#include "../src/game/audio_sample.h"

static void verify_sample_gain(uc_engine *u)
{
    for(unsigned gain=0;gain<256;++gain) for(unsigned value=0;value<256;++value) {
        uint16_t cs=0x3000,ss=0x8000,bp=0xf100,ax=value,dx;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_reg_write(u,UC_X86_REG_AX,&ax));
        word(u,0x8f112,gain);
        check(uc_emu_start(u,0x389e2,0x389fc,0,100));
        check(uc_reg_read(u,UC_X86_REG_DX,&dx));
        if((unsigned char)dx!=(unsigned char)slicks_sample_pcm(value,gain)) {
            fprintf(stderr,"Original sample gain mismatch gain=%u PCM=%u\n",gain,value);
            exit(1);
        }
    }
    puts("Original sample gain: all 65536 PCM/gain combinations pass");
}

/* Execute the whole live engine-update block, including its pan and driver
 * calls. A descending sequence checks the driver doesn't retain a high rate;
 * the voice's sample identity must remain unchanged at every speed. */
static void verify_engine_deceleration(uc_engine *u)
{
    const unsigned speeds[]={0,100,500,1000,2000,8500,4000,1000,500,100,0};
    unsigned cases=0;
    for(unsigned vehicle=0;vehicle<10;++vehicle) for(unsigned car=0;car<4;++car) {
        unsigned char role=1,kind=(unsigned char)vehicle,voice=(unsigned char)(car+1);
        check(uc_mem_write(u,0x3cbf0+0x4bc6+car,&role,1));
        check(uc_mem_write(u,0x3cbf0+0x4bc2+car,&kind,1));
        check(uc_mem_write(u,0x3cbf0+0x4c68+car,&voice,1));
        word(u,0x3cbf0+0x17bc,0x0100); word(u,0x3cbf0+0x17e8,0);
        word(u,0x3cbf0+0x17be,15000);
        word(u,0x3cbf0+0x752a+2*voice,17);
        word(u,0x3cbf0+0x74c8+2*17,11025);
        word(u,0x3cbf0+0x538c+4*car,car*10000);
        word(u,0x3cbf0+0x538e +4*car,0);
        for(unsigned s=0;s<sizeof speeds/sizeof speeds[0];++s) {
            uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xe000,bp=0xf100,ip;
            check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
            check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
            check(uc_reg_write(u,UC_X86_REG_BP,&bp));
            word(u,0x8f100-0x68,car);
            word(u,0x3cbf0+0x684e +4*car,speeds[s]); word(u,0x3cbf0+0x6850+4*car,0);
            check(uc_emu_start(u,0x22732,0x227b6,0,5000));
            check(uc_reg_read(u,UC_X86_REG_IP,&ip));
            unsigned rate=slicks_engine_frequency(vehicle,speeds[s]);
            if(ip!=0x227b6-0x19870 || readword(u,0x3cbf0+0x752a+2*voice)!=17 ||
               readword(u,0x3cbf0+0x7710+2*voice)!=rate*256U/15000U) {
                fprintf(stderr,"Engine update mismatch vehicle=%u car=%u speed=%u ip=%x\n",vehicle,car,speeds[s],ip);
                exit(1);
            }
            ++cases;
        }
    }
    printf("Original whole engine update: %u accelerating/decelerating cases retain sample identity and update pitch\n",cases);
}

/* Follow the frequency through the original Sound Blaster software-driver
 * wrapper, not just through the game's speed-to-frequency calculation.
 * This executes the original fixed-point step calculation; it is not an
 * audible DOSBox capture or an implementation of a production mixer. */
static void verify_driver_frequency(uc_engine *u)
{
    const unsigned rates[]={11025,15000,22050,44100};
    const unsigned speeds[]={0,237,500,1000,2000,4000,65535};
    unsigned cases=0;
    for(unsigned vehicle=0;vehicle<10;++vehicle)
    for(unsigned r=0;r<sizeof rates/sizeof rates[0];++r)
    for(unsigned s=0;s<sizeof speeds/sizeof speeds[0];++s) {
        uint16_t cs=0x3000,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
        unsigned frequency=slicks_engine_frequency(vehicle,speeds[s]);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));
        check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss));
        check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        word(u,0x8f004,1); word(u,0x8f006,frequency);
        word(u,0x3cbf0+0x17bc,0x0100); /* Sound Blaster enabled, not GUS. */
        word(u,0x3cbf0+0x17e8,0);
        word(u,0x3cbf0+0x17be,rates[r]);
        word(u,0x3cbf0+0x752c,0); /* voice 1 -> sample 0 */
        word(u,0x3cbf0+0x74c8,11025);
        word(u,0x3cbf0+0x7692,1000); /* sample end/start positions */
        word(u,0x3cbf0+0x76d2,0);
        check(uc_emu_start(u,0x395b8,0x396a8,0,2000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        unsigned step=readword(u,0x3cbf0+0x7712);
        unsigned expected=frequency*256U/rates[r];
        if(ip!=0x96a8 || step!=expected) {
            fprintf(stderr,"Driver pitch mismatch vehicle=%u speed=%u rate=%u frequency=%u step=%u expected=%u ip=%x\n",
                vehicle,speeds[s],rates[r],frequency,step,expected,ip);
            exit(1);
        }
        ++cases;
    }
    printf("Original Sound Blaster pitch: %u driver step cases pass (no hidden pitch multiplier)\n",cases);
}
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
    verify_driver_frequency(u);
    verify_sample_gain(u);
    verify_engine_deceleration(u);
    check(uc_close(u));
    printf("Engine pitch: %u original x86 comparisons pass (all vehicles and 16-bit speeds)\n",cases);
    return 0;
}
