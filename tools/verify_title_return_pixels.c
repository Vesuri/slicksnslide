/* Compare live native return snapshots with the original complete title
 * wrapper and its ordinary/Arcade renderers. No drawing calls are mocked. Prepared
 * background/decoded-font helpers have separate original-instruction oracles. */
#define main palette_verifier_main
#include "verify_palette_remap.c"
#undef main
#include "../src/ui/title_background.h"
#include "../src/ui/menu_icon.h"

/* Arcade's libc formatting boundary; all rendering and lookup run DOS code. */
static void format_summary(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)address;(void)size;(void)context;uint16_t ss,sp,ip,cs;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss));check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned s=ss*16U+sp;char format[64],out[96];
    check(uc_mem_read(u,getword(u,s+8)+16U*getword(u,s+10),format,sizeof format));format[63]=0;
    /* These are the original one/two-integer player and summary formats,
     * not arbitrary target strings accepted as host printf programs. */
    unsigned conversions=0;
    for(unsigned i=0;format[i];++i)if(format[i]=='%'){
        if(format[++i]!='d' || ++conversions>2)abort();
    }
    if(!conversions)abort();
    int n=snprintf(out,sizeof out,format,(short)getword(u,s+12),(short)getword(u,s+14));
    if(n<0 || n>=96)abort();check(uc_mem_write(u,getword(u,s+4)+16U*getword(u,s+6),out,n+1));
    ip=getword(u,s);cs=getword(u,s+2);sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_IP,&ip));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}

