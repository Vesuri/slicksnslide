#define main palette_verifier_main
#include "verify_palette_remap.c"
#undef main
#include "../src/ui/help_renderer.h"
#include "../src/ui/help_viewer.h"
#include "../src/ui/help_text_dirty.h"

static unsigned char font_writes[64000],reported[64000];
static unsigned bound_checks;
static void font_write(uc_engine *u,uc_mem_type type,uint64_t address,int size,int64_t value,void *context)
{
    (void)u;(void)type;(void)value;(void)context;
    for(int i=0;i<size;++i) {
        if(address+i<0x100000 || address+i>=0x100000+64000) abort();
        font_writes[address+i-0x100000]=1;
    }
}
static void report_bounds(void *context,short l,short t,short r,short b)
{
    (void)context;
    for(int y=t;y<b;++y) for(int x=l;x<r;++x) reported[y*320+x]=1;
}

struct HelpFontCpu { uc_engine *cpu; unsigned size,measure,text; };
static short help_font_call(struct HelpFontCpu *n,const unsigned char *font,const unsigned char *text,
    struct SlicksChunkyUi *ui,short x,short y,signed char spacing)
{
    uc_engine *m=n->cpu;
    check(uc_mem_write(m,0x50000,font,n->size)); check(uc_mem_write(m,0x60000,text,strlen((const char *)text)+1));
    if(ui) check(uc_mem_write(m,0x100000,ui->pixels,64000));
    memset(font_writes,0,sizeof font_writes);
    uint32_t args[7]={0x380000,0x50000,0x60000,(unsigned short)spacing,0,0,0};
    unsigned count=4;
    if(ui) { count=7; args[1]=0x100000; args[2]=0x50000; args[3]=0x60000;
        args[4]=(unsigned short)x; args[5]=(unsigned short)y; args[6]=(unsigned short)spacing; }
    unsigned char bytes[28]; for(unsigned i=0;i<count;++i) for(unsigned j=0;j<4;++j) bytes[4*i+j]=(unsigned char)(args[i]>>(24-8*j));
    uint32_t sp=0x300000,sr=0,pc,result;
    check(uc_reg_write(m,UC_M68K_REG_SR,&sr)); check(uc_reg_write(m,UC_M68K_REG_A7,&sp)); check(uc_mem_write(m,sp,bytes,4*count));
    check(uc_emu_start(m,ui?n->text:n->measure,0x380000,0,1000000));
    check(uc_reg_read(m,UC_M68K_REG_PC,&pc)); check(uc_reg_read(m,UC_M68K_REG_A7,&sp)); check(uc_reg_read(m,UC_M68K_REG_D0,&result));
    if(pc!=0x380000 || sp!=0x300004) abort();
    if(ui) {
        check(uc_mem_read(m,0x100000,ui->pixels,64000));
        memset(reported,0,sizeof reported);
        struct SlicksChunkyUi bounds={0};bounds.dirty=report_bounds;
        slicks_help_text_dirty(&bounds,font,text,x,y,spacing);
        for(unsigned p=0;p<64000;++p) if(font_writes[p] && !reported[p]) {
            fprintf(stderr,"Help bounds miss native store at %u,%u\n",p%320,p/320);abort();
        }
        ++bound_checks;
    }
    return (short)result;
}
static short help_measure(void *context,const unsigned char *font,const unsigned char *text,signed char spacing)
{ return help_font_call(context,font,text,0,0,0,spacing); }
static short help_text(void *context,struct SlicksChunkyUi *ui,unsigned char *font,const unsigned char *text,short x,short y,signed char spacing)
{
    short advance=help_font_call(context,font,text,ui,x,y,spacing);
    slicks_help_text_dirty(ui,font,text,x,y,spacing);
    return advance;
}
static unsigned be32(const unsigned char *p) { return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3]; }
struct ViewerResource { const unsigned char *source; unsigned size,loads; };
static void viewer_chapter(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)address; (void)size; struct ViewerResource *r=context;
    uint16_t ss,sp,cs,ip; check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=16U*ss+sp,offset=getword(u,stack+8)|(getword(u,stack+10)<<16);
    unsigned char chapter[16384]={0}; unsigned length;
    if(slicks_help_load_chapter(r->source,r->size,offset,chapter,sizeof chapter,&length)) abort();
    check(uc_mem_write(u,0x70000,chapter,sizeof chapter)); ++r->loads;
    ip=getword(u,stack); cs=getword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void viewer_run(uc_engine *u,unsigned start,unsigned stop)
{
    check(uc_ctl_remove_cache(u,0,0xfffff)); check(uc_emu_start(u,start,stop,0,20000000));
    uint16_t cs,ip; check(uc_reg_read(u,UC_X86_REG_CS,&cs)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
    if(16U*cs+ip!=stop) { fprintf(stderr,"Viewer missed stop %x at %x:%x\n",stop,cs,ip); abort(); }
}
static void verify_viewer_entry(uc_engine *u,struct Vga *v,struct HelpFontCpu *font_cpu,
    const unsigned char *help,unsigned help_size,const unsigned char *base,
    unsigned char *palette,unsigned char *font,unsigned char *pixels)
{
    static struct SlicksHelpViewer native;
    struct ViewerResource resource={help,help_size,0}; uc_hook hook;
    check(uc_hook_add(u,&hook,UC_HOOK_CODE,viewer_chapter,&resource,0x32048,0x32048));
    const char *topics[]={"","options","players","tracks","reg","missing","FI_OPTIONS","main"};
    unsigned cases=0,dsbase=0x3cbf0;
    for(unsigned country=0;country<2;++country) for(unsigned topic=0;topic<8;++topic) {
        memset(&native,0,sizeof native); memcpy(native.source,help,help_size); native.source_size=help_size;
        struct SlicksHelpRenderer *r=&native.renderer;
        r->ui=(struct SlicksChunkyUi){pixels,palette,0,0}; r->font=font; r->context=font_cpu;
        r->measure=help_measure; r->text=help_text;
        memcpy(pixels,base,64000); memcpy(v->pixels,base,64000); font[6]=71;
        check(uc_mem_write(u,0x60000,font,font_cpu->size));
        if(slicks_help_viewer_open(&native,(const unsigned char *)topics[topic],country?358:0)) abort();
        /* Resource builder/reader have independent original-code gates.
         * Supply their results, but run original header-first drawing,
         * topic resolution, chapter/page refresh and arrow composition. */
        for(unsigned h=0;h<native.info.headers;++h) {
            check(uc_mem_write(u,0x50000+512*h,native.headers[h],200));
            word(u,dsbase+0x6f9a+4*h,512*h); word(u,dsbase+0x6f9c+4*h,0x5000);
        }
        unsigned char count=native.info.headers,one=1,two=2,zero=0;
        check(uc_mem_write(u,dsbase+0x6fea,&count,1)); check(uc_mem_write(u,0x56000,native.index,native.info.size));
        word(u,dsbase+0x6f90,0); word(u,dsbase+0x6f92,0x5600); word(u,dsbase+0x6f96,native.info.size);
        word(u,dsbase+0x6fee,native.info.body); word(u,dsbase+0x6ff0,native.info.body>>16);
        word(u,dsbase+0x6c1c,0); word(u,dsbase+0x6c1e,0x7000);
        word(u,dsbase+0x169c,20); word(u,dsbase+0x169e,15); word(u,dsbase+0x16a0,300); word(u,dsbase+0x16a2,185);
        word(u,dsbase+0x1720,country?358:0); word(u,dsbase+0x6f8c,0); word(u,dsbase+0x6ff2,0);
        check(uc_mem_write(u,dsbase+0x6f26,&two,1)); check(uc_mem_write(u,dsbase+0x6f8f,&one,1));
        check(uc_mem_write(u,dsbase+0x1604,&one,1)); check(uc_mem_write(u,dsbase+0x16a8,&zero,1));
        check(uc_mem_write(u,0x68000,topics[topic],strlen(topics[topic])+1));
        word(u,0x8f006,0); word(u,0x8f008,0x6800);
        word(u,0x8efd4,65535); word(u,0x8efd6,65535); word(u,0x8efd2,65535);
        word(u,0x8efd0,65535); check(uc_mem_write(u,0x8efcf,&one,1));
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        viewer_run(u,0x3284e,0x328c0); /* Original palette selection. */
        viewer_run(u,0x329bc,0x32a32); /* Header-first topic entry. */
        resource.loads=0;
        for(unsigned pass=0;pass<2;++pass) {
            viewer_run(u,0x32a35,0x32b5d);
            unsigned char redraw; check(uc_mem_read(u,0x8efcf,&redraw,1)); if(!redraw) break;
        }
        unsigned char original_font[8192],target[21];
        check(uc_mem_read(u,0x60000,original_font,font_cpu->size)); check(uc_mem_read(u,dsbase+0x6f63,target,21));
        unsigned long chapter=getword(u,0x8efda)|((unsigned long)getword(u,0x8efdc)<<16);
        if(resource.loads!=1 || memcmp(pixels,v->pixels,64000) || memcmp(font,original_font,font_cpu->size) ||
           native.navigation.chapter!=chapter || native.navigation.page!=(short)getword(u,0x8efd8) ||
           r->style.selected!=(short)getword(u,dsbase+0x6f8c) ||
           r->style.total_links!=(short)getword(u,dsbase+0x6ff4) ||
           !memchr(target,0,sizeof target) || !memchr(r->style.target,0,sizeof target) ||
           strcmp((const char *)target,(const char *)r->style.target)) {
            fprintf(stderr,"Help viewer entry mismatch topic=%s country=%u chapter=%lu/%lu loads=%u\n",topics[topic],country, native.navigation.chapter,chapter,resource.loads);
            fprintf(stderr,"page=%d/%d selected=%d/%d links=%d/%d font-diff=%d target=%s/%s\n",native.navigation.page,(short)getword(u,0x8efd8),r->style.selected,(short)getword(u,dsbase+0x6f8c),r->style.total_links,(short)getword(u,dsbase+0x6ff4),memcmp(font,original_font,font_cpu->size),r->style.target,target);
            for(unsigned i=0;i<64000;++i) if(pixels[i]!=v->pixels[i]) { fprintf(stderr,"pixel %u,%u native=%u DOS=%u\n",i%320,i/320,pixels[i],v->pixels[i]); break; }
            exit(1);
        }
        viewer_run(u,0x32ed3,0x32ee7); /* Original saved font-colour restore. */
        if(slicks_help_renderer_close(r) || memcmp(pixels,base,64000) || font[6]!=71) abort();
        check(uc_mem_read(u,0x60000,original_font,font_cpu->size));
        if(memcmp(font,original_font,font_cpu->size)) abort();
        ++cases;
    }
    check(uc_hook_del(u,hook));
    printf("Original Help viewer entry: %u topic/language full-frame/font/link comparisons and close-font restorations pass\n",cases);
}
static void verify_pages(uc_engine *u,struct Vga *v,struct HelpFontCpu *n,
    const unsigned char *help,unsigned help_size,const unsigned char *base,
    unsigned char *palette,unsigned char *font,unsigned char *pixels)
{
    unsigned char index[8192],header_storage[20][200]={{0}},chapter[16384],saved[64000];
    const unsigned char *headers[20]; struct SlicksHelpIndexInfo info;
    if(slicks_help_build_index(help,help_size,index,sizeof index,&info)) abort();
    unsigned dsbase=0x3cbf0;
    for(unsigned h=0;h<info.headers;++h) {
        memcpy(header_storage[h],help+info.header_start[h],info.header_length[h]);
        if(slicks_help_preprocess(header_storage[h],200)) abort();
        headers[h]=header_storage[h]; check(uc_mem_write(u,0x50000+512*h,headers[h],200));
        word(u,dsbase+0x6f9a+4*h,512*h); word(u,dsbase+0x6f9c+4*h,0x5000);
    }
    unsigned char header_count=(unsigned char)info.headers;
    check(uc_mem_write(u,dsbase+0x6fea,&header_count,1));
    word(u,dsbase+0x6c1c,0); word(u,dsbase+0x6c1e,0x7000);
    unsigned cases=0;
    for(unsigned at=0;at<info.size;) {
        unsigned kind=index[at++];
        if(!kind) { while(index[at]) ++at; ++at; continue; }
        unsigned offset=index[at]|index[at+1]<<8|index[at+2]<<16; at+=3;
        if(kind!=1 || at==info.size) continue;
        memset(chapter,0,sizeof chapter); unsigned length;
        if(slicks_help_load_chapter(help,help_size,offset,chapter,sizeof chapter,&length)) abort();
        unsigned pages=1; for(unsigned i=0;i<length;++i) if(chapter[i]==12) ++pages;
        for(unsigned page=0;page<pages;++page) for(unsigned selected=0;selected<3;selected+=2) {
            memcpy(v->pixels,base,64000); memcpy(pixels,base,64000);
            font[6]=71; check(uc_mem_write(u,0x60000,font,n->size)); check(uc_mem_write(u,0x70000,chapter,sizeof chapter));
            struct SlicksHelpRenderer r={0}; r.ui=(struct SlicksChunkyUi){pixels,palette,0,0};
            r.font=font; r.context=n; r.measure=help_measure; r.text=help_text;
            if(slicks_help_renderer_open(&r,saved,sizeof saved,358)) abort();
            r.style.left=20; r.style.top=15; r.style.right=300; r.style.bottom=185;
            r.style.selected=(short)selected; r.style.distance=2; r.style.spacing=1; r.style.country=358;
            for(unsigned i=0;i<4;++i) r.style.colours[i]=20+10*i;
            word(u,dsbase+0x169c,20); word(u,dsbase+0x169e,15); word(u,dsbase+0x16a0,300); word(u,dsbase+0x16a2,185);
            word(u,dsbase+0x6f8c,selected); word(u,dsbase+0x6ff2,0); word(u,dsbase+0x1720,358);
            check(uc_mem_write(u,dsbase+0x6f20,r.style.colours,4));
            unsigned char distance=2,spacing=1;
            check(uc_mem_write(u,dsbase+0x6f26,&distance,1)); check(uc_mem_write(u,dsbase+0x1604,&spacing,1));
            check(uc_mem_write(u,dsbase+0x6f63,r.style.target,21)); check(uc_mem_write(u,dsbase+0x16a8,r.style.prefix,6));
            uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
            check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
            check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
            word(u,0x8f000,0); word(u,0x8f002,0x9000); word(u,0x8f004,page);
            check(uc_emu_start(u,0x32165,0x90000,0,20000000)); check(uc_reg_read(u,UC_X86_REG_IP,&ip)); if(ip) abort();
            short next=0; if(slicks_help_renderer_page(&r,headers,info.headers,chapter,length,page,&next)) abort();
            /* Execute the viewer's arrow block with the page routine's live
             * formatting state, then compare it with native arrow glyphs. */
            uint16_t bp=0xf000; sp=0xeb00; cs=0x2e0f;
            check(uc_reg_write(u,UC_X86_REG_BP,&bp)); check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_CS,&cs));
            word(u,0x8f000-0x28,page);
            check(uc_mem_write(u,dsbase+0x6f24,&r.style.colours[4],1));
            check(uc_emu_start(u,0x32abc,0x32b5d,0,1000000));
            slicks_help_renderer_arrows(&r,(short)page,next);
            unsigned char original_font[8192],target[21]; check(uc_mem_read(u,0x60000,original_font,n->size));
            check(uc_mem_read(u,dsbase+0x6f63,target,21));
            if(memcmp(pixels,v->pixels,64000) || memcmp(font,original_font,n->size) ||
               (unsigned short)next!=getword(u,dsbase+0x6ff2) || (unsigned short)r.style.links!=getword(u,dsbase+0x6ff6) ||
               (unsigned short)r.style.total_links!=getword(u,dsbase+0x6ff4) || memcmp(target,r.style.target,21)) {
                fprintf(stderr,"Help page mismatch chapter=%u page=%u selected=%u next=%d/%u links=%d/%u\n",
                    offset,page,selected,next,getword(u,dsbase+0x6ff2),r.style.links,getword(u,dsbase+0x6ff6));
                for(unsigned i=0;i<64000;++i) if(pixels[i]!=v->pixels[i]) { fprintf(stderr,"pixel %u,%u native=%u DOS=%u\n",i%320,i/320,pixels[i],v->pixels[i]); break; }
                exit(1);
            }
            if(slicks_help_renderer_close(&r) || memcmp(pixels,base,64000) || font[6]!=71) abort();
            ++cases;
        }
    }
    printf("Original help pages: %u complete-frame/font/link-state comparisons pass\n",cases);
}
#ifdef SLICKS_HELP_PIXELS_LIBRARY
int help_pixel_verifier_main(void)
#else
int main(void)
#endif
{
    unsigned char runtime[300000],resource[70000],base[64000],palette[768],font[8192],pixels[64000],help[16384],code[8192];
    FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t runtime_size=fread(runtime,1,sizeof runtime,f); fclose(f);
    unsigned width,height; unsigned long consumed;
    long loaded=host_archive_load("ref/SLICKS.000","players.bmp",resource,sizeof resource);
    if(loaded<0 || slicks_decode_menu_bitmap(resource,loaded,base,palette,&width,&height,&consumed)) abort();
    loaded=host_archive_load("ref/SLICKS.000","kirj.@f",resource,sizeof resource);
    long font_size=loaded<0?-1:slicks_decode_font_resource(resource,loaded,font,sizeof font); if(font_size<0) abort();
    long help_size=host_archive_load("ref/SLICKS.000","HELP.TXT",help,sizeof help); if(help_size<0) abort();
    f=fopen("build/font_string_test.bin","rb"); if(!f) return 2; size_t code_size=fread(code,1,sizeof code,f); fclose(f);
    struct HelpFontCpu n={0}; n.size=(unsigned)font_size; n.measure=be32(code+12); n.text=be32(code+16);
    check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&n.cpu)); check(uc_ctl_set_cpu_model(n.cpu,UC_CPU_M68K_M68020));
    check(uc_mem_map(n.cpu,0,0x400000,UC_PROT_ALL)); check(uc_mem_write(n.cpu,0,code,code_size));
    uc_hook font_hook;
    check(uc_hook_add(n.cpu,&font_hook,UC_HOOK_MEM_WRITE,font_write,0,0x100000,0x100000+63999));
    const short edge_x[]={-10,0,17,310,319},edge_y[]={-3,0,190,199};
    const signed char edge_spacing[]={-128,-10,-1,0,1,10,127};
    struct SlicksChunkyUi edge_ui={pixels,palette,0,0};
    memcpy(pixels,base,sizeof pixels);
    (void)help_font_call(&n,font,(const unsigned char *)"A",&edge_ui,20,20,1);
    unsigned single_glyph_area=0;
    for(unsigned p=0;p<64000;++p) single_glyph_area+=reported[p];
    if(!single_glyph_area || single_glyph_area>=320) abort();
    const unsigned char edge_text[]={ 'A',8,'B',207,'C',13,'D',10,'E',255,0 };
    for(unsigned ix=0;ix<5;++ix) for(unsigned iy=0;iy<4;++iy)
        for(unsigned is=0;is<7;++is) {
            memcpy(pixels,base,sizeof pixels);
            (void)help_font_call(&n,font,edge_text,&edge_ui,edge_x[ix],edge_y[iy],edge_spacing[is]);
        }
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,runtime_size));
    struct Vga v={0}; uc_hook hooks[2];
    check(uc_hook_add(u,&hooks[0],UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,vga_access,&v,0xa0000,0xaffff));
    check(uc_hook_add(u,&hooks[1],UC_HOOK_INSN,font_port,&v,1,0,UC_X86_INS_OUT));
    unsigned dsbase=0x3cbf0;
    check(uc_mem_write(u,dsbase+0x6c20,palette,768));
    word(u,dsbase+0x16a4,0); word(u,dsbase+0x16a6,0x6000);
    word(u,dsbase+0x1d7b,100); word(u,dsbase+0x1d87,0);
    word(u,dsbase+0x1d8d,0); word(u,dsbase+0x1d8f,200); word(u,dsbase+0x1d91,0); word(u,dsbase+0x1d93,79);
    word(u,dsbase+0x15fe,10); word(u,dsbase+0x1600,0); word(u,dsbase+0x1602,1);
    if(getenv("SLICKS_HELP_ENTRY_ONLY")) {
        verify_viewer_entry(u,&v,&n,help,(unsigned)help_size,base,palette,font,pixels);
        check(uc_close(u)); check(uc_close(n.cpu)); return 0;
    }
    unsigned cases=0;
    for(unsigned at=0;at<(unsigned)help_size;) {
        unsigned start=at; while(at<(unsigned)help_size && help[at]!='\n') ++at; if(at<(unsigned)help_size) ++at;
        if(help[start]=='!') ++start;
        unsigned char line[512]={0}; if(at-start>=sizeof line) abort(); memcpy(line,help+start,at-start);
        if(slicks_help_preprocess(line,sizeof line)) abort();
        for(unsigned variant=0;variant<2;++variant) {
            memcpy(v.pixels,base,64000); memcpy(pixels,base,64000);
            font[6]=71; check(uc_mem_write(u,0x60000,font,font_size)); check(uc_mem_write(u,0x70000,line,sizeof line));
            struct SlicksHelpRenderer r={0}; r.ui=(struct SlicksChunkyUi){pixels,palette,0,0};
            r.font=font; r.context=&n; r.measure=help_measure; r.text=help_text;
            r.style.left=50; r.style.top=10; r.style.right=270; r.style.bottom=190;
            r.style.selected=0; r.style.distance=2; r.style.spacing=variant?0:1; r.style.centred=variant; r.style.country=358;
            for(unsigned i=0;i<4;++i) r.style.colours[i]=20+10*i;
            word(u,dsbase+0x169c,50); word(u,dsbase+0x169e,10); word(u,dsbase+0x16a0,270); word(u,dsbase+0x16a2,190);
            word(u,dsbase+0x6f8c,0); word(u,dsbase+0x6ff6,0); word(u,dsbase+0x6ff4,0); word(u,dsbase+0x1720,358);
            check(uc_mem_write(u,dsbase+0x6f20,r.style.colours,4));
            unsigned char distance=2,spacing=(unsigned char)r.style.spacing,centred=(unsigned char)variant;
            check(uc_mem_write(u,dsbase+0x6f26,&distance,1)); check(uc_mem_write(u,dsbase+0x1604,&spacing,1)); check(uc_mem_write(u,dsbase+0x6f8e,&centred,1));
            uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,ax;
            check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds)); check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
            word(u,0x8f000,0); word(u,0x8f002,0x9000); word(u,0x8f004,0); word(u,0x8f006,0x7000); word(u,0x8f008,40);
            check(uc_emu_start(u,0x31b53,0x90000,0,5000000)); check(uc_reg_read(u,UC_X86_REG_AX,&ax));
            short y=40; if(slicks_help_renderer_line(&r,line,&y)) abort();
            unsigned char original_font[8192]; check(uc_mem_read(u,0x60000,original_font,font_size));
            if(memcmp(pixels,v.pixels,64000) || memcmp(font,original_font,font_size) || (unsigned short)y!=ax) {
                fprintf(stderr,"Help pixel mismatch case=%u source offset=%u variant=%u y=%d/%d\n",cases,start,variant,y,(short)ax);
                for(unsigned i=0;i<64000;++i) if(pixels[i]!=v.pixels[i]) { fprintf(stderr,"pixel %u,%u native=%u DOS=%u\n",i%320,i/320,pixels[i],v.pixels[i]); break; }
                return 1;
            }
            ++cases;
        }
    }
    verify_pages(u,&v,&n,help,(unsigned)help_size,base,palette,font,pixels);
    verify_viewer_entry(u,&v,&n,help,(unsigned)help_size,base,palette,font,pixels);
    printf("Help dirty bounds cover all native stores in %u strings\n",bound_checks);
    check(uc_close(u)); check(uc_close(n.cpu));
    printf("Original help pixels: %u full-frame/font comparisons against native 68020 text and measurement pass\n",cases); return 0;
}
