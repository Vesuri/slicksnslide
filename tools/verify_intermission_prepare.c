#define main palette_verifier_main
#include "verify_palette_remap.c"
#undef main
#include "../src/ui/intermission_prepare.h"
static void run_stage(uc_engine *u,unsigned start,unsigned end)
{
    check(uc_ctl_remove_cache(u,0,0xfffff)); check(uc_emu_start(u,start,end,0,20000000));
    uint16_t cs,ip,sp; check(uc_reg_read(u,UC_X86_REG_CS,&cs)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
    check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    if(cs*16U+ip!=end || sp!=0xeb00) abort();
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f); if(n<200000 || n==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n));
    struct Vga v={0}; uc_hook hook;
    check(uc_hook_add(u,&hook,UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,vga_access,&v,0xa0000,0xaffff));
    check(uc_hook_add(u,&hook,UC_HOOK_INSN,font_port,&v,1,0,UC_X86_INS_OUT));
    word(u,0x3cbf0+0x68ae,0); word(u,0x3cbf0+0x68b0,0x6500);
    word(u,0x3cbf0+0x1d7b,100); word(u,0x3cbf0+0x1d87,0);
    unsigned cases=0;
    for(unsigned pattern=0;pattern<4;++pattern) for(unsigned count=0;count<=4;++count) {
        unsigned char native[64000],palette[768],display_palette[768],table[256],dos_table[256];
        for(unsigned i=0;i<64000;++i) native[i]=v.pixels[i]=(unsigned char)(i*17+i/320+pattern*31);
        for(unsigned i=0;i<768;++i) { palette[i]=(unsigned char)((i*13+pattern*23)%64); display_palette[i]=(unsigned char)((i*17+1)%64); }
        check(uc_mem_write(u,0x65000,palette,768));
        unsigned char participants=(unsigned char)count; check(uc_mem_write(u,0x3cbf0+0x4c16,&participants,1));
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        struct Dirty bounds={0}; struct SlicksChunkyUi ui={native,display_palette,dirty,&bounds};
        if(slicks_intermission_prepare_panel(&ui,palette,table,count)) abort();
        run_stage(u,0x241f7,0x2423f);
        check(uc_mem_read(u,0x8eee8,dos_table,256));
        const short main_bounds[]={80,60,275,(short)(120+10*count)};
        if(memcmp(table,dos_table,256) || memcmp(native,v.pixels,64000) || bounds.count!=1 || memcmp(bounds.bounds,main_bounds,sizeof main_bounds)) {
            fprintf(stderr,"intermission panel mismatch pattern=%u count=%u\n",pattern,count); return 1;
        }
        if(slicks_intermission_prepare_preview(&ui,table)) abort();
        run_stage(u,0x243c9,0x243e9);
        const short preview_bounds[]={20,30,95,85};
        if(memcmp(native,v.pixels,64000) || bounds.count!=2 || memcmp(bounds.bounds,preview_bounds,sizeof preview_bounds)) {
            fprintf(stderr,"intermission preview surround mismatch pattern=%u count=%u\n",pattern,count); return 1;
        }
        ++cases;
    }
    check(uc_close(u));
    printf("Original intermission preparation: %u full-screen/table/dirty-bounds comparisons pass\n",cases);
    return 0;
}
