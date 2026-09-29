#define main palette_verifier_main
#include "verify_palette_remap.c"
#undef main
#include "../src/ui/arcade_title_painter.h"
#include "../src/ui/title_background.h"
#include "../src/ui/language_table.h"
struct PlanarText {uc_engine *cpu;unsigned entry;unsigned char *fonts[3];unsigned sizes[3];};
static void planar_text(void *p,unsigned char *logical,const unsigned char *font,const unsigned char *text,
    short x,short y,unsigned short flags,unsigned short shadow)
{
    struct PlanarText *n=p;unsigned index=0;while(index<3 && font!=n->fonts[index])++index;if(index==3)abort();
    check(uc_mem_write(n->cpu,0x100000,logical,262144));check(uc_mem_write(n->cpu,0x50000,font,n->sizes[index]));
    check(uc_mem_write(n->cpu,0x60000,text,strlen((const char *)text)+1));
    unsigned args[]={0x380000,0x100000,0x50000,0x60000,(unsigned short)x,(unsigned short)y,flags,shadow};unsigned char bytes[32];
    for(unsigned i=0;i<8;++i)for(unsigned j=0;j<4;++j)bytes[4*i+j]=(unsigned char)(args[i]>>(24-8*j));
    uint32_t sp=0x300000,sr=0,pc;check(uc_reg_write(n->cpu,UC_M68K_REG_SR,&sr));check(uc_reg_write(n->cpu,UC_M68K_REG_A7,&sp));
    check(uc_mem_write(n->cpu,sp,bytes,sizeof bytes));check(uc_emu_start(n->cpu,n->entry,0x380000,0,1000000));
    check(uc_reg_read(n->cpu,UC_M68K_REG_PC,&pc));if(pc!=0x380000)abort();check(uc_mem_read(n->cpu,0x100000,logical,262144));
}
/* Only libc formatting is a boundary. Language lookup, cropping, palette
 * selection, font mutation, rectangles and text execute DOS code. */
