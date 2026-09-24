#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/race_options.h"
#include "../src/gen/setup_defaults.h"
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void put(unsigned char *p,unsigned at,unsigned v) { p[at]=v; p[at+1]=v>>8; }
int main(void)
{
    unsigned char runtime[300000];
    FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t size=fread(runtime,1,sizeof runtime,f); int error=ferror(f); fclose(f);
    if(error || size<200000 || size==sizeof runtime) return 2;
    if(memcmp(slicks_original_mode_flags,runtime+0x3cbf0-0x10100+0x1157,6)) return 1;
    if(memcmp(slicks_original_item_capacity,runtime+0x3cbf0-0x10100+0x10a4,13)) return 1;
    /* The generated configuration is compiled here too; the configuration
     * test independently checks every field against the original startup. */
    const unsigned char *mode=runtime+0x3cbf0-0x10100+0x92;
    if((unsigned short)slicks_original_configuration.options[0]!=(unsigned)(mode[0]|mode[1]<<8)) return 1;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,size));
    static unsigned char initial[65536],expected[65536],actual[65536];
    unsigned cases=0;
    /* All signed fuel words in custom mode; all flag bytes in each preset.
     * Vary the other custom words independently, including signed extremes. */
    for(unsigned mode=0;mode<6;++mode)
    for(unsigned variant=0;variant<(mode==4?65536U:256U);++variant) {
        memset(initial,0xa5,sizeof initial);
        struct SlicksConfiguration config={0};
        for(unsigned i=0;i<15;++i) config.options[i]=(short)(variant*(2*i+1)+i*4093);
        config.options[0]=mode; config.options[9]=(short)variant;
        for(unsigned i=0;i<15;++i) put(initial,0x92+8*i,(unsigned short)config.options[i]);
        initial[0x1157+mode]=(unsigned char)variant;
        memcpy(expected,initial,sizeof expected);
        struct SlicksRaceOptions result;
        slicks_resolve_race_options(&result,&config,(unsigned char)variant);
        const short values[]={result.weapons_enabled,result.inventory_mode,result.fuel,
            result.damage,result.car_collisions,result.starting_cash,result.field_302c,
            result.field_302e,result.field_3030};
        for(unsigned i=0;i<9;++i) put(expected,0x3020+2*i,(unsigned short)values[i]);
        check(uc_mem_write(u,0x3cbf0,initial,sizeof initial));
        const unsigned char ret[]={0,0,0,0x70};
        check(uc_mem_write(u,0x8f000,ret,sizeof ret));
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x2bdd8,0x70000,0,1000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        check(uc_mem_read(u,0x3cbf0,actual,sizeof actual));
        if(ip || sp!=0xf004 || memcmp(actual,expected,sizeof actual)) {
            fprintf(stderr,"race options mismatch mode=%u variant=%u\n",mode,variant); return 1;
        }
        ++cases;
    }
    for(unsigned value=0;value<65536;++value) {
        unsigned char word[2]={(unsigned char)value,(unsigned char)(value>>8)};
        check(uc_mem_write(u,0x3cbf0+0x3028,word,2));
        uint16_t cs=0x1987,ds=0x3cbf,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        /* Stop after CMP/JNE, before either branch changes any other state. */
        check(uc_emu_start(u,0x22d2c,0x70000,0,2));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        unsigned expected=slicks_car_collisions_disabled((short)value)?0x94c3:0x94c6;
        if(ip!=expected) { fprintf(stderr,"collision option gate mismatch value=%u ip=%x\n",value,ip); return 1; }
        check(uc_mem_write(u,0x3cbf0+0x3020,word,2));
        check(uc_emu_start(u,0x1daaf,0x70000,0,2));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        if(ip!=(value?0x4249:0x4246)) { fprintf(stderr,"weapon option gate mismatch value=%u ip=%x\n",value,ip); return 1; }
    }
    puts("Original car-collision enable gate: all 65536 option words pass");
    puts("Original weapons enable gate: all 65536 option words and 13 capacity bytes pass");
    check(uc_close(u));
    printf("DOS race options: %u complete-routine comparisons pass, all signed fuel words and all preset flag bytes\n",cases);
    return 0;
}
