/* Execute original 286 steering arithmetic, with no substituted arithmetic
 * services. This checks steering and damage yaw, not AI decisions. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/race_runtime.c"

static void check(uc_err error)
{
    if (error) { fprintf(stderr, "%s\n", uc_strerror(error)); exit(1); }
}
static void word(uc_engine *uc, unsigned address, unsigned value)
{
    uint8_t bytes[2] = {value, value >> 8};
    check(uc_mem_write(uc, address, bytes, 2));
}
static void verify_damage_yaw(uc_engine *uc, const unsigned char *block)
{
    static const long speeds[]={-1,0,29,30,31,65535,65536,2147483647};
    static const short damages[]={0,1,70,999,32767};
    static const unsigned ticks[]={1,2,45};
    static const short headings[]={0,19199,32760};
    unsigned cases=0;
    if(memcmp(block,"\x8b\x5e\x98\x6b\xdb\x36\x80\xbf\x57\x30",10)) {
        fprintf(stderr,"damage yaw block signature mismatch\n"); exit(1);
    }
    check(uc_mem_write(uc,0x10d58,block,0x4a));
    for(int sign=-1;sign<=1;++sign)
    for(unsigned s=0;s<sizeof speeds/sizeof *speeds;++s)
    for(unsigned d=0;d<sizeof damages/sizeof *damages;++d)
    for(unsigned t=0;t<sizeof ticks/sizeof *ticks;++t)
    for(unsigned h=0;h<sizeof headings/sizeof *headings;++h) {
        struct SlicksRaceCar car={0};
        car.damage_turn_sign=sign; car.damage[2]=damages[d];
        car.measured_speed=speeds[s]; car.heading=headings[h];
        uint16_t cs=0x1000,ds=0x2000,ss=0x4000,bp=0x800,sp=0x700,ip;
        uint8_t sign_byte=(uint8_t)sign,raw[2];
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
        check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
        check(uc_reg_write(uc,UC_X86_REG_BP,&bp));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        word(uc,0x40800-0x68,3); /* Check nonzero car-stride indexing. */
        word(uc,0x40800-2,ticks[t]);
        check(uc_mem_write(uc,0x23057+3*0x36,&sign_byte,1));
        word(uc,0x23053+3*0x36,damages[d]);
        word(uc,0x2684e + 3*4,(uint32_t)speeds[s]);
        word(uc,0x26850+3*4,(uint32_t)speeds[s]>>16);
        word(uc,0x2681c+3*2,headings[h]);
        check(uc_emu_start(uc,0x10d58,0x10da2,0,100));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        check(uc_mem_read(uc,0x2681c+3*2,raw,2));
        short expected=(short)(car.heading+damage_heading_delta(&car,ticks[t]));
        if(ip!=0xda2 || (int16_t)(raw[0]|raw[1]<<8)!=expected) {
            fprintf(stderr,"damage yaw mismatch sign=%d speed=%ld damage=%d ticks=%u heading=%d\n",
                    sign,speeds[s],damages[d],ticks[t],headings[h]); exit(1);
        }
        ++cases;
    }
    printf("DOS damage yaw: %u original-instruction cases matched\n",cases);
}
int main(int argc, char **argv)
{
    static unsigned char runtime[300000];
    static const unsigned char signature[] = {
        0x8b,0x5e,0x98,0x03,0xdb,0x8d,0x46,0xb8,0x03,0xd8,
        0x36,0x8b,0x07,0x8b,0x5e,0x98,0x03,0xdb,0x50,
        0x8b,0x87,0x3a,0x4c
    };
    static const short scales[] = {700,1000,850,1550,32760,-100};
    static const short penalties[] = {0,26,999,-25};
    static const short properties[] = {75,104,120,32767};
    static const unsigned ticks[] = {1,2,45};
    FILE *file = fopen(argc > 1 ? argv[1] : "disasm/runtime.bin", "rb");
    if (!file) { perror("runtime"); return 1; }
    size_t size = fread(runtime, 1, sizeof runtime, file);
    fclose(file);
    size_t start=0, matches=0;
    for (size_t i=0; i+0x69<=size; ++i)
        if (!memcmp(runtime+i, signature, sizeof signature) &&
            !memcmp(runtime+i+0x65, "\x29\x87\x1c\x68", 4)) {
            start=i; ++matches;
        }
    if (matches != 1 || runtime[start+0x4d] != 0xba ||
        memcmp(runtime+start+0x5d, "\xf7\x6e\xfe", 3)) {
        fprintf(stderr, "original left steering block not uniquely identified\n");
        return 1;
    }
    unsigned property_address =
        ((runtime[start+0x4e] | runtime[start+0x4f]<<8) << 4) + 0x6aee;
    uc_engine *uc;
    check(uc_open(UC_ARCH_X86, UC_MODE_16, &uc));
    check(uc_mem_map(uc, 0, 0x200000, UC_PROT_ALL));
    check(uc_mem_write(uc, 0x10c79, runtime+start, 0x60));
    unsigned cases=0, wide_mismatches=0;
    /* 255*7/5 is the largest AI input from the unsigned profile setting. */
    for (unsigned input=0; input<=357; ++input)
    for (unsigned s=0; s<sizeof scales/sizeof *scales; ++s)
    for (unsigned p=0; p<sizeof penalties/sizeof *penalties; ++p)
    for (unsigned c=0; c<sizeof properties/sizeof *properties; ++c)
    for (unsigned t=0; t<sizeof ticks/sizeof *ticks; ++t) {
        struct SlicksRaceCar car = {0};
        car.steering_scale=scales[s];
        car.damage[3]=penalties[p];
        car.steering_property=properties[c];
        uint16_t cs=0x1000, ds=0x2000, ss=0x4000, bp=0x800, sp=0x700;
        uint16_t ax, ip, final_sp;
        check(uc_reg_write(uc, UC_X86_REG_CS, &cs));
        check(uc_reg_write(uc, UC_X86_REG_DS, &ds));
        check(uc_reg_write(uc, UC_X86_REG_SS, &ss));
        check(uc_reg_write(uc, UC_X86_REG_BP, &bp));
        check(uc_reg_write(uc, UC_X86_REG_SP, &sp));
        word(uc, 0x40800-0x68, 0);
        word(uc, 0x40800-0x48, input);
        word(uc, 0x40800-2, ticks[t]);
        word(uc, 0x24c3a, car.steering_scale);
        word(uc, 0x23055, car.damage[3]);
        word(uc, property_address, car.steering_property);
        check(uc_emu_start(uc, 0x10c79, 0x10cd9, 0, 100));
        check(uc_reg_read(uc, UC_X86_REG_AX, &ax));
        check(uc_reg_read(uc, UC_X86_REG_IP, &ip));
        check(uc_reg_read(uc, UC_X86_REG_SP, &final_sp));
        short actual=steering_delta(&car, input, ticks[t]);
        long wide=(long)input * (car.steering_scale / 10);
        wide /= 155;
        wide *= 80 - car.damage[3] / 25;
        wide /= 100;
        wide *= car.steering_property;
        wide /= 50;
        wide *= ticks[t];
        wide_mismatches += (short)wide != (int16_t)ax;
        if (actual != (int16_t)ax || ip!=0xcd9 || final_sp!=sp) {
            fprintf(stderr, "steering mismatch input=%u scale=%d penalty=%d property=%d ticks=%u native=%d DOS=%d ip=%x\n",
                    input, scales[s], penalties[p], properties[c], ticks[t],
                    actual, (int16_t)ax, ip);
            return 1;
        }
        ++cases;
    }
    if (start+0x129>size) return 1;
    verify_damage_yaw(uc,runtime+start+0xdf);
    check(uc_close(uc));
    if (!wide_mismatches) {
        fprintf(stderr, "fixtures did not detect the previous wide-product bug\n");
        return 1;
    }
    printf("DOS 286 steering: %u original-instruction delta cases matched; previous wide arithmetic fails %u\n", cases, wide_mismatches);
    return 0;
}