static void title_boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)size;(void)p;uint16_t ss,sp,ip,cs;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss));check(uc_reg_read(u,UC_X86_REG_SP,&sp));unsigned s=ss*16U+sp;
    if(address==0x12943){
        char format[64],out[96];check(uc_mem_read(u,getword(u,s+8)+16U*getword(u,s+10),format,sizeof format));format[63]=0;
        int n=snprintf(out,sizeof out,format,(short)getword(u,s+12),(short)getword(u,s+14));
        if(n<0 || n>=96)abort();check(uc_mem_write(u,getword(u,s+4)+16U*getword(u,s+6),out,n+1));
    }else abort();
    ip=getword(u,s);cs=getword(u,s+2);sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_IP,&ip));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    static unsigned char runtime[300000],asset[70000],palette[768],background[64002],logical[262144],fonts[3][8192];
    FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;size_t bytes=fread(runtime,1,sizeof runtime,f);fclose(f);
    if(host_archive_load("ref/SLICKS.000","mainmenu.@I",asset,sizeof asset)!=64003 || host_archive_load("ref/SLICKS.000","partII",palette,768)!=768)abort();
    slicks_title_prepare_background(asset+3,palette);
    background[0]=80;background[1]=200;
    for(unsigned y=0;y<200;++y)for(unsigned x=0;x<320;++x)background[2+(x&3)*16000+y*80+(x>>2)]=asset[3+y*320+x];
    struct PlanarText native={0};const char *names[]={"kirj.@f","pieni.@f","iso.@f"};
    for(unsigned i=0;i<3;++i){long n=host_archive_load("ref/SLICKS.000",names[i],asset,sizeof asset);long size=n<0?-1:slicks_decode_font_resource(asset,n,fonts[i],8192);if(size<0)abort();native.fonts[i]=fonts[i];native.sizes[i]=(unsigned)size;}
    unsigned char code[16384];f=fopen("build/arcade_title_pixels_test.bin","rb");if(!f)return 2;size_t code_size=fread(code,1,sizeof code,f);fclose(f);
    check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&native.cpu));check(uc_ctl_set_cpu_model(native.cpu,UC_CPU_M68K_M68020));check(uc_mem_map(native.cpu,0,0x400000,UC_PROT_ALL));check(uc_mem_write(native.cpu,0,code,code_size));
    native.entry=(unsigned)code[0]<<24|(unsigned)code[1]<<16|(unsigned)code[2]<<8|code[3];
    uc_engine *u;check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));check(uc_mem_write(u,0x10100,runtime,bytes));
    struct Vga v={0};uc_hook h;
    check(uc_hook_add(u,&h,UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,vga_access,&v,0xa0000,0xaffff));
    check(uc_hook_add(u,&h,UC_HOOK_INSN,font_port,&v,1,0,UC_X86_INS_OUT));
    check(uc_hook_add(u,&h,UC_HOOK_CODE,title_boundary,0,0x12943,0x12943));
    check(uc_mem_write(u,0x68000,palette,768));check(uc_mem_write(u,0x70000,background,sizeof background));
    word(u,0x3cbf0+0x71b8,0);word(u,0x3cbf0+0x71ba,0x6800);word(u,0x3cbf0+0x5b8,0);word(u,0x3cbf0+0x5ba,0x7000);
    for(unsigned i=0;i<3;++i){word(u,0x3cbf0+0x680+4*i,0x2000*i);word(u,0x3cbf0+0x682+4*i,0x6000);}
    word(u,0x3cbf0+0x1d7b,100);word(u,0x3cbf0+0x1d87,0);word(u,0x3cbf0+0x1d89,0);
    word(u,0x3cbf0+0x1d8d,0);word(u,0x3cbf0+0x1d8f,200);word(u,0x3cbf0+0x1d91,0);word(u,0x3cbf0+0x1d93,79);
    word(u,0x3cbf0+0x1604,1);word(u,0x3cbf0+0x1602,1);word(u,0x3cbf0+0x15fe,10);
    unsigned cases=0;const unsigned edges[]={0,120,124,128,248,252};
    for(unsigned language=0;language<=8;++language){
        unsigned char table[2000];unsigned used=0;
        if(language){
            char name[10];if(slicks_language_resource(name,language))abort();
            long size=host_archive_load("ref/SLICKS.000",name,asset,sizeof asset);
            if(size<=0 || slicks_language_table_load(asset,(unsigned)size,table,sizeof table,&used))abort();
            check(uc_mem_write(u,0x74000,table,used));
        }
        /* The table decoder is independently checked against original 3601a
         * by verify-language-table; this oracle exercises real painter lookups. */
        word(u,0x3cbf0+0x1722,0);word(u,0x3cbf0+0x1724,language?0x7400:0);
        const unsigned char *players_label=slicks_language_lookup(language?table:0,used,(const unsigned char *)"players",(const unsigned char *)"PLAYERS");
        const unsigned char *settings_label=slicks_language_lookup(language?table:0,used,(const unsigned char *)"settings",(const unsigned char *)"SETTINGS");
        const unsigned char *summary=slicks_language_lookup(language?table:0,used,(const unsigned char *)"arcade.settingstext",(const unsigned char *)"%d SECS\n%d TRACKS");
    for(unsigned alias=0;alias<3;++alias)for(unsigned row=0;row<2;++row)for(unsigned players=1;players<=4;++players)for(unsigned tick=0;tick<6;++tick){
        for(unsigned y=0;y<200;++y)for(unsigned x=0;x<320;++x){unsigned char pixel=background[2+(x&3)*16000+y*80+(x>>2)];v.pixels[y*320+x]=pixel;logical[(x&3)*65536+y*100+(x>>2)]=pixel;}
        for(unsigned i=0;i<3;++i){fonts[i][6]=71;check(uc_mem_write(u,0x60000+0x2000*i,fonts[i],native.sizes[i]));}
        word(u,0x3cbf0+0x6bd4,0x2000*alias);word(u,0x3cbf0+0x6bd6,0x6000);
        word(u,0x3cbf0+0xf1a,players);word(u,0x3cbf0+0xfa,120);word(u,0x3cbf0+0x102,3);
        unsigned char counter=edges[tick],refresh=tick&1?0:2;
        check(uc_mem_write(u,0x3cbf0+0x6b4e,&counter,1));check(uc_mem_write(u,0x3cbf0+0x1146,&refresh,1));
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xf000;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&ds));check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        word(u,0x8f000,0);word(u,0x8f002,0x9000);word(u,0x8f004,row);
        check(uc_emu_start(u,0x29afa,0x90000,0,2000000));
        struct SlicksArcadeTitlePainter painter={.logical=logical,.fonts={fonts[0],fonts[1],fonts[2],fonts[alias]},.background=background,.palette=palette,
            .players=players_label,.settings=settings_label,.summary=summary,.seconds=120,.tracks=3,.text=planar_text,.context=&native};
        if(slicks_arcade_title_paint(&painter,&counter,&refresh,row,players,(const signed char (*)[6])(runtime+0x3cbf0-0x10100+0x433)))abort();
        for(unsigned y=0;y<200;++y)for(unsigned x=0;x<320;++x)if(v.pixels[y*320+x]!=logical[(x&3)*65536+y*100+(x>>2)]){
            fprintf(stderr,"Arcade pixel mismatch language=%u alias=%u row=%u players=%u tick=%u at %u,%u DOS=%u native=%u\n",language,alias,row,players,edges[tick],x,y,v.pixels[y*320+x],logical[(x&3)*65536+y*100+(x>>2)]);return 1;}
        for(unsigned i=0;i<3;++i){unsigned char actual[8192];check(uc_mem_read(u,0x60000+0x2000*i,actual,native.sizes[i]));if(memcmp(actual,fonts[i],native.sizes[i])){fprintf(stderr,"Arcade font state mismatch alias=%u font=%u\n",alias,i);return 1;}}
        ++cases;
    }
    }
    uc_close(u);uc_close(native.cpu);printf("Original Arcade title pixels: %u full-screen and complete-font comparisons pass\n",cases);return 0;
}
