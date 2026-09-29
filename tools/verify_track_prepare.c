/* Full-pixel oracle: execute original DOS painting and actual 68020 text.
 * Asset loading/fades/snapshot ownership are outside this painting test. */
#define main palette_verifier_main
#include "verify_palette_remap.c"
#undef main
#include "../src/ui/track_menu_renderer.h"

static const unsigned char *pixel_track_name(void *p,unsigned index)
{
    (void)p; static unsigned char text[32];
    snprintf((char *)text,sizeof text,"TRACK%03u",index); return text;
}
static void track_pixel_boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)size; (void)p; uint16_t ss,sp,cs,ip;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    if(address==0x35e63) {
        const unsigned char *name=pixel_track_name(0,getword(u,stack+8));
        check(uc_mem_write(u,0x57000,name,strlen((const char *)name)+1));
        uint16_t ax=0,dx=0x5700;
        check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    }
    ip=getword(u,stack); cs=getword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void track_pixel_port(uc_engine *u,uint32_t port,int size,uint32_t value,void *p)
{
    struct Vga *v=p;
    if(port==0x3ce && size==1 && value==4) return;
    if(port==0x3cf && size==1 && value<4) { v->read_plane=value; return; }
    font_port(u,port,size,value,p);
}
static unsigned verify_track_redraw(uc_engine *u,struct Vga *v,
    struct SlicksPlayerMenuRenderer *surface,short total,const unsigned char *data)
{
    static unsigned char snapshot[64002],saved[64000];
    memcpy(saved,surface->ui.pixels,sizeof saved); surface->saved=saved;
    snapshot[0]=80; snapshot[1]=200;
    for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;++x)
        snapshot[2+(x&3)*16000+y*80+x/4]=saved[y*320+x];
    check(uc_mem_write(u,0x70000,snapshot,sizeof snapshot));
    word(u,0x3cbf0+0x5b8,0); word(u,0x3cbf0+0x5ba,0x7000);
    word(u,0x3cbf0+0x62a,0); word(u,0x3cbf0+0x62c,0x5800);
    word(u,0x3cbf0+0x1d8d,0); word(u,0x3cbf0+0x1d8f,200);
    word(u,0x3cbf0+0x1d91,0); word(u,0x3cbf0+0x1d93,79);
    struct SlicksTrackRenderer r;
    if(slicks_track_renderer_init(&r,surface,66,pixel_track_name,0)) abort();
    struct SlicksTrackMenuLabels labels={0};
    for(unsigned i=0;i<6;++i) labels.actions[i]=data+(data[0x10b6+4*i]|data[0x10b7+4*i]<<8);
    labels.random_on=data+0x12e0; labels.random_off=data+0x12eb; labels.separator=data+0x1307;
    uc_hook hooks[2];
    check(uc_hook_add(u,&hooks[0],UC_HOOK_CODE,track_pixel_boundary,0,0x35e63,0x35e63));
    check(uc_hook_add(u,&hooks[1],UC_HOOK_CODE,track_pixel_boundary,0,0x3b09c,0x3b09c));
    /* Build the DOS selection tint by executing the original caller. */
    check(uc_mem_write(u,0x8f000-0x31a,surface->ui.palette,768));
    check(uc_emu_start(u,0x26df3,0x26e17,0,10000000));
    unsigned cases=0;
    for(unsigned column=0;column<7;++column) for(unsigned variant=0;variant<4;++variant) {
        short cursor=total?(short)((total-1)*variant/3):-1,top=cursor>21?cursor-21:0;
        struct SlicksTrackMenu menu={cursor,top,variant==3?cursor:-2,
            variant==2?-12:(short)(total+10),(unsigned char)column,0,(unsigned char)(variant&1)};
        short tracks[256]; unsigned count=variant==0?0:variant==1?(unsigned)total:(unsigned)total/2;
        for(unsigned i=0;i<count;++i) { tracks[i]=(short)(i*2%total); word(u,0x58000+2*i,tracks[i]); }
        struct SlicksTrackPlaylist playlist={tracks,(unsigned short)count,256};
        word(u,0x3cbf0+0x90,count); word(u,0x3cbf0+0x626,menu.random_count);
        word(u,0x3cbf0+0x624,menu.random_order); word(u,0x3cbf0+0x10b2,cursor);
        word(u,0x3cbf0+0x10b4,top); word(u,0x8effa,menu.previous);
        unsigned char small[3]={menu.column,r.scroll_colour,22};
        check(uc_mem_write(u,0x8effd,small,sizeof small));
        uint16_t cs=0x266c,sp=0xeb00,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x270b9,0x274d5,0,50000000)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        if(slicks_track_renderer_draw(&r,&menu,total,&playlist,&labels)) abort();
        if(ip!=0x274d5-0x266c0 || memcmp(surface->ui.pixels,v->pixels,64000) ||
           getword(u,0x8effa)!=(unsigned short)menu.previous ||
           getword(u,0x3cbf0+0x626)!=(unsigned short)menu.random_count) {
            fprintf(stderr,"Tracks redraw mismatch total=%d column=%u variant=%u ip=%x\n",total,column,variant,ip);
            for(unsigned i=0;i<64000;++i) if(surface->ui.pixels[i]!=v->pixels[i]) {
                fprintf(stderr,"pixel %u,%u native=%u DOS=%u\n",i%320,i/320,surface->ui.pixels[i],v->pixels[i]); break;
            }
            exit(1);
        }
        unsigned char font[8192]; struct NativeText *n=surface->context;
        check(uc_mem_read(u,0x60000,font,n->sizes[0]));
        if(memcmp(font,surface->fonts[0],n->sizes[0])) abort();
        ++cases;
    }
    for(unsigned i=0;i<2;++i) check(uc_hook_del(u,hooks[i]));
    return cases;
}

