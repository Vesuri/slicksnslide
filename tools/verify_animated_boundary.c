#define main damage_oracle_main
#include "verify_dos_damage.c"
#undef main
#include "../src/game/animated_boundary.h"
struct PaletteWrites { unsigned count;unsigned char rgb[15]; };
static void palette_call(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)address;(void)size;struct PaletteWrites *p=context;
    uint16_t ss,sp,cs,ip;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss));check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned at=ss*16U+sp,index=(readword(u,at+4)&255)-199;
    if(index>=5)exit(1);
    for(unsigned c=0;c<3;++c)p->rgb[index*3+c]=readword(u,at+6+c*2);
    ++p->count;ip=readword(u,at);cs=readword(u,at+2);sp+=4;
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_IP,&ip));
}
int main(void)
{
    static unsigned char runtime[300000];FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    size_t n=fread(runtime,1,sizeof runtime,f);fclose(f);
    uc_engine *u;check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n));struct PaletteWrites writes;uc_hook hook;
    check(uc_hook_add(u,&hook,UC_HOOK_CODE,palette_call,&writes,0x3703a,0x3703a));
    short level=5,timer=120,direction=1;unsigned long seed=0x1234;unsigned char rgb[15];
    word(u,0x64c6c,level);word(u,0x807a0,timer);word(u,0x8079e,direction);dword(u,0x62aaa,seed);
    for(unsigned step=0;step<20000;++step) {
        unsigned ticks=step%7;uint16_t cs=0x1987,ds=0x6000,ss=0x8000,bp=0x800,sp=0x700,ip;
        memset(&writes,0,sizeof writes);word(u,0x807fe,ticks);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_BP,&bp));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x1fec8,0x1ff97,0,100000));check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        int changed=slicks_advance_boundary(&level,&timer,&direction,ticks,&seed,rgb);
        if(ip+0x19870!=0x1ff97 || level!=(short)readword(u,0x64c6c) || timer!=(short)readword(u,0x807a0) ||
           direction!=(short)readword(u,0x8079e) || seed!=(uint32_t)readdword(u,0x62aaa) ||
           writes.count!=(changed?5U:0U) || (changed && memcmp(rgb,writes.rgb,15))) {
            fprintf(stderr,"Boundary mismatch update %u\n",step);return 1;
        }
    }
    puts("Animated boundary: 20000 consecutive original updates match level, timer, RNG and all palette writes");
    check(uc_close(u));return 0;
}
