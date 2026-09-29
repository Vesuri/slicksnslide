#define main palette_verifier_main
#include "verify_palette_remap.c"
#undef main
#include "../src/game/race_runtime.c"

struct DemoNative { struct NativeText text; struct SlicksChunkyUi ui; };
static unsigned char demo_nearest(void *context,unsigned char r,unsigned char g,unsigned char b)
{ struct DemoNative *p=context; return slicks_ui_nearest(&p->ui,r,g,b); }
static void demo_colour(void *context,unsigned char index,unsigned char value)
{ struct DemoNative *p=context; p->text.fonts[0][6+index]=value; }
static void demo_text(void *context,const unsigned char *label,short x,short y,unsigned char flags)
{ struct DemoNative *p=context; native_text(&p->text,0,label,x,y,flags); }

/* Execute the complete original overlay, palette lookup and font painter.
 * Only VGA port/memory accesses are adapted to a visible pixel array. */
int main(void)
{
    unsigned char runtime[300000],resource[8192],palette[768],pixels[64000];
    unsigned char before[64000],font[sizeof ((struct SlicksRaceFont *)0)->runtime],logical[262144];
    FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t size=fread(runtime,1,sizeof runtime,f); fclose(f);
    if(size<200000 || size==sizeof runtime) return 2;
    long loaded=host_archive_load("ref/SLICKS.000","kirj.@f",resource,sizeof resource);
    if(loaded<0) return 2;
    static struct SlicksRaceRuntime race;
    if(slicks_race_add_font(&race,resource,(unsigned long)loaded)) abort();
    memcpy(font,race.font.runtime,sizeof font);
    race.chunky=pixels; race.race_mode=5;
    unsigned char code[8192],native_pixels[64000],native_font[sizeof font];
    f=fopen("build/font_string_test.bin","rb"); if(!f) return 2;
    size_t code_size=fread(code,1,sizeof code,f); fclose(f);
    if(code_size<20 || code_size==sizeof code) return 2;
    struct DemoNative native={0};
    check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&native.text.cpu));
    check(uc_ctl_set_cpu_model(native.text.cpu,UC_CPU_M68K_M68020));
    check(uc_mem_map(native.text.cpu,0,0x400000,UC_PROT_ALL));
    check(uc_mem_write(native.text.cpu,0,code,code_size));
    native.text.bridge=(unsigned)code[4]<<24|(unsigned)code[5]<<16|(unsigned)code[6]<<8|code[7];
    native.text.spacing=1; native.text.tab=10; native.text.shadow=0x0100;
    native.text.pixels=native_pixels; native.text.fonts[0]=native_font; native.text.sizes[0]=sizeof font;
    native.ui.palette=palette;
    const struct SlicksDemoOverlayOps native_ops={demo_nearest,demo_colour,demo_text,&native};
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,size));
    struct Vga v={0}; uc_hook h;
    check(uc_hook_add(u,&h,UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,vga_access,&v,0xa0000,0xaffff));
    check(uc_hook_add(u,&h,UC_HOOK_INSN,font_port,&v,1,0,UC_X86_INS_OUT));
    const unsigned dsbase=0x3cbf0;
    word(u,dsbase+0x680,0); word(u,dsbase+0x682,0x6000);
    word(u,dsbase+0x1d7b,100); word(u,dsbase+0x1d87,0);
    word(u,dsbase+0x1d8d,0); word(u,dsbase+0x1d8f,200);
    word(u,dsbase+0x1d91,0); word(u,dsbase+0x1d93,79);
    word(u,dsbase+0x71b8,0x71bc); word(u,dsbase+0x71ba,0x3cbf);
    unsigned cases=0;
    for(unsigned pattern=0;pattern<8;++pattern) for(unsigned flag=128;flag<256;++flag) {
        for(unsigned i=0;i<768;++i) palette[i]=(unsigned char)((i*13+(i>>3)+pattern*7)%64);
        slicks_race_set_status_palette(&race,palette);
        /* Mutation after binding proves lookups use the live palette. */
        palette[3]=palette[4]=palette[5]=pattern&1?50:0;
        check(uc_mem_write(u,dsbase+0x71bc,palette,sizeof palette));
        for(unsigned i=0;i<64000;++i) before[i]=pixels[i]=v.pixels[i]=(unsigned char)(i*17+(i>>7)+pattern);
        memset(logical,0xa5,sizeof logical);
        for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;++x)
            logical[(x&3)*65536+y*100+x/4]=pixels[y*320+x];
        memcpy(race.font.runtime,font,sizeof font);
        check(uc_mem_write(u,0x60000,font,sizeof font));
        unsigned char value=(unsigned char)flag;
        check(uc_mem_write(u,dsbase+0x459,&value,1));
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x1f84d,0x1fb46,0,1000000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        if(cs*16U+ip!=0x1fb46) abort();
        slicks_race_clear_dirty_rows(&race);
        if(slicks_race_set_demo(&race,(signed char)flag,runtime+dsbase-0x10100+0xbff)) abort();
        draw_arcade_timer(&race,logical);
        unsigned char original_font[sizeof font]; check(uc_mem_read(u,0x60000,original_font,sizeof original_font));
        if(memcmp(pixels,v.pixels,sizeof pixels) || memcmp(race.font.runtime,original_font,sizeof original_font)) {
            fprintf(stderr,"Demo overlay mismatch pattern=%u flag=%u\n",pattern,flag);
            for(unsigned i=0;i<64000;++i) if(pixels[i]!=v.pixels[i]) {
                fprintf(stderr,"pixel %u,%u: native=%u DOS=%u\n",i%320,i/320,pixels[i],v.pixels[i]); break;
            }
            for(unsigned i=0;i<sizeof font;++i) if(race.font.runtime[i]!=original_font[i]) {
                fprintf(stderr,"font %u: native=%u DOS=%u\n",i,race.font.runtime[i],original_font[i]); break;
            }
            return 1;
        }
        unsigned covered=0;
        for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;++x) {
            unsigned at=y*320+x,hit=0;
            for(unsigned r=0;r<race.dirty_row_count;++r)
                if(x>=race.dirty_rows[r].left && x<race.dirty_rows[r].right &&
                   y>=race.dirty_rows[r].top && y<race.dirty_rows[r].bottom) hit=1;
            if(before[at]!=pixels[at] && !hit) { fprintf(stderr,"Uncovered pixel %u,%u\n",x,y); return 1; }
            if(hit && logical[(x&3)*65536+y*100+x/4]!=pixels[at]) { fprintf(stderr,"Logical mismatch %u,%u\n",x,y); return 1; }
            covered+=hit;
        }
        /* The race publisher aligns horizontally to C2P blocks. Still only
         * a narrow label rectangle, never a full-width row publication. */
        if(!covered || covered>64U*(race.font.height+1U)) { fprintf(stderr,"Unexpected area %u\n",covered); return 1; }
        memcpy(native_pixels,before,sizeof before); memcpy(native_font,font,sizeof font);
        if(!slicks_demo_overlay((signed char)flag,race.demo_label,&native_ops) ||
           memcmp(native_pixels,v.pixels,sizeof native_pixels) || memcmp(native_font,original_font,sizeof font)) {
            fprintf(stderr,"68020 demo font mismatch pattern=%u flag=%u\n",pattern,flag); return 1;
        }
        ++cases;
    }
    if(slicks_race_set_demo(&race,-1,(const unsigned char *)"123456789")!=-1 ||
       slicks_race_set_demo(&race,-1,0)!=-1 || slicks_race_set_demo(&race,0,0)) abort();
    race.race_mode=0;
    for(unsigned flag=0;flag<128;++flag) {
        memcpy(pixels,before,sizeof before); memcpy(race.font.runtime,font,sizeof font);
        slicks_race_clear_dirty_rows(&race);
        if(slicks_race_set_demo(&race,(signed char)flag,0)) abort();
        draw_arcade_timer(&race,0);
        if(memcmp(pixels,before,sizeof pixels) || memcmp(race.font.runtime,font,sizeof font) ||
           race.dirty_row_count) abort();
    }
    check(uc_close(u));
    check(uc_close(native.text.cpu));
    printf("Demo race renderer: %u original/host/68020 full-frame/font comparisons, live palettes, logical mirrors and bounded dirty coverage pass\n",cases);
    puts("Nonnegative flags: 128 production renderer bypass cases pass");
    return 0;
}
