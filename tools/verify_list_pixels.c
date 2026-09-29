/* Reuse the established real-font 68020 runner and modeled VGA ports. */
#define main palette_verifier_main
#include "verify_palette_remap.c"
#undef main
#include "../src/ui/list_renderer.h"
#include "../src/ui/profile_delete_prompt.h"
#include "../src/ui/name_dialog.h"
#include "../src/ui/message_dialog.h"
#include "../src/ui/colour_dialog.h"
#include "../src/ui/options_menu_renderer.h"
#include "../src/ui/controllers_dialog_renderer.h"
#include "../src/ui/menu_icon.h"
#include "../src/gen/setup_defaults.h"

static short list_measure(void *context,const unsigned char *font,const unsigned char *string)
{
    struct NativeText *n=context; uc_engine *m=n->cpu; unsigned char entry_bytes[4];
    check(uc_mem_read(m,8,entry_bytes,4));
    unsigned entry=(unsigned)entry_bytes[0]<<24|(unsigned)entry_bytes[1]<<16|(unsigned)entry_bytes[2]<<8|entry_bytes[3];
    check(uc_mem_write(m,0x50000,font,n->sizes[0]));
    check(uc_mem_write(m,0x60000,string,strlen((const char *)string)+1));
    unsigned char args[]={0,0x38,0,0,0,5,0,0,0,6,0,0};
    uint32_t sp=0x300000,sr=0,pc,d0;
    check(uc_reg_write(m,UC_M68K_REG_SR,&sr)); check(uc_reg_write(m,UC_M68K_REG_A7,&sp));
    check(uc_mem_write(m,sp,args,sizeof args)); check(uc_emu_start(m,entry,0x380000,0,100000));
    check(uc_reg_read(m,UC_M68K_REG_PC,&pc)); check(uc_reg_read(m,UC_M68K_REG_D0,&d0));
    if(pc!=0x380000) abort(); return (short)d0;
}
struct Allocator { unsigned next,calls,frees; };
static void platform_call(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size; struct Allocator *a=context; uint16_t ss,sp,cs,ip,ax=0,dx=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=16U*ss+sp;
    if(address==0x13b07) {
        unsigned count=getword(u,stack+4); ax=a->next&15; dx=a->next>>4;
        a->next=(a->next+count+15)&~15U; if(a->next>=0x60000) abort(); ++a->calls;
        check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    } else if(address==0x139fd) ++a->frees;
    ip=getword(u,stack); cs=getword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void list_port(uc_engine *u,uint32_t port,int size,uint32_t value,void *context)
{
    struct Vga *v=context;
    if(port==0x3ce && size==1 && value==4) return;
    if(port==0x3cf && size==1) { v->read_plane=value&3; return; }
    font_port(u,port,size,value,context);
}
static void compare_list(uc_engine *u,struct Vga *v,struct SlicksListRenderer *r,const char *stage,unsigned test)
{
    unsigned char colour; check(uc_mem_read(u,0x60006,&colour,1));
    if(memcmp(v->pixels,r->ui.pixels,64000) || colour!=r->font[6]) {
        uint16_t bp; check(uc_reg_read(u,UC_X86_REG_BP,&bp));
        fprintf(stderr,"caption width/count DOS=%u/%u native=%d/%d\n",getword(u,0x80000+bp-0x12),getword(u,0x80000+bp-0xc),r->captions.width,r->captions.count);
        fprintf(stderr,"List pixel mismatch test=%u stage=%s font=%u/%u\n",test,stage,colour,r->font[6]);
        for(unsigned i=0;i<64000;++i) if(v->pixels[i]!=r->ui.pixels[i]) {
            fprintf(stderr,"first pixel (%u,%u) DOS=%u native=%u\n",i%320,i/320,v->pixels[i],r->ui.pixels[i]); break;
        }
        exit(1);
    }
}
static void name_run(uc_engine *u,unsigned start,unsigned end)
{
    /* These stages stop inside blocks used by earlier stages. Invalidate
     * translated blocks so a prior stop cannot hide the new boundary. */
    check(uc_ctl_remove_cache(u,0,0xfffff));
    uc_err error=uc_emu_start(u,start,end,0,50000000);
    if(error) {
        uint16_t cs,ip; check(uc_reg_read(u,UC_X86_REG_CS,&cs)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        fprintf(stderr,"Name oracle %x..%x stopped at %x:%x\n",start,end,cs,ip);
    }
    check(error);
}
static void verify_name_pixels(const unsigned char *runtime,size_t runtime_size,
    const unsigned char *base,unsigned char *palette,unsigned char *font,unsigned font_size,
    unsigned char *pixels,struct NativeText *n)
{
    unsigned cases=0;
    for(unsigned test=0;test<18;++test) {
        uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
        check(uc_mem_write(u,0x10100,runtime,runtime_size));
        struct Vga v={0}; v.allow_margin=1;
        memset(v.margin,0xa5,sizeof v.margin);
        memcpy(v.pixels,base,64000); memcpy(pixels,base,64000);
        font[6]=71; check(uc_mem_write(u,0x60000,font,font_size));
        check(uc_mem_write(u,0x67000,palette,768));
        word(u,0x3cbf0+0x71b8,0); word(u,0x3cbf0+0x71ba,0x6700);
        word(u,0x3cbf0+0x1d7b,100); word(u,0x3cbf0+0x1d87,0); word(u,0x3cbf0+0x16fd,66);
        word(u,0x3cbf0+0x1d8d,0); word(u,0x3cbf0+0x1d8f,200);
        word(u,0x3cbf0+0x1d91,0); word(u,0x3cbf0+0x1d93,79);
        struct Allocator allocator={0x50000,0,0}; uc_hook hooks[5];
        check(uc_hook_add(u,&hooks[0],UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,vga_access,&v,0xa0000,0xaffff));
        check(uc_hook_add(u,&hooks[1],UC_HOOK_INSN,list_port,&v,1,0,UC_X86_INS_OUT));
        const unsigned calls[]={0x13b07,0x139fd,0x36ca5};
        for(unsigned i=0;i<3;++i) check(uc_hook_add(u,&hooks[i+2],UC_HOOK_CODE,platform_call,&allocator,calls[i],calls[i]));
        unsigned char name[21]={0},caption[32]; unsigned length=test%3==0?0:test%3==1?3:20;
        unsigned x=test<6?170:185,y=test<6?85:40,caption_at=test<6?0x1338:0x15bb;
        unsigned limit=20,flags=0x203;
        if(test>=12) { x=100; y=65; caption_at=0xbbb; limit=8; flags=0x1b; length=test%3==0?0:test%3==1?3:8; }
        for(unsigned i=0;i<length;++i) name[i]='A'+i;
        check(uc_mem_write(u,0x65000,name,sizeof name)); check(uc_mem_read(u,0x3cbf0+caption_at,caption,sizeof caption));
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,bp,ip;
        word(u,0x8f000,0); word(u,0x8f002,0x9000);
        unsigned args[]={x,y,caption_at,0x3cbf,0,0x6500,limit,0,0x6000,flags};
        for(unsigned i=0;i<10;++i) word(u,0x8f004+2*i,args[i]);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        name_run(u,0x307b6,0x2f6da);
        name_run(u,0x2f6da,0x2f7f4);
        /* Non-preserving filename mode starts empty and waits for input;
         * unlike profile mode it has no initial text redraw to consume. */
        unsigned initial_stop=(flags&512)?0x2fb49:0x2f826;
        name_run(u,0x2f7f4,initial_stop);
        check(uc_reg_read(u,UC_X86_REG_BP,&bp)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        if(ip!=initial_stop-0x2e0f0 || allocator.calls!=3) { fprintf(stderr,"initial IP=%x allocations=%u\n",ip,allocator.calls); abort(); }
        unsigned char original[8192],field[8192],cursor[256]; struct SlicksNameDialog d={0};
        d.painter.ui=(struct SlicksChunkyUi){pixels,palette,0,0}; d.painter.font=font;
        d.painter.measure=list_measure; d.painter.text=renderer_text; d.painter.context=n;
        if(slicks_name_dialog_open_field(&d,name,caption,x,y,66,limit,flags,original,sizeof original,field,sizeof field,cursor,sizeof cursor)) {
            fprintf(stderr,"native name open failed font %u %u %u\n",font[1],font[2],font[3]); abort();
        }
        compare_list(u,&v,&d.painter,"name open",test); ++cases;
        const unsigned char keys[]={'z',8,'9',' ',0,8,'X','a','b','c','d','e','f','g','h','i'};
        for(unsigned k=0;k<sizeof keys;++k) {
            check(uc_mem_write(u,0x80000+bp-7,keys+k,1));
            name_run(u,0x2f92d,0x2fb49);
            if(slicks_name_dialog_key(&d,keys[k])) { fprintf(stderr,"native key failed %u\n",k); abort(); }
            unsigned char actual[21]; check(uc_mem_read(u,0x65000,actual,sizeof actual));
            if(memcmp(actual,name,sizeof name) || getword(u,0x80000+bp-4)!=d.entry.position) {
                fprintf(stderr,"key %u name/position differs %s/%s %u/%u\n",k,actual,name,getword(u,0x80000+bp-4),d.entry.position); abort();
            }
            compare_list(u,&v,&d.painter,"name key",test); ++cases;
            /* Capture at the updated cursor, then paint/restore its underline.
             * Stop before the BIOS time and keyboard boundary. */
            name_run(u,0x2f7f4,0x2f826);
            for(unsigned phase=0;phase<2;++phase) {
                unsigned char hidden=(unsigned char)phase; check(uc_mem_write(u,0x80000+bp-8,&hidden,1));
                name_run(u,0x2f829,0x2f898);
                if(slicks_name_dialog_cursor(&d,!phase)) abort();
                compare_list(u,&v,&d.painter,"name cursor",test); ++cases;
            }
        }
        unsigned char end=test&1?27:13; check(uc_mem_write(u,0x80000+bp-7,&end,1));
        name_run(u,0x2f92d,0x2fb58);
        if(slicks_name_dialog_key(&d,end)!=(test&1?2:1)) abort();
        name_run(u,0x2fb58,0x90000);
        /* The DOS wrapper restores its outer allocation but has no explicit
         * free call; only the two inner buffers reach 139fd. Native buffers
         * are caller-owned and all are reclaimed by the platform adapter. */
        if(slicks_name_dialog_close(&d) || allocator.frees!=2) { fprintf(stderr,"close/frees %u\n",allocator.frees); abort(); }
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        if(ip || sp!=0xf004) abort();
        compare_list(u,&v,&d.painter,"name close",test); ++cases;
        check(uc_close(u));
    }
    printf("Original name dialog: %u full-screen/font-state comparisons including typing, cursor, accept/cancel and restoration pass\n",cases);
}
static void verify_colour_pixels(const unsigned char *runtime,size_t runtime_size,
    const unsigned char *base,unsigned char *palette,unsigned char *font,unsigned font_size,
    unsigned char *pixels,struct NativeText *n)
{
    unsigned cases=0;
    for(unsigned test=0;test<8;++test) {
        uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
        check(uc_mem_write(u,0x10100,runtime,runtime_size));
        struct Vga v={0}; memcpy(v.pixels,base,64000); memcpy(pixels,base,64000);
        font[6]=(unsigned char)(71+test); check(uc_mem_write(u,0x60000,font,font_size));
        check(uc_mem_write(u,0x67000,palette,768));
        word(u,0x3cbf0+0x71b8,0); word(u,0x3cbf0+0x71ba,0x6700);
        word(u,0x3cbf0+0x1d7b,100); word(u,0x3cbf0+0x1d87,0);
        word(u,0x3cbf0+0x1d8d,0); word(u,0x3cbf0+0x1d8f,200);
        word(u,0x3cbf0+0x1d91,0); word(u,0x3cbf0+0x1d93,79);
        struct Allocator allocator={0x50000,0,0}; uc_hook hooks[5];
        check(uc_hook_add(u,&hooks[0],UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,vga_access,&v,0xa0000,0xaffff));
        check(uc_hook_add(u,&hooks[1],UC_HOOK_INSN,list_port,&v,1,0,UC_X86_INS_OUT));
        const unsigned calls[]={0x13b07,0x139fd,0x36ca5};
        for(unsigned i=0;i<3;++i) check(uc_hook_add(u,&hooks[i+2],UC_HOOK_CODE,platform_call,&allocator,calls[i],calls[i]));
        unsigned char rgb[3]={(unsigned char)(test*9%64),(unsigned char)(test*13%64),(unsigned char)(test*17%64)},caption[16];
        check(uc_mem_write(u,0x65000,rgb,3)); check(uc_mem_read(u,0x3cbf0+0x133d,caption,sizeof caption));
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,bp,ip;
        short y=test&1?160:145;
        word(u,0x8f000,0); word(u,0x8f002,0x9000);
        unsigned args[]={160,(unsigned)y,0x133d,0x3cbf,0,0x6500,1,0x6500,2,0x6500,0,0x6000};
        for(unsigned i=0;i<12;++i) word(u,0x8f004+2*i,args[i]);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        name_run(u,0x2f290,0x2f43f); check(uc_reg_read(u,UC_X86_REG_BP,&bp));
        unsigned char saved[2400]; struct SlicksColourDialog d={0};
        d.painter.ui=(struct SlicksChunkyUi){pixels,palette,0,0}; d.painter.font=font;
        d.painter.text=renderer_text; d.painter.context=n;
        if(slicks_colour_dialog_open(&d,rgb,caption,160,y,saved,sizeof saved)) abort();
        compare_list(u,&v,&d.painter,"colour open",test); ++cases;
        const unsigned char keys[]={0x4d,0x4d,0x50,0x4b,0x4b,0x50,0x4d,0x48,0x48,0x48,0};
        for(unsigned k=0;k<sizeof keys;++k) {
            unsigned long tick=k/2; word(u,0x46c,tick); word(u,0x46e,0);
            name_run(u,0x2f43f,0x2f5b6);
            if(slicks_colour_dialog_draw(&d,tick)) abort();
            compare_list(u,&v,&d.painter,"colour draw",test); ++cases;
            uint16_t ax=keys[k]; check(uc_reg_write(u,UC_X86_REG_AX,&ax));
            name_run(u,0x2f5be,0x2f676); slicks_colour_picker_key(&d.state,keys[k]);
            unsigned char state[4]; check(uc_mem_read(u,0x80000+bp-10,state,4));
            if(memcmp(state,d.state.rgb,3) || state[3]!=d.state.channel) abort();
        }
        uint16_t ax=test&2?1:0x1c; check(uc_reg_write(u,UC_X86_REG_AX,&ax));
        name_run(u,0x2f5be,0x2f676); slicks_colour_picker_key(&d.state,(unsigned char)ax);
        name_run(u,0x2f676,0x90000);
        int result=slicks_colour_dialog_close(&d); check(uc_reg_read(u,UC_X86_REG_AX,&ax));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        unsigned char actual[3]; check(uc_mem_read(u,0x65000,actual,3));
        if(result!=(ax&255) || ip || sp!=0xf004 || allocator.frees!=1 || memcmp(actual,rgb,3)) abort();
        compare_list(u,&v,&d.painter,"colour close",test); ++cases;
        check(uc_close(u));
    }
    printf("Original colour dialog: %u full-screen/font-state comparisons with controls, accept/cancel and restoration pass\n",cases);
}
static struct NativeText *options_font_cpu;
static short options_measure(const unsigned char *font,const unsigned char *text)
{ return list_measure(options_font_cpu,font,text); }
static void options_vga_access(uc_engine *u,uc_mem_type type,uint64_t address,
    int size,int64_t value,void *context)
{
    /* Options' tint extends below the visible 200-line page. Invisible VGA
     * bytes stay in Unicorn RAM; they cannot affect the compared pixels. */
    if(address>=0xa0000+20000) return;
    vga_access(u,type,address,size,value,context);
}
static void verify_options_pixels(const unsigned char *runtime,size_t runtime_size,
    const unsigned char *base,unsigned char *palette,unsigned char *font,unsigned font_size,
    unsigned char *pixels,struct NativeText *n)
{
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,runtime_size));
    struct Vga v={0}; uc_hook hooks[2];
    check(uc_hook_add(u,&hooks[0],UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,options_vga_access,&v,0xa0000,0xaffff));
    check(uc_hook_add(u,&hooks[1],UC_HOOK_INSN,list_port,&v,1,0,UC_X86_INS_OUT));
    check(uc_mem_write(u,0x65000,palette,768));
    word(u,0x3cbf0+0x71b8,0); word(u,0x3cbf0+0x71ba,0x6500);
    word(u,0x3cbf0+0x68aa,0); word(u,0x3cbf0+0x68ac,0x6500);
    word(u,0x3cbf0+0x680,0); word(u,0x3cbf0+0x682,0x6000);
    word(u,0x3cbf0+0x1d7b,100); word(u,0x3cbf0+0x1d87,0);
    word(u,0x3cbf0+0x1d8d,0); word(u,0x3cbf0+0x1d8f,200);
    word(u,0x3cbf0+0x1d91,0); word(u,0x3cbf0+0x1d93,79);
    static unsigned char saved[64000],snapshot[65536]; memcpy(saved,base,64000);
    snapshot[0]=80; snapshot[1]=200;
    for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;++x)
        snapshot[2+(x&3)*16000+y*80+x/4]=saved[y*320+x];
    check(uc_mem_write(u,0x70000,snapshot,sizeof snapshot));
    word(u,0x3cbf0+0x5b8,0); word(u,0x3cbf0+0x5ba,0x7000);
    struct SlicksPlayerMenuRenderer surface={0};
    surface.ui=(struct SlicksChunkyUi){pixels,palette,0,0}; surface.saved=saved;
    surface.fonts[0]=font; surface.text=renderer_text; surface.context=n;
    options_font_cpu=n;
    struct SlicksOptionsRenderer renderer;
    if(slicks_options_renderer_init(&renderer,&surface,options_measure)) abort();
    const unsigned char *off=runtime+0x3cbf0-0x10100+0x113c;
    struct SlicksOptionsLabels labels={slicks_original_option_labels,slicks_original_option_suffixes,
        slicks_original_mode_labels,off,off+4};
    uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
    /* Build the oracle's table with original code, not the native table. */
    name_run(u,0x28f1e,0x28f43);
    unsigned cases=0;
    for(unsigned mode=0;mode<6;++mode) {
        memcpy(pixels,base,64000); memcpy(v.pixels,base,64000);
        font[6]=71; check(uc_mem_write(u,0x60000,font,font_size));
        for(unsigned row=0;row<18;++row) for(unsigned variant=0;variant<3;++variant) {
            struct SlicksConfiguration c=slicks_original_configuration; c.options[0]=(short)mode;
            for(unsigned i=1;i<15;++i) if(variant)
                c.options[i]=variant==1?slicks_original_option_specs[i].minimum:slicks_original_option_specs[i].maximum;
            for(unsigned i=0;i<15;++i) word(u,0x3cbf0+0x92+8*i,(unsigned short)c.options[i]);
            unsigned char locals[6]={row?row-1:255,(unsigned char)row,renderer.colours[3],
                renderer.colours[2],renderer.colours[1],renderer.colours[0]};
            if(variant) locals[0]=111;
            check(uc_mem_write(u,0x8effa,locals,sizeof locals));
            cs=0x266c; sp=0xeb00;
            check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
            name_run(u,0x28f9d,0x293ef);
            struct SlicksOptionsMenu menu={(unsigned char)row,0,0,(signed char)locals[0]};
            if(slicks_options_renderer_draw(&renderer,&menu,&c,slicks_original_option_specs,&labels)) abort();
            unsigned char original_font[8192]; check(uc_mem_read(u,0x60000,original_font,font_size));
            if(memcmp(pixels,v.pixels,64000) || memcmp(original_font,font,font_size)) {
                fprintf(stderr,"Options pixel/font mismatch mode=%u row=%u variant=%u\n",mode,row,variant);
                for(unsigned i=0;i<64000;++i) if(pixels[i]!=v.pixels[i]) {
                    fprintf(stderr,"pixel %u,%u native=%u DOS=%u\n",i%320,i/320,pixels[i],v.pixels[i]); break;
                }
                exit(1);
            }
            ++cases;
        }
    }
    unsigned char resource[8192],small_font[8192];
    long loaded=host_archive_load("ref/SLICKS.000","pieni.@f",resource,sizeof resource);
    long small_size=loaded<0?-1:slicks_decode_font_resource(resource,(unsigned long)loaded,small_font,sizeof small_font);
    if(small_size<0) abort();
    surface.fonts[1]=small_font; n->fonts[1]=small_font; n->sizes[1]=(unsigned)small_size;
    memcpy(pixels,base,64000); memcpy(v.pixels,base,64000);
    check(uc_mem_write(u,0x62000,small_font,(size_t)small_size));
    word(u,0x3cbf0+0x688,0); word(u,0x3cbf0+0x68a,0x6200);
    word(u,0x3cbf0+0x1722,0); word(u,0x3cbf0+0x1724,0);
    cs=0x266c; sp=0xeb00;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    name_run(u,0x28e7f,0x28f43);
    if(slicks_options_renderer_prepare(&renderer,runtime+0x3cbf0-0x10100+0x13ca)) abort();
    check(uc_mem_read(u,0x62000,resource,(size_t)small_size));
    if(memcmp(pixels,v.pixels,64000) || memcmp(saved,v.pixels,64000) ||
        memcmp(resource,small_font,(size_t)small_size)) {
        fputs("Options preparation pixel/font mismatch\n",stderr); exit(1);
    }
    n->fonts[1]=0; n->sizes[1]=0;
    check(uc_close(u)); options_font_cpu=0;
    printf("Original options renderer: %u sequential full-frame/font comparisons with DOS and native 68020 fonts pass\n",cases);
    puts("Original options preparation: tinted panel, small-font heading and saved-background pixels match DOS");
}

