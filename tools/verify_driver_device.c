#define main options_verifier_main
#include "verify_options_menu.c"
#undef main
#include "../src/game/driver_device.h"
static void sample_boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)address;(void)size; ++*(unsigned *)p;
    uint16_t sp,ss,cs,ip; check(uc_reg_read(u,UC_X86_REG_SP,&sp)); check(uc_reg_read(u,UC_X86_REG_SS,&ss));
    unsigned char b[4]; check(uc_mem_read(u,16U*ss+sp,b,4));
    ip=b[0]|b[1]<<8; cs=b[2]|b[3]<<8; sp+=4;
    check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n)); uc_hook hook; unsigned polls=0,cases=0;
    check(uc_hook_add(u,&hook,UC_HOOK_CODE,sample_boundary,&polls,0x36a05,0x36a05));
    for(unsigned driver=0;driver<4;++driver) for(unsigned device=0;device<7;++device)
    for(int role=-1;role<=1;++role) for(unsigned variant=0;variant<96;++variant) {
        unsigned char control=(unsigned char)(variant&31),bytes[5];
        struct SlicksDriverDeviceState s={(short)(variant%3),(signed char)((variant%3)-1),(unsigned char)(variant>>1),0};
        struct SlicksDeviceSample sample={(signed char)((variant%3)-1),(signed char)((variant/3%3)-1),(unsigned char)(variant/9%4)};
        unsigned char v=device; check(uc_mem_write(u,0x3cbf0+0x5e2+driver,&v,1));
        v=(unsigned char)role; check(uc_mem_write(u,0x3cbf0+0x4bc6+driver,&v,1));
        word(u,0x3cbf0+0x6e4+2*driver,s.countdown); v=3; check(uc_mem_write(u,0x3cbf0+0x6ec,&v,1));
        v=(unsigned char)(variant&1); check(uc_mem_write(u,0x3cbf0+0x62e,&v,1)); word(u,0x3cbf0+0x3020,variant&2);
        v=(unsigned char)s.previous_axis; check(uc_mem_write(u,0x3cbf0+0x68be +driver,&v,1));
        for(unsigned i=0;i<5;++i) bytes[i]=(control>>i)&1;
        check(uc_mem_write(u,0x3cbf0+0x5344+5*driver,bytes,5));
        for(unsigned i=0;i<4;++i) bytes[i]=(s.previous>>i)&1;
        check(uc_mem_write(u,0x3cbf0+0x68d2+4*driver,bytes,4));
        if(device) {
            unsigned at=device-1;
            check(uc_mem_write(u,0x3cbf0+0x71a0+at,&sample.x,1));
            check(uc_mem_write(u,0x3cbf0+0x71a6+at,&sample.y,1));
            check(uc_mem_write(u,0x3cbf0+0x71ac+at,&sample.buttons,1));
        }
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        word(u,0x8f000,0); word(u,0x8f002,0x9000); word(u,0x8f004,driver); word(u,0x8f006,1);
        polls=0; check(uc_emu_start(u,0x199cf,0x90000,0,10000));
        int result=slicks_driver_device(&s,&control,device,(signed char)role,1,3,variant&1,variant&2,&sample);
        check(uc_mem_read(u,0x3cbf0+0x5344+5*driver,bytes,5));
        unsigned actual=0; for(unsigned i=0;i<5;++i) actual|=(unsigned)bytes[i]<<i;
        unsigned char previous; check(uc_mem_read(u,0x3cbf0+0x68be +driver,&previous,1));
        unsigned char timer[2]; check(uc_mem_read(u,0x3cbf0+0x6e4+2*driver,timer,2));
        check(uc_mem_read(u,0x3cbf0+0x68c2+4*driver,bytes,4));
        unsigned rising=0; for(unsigned i=0;i<4;++i) rising|=(unsigned)bytes[i]<<i;
        if(result!=(int)polls || actual!=control || previous!=(unsigned char)s.previous_axis ||
            (short)(timer[0]|timer[1]<<8)!=s.countdown || rising!=s.rising) {
            fprintf(stderr,"Device mismatch driver=%u device=%u role=%d variant=%u controls=%u/%u polls=%u/%d\n",driver,device,role,variant,actual,control,polls,result); return 1;
        }
        ++cases;
    }
    check(uc_close(u)); printf("Original driver-device polling: %u control/countdown/edge comparisons pass\n",cases); return 0;
}
