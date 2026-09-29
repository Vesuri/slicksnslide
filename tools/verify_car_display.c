#define main options_verifier_main
#include "verify_options_menu.c"
#undef main
#include "../src/game/car_display.h"

static short submitted;
static unsigned calls;
static void actor_update(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)address;(void)size;(void)context;
    uint16_t ss,sp;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss));
    check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    submitted=(short)readword(u,ss*16U+sp+20);
    ++calls;
}
static void compare(uc_engine *u,struct SlicksCarDisplay *native,short heading,
    signed char role,unsigned short ticks,unsigned driver)
{
    regs(u,0);
    uint16_t cs=0x1987;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs));
    word(u,0x8ef98,driver);word(u,0x8effe,ticks);
    word(u,0x3cbf0+0x681c+2*driver,(unsigned short)heading);
    check(uc_mem_write(u,0x3cbf0+0x4bc6+driver,&role,1));
    unsigned char state[2]={(unsigned char)native->direction,native->ticks};
    check(uc_mem_write(u,0x3cbf0+0x3068+54*driver,state,2));
    calls=0; submitted=-30000;
    check(uc_emu_start(u,0x23d9c,0x23e7a,0,150));
    uint16_t ip;check(uc_reg_read(u,UC_X86_REG_IP,&ip));
    if(cs*16U+ip!=0x23e7a || calls!=1) abort();
    check(uc_mem_read(u,0x3cbf0+0x3068+54*driver,state,2));
    slicks_car_display_step(native,heading,role,ticks);
    if(native->direction!=(signed char)state[0] || native->ticks!=state[1] ||
       native->frame!=submitted) {
        fprintf(stderr,"display role=%d heading=%d ticks=%u: frame %d/%d state %d,%u/%d,%u\n",
            role,heading,ticks,native->frame,submitted,native->direction,native->ticks,
            (signed char)state[0],state[1]);abort();
    }
}
int main(void)
{
    unsigned char runtime[300000];FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    size_t size=fread(runtime,1,sizeof runtime,f);fclose(f);
    if(size<200000 || size==sizeof runtime)return 2;
    uc_engine *u;check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));check(uc_mem_write(u,0x10100,runtime,size));
    /* Stub only the actor device call; inspect the frame actually submitted
     * between the original pre-draw and post-draw state updates. */
    unsigned char retf=0xcb;check(uc_mem_write(u,0x32ef2,&retf,1));
    uc_hook hook;check(uc_hook_add(u,&hook,UC_HOOK_CODE,actor_update,0,0x32ef2,0x32ef2));
    unsigned cases=0;
    for(unsigned role=0;role<3;++role) for(unsigned counter=0;counter<256;++counter)
    for(unsigned ticks=0;ticks<256;++ticks) {
        struct SlicksCarDisplay state={3,(unsigned char)counter,0};
        compare(u,&state,4800,(signed char[]){-1,0,1}[role],ticks,(counter+ticks)%4);
        ++cases;
    }
    for(unsigned heading=0;heading<65536;++heading) {
        struct SlicksCarDisplay state={(signed char)(heading>>8),(unsigned char)heading,0};
        compare(u,&state,(short)heading,(signed char)heading,(unsigned short)(heading*37U),heading%4);
        ++cases;
    }
    /* Repeated changes and returns to the held direction retain the partial
     * counter. Threshold crossing publishes the prior frame one final time. */
    for(unsigned role=0;role<256;++role) {
        struct SlicksCarDisplay state={-1,0,0};
        for(unsigned step=0;step<64;++step) {
            compare(u,&state,(short)((step/3+step%2)%16*1200),(signed char)role,
                (unsigned short)(step%7),role%4);
            ++cases;
        }
    }
    check(uc_close(u));
    printf("Original car display: %u comparisons pass, including submitted frame, signed/wrapping timers and persistent sequences\n",cases);
    return 0;
}
