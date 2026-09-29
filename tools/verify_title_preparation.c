#define main palette_verifier_main
#include "verify_palette_remap.c"
#undef main
#include "../src/ui/title_background.h"

int main(void)
{
    static unsigned char runtime[300000],asset[64003],palette[768],pixels[64000];
    FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    size_t n=fread(runtime,1,sizeof runtime,f);fclose(f);
    if(n<200000 || n==sizeof runtime)return 2;
    if(host_archive_load("ref/SLICKS.000","mainmenu.@I",asset,sizeof asset)!=64003 ||
       asset[0]!=1 || asset[1]!=64 || asset[2]!=200)return 2;
    uc_engine *u;check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));check(uc_mem_write(u,0x10100,runtime,n));
    struct Vga v={0};uc_hook h;
    check(uc_hook_add(u,&h,UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,vga_access,&v,0xa0000,0xaffff));
    check(uc_hook_add(u,&h,UC_HOOK_INSN,font_port,&v,1,0,UC_X86_INS_OUT));
    word(u,0x3cbf0+0x68aa,0);word(u,0x3cbf0+0x68ac,0x6000);
    word(u,0x3cbf0+0x1d7b,100);word(u,0x3cbf0+0x1d87,0);
    word(u,0x3cbf0+0x1d8d,0);word(u,0x3cbf0+0x1d8f,200);
    word(u,0x3cbf0+0x1d91,0);word(u,0x3cbf0+0x1d93,79);
    unsigned changed=0;
    for(unsigned pattern=0;pattern<3;++pattern) {
        if(!pattern) {
            if(host_archive_load("ref/SLICKS.000","partII",palette,sizeof palette)!=768)return 2;
            memcpy(pixels,asset+3,sizeof pixels);
        } else {
            for(unsigned i=0;i<768;++i)palette[i]=(i*13+(i>>3)+pattern*7)%64;
            for(unsigned i=0;i<64000;++i)pixels[i]=(i*17+(i>>7)+pattern);
        }
        memcpy(v.pixels,pixels,sizeof pixels);check(uc_mem_write(u,0x60000,palette,sizeof palette));
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x261e8,0x26240,0,20000000));
        check(uc_reg_read(u,UC_X86_REG_CS,&cs));check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        if((unsigned)cs*16+ip!=0x26240)abort();
        if(!pattern)for(unsigned i=0;i<64000;++i)changed+=pixels[i]!=v.pixels[i];
        slicks_title_prepare_background(pixels,palette);
        for(unsigned i=0;i<64000;++i)if(pixels[i]!=v.pixels[i]) {
            fprintf(stderr,"Title preparation mismatch pattern=%u at %u,%u native=%u DOS=%u\n",
                pattern,i%320,i/320,pixels[i],v.pixels[i]);return 1;
        }
    }
    if(!changed)abort();
    uc_close(u);printf("Original title preparation: 3 full-screen comparisons pass; raw artwork differs at %u pixels\n",changed);
    return 0;
}
