#define main palette_verifier_main
#include "verify_palette_remap.c"
#undef main
#include "../src/game/race_runtime.c"
#include "../src/ui/loading_presentation.h"

struct DemoNative { struct NativeText text;struct SlicksChunkyUi ui; };
static unsigned char demo_nearest(void *context,unsigned char r,unsigned char g,unsigned char b)
{ struct DemoNative *p=context;return slicks_ui_nearest(&p->ui,r,g,b); }
static void demo_colour(void *context,unsigned char index,unsigned char value)
{ struct DemoNative *p=context;p->text.fonts[0][6+index]=value; }
static void demo_text(void *context,const unsigned char *label,short x,short y,unsigned char flags)
{ struct DemoNative *p=context;native_text(&p->text,0,label,x,y,flags); }

static void loading_tint(void *context,short l,short t,short r,short b,
    unsigned char red,unsigned char green,unsigned char blue,short percent)
{
    struct DemoNative *p=context;unsigned char table[256];
    slicks_ui_tint_table(p->ui.palette,table,red,green,blue,percent);
    if(slicks_ui_remap(&p->ui,l,t,r,b,table)) abort();
}

int main(void)
{
    unsigned char runtime[300000],resource[8192],palette[768],code[8192];
    static unsigned char pixels[64000],font[sizeof ((struct SlicksRaceFont *)0)->runtime];
    FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    size_t n=fread(runtime,1,sizeof runtime,f);fclose(f);if(n<200000||n==sizeof runtime)return 2;
    f=fopen("build/font_string_test.bin","rb");if(!f)return 2;
    size_t code_size=fread(code,1,sizeof code,f);fclose(f);if(code_size<20||code_size==sizeof code)return 2;
    long loaded=host_archive_load("ref/SLICKS.000","kirj.@f",resource,sizeof resource);
    static struct SlicksRaceRuntime race;
    if(loaded<0 || slicks_race_add_font(&race,resource,(unsigned long)loaded))return 2;
    struct DemoNative native={0};
    check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&native.text.cpu));
    check(uc_ctl_set_cpu_model(native.text.cpu,UC_CPU_M68K_M68020));
    check(uc_mem_map(native.text.cpu,0,0x400000,UC_PROT_ALL));
    check(uc_mem_write(native.text.cpu,0,code,code_size));
    native.text.bridge=(unsigned)code[4]<<24|(unsigned)code[5]<<16|(unsigned)code[6]<<8|code[7];
    native.text.spacing=1;native.text.tab=10;native.text.shadow=0x0100;
    native.text.pixels=pixels;native.text.fonts[0]=font;native.text.sizes[0]=sizeof font;
    native.ui=(struct SlicksChunkyUi){pixels,palette,0,0};
    const struct SlicksLoadingPresentationOps ops={{demo_nearest,demo_colour,demo_text,&native},loading_tint};
    uc_engine *u;check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));check(uc_mem_write(u,0x10100,runtime,n));
    struct Vga v={0};uc_hook h;
    check(uc_hook_add(u,&h,UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,vga_access,&v,0xa0000,0xaffff));
    check(uc_hook_add(u,&h,UC_HOOK_INSN,font_port,&v,1,0,UC_X86_INS_OUT));
    unsigned dsbase=0x3cbf0;
    word(u,dsbase+0x680,0);word(u,dsbase+0x682,0x6000);
    word(u,dsbase+0x1d7b,100);word(u,dsbase+0x1d87,0);
    word(u,dsbase+0x1d8d,0);word(u,dsbase+0x1d8f,200);
    word(u,dsbase+0x1d91,0);word(u,dsbase+0x1d93,79);
    word(u,dsbase+0x71b8,0x71bc);word(u,dsbase+0x71ba,0x3cbf);
    /* Signed-flag classification has an exhaustive branch oracle already.
     * Exercise its boundaries here; palette searches make pixel runs costly. */
    const unsigned char flags[]={0,1,127,128,255};unsigned cases=0;
    for(unsigned pattern=0;pattern<2;++pattern)for(unsigned k=0;k<sizeof flags;++k) {
        for(unsigned i=0;i<768;++i)palette[i]=(unsigned char)((i*13+(i>>3)+pattern*7)%64);
        for(unsigned i=0;i<64000;++i)pixels[i]=v.pixels[i]=(unsigned char)(i*17+(i>>7)+pattern);
        memcpy(font,race.font.runtime,sizeof font);
        check(uc_mem_write(u,0x60000,font,sizeof font));
        check(uc_mem_write(u,dsbase+0x71bc,palette,sizeof palette));
        unsigned char flag=flags[k];
        check(uc_mem_write(u,dsbase+0x459,&flag,1));
        const unsigned char *track=(const unsigned char *)(pattern?"F1-TEST8":"BASIC");
        check(uc_mem_write(u,0x70000,track,strlen((const char *)track)+1));
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,bp=0xf000,sp=0xef00;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        word(u,0x8f006,0);word(u,0x8f008,0x7000);
        check(uc_emu_start(u,0x1b488,0x1b58f,0,20000000));
        uint16_t ip;check(uc_reg_read(u,UC_X86_REG_CS,&cs));check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        if((unsigned)cs*16+ip!=0x1b58f) {
            fprintf(stderr,"Original loading caller did not reach its boundary\n");return 1;
        }
        unsigned char caption[88],original_font[sizeof font];
        check(uc_mem_read(u,0x8efa8,caption,sizeof caption));
        check(uc_mem_read(u,0x60000,original_font,sizeof original_font));
        unsigned char native_caption[88];
        if(slicks_loading_caption(native_caption,sizeof native_caption,track,
            runtime+dsbase-0x10100+0x99c) || strcmp((const char *)native_caption,(const char *)caption)) abort();
        slicks_loading_presentation((signed char)flag,native_caption,runtime+dsbase-0x10100+0x9a0,&ops);
        if(memcmp(pixels,v.pixels,sizeof pixels)||memcmp(font,original_font,sizeof font)) {
            fprintf(stderr,"Loading mismatch pattern=%u flag=%u\n",pattern,flag);
            for(unsigned i=0;i<64000;++i)if(pixels[i]!=v.pixels[i]) {
                fprintf(stderr,"pixel %u,%u native=%u DOS=%u\n",i%320,i/320,pixels[i],v.pixels[i]);break;
            }
            return 1;
        }
        ++cases;
    }
    check(uc_close(u));check(uc_close(native.text.cpu));
    unsigned char guard[8];memset(guard,0xa5,sizeof guard);
    if(slicks_loading_caption(guard,sizeof guard,(const unsigned char *)"12345678",
        (const unsigned char *)"")!=-1 || slicks_loading_caption(guard,sizeof guard,
        (const unsigned char *)"1234",(const unsigned char *)"5678")!=-1) abort();
    for(unsigned i=0;i<sizeof guard;++i)if(guard[i]!=0xa5)abort();
    printf("Loading presentation: %u complete original/68020 pixel and font comparisons pass\n",cases);
    return 0;
}