static void controller_pixel_icon(void *p,struct SlicksChunkyUi *ui,
    const struct SlicksMenuIcon *icon,short x,short y)
{
    (void)p;
    for(unsigned row=0;row<icon->height;++row) for(unsigned col=0;col<icon->width;++col) {
        unsigned char value=icon->pixels[row*icon->width+col];
        if(value) ui->pixels[(y+row)*320+x+col]=value;
    }
}
/* Resource loading/freeing is the only substituted preparation boundary;
 * original tinting, icon drawing, number/text layout and colour changes run. */
static void controller_resource(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size; unsigned *loads=context; uint16_t ss,sp,cs,ip;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=16U*ss+sp;
    if(address==0x2e0f8) {
        unsigned at=getword(u,stack+4)+16U*getword(u,stack+6);
        unsigned char name[13]; check(uc_mem_read(u,at,name,sizeof name));
        if(memcmp(name,"/keys_m",7) || name[7]<'1' || name[7]>'5' || memcmp(name+8,".@16",5)) abort();
        uint16_t ax=0,dx=(uint16_t)(0x6a00+0x80*(name[7]-'1'));
        check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
        ++*loads;
    }
    ip=getword(u,stack); cs=getword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void controller_run(uc_engine *u,unsigned start,unsigned end)
{
    name_run(u,start,end);
    uint16_t cs,ip; check(uc_reg_read(u,UC_X86_REG_CS,&cs)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
    if(16U*cs+ip!=end) { fprintf(stderr,"Controller oracle missed boundary %x: stopped at %x:%x\n",end,cs,ip); abort(); }
}
static void verify_controller_pixels_at(const unsigned char *runtime,size_t runtime_size,
    const unsigned char *base,unsigned char *palette,unsigned char *font,unsigned font_size,
    unsigned char *pixels,struct NativeText *n,short left,short top)
{
    unsigned char resource[8192],small[8192],icon_pixels[8][512],outer[16800],inner[16000];
    long loaded=host_archive_load("ref/SLICKS.000","pieni.@f",resource,sizeof resource);
    long small_size=loaded<0?-1:slicks_decode_font_resource(resource,loaded,small,sizeof small);
    if(small_size<0) abort();
    struct SlicksMenuIcon icons[8];
    const char *names[]={"ohj_key.@I","ohj_joy.@I","ohj_lptc.@I",
        "keys_m1.@16","keys_m2.@16","keys_m3.@16","keys_m4.@16","keys_m5.@16"};
    for(unsigned i=0;i<8;++i) {
        loaded=host_archive_load("ref/SLICKS.000",names[i],resource,sizeof resource); if(loaded<0) abort();
        icons[i].pixels=icon_pixels[i];
        if(i<3?slicks_decode_indexed_menu_icon(resource,loaded,icon_pixels[i],512,&icons[i].width,&icons[i].height):
               slicks_decode_menu_icon(resource,loaded,palette,icon_pixels[i],512,&icons[i].width,&icons[i].height)) abort();
    }
    struct SlicksPlayerMenuRenderer surface={0}; surface.ui=(struct SlicksChunkyUi){pixels,palette,0,0};
    surface.fonts[0]=font; surface.fonts[1]=small; surface.text=renderer_text;
    surface.icon=controller_pixel_icon; surface.context=n;
    n->fonts[1]=small; n->sizes[1]=(unsigned)small_size;
    struct SlicksControllersRenderer r={0}; r.surface=&surface; r.icons=icons;
    struct SlicksControllersDialog d;
    unsigned char old_colour=font[6],old_small=small[6];
    memcpy(pixels,base,64000);
    if(slicks_controllers_renderer_open(&r,&d,left,top,outer,sizeof outer,inner,sizeof inner)) abort();
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,runtime_size));
    struct Vga v={0}; memcpy(v.pixels,base,64000); uc_hook hook;
    check(uc_hook_add(u,&hook,UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,vga_access,&v,0xa0000,0xaffff));
    check(uc_hook_add(u,&hook,UC_HOOK_INSN,list_port,&v,1,0,UC_X86_INS_OUT));
    check(uc_mem_write(u,0x65000,palette,768)); check(uc_mem_write(u,0x60000,font,font_size));
    check(uc_mem_write(u,0x62000,small,small_size));
    check(uc_mem_write(u,0x60006,&old_colour,1)); check(uc_mem_write(u,0x62006,&old_small,1));
    word(u,0x3cbf0+0x71b8,0); word(u,0x3cbf0+0x71ba,0x6500);
    word(u,0x3cbf0+0x680,0); word(u,0x3cbf0+0x682,0x6000);
    word(u,0x3cbf0+0x688,0); word(u,0x3cbf0+0x68a,0x6200);
    word(u,0x3cbf0+0x1d7b,100); word(u,0x3cbf0+0x1d87,0);
    word(u,0x3cbf0+0x1d8d,0); word(u,0x3cbf0+0x1d8f,200);
    word(u,0x3cbf0+0x1d91,0); word(u,0x3cbf0+0x1d93,79);
    for(unsigned i=3;i<8;++i) {
        loaded=host_archive_load("ref/SLICKS.000",names[i],resource,sizeof resource);
        if(loaded<8 || loaded>2048) abort();
        unsigned char raw[2048]={0,0xb1,resource[5],0,resource[7],0};
        memcpy(raw+6,resource+8,(size_t)loaded-8);
        check(uc_mem_write(u,0x6a000+0x800*(i-3),raw,loaded-2));
    }
    unsigned loads=0;
    check(uc_hook_add(u,&hook,UC_HOOK_CODE,controller_resource,&loads,0x2e0f8,0x2e0f8));
    check(uc_hook_add(u,&hook,UC_HOOK_CODE,controller_resource,&loads,0x139fd,0x139fd));
    uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000;
    word(u,0x8f006,left); word(u,0x8f008,top);
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    check(uc_reg_write(u,UC_X86_REG_BP,&bp));
    controller_run(u,0x2d8e3,0x2da3b);
    unsigned char actual_colour,actual_small;
    check(uc_mem_read(u,0x60006,&actual_colour,1)); check(uc_mem_read(u,0x62006,&actual_small,1));
    if(loads!=5 || memcmp(pixels,v.pixels,64000) || actual_colour!=font[6] || actual_small!=small[6]) {
        fprintf(stderr,"Controller preparation mismatch loads=%u fonts=%u/%u,%u/%u\n",loads,actual_colour,font[6],actual_small,small[6]);
        for(unsigned i=0;i<64000;++i) if(pixels[i]!=v.pixels[i]) { fprintf(stderr,"pixel %u,%u native=%u DOS=%u\n",i%320,i/320,pixels[i],v.pixels[i]); break; }
        exit(1);
    }
    resource[0]=50; resource[1]=80;
    unsigned char snapshot[16002]; snapshot[0]=50; snapshot[1]=80;
    for(unsigned y=0;y<80;++y) for(unsigned x=0;x<200;++x)
        snapshot[2+(x&3)*4000+y*50+x/4]=inner[y*200+x];
    check(uc_mem_write(u,0x70000,snapshot,sizeof snapshot));
    word(u,0x8effc,0); word(u,0x8effe,0x7000);
    for(unsigned i=0;i<3;++i) {
        unsigned stride=(icons[i].width+3)/4,plane=stride*icons[i].height;
        memset(resource,0,sizeof resource); resource[0]=stride; resource[1]=icons[i].height;
        for(unsigned y=0;y<icons[i].height;++y) for(unsigned x=0;x<icons[i].width;++x)
            resource[2+(x&3)*plane+y*stride+x/4]=icon_pixels[i][y*icons[i].width+x];
        check(uc_mem_write(u,0x68000+512*i,resource,4*plane+2));
        word(u,0x3cbf0+0x68c+4*i,512*i); word(u,0x3cbf0+0x68e +4*i,0x6800);
    }
    const unsigned char *data=runtime+0x3cbf0-0x10100;
    struct SlicksControllersLabels labels={slicks_original_controller_key_names,
        slicks_original_controller_scans,data+0x15e7,data+0x1523};
    unsigned cases=0;
    for(unsigned row=0;row<5;++row) for(unsigned col=0;col<6;++col) for(unsigned variant=0;variant<4;++variant) {
        struct SlicksConfiguration c=slicks_original_configuration;
        for(unsigned i=0;i<4;++i) c.player_input[i]=(i+variant)%5;
        for(unsigned i=0;i<20;++i) c.keys[i]=slicks_original_controller_scans[(i*3+variant)%69];
        check(uc_mem_write(u,0x3cbf0+0x5358,c.keys,20)); check(uc_mem_write(u,0x3cbf0+0x5e2,c.player_input,4));
        unsigned char state[]={variant==0?255:variant==1?111:row,0,col,row};
        check(uc_mem_write(u,0x8eff0,state,4)); word(u,0x8f006,left); word(u,0x8f008,top);
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        controller_run(u,0x2da76,0x2dd89);
        d=(struct SlicksControllersDialog){row,col,0,0,(signed char)state[0]};
        if(slicks_controllers_renderer_draw(&r,&d,&c,&labels)) abort();
        unsigned char colour; check(uc_mem_read(u,0x60006,&colour,1));
        if(memcmp(pixels,v.pixels,64000) || colour!=font[6]) {
            fprintf(stderr,"Controller pixels mismatch row=%u col=%u variant=%u\n",row,col,variant);
            for(unsigned i=0;i<64000;++i) if(pixels[i]!=v.pixels[i]) { fprintf(stderr,"pixel %u,%u native=%u DOS=%u\n",i%320,i/320,pixels[i],v.pixels[i]); break; }
            exit(1);
        }
        ++cases;
    }
    unsigned captures=0;
    for(unsigned row=0;row<4;++row) for(unsigned col=1;col<=5;++col) {
        unsigned char column=(unsigned char)col;
        check(uc_mem_write(u,0x8eff2,&column,1));
        uint16_t dx=(uint16_t)row;
        check(uc_reg_write(u,UC_X86_REG_DX,&dx));
        controller_run(u,0x2dece,0x2df10);
        d=(struct SlicksControllersDialog){row,col,0,1,111};
        if(slicks_controllers_renderer_capture(&r,&d,data+0x15f0)) abort();
        check(uc_mem_read(u,0x62006,&actual_small,1));
        if(memcmp(pixels,v.pixels,64000) || actual_small!=small[6]) {
            fprintf(stderr,"Controller capture pixels mismatch row=%u col=%u\n",row,col); exit(1);
        }
        ++captures;
    }
    if(slicks_controllers_renderer_close(&r) || memcmp(pixels,base,64000) ||
        font[6]!=old_colour || small[6]!=old_small) abort();
    check(uc_close(u)); n->fonts[1]=0; n->sizes[1]=0;
    printf("Original Controllers redraw: %u sequential full-frame/font comparisons pass; native close restores pixels and both fonts\n",cases);
    printf("Original Controllers at %d,%d preparation and %u capture prompts: full-frame/font comparisons pass\n",left,top,captures);
}
static void verify_controller_pixels(const unsigned char *runtime,size_t runtime_size,
    const unsigned char *base,unsigned char *palette,unsigned char *font,unsigned font_size,
    unsigned char *pixels,struct NativeText *n)
{
    verify_controller_pixels_at(runtime,runtime_size,base,palette,font,font_size,pixels,n,100,80);
    verify_controller_pixels_at(runtime,runtime_size,base,palette,font,font_size,pixels,n,45,65);
}

