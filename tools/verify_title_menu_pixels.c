/* Original title composition, language lookup and rasterizers versus the
 * complete production 68020 menu/text/bevel path, with no drawing hooks. */
#define main palette_verifier_main
#include "verify_palette_remap.c"
#undef main
#include "../src/ui/title_background.h"
static unsigned big32(const unsigned char *p)
{return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void native_long(uc_engine *m,unsigned at,unsigned v)
{unsigned char b[]={v>>24,v>>16,v>>8,v};check(uc_mem_write(m,at,b,4));}
static void native_reg(uc_engine *m,int r,unsigned v)
{check(uc_reg_write(m,r,&v));}
int main(void)
{
    static unsigned char runtime[300000],code[131072],elf[200000],asset[70000],palette[768],font[8192],logical[262144],actual[262144],base[64000];
    FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    size_t size=fread(runtime,1,sizeof runtime,f);fclose(f);
    f=fopen("build/title_menu_pixels_test.elf","rb");if(!f)return 2;
    size_t elf_size=fread(elf,1,sizeof elf,f);fclose(f);
    if(elf_size<52 || memcmp(elf,"\177ELF\1\2",6))abort();
    unsigned sections=big32(elf+32),stride=(unsigned)elf[46]*256+elf[47],count=(unsigned)elf[48]*256+elf[49];
    if(stride<40 || sections>elf_size || count>(elf_size-sections)/stride)abort();
    for(unsigned i=0;i<count;++i){const unsigned char *s=elf+sections+i*stride;
        if(big32(s+4)!=1 || !(big32(s+8)&2))continue;
        unsigned at=big32(s+12),offset=big32(s+16),bytes=big32(s+20);
        if(at>sizeof code || bytes>sizeof code-at || offset>elf_size || bytes>elf_size-offset)abort();
        memcpy(code+at,elf+offset,bytes);}
    if(host_archive_load("ref/SLICKS.000","mainmenu.@I",asset,sizeof asset)!=64003 ||
       host_archive_load("ref/SLICKS.000","partII",palette,sizeof palette)!=768)abort();
    slicks_title_prepare_background(asset+3,palette);memcpy(base,asset+3,sizeof base);
    long loaded=host_archive_load("ref/SLICKS.000","iso.@f",asset,sizeof asset);
    long font_size=loaded<0?-1:slicks_decode_font_resource(asset,(unsigned)loaded,font,sizeof font);
    if(font_size<=0)abort();
    uc_engine *x,*m;check(uc_open(UC_ARCH_X86,UC_MODE_16,&x));check(uc_mem_map(x,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(x,0x10100,runtime,size));
    check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&m));check(uc_ctl_set_cpu_model(m,UC_CPU_M68K_M68020));
    check(uc_mem_map(m,0,0x400000,UC_PROT_ALL));check(uc_mem_write(m,0,code,sizeof code));
    check(uc_mem_write(x,0x55000,palette,768));check(uc_mem_write(m,0x50000,palette,768));
    word(x,0x3cbf0+0x71b8,0);word(x,0x3cbf0+0x71ba,0x5500);
    word(x,0x3cbf0+0x688,0);word(x,0x3cbf0+0x68a,0x6000);
    word(x,0x3cbf0+0x1d7b,100);word(x,0x3cbf0+0x1d87,0);word(x,0x3cbf0+0x1d89,0);
    word(x,0x3cbf0+0x1d8d,0);word(x,0x3cbf0+0x1d8f,200);word(x,0x3cbf0+0x1d91,0);word(x,0x3cbf0+0x1d93,79);
    word(x,0x3cbf0+0x1604,1);word(x,0x3cbf0+0x1602,1);word(x,0x3cbf0+0x15fe,10);word(x,0x3cbf0+0x1600,9);
    native_long(m,big32(code+8),0x60000);unsigned char shadow[]={0,9};check(uc_mem_write(m,big32(code+12),shadow,2));
    struct Vga v={0};uc_hook hook;
    check(uc_hook_add(x,&hook,UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,vga_access,&v,0xa0000,0xaffff));
    check(uc_hook_add(x,&hook,UC_HOOK_INSN,font_port,&v,1,0,UC_X86_INS_OUT));
    unsigned cases=0;
    for(unsigned language=0;language<=8;++language){
        for(unsigned i=0;i<7;++i){unsigned char key[]="menu1";key[4]=(unsigned char)('1'+i);
            const unsigned char *text=menu_language_title(x,language,key);
            unsigned at=0x70000+128*i;
            check(uc_mem_write(m,at,text,strlen((const char *)text)+1));native_long(m,big32(code+4)+4*i,at);}
        for(unsigned row=0;row<7;++row){
            memcpy(v.pixels,base,sizeof base);memset(logical,0,sizeof logical);
            for(unsigned y=0;y<200;++y)for(unsigned col=0;col<320;++col)
                logical[(col&3)*65536+y*100+(col>>2)]=base[y*320+col];
            check(uc_mem_write(m,0x100000,logical,sizeof logical));
            check(uc_mem_write(x,0x60000,font,font_size));check(uc_mem_write(m,0x60000,font,font_size));
            uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xe000,bp=0xf000,ip;
            check(uc_reg_write(x,UC_X86_REG_CS,&cs));check(uc_reg_write(x,UC_X86_REG_DS,&ds));check(uc_reg_write(x,UC_X86_REG_SS,&ss));
            check(uc_reg_write(x,UC_X86_REG_SP,&sp));check(uc_reg_write(x,UC_X86_REG_BP,&bp));
            word(x,0x8f006,row);word(x,0x8efff,60);word(x,0x8effb,120);
            check(uc_emu_start(x,0x29852,0x29928,0,2000000));check(uc_reg_read(x,UC_X86_REG_IP,&ip));if(ip!=0x29928-0x266c0)abort();
            native_reg(m,UC_M68K_REG_SR,0);native_reg(m,UC_M68K_REG_A7,0x300000);native_long(m,0x300000,0x380000);
            native_reg(m,UC_M68K_REG_A0,0x100000);native_reg(m,UC_M68K_REG_A1,0x50000);
            native_reg(m,UC_M68K_REG_D0,60);native_reg(m,UC_M68K_REG_D1,120);native_reg(m,UC_M68K_REG_D2,row);native_reg(m,UC_M68K_REG_D7,0);
            check(uc_emu_start(m,big32(code),0x380000,0,2000000));uint32_t pc;check(uc_reg_read(m,UC_M68K_REG_PC,&pc));if(pc!=0x380000)abort();
            check(uc_mem_read(m,0x100000,actual,sizeof actual));
            for(unsigned y=0;y<200;++y)for(unsigned col=0;col<320;++col){unsigned at=(col&3)*65536+y*100+(col>>2);
                if(v.pixels[y*320+col]!=actual[at]){fprintf(stderr,"Title pixel mismatch language=%u row=%u at %u,%u\n",language,row,col,y);return 1;}
                if(actual[at]!=logical[at] && (col<108 || col>=208 || y<77 || y>=174)){
                    fprintf(stderr,"Title dirty crop insufficient language=%u row=%u at %u,%u\n",language,row,col,y);return 1;}}
            unsigned char original_font[8192],native_font[8192];check(uc_mem_read(x,0x60000,original_font,font_size));check(uc_mem_read(m,0x60000,native_font,font_size));
            if(memcmp(original_font,native_font,font_size))abort();++cases;
        }
    }
    uc_close(x);uc_close(m);printf("Original title menu pixels: %u full-screen/font and dirty-crop comparisons across eight languages plus fallback pass\n",cases);return 0;
}