int main(int argc,char **argv)
{
    if(argc!=2 && argc!=3)return 2;
    unsigned expected=2;
    if(argc==3){char *end;unsigned long n=strtoul(argv[2],&end,10);
        if(!*argv[2] || *end || !n || n>1000)return 2;expected=(unsigned)n;}
    static unsigned char runtime[300000],asset[70000],palette[768],background[64002],logical[262144],font[8192];
    FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f);fclose(f);
    if(bytes<200000 || bytes==sizeof runtime)return 2;
    uc_engine *u;check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));check(uc_mem_write(u,0x10100,runtime,bytes));
    if(host_archive_load("ref/SLICKS.000","mainmenu.@I",asset,sizeof asset)!=64003 ||
       host_archive_load("ref/SLICKS.000","partII",palette,sizeof palette)!=768)abort();
    slicks_title_prepare_background(asset+3,palette);
    background[0]=80;background[1]=200;
    for(unsigned y=0;y<200;++y)for(unsigned x=0;x<320;++x)
        background[2+(x&3)*16000+y*80+(x>>2)]=asset[3+y*320+x];
    check(uc_mem_write(u,0x70000,background,sizeof background));
    check(uc_mem_write(u,0x55000,palette,sizeof palette));
    word(u,0x3cbf0+0x71b8,0);word(u,0x3cbf0+0x71ba,0x5500);
    word(u,0x3cbf0+0x5b8,0);word(u,0x3cbf0+0x5ba,0x7000);
    const char *fonts[]={"kirj.@f","pieni.@f","iso.@f"};
    for(unsigned i=0;i<3;++i){
        long n=host_archive_load("ref/SLICKS.000",fonts[i],asset,sizeof asset);
        long size=n<0?-1:slicks_decode_font_resource(asset,n,font,sizeof font);if(size<0)abort();
        check(uc_mem_write(u,0x60000+0x2000*i,font,size));
        word(u,0x3cbf0+0x680+4*i,0x2000*i);word(u,0x3cbf0+0x682+4*i,0x6000);
    }
    const char *icons[]={"val1.@I","val2.@I","pel_on.@I","pel_ei.@I","pel_t.@I"};
    const unsigned slots[]={0x4c24,0x4c28,0x2fc4,0x2fc8,0x2fcc};
    for(unsigned i=0;i<5;++i){
        unsigned char pixels[64],packed[100]={0,0xb2};unsigned short w,h;
        long n=host_archive_load("ref/SLICKS.000",icons[i],asset,sizeof asset);
        if(n<0 || slicks_decode_indexed_menu_icon(asset,n,pixels,sizeof pixels,&w,&h))abort();
        unsigned stride=(w+3)/4;
        if(4+4*stride*h>sizeof packed)abort();
        packed[2]=stride;packed[3]=h;
        for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x)
            packed[4+(x&3)*stride*h+y*stride+(x>>2)]=pixels[y*w+x];
        check(uc_mem_write(u,0x68000+0x100*i,packed,sizeof packed));
        word(u,0x3cbf0+slots[i],0x100*i);word(u,0x3cbf0+slots[i]+2,0x6800);
    }
    word(u,0x3cbf0+0x1d7b,100);word(u,0x3cbf0+0x1d87,0);word(u,0x3cbf0+0x1d89,0);
    word(u,0x3cbf0+0x1d8d,0);word(u,0x3cbf0+0x1d8f,200);word(u,0x3cbf0+0x1d91,0);word(u,0x3cbf0+0x1d93,79);
    word(u,0x3cbf0+0x1604,1);word(u,0x3cbf0+0x1602,1);word(u,0x3cbf0+0x15fe,10);
    struct Vga v={0};uc_hook hook;
    check(uc_hook_add(u,&hook,UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,vga_access,&v,0xa0000,0xaffff));
    check(uc_hook_add(u,&hook,UC_HOOK_INSN,font_port,&v,1,0,UC_X86_INS_OUT));
    check(uc_hook_add(u,&hook,UC_HOOK_CODE,format_summary,0,0x12943,0x12943));
    char path[1024],line[2048];snprintf(path,sizeof path,"%s/debug.log",argv[1]);
    f=fopen(path,"r");if(!f)return 2;unsigned cases=0,registered=0,previous_counter=0,previous_phase=0;
    while(fgets(line,sizeof line,f)){
        unsigned index,row,counter,phase,language=1;int selected,total,mode,weapons,inventory,roles[4],players=1,seconds=120,tracks=3;
        int fields=sscanf(line,"TITLE_RETURN_STATE %u %u %u %d %d %d %d %d %d %d %d %d %u %u %d %d %d",
            &index,&row,&counter,&selected,&total,&mode,&weapons,&inventory,&roles[0],&roles[1],&roles[2],&roles[3],&phase,&language,&players,&seconds,&tracks);
        /* Older captures explicitly rejected Arcade and non-English fixtures. */
        if(fields!=17 && !(fields==13 && mode>=0 && mode<5))continue;
        if(index!=cases || mode<0 || mode>5 || row>=7)abort();
        /* A requested complete pulse cycle must not pass with duplicate
         * debugger locations or missing colour steps. */
        if(expected==65 && cases && (counter!=((previous_counter+4)&255) ||
           phase!=(previous_phase==2000?0:previous_phase+1)))abort();
        previous_counter=counter;previous_phase=phase;
        (void)menu_language_title(u,language>=1 && language<=8?language:1,(const unsigned char *)"menu1");
        word(u,0x3cbf0+0xf1a,players);word(u,0x3cbf0+0xfa,seconds);word(u,0x3cbf0+0x102,tracks);
        /* Startup's final font load leaves the shared alias at iso.@f.
         * This checks direct demo returns, not arbitrary nested-menu lifetime. */
        word(u,0x3cbf0+0x6bd4,0x4000);word(u,0x3cbf0+0x6bd6,0x6000);
        snprintf(path,sizeof path,"%s/.run/title-return/%u.bin",argv[1],index);
        FILE *snapshot=fopen(path,"rb");if(!snapshot || fread(logical,1,sizeof logical,snapshot)!=sizeof logical)abort();fclose(snapshot);
        unsigned char owner[61];snprintf(path,sizeof path,"%s/.run/title-return/%u.owner",argv[1],index);
        snapshot=fopen(path,"rb");if(!snapshot || fread(owner,1,sizeof owner,snapshot)!=sizeof owner || !memchr(owner,0,sizeof owner))abort();fclose(snapshot);
        registered+=owner[0]!=0;
        check(uc_mem_write(u,0x3cbf0+0x62f,owner,sizeof owner));
        if(phase>2000)abort();word(u,0x3cbf0+0x1144,phase?phase-1:2000);
        for(unsigned y=0;y<200;++y)for(unsigned x=0;x<320;++x)
            v.pixels[y*320+x]=background[2+(x&3)*16000+y*80+(x>>2)];
        const unsigned flags[]={0,10,1,11,0,0};
        word(u,0x3cbf0+0x90,selected);word(u,0x3cbf0+0x4da8,total);word(u,0x3cbf0+0x92,mode);
        word(u,0x3cbf0+0x3020,mode==4?weapons:flags[mode]&2);
        word(u,0x3cbf0+0x3022,mode==4?inventory:flags[mode]&1);
        for(unsigned i=0;i<4;++i){signed char role=roles[i];check(uc_mem_write(u,0x3cbf0+0x4bc6+i,&role,1));}
        unsigned char tick=(unsigned char)(counter-4),refresh=2;
        check(uc_mem_write(u,0x3cbf0+0x6b4e,&tick,1));check(uc_mem_write(u,0x3cbf0+0x1146,&refresh,1));
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xf000;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&ds));check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        word(u,0x8f000,0);word(u,0x8f002,0x9000);word(u,0x8f004,row);
        /* Include the original owner-name pulse/text; stop only before the
         * VGA start-address publication, which cannot affect logical pixels. */
        check(uc_emu_start(u,0x29f2c,0x29fef,0,3000000));
        uint16_t ip;check(uc_reg_read(u,UC_X86_REG_IP,&ip));if(ip!=0x29fef-0x266c0)abort();
        for(unsigned y=0;y<200;++y)for(unsigned x=0;x<320;++x)
            if(v.pixels[y*320+x]!=logical[(x&3)*65536+y*100+(x>>2)]){
                fprintf(stderr,"Title return %u mismatch at %u,%u DOS=%u native=%u\n",index,x,y,v.pixels[y*320+x],logical[(x&3)*65536+y*100+(x>>2)]);return 1;}
        ++cases;
    }
    fclose(f);uc_close(u);if(cases!=expected)return 1;
    printf("%u live title compositions match all 64000 original pixels; registered=%u\n",cases,registered);return 0;
}