static void verify_message_pixels(const unsigned char *runtime,size_t runtime_size,
    const unsigned char *base,unsigned char *palette,unsigned char *font,unsigned font_size,
    unsigned char *pixels,struct NativeText *n)
{
    const unsigned topics[]={0x951,0x974,0x15ca}; unsigned cases=0;
    const unsigned char keys[]={0x15,0x31,1,0x1c,0x44,0x39};
    for(unsigned topic=0;topic<3;++topic) for(unsigned variant=0;variant<4;++variant) {
        font=n->fonts[topic==2?2:0]; font_size=n->sizes[topic==2?2:0];
        uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
        check(uc_mem_write(u,0x10100,runtime,runtime_size));
        struct Vga v={0}; memcpy(v.pixels,base,64000); memcpy(pixels,base,64000);
        font[6]=71; check(uc_mem_write(u,0x60000,font,font_size)); check(uc_mem_write(u,0x67000,palette,768));
        word(u,0x3cbf0+0x71b8,0); word(u,0x3cbf0+0x71ba,0x6700);
        unsigned percent=variant==3?100:66;
        word(u,0x3cbf0+0x1d7b,100); word(u,0x3cbf0+0x1d87,0); word(u,0x3cbf0+0x16fd,percent);
        word(u,0x3cbf0+0x1d8d,0); word(u,0x3cbf0+0x1d8f,200);
        word(u,0x3cbf0+0x1d91,0); word(u,0x3cbf0+0x1d93,79);
        struct Allocator allocator={0x50000,0,0}; uc_hook hooks[4];
        check(uc_hook_add(u,&hooks[0],UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,vga_access,&v,0xa0000,0xaffff));
        check(uc_hook_add(u,&hooks[1],UC_HOOK_INSN,list_port,&v,1,0,UC_X86_INS_OUT));
        check(uc_hook_add(u,&hooks[2],UC_HOOK_CODE,platform_call,&allocator,0x13b07,0x13b07));
        check(uc_hook_add(u,&hooks[3],UC_HOOK_CODE,platform_call,&allocator,0x139fd,0x139fd));
        short x=(short)(158+variant),y=(short)(90+variant*5);
        const unsigned char *message=runtime+0x3cbf0-0x10100+topics[topic];
        for(unsigned key=0;key<sizeof keys;++key) {
            uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,ax,ip;
            allocator=(struct Allocator){0x50000,0,0};
            word(u,0x8f000,0); word(u,0x8f002,0x9000);
            unsigned args[]={topics[topic],0x3cbf,(unsigned short)x,(unsigned short)y,0,0x6000,0};
            for(unsigned i=0;i<7;++i) word(u,0x8f004+2*i,args[i]);
            check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
            check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
            name_run(u,0x347da,0x36d8b);
            unsigned char saved[8192]; struct SlicksMessageDialog d={0};
            d.painter.ui=(struct SlicksChunkyUi){pixels,palette,0,0}; d.painter.font=font;
            d.painter.measure=list_measure; d.painter.text=renderer_text; d.painter.context=n;
            if(slicks_message_dialog_open(&d,message,x,y,percent,saved,0)!=-1 ||
               d.active || memcmp(pixels,base,64000) || font[6]!=71) abort();
            if(slicks_message_dialog_open(&d,message,x,y,percent,saved,sizeof saved)) abort();
            compare_list(u,&v,&d.painter,"message open",cases);
            if(allocator.calls!=1 || allocator.frees) abort();
            /* Substitute only the blocking DOS keyboard boundary. */
            check(uc_reg_read(u,UC_X86_REG_SP,&sp)); sp+=4; ax=keys[key]; cs=0x2e0f;
            check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_CS,&cs));
            name_run(u,0x34944,0x90000);
            check(uc_reg_read(u,UC_X86_REG_AX,&ax)); check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
            if(ip || sp!=0xf004 || (ax&255)!=keys[key] || allocator.frees!=1 || slicks_message_dialog_close(&d)) abort();
            compare_list(u,&v,&d.painter,"message close",cases);
            if(memcmp(base,pixels,64000) || d.active || font[6]!=71) abort();
            ++cases;
        }
        check(uc_close(u));
    }
    printf("Original message dialog: %u open/close full-screen/font comparisons; original key results and restoration pass\n",cases);
}

