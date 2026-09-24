/* Test the production refresh orchestration with chapter loading/page drawing
 * replaced at the same boundaries in native and original code. Their text,
 * pixel and preprocessing implementations have separate differential gates. */
#define main options_verifier_main
#include "verify_options_menu.c"
#undef main
#include "../src/ui/help_renderer.h"
#include "../src/ui/help_navigation.h"
static void longword(uc_engine *u,unsigned at,unsigned long value)
{ word(u,at,value); word(u,at+2,value>>16); }
static unsigned long readlong(uc_engine *u,unsigned at)
{ return readword(u,at)|((unsigned long)readword(u,at+2)<<16); }
struct RefreshTrace { unsigned loads,draws; short page[2],selected[2],links; unsigned long chapter; };
static struct RefreshTrace native_trace,dos_trace;
static int refresh_load(const unsigned char *source,unsigned long size,unsigned long offset,
    unsigned char *out,unsigned long capacity,unsigned *length)
{
    (void)source; (void)size; (void)out; (void)capacity;
    ++native_trace.loads; native_trace.chapter=offset; *length=0; return 0;
}
static int refresh_page(struct SlicksHelpRenderer *r,const unsigned char *const *headers,
    unsigned count,const unsigned char *chapter,unsigned length,unsigned page,short *next)
{
    (void)headers; (void)count; (void)chapter; (void)length;
    unsigned at=native_trace.draws++; if(at>=2) abort();
    native_trace.page[at]=(short)page; native_trace.selected[at]=r->style.selected;
    r->style.total_links=native_trace.links; *next=3; return 0;
}
static void refresh_arrows(struct SlicksHelpRenderer *r,short page,short next)
{ (void)r; (void)page; (void)next; }
#define slicks_help_load_chapter refresh_load
#define slicks_help_renderer_page refresh_page
#define slicks_help_renderer_arrows refresh_arrows
#include "../src/ui/help_viewer.h"
#undef slicks_help_load_chapter
#undef slicks_help_renderer_page
#undef slicks_help_renderer_arrows
static void refresh_boundary(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size; (void)context; uint16_t ss,sp,cs,ip;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=16U*ss+sp;
    if(address==0x32048) {
        ++dos_trace.loads; dos_trace.chapter=readlong(u,stack+8);
    } else {
        unsigned at=dos_trace.draws++; if(at>=2) abort();
        dos_trace.page[at]=(short)readword(u,stack+4);
        dos_trace.selected[at]=(short)readword(u,0x3cbf0+0x6f8c);
        word(u,0x3cbf0+0x6ff4,(unsigned short)dos_trace.links);
        word(u,0x3cbf0+0x6ff2,3);
    }
    ip=readword(u,stack); cs=readword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t size=fread(runtime,1,sizeof runtime,f); int error=ferror(f); fclose(f);
    if(error || size<200000 || size==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,size));
    uc_hook hooks[2];
    check(uc_hook_add(u,&hooks[0],UC_HOOK_CODE,refresh_boundary,0,0x32048,0x32048));
    check(uc_hook_add(u,&hooks[1],UC_HOOK_CODE,refresh_boundary,0,0x32165,0x32165));
    const short selections[]={-32768,-1,0,1,6,127,32767},links[]={1,2,7};
    const unsigned char modes[]={0,1,2,127,128,255}; unsigned cases=0;
    static struct SlicksHelpViewer v;
    for(unsigned changed=0;changed<8;++changed) for(int redraw=-1;redraw<=1;++redraw)
    for(unsigned l=0;l<3;++l) for(unsigned m=0;m<6;++m) for(unsigned s=0;s<7;++s) {
        memset(&v,0,sizeof v); v.navigation.chapter=0x123456;
        v.loaded_chapter=changed&1?0x112233:v.navigation.chapter;
        v.navigation.page=2; v.drawn_page=changed&2?1:2;
        v.renderer.style.selected=selections[s]; v.drawn_selection=changed&4?3:selections[s];
        v.navigation.redraw=(signed char)redraw; v.renderer.style.link_mode=modes[m];
        v.renderer.style.total_links=links[l]; v.navigation.next_page=1;
        native_trace=dos_trace=(struct RefreshTrace){.links=links[l]};
        longword(u,0x8efda,v.navigation.chapter); longword(u,0x8efd4,v.loaded_chapter);
        word(u,0x8efd8,v.navigation.page); word(u,0x8efd2,v.drawn_page); word(u,0x8efd0,v.drawn_selection);
        check(uc_mem_write(u,0x8efcf,&v.navigation.redraw,1));
        check(uc_mem_write(u,0x3cbf0+0x6f8f,&modes[m],1));
        word(u,0x3cbf0+0x6f8c,v.renderer.style.selected);
        word(u,0x3cbf0+0x6ff4,links[l]); word(u,0x3cbf0+0x6ff2,1);
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        unsigned char actual_redraw=0;
        /* Native finishes the clamp's corrective redraw immediately; compare
         * the original after that same pending redraw, without a new key. */
        for(unsigned pass=0;pass<2;++pass) {
            check(uc_emu_start(u,0x32a35,0x32abc,0,10000));
            check(uc_reg_read(u,UC_X86_REG_IP,&ip)); if(ip!=0x49cc) abort();
            check(uc_mem_read(u,0x8efcf,&actual_redraw,1)); if(!actual_redraw) break;
        }
        if(slicks_help_viewer_refresh(&v) || memcmp(&native_trace,&dos_trace,sizeof native_trace) ||
           v.loaded_chapter!=readlong(u,0x8efd4) || v.drawn_page!=(short)readword(u,0x8efd2) ||
           v.drawn_selection!=(short)readword(u,0x8efd0) ||
           v.renderer.style.selected!=(short)readword(u,0x3cbf0+0x6f8c) ||
           v.navigation.next_page!=(short)readword(u,0x3cbf0+0x6ff2) || v.navigation.redraw!=(signed char)actual_redraw) {
            fprintf(stderr,"Help refresh mismatch changed=%u redraw=%d links=%d mode=%u selection=%d loads=%u/%u draws=%u/%u\n",
                changed,redraw,links[l],modes[m],selections[s],native_trace.loads,dos_trace.loads,native_trace.draws,dos_trace.draws); return 1;
        }
        ++cases;
    }
    check(uc_close(u)); printf("Original Help refresh: %u chapter/page/link/clamp state and call-order cases pass\n",cases); return 0;
}
