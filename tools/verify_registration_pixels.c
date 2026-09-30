#define main palette_verifier_main
#include "verify_palette_remap.c"
#undef main
#include "../src/ui/registration_ui.h"
static void registration_text(void *p,const unsigned char *s,short x,short y,unsigned char flags)
{ native_text(p,0,s,x,y,flags); }
static void dump_trial(const char *directory,const char *name,
    const unsigned char *pixels,const unsigned char *palette)
{
    char path[1024];
    if(snprintf(path,sizeof path,"%s/%s.ppm",directory,name)>=(int)sizeof path) abort();
    FILE *f=fopen(path,"wb");if(!f) abort();
    fprintf(f,"P6\n320 200\n255\n");
    for(unsigned i=0;i<64000;++i) for(unsigned c=0;c<3;++c) {
        unsigned char v=palette[3*pixels[i]+c];
        if(fputc((v<<2)|(v>>4),f)==EOF) abort();
    }
    if(fclose(f)) abort();
}
int main(int argc,char **argv)
{
    static unsigned char runtime[300000],source[70000],base[64000],pixels[64000],palette[768],font[8192];
    FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f);fclose(f);
    uc_engine *u;check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,bytes));
    long size=host_archive_load("ref/SLICKS.000","loading.bmp",source,sizeof source);unsigned w,h;
    if(size<0 || slicks_decode_menu_bitmap(source,size,base,palette,&w,&h,0) || w!=320 || h!=200)return 1;
    size=host_archive_load("ref/SLICKS.000","kirj.@f",source,sizeof source);
    long font_size=size<0?-1:slicks_decode_font_resource(source,size,font,sizeof font);if(font_size<0)return 1;
    f=fopen("build/font_string_test.bin","rb");if(!f)return 2;
    bytes=fread(source,1,sizeof source,f);fclose(f);
    struct NativeText n={0};check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&n.cpu));
    check(uc_ctl_set_cpu_model(n.cpu,UC_CPU_M68K_M68020));check(uc_mem_map(n.cpu,0,0x400000,UC_PROT_ALL));
    check(uc_mem_write(n.cpu,0,source,bytes));
    n.records_bridge=(unsigned)source[20]<<24|(unsigned)source[21]<<16|(unsigned)source[22]<<8|source[23];
    n.pixels=pixels;n.fonts[0]=font;n.sizes[0]=(unsigned)font_size;n.spacing=1;n.tab=10;n.shadow=0x100;
    struct Vga v={0};uc_hook hooks[2];
    check(uc_hook_add(u,&hooks[0],UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,vga_access,&v,0xa0000,0xaffff));
    check(uc_hook_add(u,&hooks[1],UC_HOOK_INSN,font_port,&v,1,0,UC_X86_INS_OUT));
    check(uc_mem_write(u,0x60000,font,font_size));word(u,0x3cbf0+0x680,0);word(u,0x3cbf0+0x682,0x6000);
    check(uc_mem_write(u,0x3cbf0+0x71bc,palette,sizeof palette));
    word(u,0x3cbf0+0x71b8,0x71bc);word(u,0x3cbf0+0x71ba,0x3cbf);
    word(u,0x3cbf0+0x1d7b,100);word(u,0x3cbf0+0x1d87,0);
    unsigned char text[6][64];const unsigned offsets[]={0xe11,0xe25,0xe41,0xe62,0xe83,0x85e};
    for(unsigned i=0;i<6;++i)check(uc_mem_read(u,0x3cbf0+offsets[i],text[i],64));
    memcpy(pixels,base,sizeof pixels);memcpy(v.pixels,base,sizeof base);
    struct SlicksChunkyUi ui={pixels,palette,0,0};
    slicks_registration_trial_background(&ui);font[6]=slicks_ui_nearest(&ui,70,70,70);
    slicks_registration_trial_text(text,0,registration_text,&n);
    uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xe000,bp=0xf000;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_SP,&sp));check(uc_reg_write(u,UC_X86_REG_BP,&bp));
    check(uc_emu_start(u,0x25fcb,0x260e7,0,30000000));
    if(memcmp(pixels,v.pixels,sizeof pixels)) { fputs("Trial pixels differ\n",stderr);return 1; }
    /* Optional local-only visual evidence from executing the original code,
     * before the prompt and before any synthetic owner-label test. */
    if(argc==2) {
        dump_trial(argv[1],"trial-original",v.pixels,palette);
        dump_trial(argv[1],"trial-native",pixels,palette);
    }
    slicks_registration_trial_text(text,1,registration_text,&n);
    check(uc_emu_start(u,0x260ef,0x26115,0,1000000));
    if(memcmp(pixels,v.pixels,sizeof pixels)) { fputs("Trial prompt pixels differ\n",stderr);return 1; }
    puts("Original trial screen: complete tint/text/prompt pixels match actual 68020 rendering");
    /* Synthetic display label, not a personal key or a generated valid key. */
    const unsigned char label[]="REGISTRATION TEST";
    check(uc_mem_write(u,0x3cbf0+0x62f,label,sizeof label));
    memcpy(pixels,base,sizeof pixels);memcpy(v.pixels,base,sizeof base);
    check(uc_mem_write(u,0x60000,font,font_size));
    native_text(&n,0,label,310,190,2);
    cs=0x266c;check(uc_reg_write(u,UC_X86_REG_CS,&cs));
    check(uc_emu_start(u,0x29fb3,0x29fef,0,1000000));
    if(memcmp(pixels,v.pixels,sizeof pixels)) { fputs("Owner label pixels differ\n",stderr);return 1; }
    puts("Original registered-owner label: right-aligned full-frame comparison passes");
    check(uc_close(u));check(uc_close(n.cpu));return 0;
}
