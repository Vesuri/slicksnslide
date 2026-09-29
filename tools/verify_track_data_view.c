#define main options_verifier_main
#include "verify_options_menu.c"
#undef main
#include "../src/ui/track_data_view.h"

static unsigned plots,dirty_calls;
static unsigned char expected[64000];
static void plot(uc_engine *u,uint64_t address,uint32_t size,void *opaque)
{
    (void)address;(void)size;(void)opaque;
    uint16_t sp,ss; check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    check(uc_reg_read(u,UC_X86_REG_SS,&ss));
    unsigned stack=16U*ss+sp;
    unsigned x=readword(u,stack+4),y=readword(u,stack+6),colour=readword(u,stack+8);
    if(x!=plots/190 || y!=plots%190 || colour>31) abort();
    expected[y*320+x]=(unsigned char)colour; ++plots;
}
static void dirty(void *p,short l,short t,short r,short b)
{
    (void)p;
    if(l || t || r!=320 || b!=190) abort();
    ++dirty_calls;
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb");
    if(!f)return 2;
    size_t size=fread(runtime,1,sizeof runtime,f); fclose(f);
    if(size<200000 || size==sizeof runtime)return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,size));
    /* Only intercept the final VGA plot boundary; the original complete
     * loops and material sampler run unchanged, including near-call wrap. */
    unsigned char retf=0xcb;
    check(uc_mem_write(u,0x3b55e,&retf,1));
    uc_hook hook; check(uc_hook_add(u,&hook,UC_HOOK_CODE,plot,0,0x3b55e,0x3b55e));
    static unsigned char raw[64000],packed[16000],lower[64000],upper[64000],pixels[64000];
    unsigned cases=0;
    for(unsigned pattern=0;pattern<4;++pattern) {
        for(unsigned i=0;i<64000;++i) raw[i]=(unsigned char)(i*(pattern*38+1)+(i/320)*71+pattern*67);
        for(unsigned i=0;i<16000;++i) packed[i]=(unsigned char)(i*73+pattern*59);
        for(unsigned i=0;i<64000;++i) {
            lower[i]=raw[i]>>3;
            upper[i]=(raw[i]&7)|(((packed[i/4]>>((i&3)*2))&3)<<3);
        }
        check(uc_mem_write(u,0x50000,raw,sizeof raw));
        check(uc_mem_write(u,0x60000,packed,sizeof packed));
        word(u,0x3cbf0+0x5b8,0); word(u,0x3cbf0+0x5ba,0x5000);
        word(u,0x3cbf0+0x5bc,0); word(u,0x3cbf0+0x5be,0x6000);
        for(unsigned layer=0;layer<2;++layer) {
            regs(u,0); uint16_t cs=0x1987; check(uc_reg_write(u,UC_X86_REG_CS,&cs));
            word(u,0x8ef8c,0x57+layer);
            memset(expected,0xa5,sizeof expected); memset(pixels,0xa5,sizeof pixels);
            plots=dirty_calls=0;
            check(uc_emu_start(u,0x23f87,0x23ff9,0,10000000));
            struct SlicksChunkyUi ui={.pixels=pixels,.dirty=dirty};
            if(slicks_track_data_view_draw(&ui,lower,upper,(unsigned char)layer) ||
                plots!=60800 || dirty_calls!=1 || memcmp(pixels,expected,sizeof pixels)) abort();
            ++cases;
        }
    }
    check(uc_close(u));
    printf("Original track data views: %u complete 320x190 plots match; column order, HUD preservation and dirty bounds pass\n",cases);
    return 0;
}