int main(void)
{
    unsigned char runtime[300000],source[70000],base[64000],palette[768],font[8192],large_font[8192],pixels[64000];
    FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t runtime_size=fread(runtime,1,sizeof runtime,f); fclose(f);
    unsigned width,height; unsigned long consumed;
    long size=host_archive_load("ref/SLICKS.000","players.bmp",source,sizeof source);
    if(size<0 || slicks_decode_menu_bitmap(source,size,base,palette,&width,&height,&consumed)) abort();
    size=host_archive_load("ref/SLICKS.000","kirj.@f",source,sizeof source);
    long font_size=size<0?-1:slicks_decode_font_resource(source,size,font,sizeof font); if(font_size<0) abort();
    unsigned char code[8192]; f=fopen("build/font_string_test.bin","rb"); if(!f) abort();
    size_t code_size=fread(code,1,sizeof code,f); fclose(f);
    struct NativeText n={0}; check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&n.cpu));
    check(uc_ctl_set_cpu_model(n.cpu,UC_CPU_M68K_M68020)); check(uc_mem_map(n.cpu,0,0x400000,UC_PROT_ALL));
    check(uc_mem_write(n.cpu,0,code,code_size));
    n.bridge=(unsigned)code[4]<<24|(unsigned)code[5]<<16|(unsigned)code[6]<<8|code[7];
    n.pixels=pixels; n.fonts[0]=font; n.sizes[0]=(unsigned)font_size; n.spacing=1; n.tab=10; n.shadow=0x100;
    size=host_archive_load("ref/SLICKS.000","iso.@f",source,sizeof source);
    long large_size=size<0?-1:slicks_decode_font_resource(source,size,large_font,sizeof large_font);
    if(large_size<0) abort();
    n.fonts[2]=large_font; n.sizes[2]=(unsigned)large_size;
    if(getenv("SLICKS_CONTROLLERS_ONLY")) {
        verify_controller_pixels(runtime,runtime_size,base,palette,font,(unsigned)font_size,pixels,&n);
        check(uc_close(n.cpu)); return 0;
    }
    unsigned cases=0,original_scrollbar_leaks=0;
    const short profile_counts[]={1,2,3,100,101,127,128,255,256,512,1000,1560,2849};
    for(unsigned test=0;test<2*sizeof profile_counts/sizeof profile_counts[0];++test) {
        uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
        check(uc_mem_write(u,0x10100,runtime,runtime_size));
        struct Vga v={0}; memcpy(v.pixels,base,64000); memcpy(pixels,base,64000);
        font[6]=71; check(uc_mem_write(u,0x60000,font,font_size));
        unsigned char names[2849][21]={{0}}; for(unsigned i=0;i<2849;++i) snprintf((char *)names[i],21,"PLAYER %u",i);
        unsigned char indexed_names[2849][22]; unsigned short name_offsets[2849];
        for(unsigned i=0;i<2849;++i) {
            memcpy(indexed_names[i],names[i],21); indexed_names[i][21]=0xa5;
            name_offsets[i]=(unsigned short)(i*22);
        }
        /* Keep the maximum catalogue clear of font, labels, palette and stack. */
        check(uc_mem_write(u,0x70000,names,sizeof names));
        const unsigned char *labels=(const unsigned char *)(test&1?"MODIFY,REMOVE,CANCEL":"SELECT");
        check(uc_mem_write(u,0x66000,labels,strlen((const char *)labels)+1));
        check(uc_mem_write(u,0x67000,palette,768));
        word(u,0x3cbf0+0x71b8,0); word(u,0x3cbf0+0x71ba,0x6700);
        word(u,0x3cbf0+0x1d7b,100); word(u,0x3cbf0+0x1d87,0); word(u,0x3cbf0+0x16fd,66);
        word(u,0x3cbf0+0x1d8d,0); word(u,0x3cbf0+0x1d8f,200);
        word(u,0x3cbf0+0x1d91,0); word(u,0x3cbf0+0x1d93,79);
        struct Allocator allocator={0x50000,0,0}; uc_hook hooks[5];
        check(uc_hook_add(u,&hooks[0],UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,vga_access,&v,0xa0000,0xaffff));
        check(uc_hook_add(u,&hooks[1],UC_HOOK_INSN,list_port,&v,1,0,UC_X86_INS_OUT));
        const unsigned calls[]={0x13b07,0x139fd,0x36ca5};
        for(unsigned i=0;i<3;++i) check(uc_hook_add(u,&hooks[i+2],UC_HOOK_CODE,platform_call,&allocator,calls[i],calls[i]));
        short top=(short)(30+16*(test%4)),count=profile_counts[test/2];
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,bp,ip;
        word(u,0x8f000,0); word(u,0x8f002,0x9000);
        unsigned args[]={160,(unsigned)top,310,(unsigned)top+100,0,0x6600,0,0x7000,21,(unsigned)count,1,0,0x6000,1};
        for(unsigned i=0;i<14;++i) word(u,0x8f004+2*i,args[i]);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x30cc4,0x31155,0,50000000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_BP,&bp));
        if(ip!=0x31155-0x2e0f0 || allocator.calls!=2) abort();
        unsigned char original[64000],tinted[64000],caption[64000]; struct SlicksListRenderer r={0};
        r.ui=(struct SlicksChunkyUi){pixels,palette,0,0}; r.font=font; r.names=&names[0][0]; r.stride=21;
        if(test&1) { r.names=&indexed_names[0][0]; r.name_offsets=name_offsets; }
        r.left=160; r.top=top; r.right=310; r.bottom=top+100; r.measure=list_measure; r.text=renderer_text; r.context=&n;
        if(slicks_list_renderer_open(&r,1,count,labels,66,1,original,sizeof original,tinted,sizeof tinted,caption,sizeof caption)) abort();
        compare_list(u,&v,&r,"prepare",test); ++cases;
        word(u,0x80000+bp-0x24,0); word(u,0x80000+bp-0x22,0); word(u,0x46c,0); word(u,0x46e,0);
        const unsigned char keys[]={0,0x50,0x51,0x4f,0x48,0x49,0x47,0x4b,0x50,0x4d,0};
        for(unsigned frame=0;frame<sizeof keys;++frame) {
            if(frame) {
                uint16_t ax=keys[frame]; check(uc_reg_write(u,UC_X86_REG_AX,&ax));
                check(uc_emu_start(u,0x315b6,0x316cf,0,10000));
                check(uc_reg_read(u,UC_X86_REG_IP,&ip)); if(ip!=0x316cf-0x2e0f0) abort();
                slicks_list_dialog_key(&r.state,keys[frame]);
            }
            word(u,0x46c,frame);
            check(uc_emu_start(u,0x3115d,0x315a7,0,50000000));
            check(uc_reg_read(u,UC_X86_REG_IP,&ip)); if(ip!=0x315a7-0x2e0f0) abort();
            slicks_list_renderer_draw(&r,frame); compare_list(u,&v,&r,"draw",test); ++cases;
        }
        check(uc_emu_start(u,0x316d8,0x90000,0,50000000));
        uint16_t result; check(uc_reg_read(u,UC_X86_REG_AX,&result));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        if(ip || sp!=0xf004 || slicks_list_renderer_close(&r)!=(short)result || allocator.frees!=2) abort();
        compare_list(u,&v,&r,"close",test); ++cases;
        /* Deliberately record rather than hide the original signed-product
         * overflow: its thumb may paint outside the saved rectangle. */
        if(memcmp(v.pixels,base,64000)) {
            if(count<=100) abort();
            ++original_scrollbar_leaks;
        }
        /* Original delete-confirmation painter, before its blocking key read. */
        word(u,0x3cbf0+0x680,0); word(u,0x3cbf0+0x682,0x6000);
        check(uc_mem_write(u,0x3cbf0+0x36aa+2*21,names[2],21));
        unsigned char question[64]; check(uc_mem_read(u,0x3cbf0+0x13b5,question,sizeof question));
        cs=0x266c; bp=0xf000; sp=0xe000;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_reg_write(u,UC_X86_REG_SP,&sp)); word(u,0x8eff0,2);
        check(uc_emu_start(u,0x28b48,0x28c21,0,50000000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); if(ip!=0x28c21-0x266c0) abort();
        const struct SlicksListCaptionOps prompt={slicks_list_measure,slicks_list_colour,slicks_list_nearest,slicks_list_text,&r};
        unsigned char previous=slicks_profile_delete_prompt(&r.ui,names[2],question,66,&prompt);
        if(previous!=getword(u,0x8efee)) abort();
        compare_list(u,&v,&r,"delete prompt",test); ++cases;
        check(uc_emu_start(u,0x28cbd,0x28ce4,0,10000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); if(ip!=0x28ce4-0x266c0) abort();
        slicks_list_colour(&r,previous); compare_list(u,&v,&r,"delete font restore",test); ++cases;
        check(uc_close(u));
    }
    verify_name_pixels(runtime,runtime_size,base,palette,font,(unsigned)font_size,pixels,&n);
    verify_message_pixels(runtime,runtime_size,base,palette,font,(unsigned)font_size,pixels,&n);
    verify_colour_pixels(runtime,runtime_size,base,palette,font,(unsigned)font_size,pixels,&n);
    verify_options_pixels(runtime,runtime_size,base,palette,font,(unsigned)font_size,pixels,&n);
    verify_controller_pixels(runtime,runtime_size,base,palette,font,(unsigned)font_size,pixels,&n);
    check(uc_close(n.cpu));
    if(!original_scrollbar_leaks) abort();
    printf("Original composed list dialog: %u full-screen/font-state comparisons with DOS and native 68020 fonts pass; %u large-list cases reproduce DOS scrollbar pixels outside the restored dialog\n",cases,original_scrollbar_leaks);
    return 0;
}
