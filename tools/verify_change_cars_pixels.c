#define main palette_verifier_main
#include "verify_palette_remap.c"
#undef main
#include "../src/ui/change_cars_renderer.h"
#include "../src/ui/intermission_renderer.h"
struct IntermissionPixels { struct NativeText *native; const struct SlicksMenuIcon *icons; };
static int intermission_text(void *p,struct SlicksChunkyUi *ui,unsigned char *font,
    const unsigned char *s,short x,short y,unsigned char flags,unsigned char highlight)
{
    struct IntermissionPixels *c=p; c->native->highlight=highlight;
    renderer_text(c->native,ui,font,s,x,y,flags); return 0;
}
static int intermission_icon(void *p,struct SlicksChunkyUi *ui,short id,short x,short y)
{
    struct IntermissionPixels *c=p; if(id<1 || id>11) abort();
    renderer_icon(c->native,ui,&c->icons[id],x,y); return 0;
}
static unsigned preview_calls;
static int preview_boundary(void *p,struct SlicksChunkyUi *ui,short x,short y)
{ (void)p; (void)ui; if(x!=25 || y!=35) abort(); return 0; }
static void intermission_boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)size; (void)p; uint16_t ss,sp,cs,ip,ax=0,dx=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=16U*ss+sp;
    if(address==0x13b07) {
        unsigned bytes=getword(u,stack+4);
        if(bytes!=3034 && bytes!=484) abort(); dx=bytes==3034?0x5000:0x5100;
    } else if(address==0x35e63) dx=0x6800;
    else if(address==0x241cc) ax=2;
    else if(address==0x1a32d) {
        if(getword(u,stack+8)!=25 || getword(u,stack+10)!=35) abort(); ++preview_calls;
    } else abort();
    ip=getword(u,stack); cs=getword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void run_stage(uc_engine *u,unsigned start,unsigned end)
{
    check(uc_ctl_remove_cache(u,0,0xfffff));
    check(uc_emu_start(u,start,end,0,50000000));
    uint16_t cs,ip; check(uc_reg_read(u,UC_X86_REG_CS,&cs)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
    if(16U*cs+ip!=end) { fprintf(stderr,"Change Cars missed boundary %x at %x:%x\n",end,cs,ip); exit(1); }
}
static void allocation(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)size; (void)p;
    uint16_t sp,ss,cs,ip; check(uc_reg_read(u,UC_X86_REG_SP,&sp)); check(uc_reg_read(u,UC_X86_REG_SS,&ss));
    unsigned stack=16U*ss+sp;
    if(address==0x13b07) {
        if(getword(u,stack+4)!=8008) abort();
        uint16_t ax=0,dx=0x5000;
        check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    }
    ip=getword(u,stack); cs=getword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void compare(uc_engine *u,struct Vga *v,unsigned char *pixels,unsigned char *font,const char *stage,unsigned test)
{
    unsigned char colour; check(uc_mem_read(u,0x60006,&colour,1));
    if(memcmp(v->pixels,pixels,64000) || colour!=font[6]) {
        fprintf(stderr,"Change Cars pixels mismatch %s test=%u font=%u/%u\n",stage,test,colour,font[6]);
        for(unsigned i=0;i<64000;++i) if(v->pixels[i]!=pixels[i]) {
            fprintf(stderr,"first pixel %u,%u DOS=%u native=%u\n",i%320,i/320,v->pixels[i],pixels[i]); break;
        }
        exit(1);
    }
}
static void verify_intermission_pixels(const unsigned char *runtime,unsigned long runtime_size,
    const unsigned char *base,unsigned char *palette,unsigned char *font,struct NativeText *n,
    const struct SlicksMenuIcon *icons,unsigned char raw[10][2048],const unsigned raw_size[10])
{
    unsigned char entry[4]; check(uc_mem_read(n->cpu,20,entry,4));
    n->records_bridge=(unsigned)entry[0]<<24|(unsigned)entry[1]<<16|(unsigned)entry[2]<<8|entry[3];
    struct SlicksMenuIcon all_icons[12]; memcpy(all_icons,icons,11*sizeof *icons);
    unsigned char clock_resource[256],clock_pixels[64],clock_raw[256]={0};
    long clock_size=host_archive_load("ref/SLICKS.000","clock.@16",clock_resource,sizeof clock_resource);
    all_icons[11].pixels=clock_pixels;
    if(clock_size<8 || slicks_decode_menu_icon(clock_resource,(unsigned long)clock_size,palette,
        clock_pixels,sizeof clock_pixels,&all_icons[11].width,&all_icons[11].height) ||
        all_icons[11].width!=7 || all_icons[11].height!=7) abort();
    clock_raw[1]=0xb1; clock_raw[2]=clock_resource[5]; clock_raw[4]=clock_resource[7];
    memcpy(clock_raw+6,clock_resource+8,(size_t)clock_size-8);
    unsigned char *pixels=n->pixels; unsigned frames=0;
    for(unsigned mask=1;mask<16;++mask) {
        uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
        check(uc_mem_write(u,0x10100,runtime,runtime_size));
        struct Vga v={0}; memcpy(v.pixels,base,64000); memcpy(pixels,base,64000); font[6]=71;
        check(uc_mem_write(u,0x60000,font,n->sizes[0])); check(uc_mem_write(u,0x65000,palette,768));
        check(uc_mem_write(u,0x68000,"BASIC",6));
        uc_hook hook; check(uc_hook_add(u,&hook,UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,vga_access,&v,0xa0000,0xaffff));
        check(uc_hook_add(u,&hook,UC_HOOK_INSN,font_port,&v,1,0,UC_X86_INS_OUT));
        const unsigned calls[]={0x13b07,0x35e63,0x241cc,0x1a32d};
        for(unsigned i=0;i<4;++i) check(uc_hook_add(u,&hook,UC_HOOK_CODE,intermission_boundary,NULL,calls[i],calls[i]));
        const unsigned pointers[]={0x71b8,0x68ae};
        for(unsigned i=0;i<2;++i) { word(u,0x3cbf0+pointers[i],0); word(u,0x3cbf0+pointers[i]+2,0x6500); }
        word(u,0x3cbf0+0x680,0); word(u,0x3cbf0+0x682,0x6000);
        word(u,0x3cbf0+0x1d7b,100); word(u,0x3cbf0+0x1d87,0);
        word(u,0x3cbf0+0x1d8d,0); word(u,0x3cbf0+0x1d8f,200);
        word(u,0x3cbf0+0x1d91,0); word(u,0x3cbf0+0x1d93,79);
        word(u,0x3cbf0+0x1722,0); word(u,0x3cbf0+0x1724,0);
        word(u,0x3cbf0+0x628,0); word(u,0x3cbf0+0x62a,0); word(u,0x3cbf0+0x62c,0x6900);
        for(unsigned i=0;i<10;++i) {
            check(uc_mem_write(u,0x70000+2048*i,raw[i],raw_size[i]));
            word(u,0x3cbf0+0x4e44+4*i,2048*i); word(u,0x3cbf0+0x4e46+4*i,0x7000);
        }
        check(uc_mem_write(u,0x78000,clock_raw,(size_t)clock_size-2));
        word(u,0x3cbf0+0x4c30,0); word(u,0x3cbf0+0x4c32,0x7800);
        struct SlicksIntermissionContent c={.track_name=(const unsigned char *)"BASIC",.slash=(const unsigned char *)"/",.track_total=2,.fastest=100};
        const unsigned char *data=runtime+0x3cbf0-0x10100; unsigned char count=0;
        for(unsigned i=0;i<4;++i) {
            c.roles[i]=(mask&(1U<<i))?(i&1?-1:1):0; count+=c.roles[i]!=0;
            c.vehicles[i]=(signed char)((mask+i*3)%10); c.points[i]=(short)(i==0?-32768:i==1?-1:mask*173+i*7);
            c.laps[i]=i&1?100:200; c.names[i]=(const unsigned char *)"DRIVER";
            c.labels[i]=data+(data[0x7c2+4*i]|data[0x7c3+4*i]<<8);
            word(u,0x3cbf0+0x44c+2*i,i); check(uc_mem_write(u,0x3cbf0+0x36aa+21*i,"DRIVER",7));
            word(u,0x3cbf0+0x6826+2*i,(unsigned short)c.points[i]);
            word(u,0x3cbf0+0x4c06+4*i,c.laps[i]); word(u,0x3cbf0+0x4c08+4*i,0);
        }
        check(uc_mem_write(u,0x3cbf0+0x4bc6,c.roles,4)); check(uc_mem_write(u,0x3cbf0+0x4bc2,c.vehicles,4));
        check(uc_mem_write(u,0x3cbf0+0x4c16,&count,1));
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000,bp;
        word(u,0x8f000,0); word(u,0x8f002,0x9000); word(u,0x8f004,100); word(u,0x8f006,0);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        preview_calls=0; run_stage(u,0x241e2,0x24702); if(preview_calls!=1) abort();
        struct IntermissionPixels context={n,all_icons};
        struct SlicksRecordsRenderer surface={.ui={pixels,palette,0,0},.fonts={font,0},.text=intermission_text,.icon=intermission_icon,.context=&context};
        struct SlicksIntermissionRenderer r={.surface=&surface,.fastest_icon=11};
        struct SlicksIntermissionMenu m; unsigned char buttons[3024],cars[320];
        if(slicks_intermission_renderer_open(&r,&m,&c,palette,buttons,sizeof buttons,cars,sizeof cars,preview_boundary,NULL)) abort();
        compare(u,&v,pixels,font,"intermission open",mask); ++frames;
        check(uc_reg_read(u,UC_X86_REG_BP,&bp));
        for(unsigned row=0;row<4;++row) {
            unsigned char local[3]={1,1,(unsigned char)row}; check(uc_mem_write(u,0x80000+bp-13,local,3));
            run_stage(u,0x2453a,0x24702);
            m.selected=(signed char)row; m.redraw=m.cars_redraw=1;
            if(slicks_intermission_renderer_draw(&r,&m,&c)) abort();
            compare(u,&v,pixels,font,"intermission redraw",mask); ++frames;
        }
        check(uc_close(u));
    }
    printf("Original intermission UI composition: %u full-screen/font comparisons pass (real clock marker; track preview tested separately)\n",frames);
}
int main(void)
{
    unsigned char runtime[300000],source[70000],base[64000],palette[768],font[8192],pixels[64000];
    FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t size=fread(runtime,1,sizeof runtime,f); fclose(f);
    unsigned w,h; unsigned long consumed;
    long loaded=host_archive_load("ref/SLICKS.000","players.bmp",source,sizeof source);
    if(loaded<0 || slicks_decode_menu_bitmap(source,loaded,base,palette,&w,&h,&consumed)) abort();
    loaded=host_archive_load("ref/SLICKS.000","kirj.@f",source,sizeof source);
    long font_size=loaded<0?-1:slicks_decode_font_resource(source,loaded,font,sizeof font); if(font_size<0) abort();
    unsigned char code[8192]; f=fopen("build/font_string_test.bin","rb"); if(!f) return 2;
    size_t code_size=fread(code,1,sizeof code,f); fclose(f);
    struct NativeText n={0}; check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&n.cpu));
    check(uc_ctl_set_cpu_model(n.cpu,UC_CPU_M68K_M68020)); check(uc_mem_map(n.cpu,0,0x400000,UC_PROT_ALL));
    check(uc_mem_write(n.cpu,0,code,code_size));
    n.bridge=(unsigned)code[4]<<24|(unsigned)code[5]<<16|(unsigned)code[6]<<8|code[7];
    n.pixels=pixels; n.fonts[0]=font; n.sizes[0]=(unsigned)font_size; n.spacing=1; n.tab=10; n.shadow=0x100;
    f=fopen("build/hud_icon_test.bin","rb"); if(!f) return 2;
    code_size=fread(code,1,sizeof code,f); fclose(f);
    check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&n.icon_cpu));
    check(uc_ctl_set_cpu_model(n.icon_cpu,UC_CPU_M68K_M68020));
    check(uc_mem_map(n.icon_cpu,0,0x400000,UC_PROT_ALL)); check(uc_mem_write(n.icon_cpu,0,code,code_size));
    struct SlicksMenuIcon icons[11]={{0}};
    unsigned char icon_pixels[10][1024],raw[10][2048]; unsigned raw_size[10];
    for(unsigned i=0;i<10;++i) {
        char name[]="auto01.@16"; name[5]=(char)('0'+i);
        loaded=host_archive_load("ref/SLICKS.000",i?name:"carimage16",source,sizeof source);
        if(loaded<8 || loaded>2048) abort();
        icons[i+1].pixels=icon_pixels[i];
        if(slicks_decode_menu_icon(source,loaded,palette,icon_pixels[i],1024,&icons[i+1].width,&icons[i+1].height)) abort();
        memset(raw[i],0,2048); raw[i][1]=0xb1; raw[i][2]=source[5]; raw[i][4]=source[7];
        memcpy(raw[i]+6,source+8,loaded-8); raw_size[i]=(unsigned)loaded-2;
    }
    unsigned draws=0;
    for(unsigned test=0;test<8;++test) {
        uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
        check(uc_mem_write(u,0x10100,runtime,size));
        struct Vga v={0}; memcpy(v.pixels,base,64000); memcpy(pixels,base,64000);
        font[6]=71; check(uc_mem_write(u,0x60000,font,font_size)); check(uc_mem_write(u,0x65000,palette,768));
        uc_hook hook;
        check(uc_hook_add(u,&hook,UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,vga_access,&v,0xa0000,0xaffff));
        check(uc_hook_add(u,&hook,UC_HOOK_INSN,font_port,&v,1,0,UC_X86_INS_OUT));
        check(uc_hook_add(u,&hook,UC_HOOK_CODE,allocation,NULL,0x13b07,0x13b07));
        check(uc_hook_add(u,&hook,UC_HOOK_CODE,allocation,NULL,0x139fd,0x139fd));
        word(u,0x3cbf0+0x71b8,0); word(u,0x3cbf0+0x71ba,0x6500);
        word(u,0x3cbf0+0x680,0); word(u,0x3cbf0+0x682,0x6000);
        word(u,0x3cbf0+0x1d7b,100); word(u,0x3cbf0+0x1d87,0); word(u,0x3cbf0+0x1d89,0);
        word(u,0x3cbf0+0x1d8d,0); word(u,0x3cbf0+0x1d8f,200);
        word(u,0x3cbf0+0x1d91,0); word(u,0x3cbf0+0x1d93,79);
        unsigned char count=1+test%4; check(uc_mem_write(u,0x3cbf0+0x4c16,&count,1));
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000,x=test<4?210:211,y=71;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp)); check(uc_reg_write(u,UC_X86_REG_SI,&x)); check(uc_reg_write(u,UC_X86_REG_DI,&y));
        run_stage(u,0x24862,0x24956);
        struct SlicksPlayerMenuRenderer surface={0}; surface.ui=(struct SlicksChunkyUi){pixels,palette,0,0};
        surface.fonts[0]=font; surface.text=renderer_text; surface.icon=renderer_icon; surface.context=&n;
        surface.icons=icons; surface.icon_count=11;
        struct SlicksChangeCarsRenderer r={0}; r.surface=&surface;
        struct SlicksChangeCarsDialog d; unsigned char original[4000],decorated[4000];
        if(slicks_change_cars_renderer_open(&r,&d,x,y,count,runtime+0x3cbf0-0x10100+0xc3d,
            original,sizeof original,decorated,sizeof decorated)) abort();
        compare(u,&v,pixels,font,"open",test);
        for(unsigned i=0;i<10;++i) {
            check(uc_mem_write(u,0x70000+2048*i,raw[i],raw_size[i]));
            word(u,0x3cbf0+0x4e44+4*i,2048*i); word(u,0x3cbf0+0x4e46+4*i,0x7000);
        }
        signed char roles[4],vehicles[4];
        for(unsigned i=0;i<4;++i) {
            roles[i]=((i+test)%4<count)?(i&1?-1:1):0;
            vehicles[i]=(signed char)((test+i*3)%10);
        }
        check(uc_mem_write(u,0x3cbf0+0x4bc6,roles,4));
        check(uc_mem_write(u,0x3cbf0+0x4bc2,vehicles,4));
        for(unsigned row=0;row<count;++row) {
            unsigned char zero=0,selected=(unsigned char)row;
            check(uc_mem_write(u,0x8efe9,&zero,1)); check(uc_mem_write(u,0x8eff2,&selected,1));
            run_stage(u,0x24992,0x24a51);
            d.row=(signed char)row;
            if(slicks_change_cars_renderer_draw(&r,&d,roles,vehicles)) abort();
            compare(u,&v,pixels,font,"rows",test); ++draws;
        }
        run_stage(u,0x24af9,0x24b50);
        if(slicks_change_cars_renderer_close(&r)) abort();
        compare(u,&v,pixels,font,"close",test);
        if(memcmp(pixels,base,64000) || font[6]!=71) abort();
        check(uc_close(u));
    }
    verify_intermission_pixels(runtime,size,base,palette,font,&n,icons,raw,raw_size);
    check(uc_close(n.cpu));
    check(uc_close(n.icon_cpu));
    printf("Original Change Cars: 8 full-screen opening/closing and %u row redraws pass (real DOS and 68020 font/icon painters)\n",draws);
    return 0;
}
