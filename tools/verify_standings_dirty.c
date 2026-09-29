#define SLICKS_HELP_PIXELS_LIBRARY
#include "verify_help_pixels.c"

int main(void)
{
    unsigned char resource[70000],font[8192],code[8192];
    long loaded=host_archive_load("ref/SLICKS.000","kirj.@f",resource,sizeof resource);
    long font_size=loaded<0?-1:slicks_decode_font_resource(resource,loaded,font,sizeof font);
    if(font_size<0) abort();
    FILE *f=fopen("build/font_string_test.bin","rb"); if(!f) return 2;
    size_t code_size=fread(code,1,sizeof code,f); fclose(f);
    struct HelpFontCpu n={0}; n.size=(unsigned)font_size; n.measure=be32(code+12);
    check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&n.cpu));
    check(uc_ctl_set_cpu_model(n.cpu,UC_CPU_M68K_M68020));
    check(uc_mem_map(n.cpu,0,0x400000,UC_PROT_ALL));
    check(uc_mem_write(n.cpu,0,code,code_size));
    uc_hook hook;
    check(uc_hook_add(n.cpu,&hook,UC_HOOK_MEM_WRITE,font_write,0,0x100000,0x100000+63999));
    const unsigned char *texts[]={(const unsigned char *)"",(const unsigned char *)"A",
        (const unsigned char *)"VESURI",(const unsigned char *)"-32768",
        (const unsigned char *)"A\bB\317C\rD\nE\377",font+6+font[5]};
    /* A dedicated glyph-zero string avoids reading the font's code table
     * as a terminated string. */
    unsigned char zero[]={font[6+font[5]],0}; texts[5]=zero;
    const short xs[]={-10,0,17,86,98,160,233,310,319};
    const short ys[]={-3,0,92,122,190,199};
    unsigned cases=0;
    for(unsigned t=0;t<6;++t) for(unsigned ix=0;ix<9;++ix)
        for(unsigned iy=0;iy<6;++iy) for(unsigned flags=0;flags<8;++flags) {
            short width=help_font_call(&n,font,texts[t],0,0,0,1);
            unsigned char bytes[32];
            uint32_t args[]={0x380000,0x100000,0x50000,0x60000,
                (unsigned short)xs[ix],(unsigned short)ys[iy],flags,37};
            for(unsigned i=0;i<8;++i) for(unsigned j=0;j<4;++j)
                bytes[i*4+j]=(unsigned char)(args[i]>>(24-8*j));
            uint32_t sp=0x300000,sr=0,pc;
            check(uc_reg_write(n.cpu,UC_M68K_REG_SR,&sr));
            check(uc_reg_write(n.cpu,UC_M68K_REG_A7,&sp));
            check(uc_mem_write(n.cpu,sp,bytes,sizeof bytes));
            memset(font_writes,0,sizeof font_writes);
            check(uc_emu_start(n.cpu,be32(code+24),0x380000,0,1000000));
            check(uc_reg_read(n.cpu,UC_M68K_REG_PC,&pc));
            if(pc!=0x380000) abort();
            memset(reported,0,sizeof reported);
            struct SlicksChunkyUi ui={0}; ui.dirty=report_bounds;
            slicks_font_text_dirty(&ui,font,texts[t],xs[ix],ys[iy],1,flags,width,1);
            unsigned area=0;
            for(unsigned p=0;p<64000;++p) {
                area+=reported[p];
                if(font_writes[p] && !reported[p]) {
                    fprintf(stderr,"Standings bounds miss (%u,%u): text=%u x=%d y=%d flags=%u\n",
                        p%320,p/320,t,xs[ix],ys[iy],flags); abort();
                }
            }
            if(t==1 && xs[ix]==98 && ys[iy]==92 && (!area || area>=320)) abort();
            ++cases;
        }
    check(uc_close(n.cpu));
    printf("Standings dirty bounds cover all native stores in %u strings, including alignment, shadow, tabs, newlines and clipping\n",cases);
    return 0;
}
