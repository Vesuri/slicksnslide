#define main options_verifier_main
#include "verify_options_menu.c"
#undef main

static unsigned event,shade;
static void boundary(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size;(void)context;
    if(address==0x1f901) { check(uc_emu_stop(u)); return; }
    if(address!=0x36fae && address!=0x2fe63 && address!=0x301ab) return;
    uint16_t sp,ss; check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); unsigned at=16U*ss+sp;
    unsigned pass=event/3,kind=event%3;
    if(event>=6) abort();
    if(kind==0) {
        unsigned rgb=pass?50:0;
        if(address!=0x36fae || readword(u,at+4)!=rgb ||
            readword(u,at+6)!=rgb || readword(u,at+8)!=rgb) abort();
        uint16_t ax=(uint16_t)(shade+pass);
        check(uc_reg_write(u,UC_X86_REG_AX,&ax));
    } else if(kind==1) {
        if(address!=0x2fe63 || readword(u,at+4)!=shade+pass ||
            readword(u,at+6)!=0x1234 || readword(u,at+8)!=0x7000 || readword(u,at+10)) abort();
    } else {
        if(address!=0x301ab || readword(u,at+4)!=(pass?10:11) ||
            readword(u,at+6)!=(pass?10:11) || readword(u,at+8)!=0xbff ||
            readword(u,at+10)!=0x3cbf || readword(u,at+12)!=0x1234 ||
            readword(u,at+14)!=0x7000 || readword(u,at+16)) abort();
    }
    ++event;
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f)return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f); if(n<200000 || n==sizeof runtime)return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,n));
    /* Intercept only renderer service boundaries. The actual signed-flag
     * helper, complete overlay branch and all call arguments execute. */
    unsigned char retf=0xcb;
    check(uc_mem_write(u,0x36fae,&retf,1));
    check(uc_mem_write(u,0x2fe63,&retf,1));
    check(uc_mem_write(u,0x301ab,&retf,1));
    uc_hook hook; check(uc_hook_add(u,&hook,UC_HOOK_CODE,boundary,0,1,0));
    word(u,0x3cbf0+0x680,0x1234); word(u,0x3cbf0+0x682,0x7000);
    unsigned cases=0;
    for(unsigned flag=0;flag<256;++flag) for(shade=0;shade<255;shade+=127) {
        regs(u,0); uint16_t cs=0x1987,ip; check(uc_reg_write(u,UC_X86_REG_CS,&cs));
        unsigned char value=(unsigned char)flag;
        check(uc_mem_write(u,0x3cbf0+0x459,&value,1)); event=0;
        check(uc_emu_start(u,0x1f84d,0x1fb46,0,1000));
        check(uc_reg_read(u,UC_X86_REG_CS,&cs)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        unsigned negative=flag>=128;
        if(event!=(negative?6U:0U) || cs*16U+ip!=(negative?0x1fb46U:0x1f901U)) abort();
        ++cases;
    }
    check(uc_close(u));
    printf("Original demo overlay: %u signed-flag/palette-result cases verify both ordered text passes and normal-owner bypass\n",cases);
    return 0;
}
