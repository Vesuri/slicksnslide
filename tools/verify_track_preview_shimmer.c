#define main track_info_verifier_main
#include "verify_track_info.c"
#undef main
static unsigned char original[64000];
static void plot(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)address; (void)size; (void)context;
    uint16_t ss,sp,cs,ip; check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=16U*ss+sp,x=get(u,stack+4),y=get(u,stack+6);
    REQUIRE(x>=245 && x<309 && y>=20 && y<60);
    original[mult320[y]+x]=(unsigned char)get(u,stack+8);
    ip=get(u,stack); cs=get(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    static unsigned char runtime[300000],pixels[64000],preview[2560],bank[2564],palette[768],base[768];
    FILE *f=fopen("disasm/runtime.bin","rb"); REQUIRE(f);
    size_t bytes=fread(runtime,1,sizeof runtime,f); fclose(f); REQUIRE(bytes && bytes<sizeof runtime);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,bytes));
    uc_hook h; check(uc_hook_add(u,&h,UC_HOOK_CODE,plot,0,0x3b55e,0x3b55e));
    bank[0]=16; bank[1]=40;
    for(unsigned y=0;y<40;++y) for(unsigned x=0;x<64;++x)
        bank[2+(x&3)*640+y*16+x/4]=preview[y*64+x]=(unsigned char)(x*7+y*11);
    check(uc_mem_write(u,0x50000,bank,sizeof bank));
    word(u,0x8eefc,0); word(u,0x8eefe,0x5000);
    word(u,0x3cbf0+0x71b8,0); word(u,0x3cbf0+0x71ba,0x6700);
    word(u,0x3cbf0+0x68ae,0); word(u,0x3cbf0+0x68b0,0x6800);
    word(u,0x3cbf0+0x1d87,0);
    unsigned cases=0;
    for(unsigned variant=0;variant<8;++variant) {
        for(unsigned i=0;i<768;++i) { palette[i]=(unsigned char)((i*11+variant*7)%64); base[i]=(unsigned char)((i*17+variant*31)%256); }
        check(uc_mem_write(u,0x67000,palette,sizeof palette)); check(uc_mem_write(u,0x68000,base,sizeof base));
        unsigned long seed=(0xabcdef01UL*variant)&0xffffffffUL;
        word(u,0x3cbf0+0x2aaa,seed); word(u,0x3cbf0+0x2aac,seed>>16);
        memset(pixels,0xa5,sizeof pixels); memset(original,0xa5,sizeof original);
        struct SlicksChunkyUi ui={pixels,palette,0,0};
        for(unsigned iteration=0;iteration<128;++iteration) {
            uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xee00,bp=0xef00,ip;
            check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
            check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
            check(uc_emu_start(u,0x26a53,0x26b6b,0,1000000)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
            REQUIRE(ip==0x26b6b-0x266c0);
            slicks_track_preview_shimmer(&ui,preview,base,&seed);
            unsigned long actual=get(u,0x3cbf0+0x2aaa)|((unsigned long)get(u,0x3cbf0+0x2aac)<<16);
            REQUIRE(seed==actual && !memcmp(pixels,original,sizeof pixels)); ++cases;
        }
    }
    check(uc_close(u)); printf("Original preview shimmer: %u sequential full-screen/RNG comparisons with distinct source/output palettes and byte wrapping pass\n",cases); return 0;
}