int main(void)
{
    unsigned char runtime[300000];
    FILE *file=fopen("disasm/runtime.bin","rb"); if(!file) return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,file); fclose(file);
    if(bytes<200000 || bytes==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,bytes));
    static unsigned char source[70000],base[64000],palette[768],fonts[3][8192],pixels[64000],saved[64000];
    unsigned short width,height; unsigned sizes[3];
    long loaded=host_archive_load("ref/SLICKS.000","trckmenu.@I",source,sizeof source);
    if(loaded<0 || slicks_decode_indexed_menu_icon(source,(unsigned long)loaded,
        base,sizeof base,&width,&height) || width!=320 || height!=200) abort();
    if(host_archive_load("ref/SLICKS.000","trckmenu.@p",palette,sizeof palette)!=768) abort();
    const char *names[]={"kirj.@f","pieni.@f","iso.@f"};
    for(unsigned i=0;i<3;++i) {
        loaded=host_archive_load("ref/SLICKS.000",names[i],source,sizeof source);
        long size=loaded<0?-1:slicks_decode_font_resource(source,(unsigned long)loaded,fonts[i],sizeof fonts[i]);
        if(size<0) abort(); sizes[i]=(unsigned)size;
    }
    unsigned char code[8192]; file=fopen("build/font_string_test.bin","rb"); if(!file) return 2;
    bytes=fread(code,1,sizeof code,file); fclose(file); if(bytes<8 || bytes==sizeof code) abort();
    struct NativeText n={0}; check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&n.cpu));
    check(uc_ctl_set_cpu_model(n.cpu,UC_CPU_M68K_M68020));
    check(uc_mem_map(n.cpu,0,0x400000,UC_PROT_ALL)); check(uc_mem_write(n.cpu,0,code,bytes));
    n.bridge=(unsigned)code[4]<<24|(unsigned)code[5]<<16|(unsigned)code[6]<<8|code[7];
    n.pixels=pixels; n.spacing=1; n.tab=10; n.shadow=0x100;
    struct Vga v={0}; struct TextTrace unused={0}; uc_hook hooks[3];
    check(uc_hook_add(u,&hooks[0],UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,vga_access,&v,0xa0000,0xaffff));
    check(uc_hook_add(u,&hooks[1],UC_HOOK_CODE,prepare_boundary,&unused,0x2e0f8,0x2e0f8));
    check(uc_hook_add(u,&hooks[2],UC_HOOK_INSN,track_pixel_port,&v,1,0,UC_X86_INS_OUT));
    check(uc_mem_write(u,0x55000,palette,sizeof palette));
    word(u,0x3cbf0+0x71b8,0); word(u,0x3cbf0+0x71ba,0x5500);
    word(u,0x3cbf0+0x1d7b,100); word(u,0x3cbf0+0x1d87,0);
    word(u,0x3cbf0+0x1722,0); word(u,0x3cbf0+0x1724,0);
    unsigned char title[128],footer[128];
    check(uc_mem_read(u,0x3cbf0+0x1296,title,sizeof title));
    check(uc_mem_read(u,0x3cbf0+0x129d,footer,sizeof footer));
    const unsigned percentages[]={66,0,50,100,127,128,255};
    const short totals[]={0,1,22,23,195,256}; unsigned cases=0,redraw_cases=0;
    for(unsigned language=0;language<=8;++language){
    const unsigned char *heading=menu_language_title(u,language,title);
    for(unsigned p=0;p<sizeof percentages/sizeof percentages[0];++p)
    for(unsigned t=0;t<sizeof totals/sizeof totals[0];++t) {
        memcpy(pixels,base,sizeof pixels); memcpy(v.pixels,base,sizeof base);
        for(unsigned f=0;f<3;++f) {
            n.fonts[f]=fonts[f]; n.sizes[f]=sizes[f];
            check(uc_mem_write(u,0x60000+0x2000*f,fonts[f],sizes[f]));
            word(u,0x3cbf0+0x680+4*f,0x2000*f); word(u,0x3cbf0+0x682+4*f,0x6000);
        }
        word(u,0x3cbf0+0x16fd,percentages[p]); word(u,0x3cbf0+0x4da8,totals[t]);
        word(u,0x8efff,22);
        struct SlicksPlayerMenuRenderer renderer={0};
        renderer.ui=(struct SlicksChunkyUi){pixels,palette,0,0};
        for(unsigned f=0;f<3;++f) renderer.fonts[f]=fonts[f];
        renderer.text=renderer_text; renderer.context=&n; renderer.saved=saved;
        struct SlicksTrackRenderer track_renderer;
        if(slicks_track_renderer_init(&track_renderer,&renderer,(unsigned char)percentages[p],pixel_track_name,0) ||
           slicks_track_renderer_prepare(&track_renderer,heading,footer,totals[t],(unsigned char)percentages[p]) ||
           memcmp(saved,pixels,sizeof saved)) abort();
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_emu_start(u,0x26e62,0x2702e,0,50000000)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        if(ip!=0x2702e - 0x266c0 || memcmp(pixels,v.pixels,sizeof pixels)) {
            fprintf(stderr,"Tracks preparation mismatch percent=%u total=%d ip=%x\n",percentages[p],totals[t],ip);
            for(unsigned i=0;i<64000;++i) if(pixels[i]!=v.pixels[i]) {
                fprintf(stderr,"pixel %u,%u native=%u DOS=%u\n",i%320,i/320,pixels[i],v.pixels[i]); break;
            }
            return 1;
        }
        for(unsigned f=0;f<3;++f) {
            check(uc_mem_read(u,0x60000+0x2000*f,source,sizes[f]));
            if(memcmp(source,fonts[f],sizes[f])) { fprintf(stderr,"Font %u mismatch\n",f); return 1; }
        }
        ++cases;
        if(!p && !language) redraw_cases+=verify_track_redraw(u,&v,&renderer,totals[t],runtime+0x3cbf0-0x10100);
    }
    }
    check(uc_close(u)); check(uc_close(n.cpu));
    printf("Original Tracks preparation: %u full-screen/font comparisons, actual assets and DOS/68020 text, scrollbar boundary and signed tint percentages pass\n",cases);
    printf("Original Tracks redraw: %u sequential full-screen/font/state comparisons, all columns, selection tints, scrolling, count clamping and no-redraw pass\n",redraw_cases);
    return 0;
}
