/* Execute the complete original font glyph + VGA plotter against the native
 * 68020 port. Only VGA hardware is modelled; no original drawing call is stubbed. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include <unicorn/m68k.h>
#include "host_archive.h"
#include "../src/ui/font_resource.h"

static unsigned char dos[64000];
static unsigned plane, writes;
static void check(uc_err e)
{ if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *uc,unsigned a,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(uc,a,b,2)); }
static void output(uc_engine *uc,uint32_t port,int size,uint32_t value,void *user)
{
    (void)uc; (void)user;
    if(port!=0x3c4 || size!=2 || (value&255)!=2) abort();
    for(plane=0;plane<4;++plane) if((value>>8)==(1U<<plane)) return;
    abort();
}
static void pixel(uc_engine *uc,uc_mem_type type,uint64_t address,int size,int64_t value,void *user)
{
    (void)uc; (void)type; (void)user;
    unsigned offset=(unsigned)address-0xa0000;
    unsigned x=offset%100*4+plane,y=offset/100;
    if(size!=1) abort();
    /* VGA also stores invisible right/bottom margin bytes. Compare only
     * scanout pixels, while native guards below prohibit off-surface writes. */
    if(x>=320 || y>=200) return;
    dos[y*320+x]=(unsigned char)value; ++writes;
}

int main(int argc,char **argv)
{
    static unsigned char runtime[300000],code[4096],resource[8192],font[8192],native[64000];
    if(argc<4 || argc>6) return 2;
    unsigned planar=argc==6;
    const char *font_name=argc>=5?argv[4]:"pieni.@f";
    FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f); if(ferror(f) || !feof(f)) return 2; fclose(f);
    f=fopen(argv[1],"rb"); if(!f) return 2;
    size_t codesize=fread(code,1,sizeof code,f); if(ferror(f) || !feof(f)) return 2; fclose(f);
    long resource_size=host_archive_load("ref/SLICKS.000",font_name,resource,sizeof resource);
    if(resource_size<12 || resource[9]!=2) return 2;
    long decoded=slicks_decode_font_resource(resource,(unsigned long)resource_size,font,sizeof font);
    if(decoded<0) return 2;
    unsigned count=font[0],height=font[2],dst=(unsigned)decoded;
    printf("Font resource: %s (%u glyphs, height %u)\n",font_name,count,height);
    uc_engine *x86,*m68k;
    check(uc_open(UC_ARCH_X86,UC_MODE_16,&x86));
    check(uc_mem_map(x86,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(x86,0x10100,runtime,bytes));
    uc_hook ports,window;
    check(uc_hook_add(x86,&ports,UC_HOOK_INSN,output,NULL,1,0,UC_X86_INS_OUT));
    check(uc_hook_add(x86,&window,UC_HOOK_MEM_WRITE,pixel,NULL,0xa0000,0xaffff));
    word(x86,0x3cbf0+0x1d7b,100);
    check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&m68k));
    check(uc_ctl_set_cpu_model(m68k,UC_CPU_M68K_M68020));
    check(uc_mem_map(m68k,0,0x400000,UC_PROT_ALL));
    check(uc_mem_write(m68k,0,code,codesize));
    static const int registers[]={UC_M68K_REG_D0,UC_M68K_REG_D1,UC_M68K_REG_D2,
        UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,UC_M68K_REG_D6,UC_M68K_REG_D7,
        UC_M68K_REG_A0,UC_M68K_REG_A1,UC_M68K_REG_A2,UC_M68K_REG_A3,
        UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6};
    unsigned cases=0;
    for(unsigned palette=0;palette<3;++palette)
    for(unsigned glyph=0;glyph<count;++glyph)
    for(unsigned location=0;location<8;++location) {
        unsigned width=font[8+count+glyph];
        unsigned x=location==3?320-width:location*97;
        unsigned y=location==3?200-height:location*61;
        if(location>=4) {
            const unsigned edge_x[]={318,321,100,350},edge_y[]={60,90,198,199};
            x=edge_x[location-4]; y=edge_y[location-4];
        }
        font[6]=palette==0?254:palette==1?0:73;
        font[7]=palette==0?1:palette==1?217:19;
        check(uc_mem_write(x86,0x50000,font,dst));
        check(uc_mem_write(m68k,0x50000,font,dst));
        for(unsigned at=0;at<64000;++at) dos[at]=(unsigned char)(at*17+at/320+cases);
        if(planar) {
            static unsigned char planes[0x40000]; memset(planes,0xa5,sizeof planes);
            for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;++x)
                planes[(x&3)*65536+y*100+x/4]=dos[y*320+x];
            check(uc_mem_write(m68k,0x100000,planes,sizeof planes));
        } else check(uc_mem_write(m68k,0x100000,dos,sizeof dos));
        unsigned char guard[256]; memset(guard,0xa5,sizeof guard);
        check(uc_mem_write(m68k,0x100000-256,guard,sizeof guard));
        check(uc_mem_write(m68k,0x100000+(planar?0x40000:64000),guard,sizeof guard));
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
        word(x86,0x8f000,0); word(x86,0x8f002,0x7000);
        word(x86,0x8f004,x); word(x86,0x8f006,y);
        word(x86,0x8f008,0); word(x86,0x8f00a,0x5000);
        word(x86,0x8f00c,0); word(x86,0x8f00e,glyph);
        check(uc_reg_write(x86,UC_X86_REG_CS,&cs)); check(uc_reg_write(x86,UC_X86_REG_DS,&ds));
        check(uc_reg_write(x86,UC_X86_REG_SS,&ss)); check(uc_reg_write(x86,UC_X86_REG_SP,&sp));
        writes=0; plane=99;
        check(uc_emu_start(x86,0x2fef2,0x70000,0,100000));
        check(uc_reg_read(x86,UC_X86_REG_IP,&ip)); check(uc_reg_read(x86,UC_X86_REG_SP,&sp));
        if(ip || sp!=0xf004) return 1;
        uint32_t values[]={0xabcd0000|x,0xbcde0000|y,0xcdef0000|glyph,
            0x33334444,0x44445555,0x55556666,0x66667777,0x77778888,
            0x100000,0x50000,0x22223333,0x33334444,0x44445555,0x55556666,0x66667777};
        uint32_t stack=0x300000,sr=0,pc;
        unsigned char return_address[]={0,0x38,0,0};
        check(uc_reg_write(m68k,UC_M68K_REG_SR,&sr));
        check(uc_reg_write(m68k,UC_M68K_REG_A7,&stack));
        check(uc_mem_write(m68k,stack,return_address,4));
        for(unsigned r=0;r<15;++r) check(uc_reg_write(m68k,registers[r],&values[r]));
        check(uc_emu_start(m68k,0,0x380000,0,100000));
        check(uc_reg_read(m68k,UC_M68K_REG_PC,&pc));
        check(uc_reg_read(m68k,UC_M68K_REG_A7,&stack));
        if(planar) {
            static unsigned char planes[0x40000];
            check(uc_mem_read(m68k,0x100000,planes,sizeof planes));
            for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;++x)
                native[y*320+x]=planes[(x&3)*65536+y*100+x/4];
        } else check(uc_mem_read(m68k,0x100000,native,sizeof native));
        unsigned char actual_guard[256];
        check(uc_mem_read(m68k,0x100000-256,actual_guard,sizeof actual_guard));
        if(memcmp(guard,actual_guard,sizeof guard)) abort();
        check(uc_mem_read(m68k,0x100000+(planar?0x40000:64000),actual_guard,sizeof actual_guard));
        if(memcmp(guard,actual_guard,sizeof guard)) abort();
        if(pc!=0x380000 || stack!=0x300004 || memcmp(native,dos,sizeof dos)) {
            fprintf(stderr,"Font glyph mismatch glyph=%u palette=%u xy=%u,%u writes=%u\n",glyph,palette,x,y,writes);
            return 1;
        }
        for(unsigned r=0;r<15;++r) {
            uint32_t value; check(uc_reg_read(m68k,registers[r],&value));
            if(value!=values[r]) { fprintf(stderr,"Font register %u corrupted\n",r); return 1; }
        }
        ++cases;
    }
    printf("Original glyph rasterizer vs 68020: %u full-frame pixel comparisons; transparency, palette changes, edges and register preservation passed\n",cases);
    if(planar) { check(uc_close(x86)); check(uc_close(m68k)); return 0; }
    f=fopen(argv[2],"rb"); if(!f) return 2;
    codesize=fread(code,1,sizeof code,f); if(ferror(f) || !feof(f)) return 2; fclose(f);
    check(uc_mem_write(m68k,0x2000,code,codesize));
    static const signed char adjustments[]={-2,0,1,3};
    static const short positions[]={0,3,-5,32760};
    static const unsigned char strings[][64]={"01.00","2.","12\010ab\317Z","ab\rCD","ab\nCD","players","Vesuri","\030\031\033\032,ENTER","\030\031\033\032,ENTER: SELECT  C: CAR  ESC: EXIT"};
    const unsigned string_count=sizeof strings/sizeof strings[0];
    cases=0;
    for(unsigned character=0;character<256+string_count;++character)
    for(unsigned adjustment=0;adjustment<4;++adjustment)
    for(unsigned position=0;position<4;++position) {
        unsigned char text[64]={0};
        if(character<256) text[0]=(unsigned char)character;
        else memcpy(text,strings[character-256],sizeof text);
        check(uc_mem_write(x86,0x60000,text,sizeof text));
        check(uc_mem_write(m68k,0x60000,text,sizeof text));
        check(uc_mem_write(x86,0x3cbf0+0x1604,&adjustments[adjustment],1));
        word(x86,0x3cbf0+0x15fe,10);
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,ip,ax;
        word(x86,0x8f000,0); word(x86,0x8f002,0x7000);
        word(x86,0x8f004,0); word(x86,0x8f006,0x6000);
        word(x86,0x8f008,0); word(x86,0x8f00a,0x5000);
        word(x86,0x8f00c,(unsigned short)positions[position]);
        check(uc_reg_write(x86,UC_X86_REG_CS,&cs)); check(uc_reg_write(x86,UC_X86_REG_DS,&ds));
        check(uc_reg_write(x86,UC_X86_REG_SS,&ss)); check(uc_reg_write(x86,UC_X86_REG_SP,&sp));
        check(uc_emu_start(x86,0x300e5,0x70000,0,100000));
        check(uc_reg_read(x86,UC_X86_REG_IP,&ip)); check(uc_reg_read(x86,UC_X86_REG_SP,&sp));
        check(uc_reg_read(x86,UC_X86_REG_AX,&ax));
        if(ip || sp!=0xf004) return 1;
        uint32_t values[]={0xabcd0000|(unsigned short)positions[position],0xbcde0000,0xcdef0000,
            0x33334400|(unsigned char)adjustments[adjustment],10,0x55556666,0x66667777,0x77778888,
            0x100000,0x50000,0x60000,0x33334444,0x44445555,0x55556666,0x66667777};
        uint32_t stack=0x300000,sr=0,pc,value;
        unsigned char return_address[]={0,0x38,0,0};
        check(uc_reg_write(m68k,UC_M68K_REG_SR,&sr));
        check(uc_reg_write(m68k,UC_M68K_REG_A7,&stack));
        check(uc_mem_write(m68k,stack,return_address,4));
        for(unsigned r=0;r<15;++r) check(uc_reg_write(m68k,registers[r],&values[r]));
        check(uc_emu_start(m68k,0x2000,0x380000,0,100000));
        check(uc_reg_read(m68k,UC_M68K_REG_PC,&pc));
        check(uc_reg_read(m68k,UC_M68K_REG_A7,&stack));
        check(uc_reg_read(m68k,UC_M68K_REG_D0,&value));
        if(pc!=0x380000 || stack!=0x300004 || (value&65535)!=ax) {
            fprintf(stderr,"Font measure mismatch char=%u adjustment=%d start=%d native=%u dos=%u\n",
                character,adjustments[adjustment],positions[position],value&65535,ax); return 1;
        }
        for(unsigned r=1;r<15;++r) {
            check(uc_reg_read(m68k,registers[r],&value));
            if(value!=values[r]) { fprintf(stderr,"Measure register %u corrupted\n",r); return 1; }
        }
        ++cases;
    }
    printf("Original font measure vs 68020: %u cases including all bytes, glyph zero, signed spacing, tabs and newlines passed\n",cases);
    f=fopen(argv[3],"rb"); if(!f) return 2;
    codesize=fread(code,1,sizeof code,f); if(ferror(f) || !feof(f)) return 2; fclose(f);
    unsigned entry=(unsigned)code[0]<<24|(unsigned)code[1]<<16|(unsigned)code[2]<<8|code[3];
    unsigned records_entry=(unsigned)code[20]<<24|(unsigned)code[21]<<16|(unsigned)code[22]<<8|code[23];
    check(uc_mem_write(m68k,0,code,codesize));
    check(uc_ctl_remove_cache(m68k,0,0x4000));
    cases=0;
    for(unsigned abi=0;abi<2;++abi)
    for(unsigned string=0;string<string_count;++string)
    for(unsigned flags=0;flags<8;++flags)
    for(unsigned adjustment=0;adjustment<4;++adjustment)
    for(unsigned palette=0;palette<2;++palette) {
        if(abi && adjustments[adjustment]!=1) continue;
        if(string==string_count-1 && (flags || height>5)) continue;
        unsigned anchor_x=string==string_count-1?1:160;
        unsigned anchor_y=string==string_count-1?193:150;
        check(uc_mem_write(x86,0x60000,strings[string],sizeof strings[string]));
        check(uc_mem_write(m68k,0x60000,strings[string],sizeof strings[string]));
        font[6]=palette?73:0; font[7]=palette?19:217;
        check(uc_mem_write(x86,0x50000,font,dst));
        check(uc_mem_write(m68k,0x50000,font,dst));
        check(uc_mem_write(x86,0x3cbf0+0x1604,&adjustments[adjustment],1));
        word(x86,0x3cbf0+0x1600,palette?217:0);
        word(x86,0x3cbf0+0x1602,abi?1:palette?0x02ff:0xff01);
        for(unsigned at=0;at<64000;++at) dos[at]=(unsigned char)(at*17+at/320+cases);
        check(uc_mem_write(m68k,0x100000,dos,sizeof dos));
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,ip,ax;
        word(x86,0x8f000,0); word(x86,0x8f002,0x7000);
        word(x86,0x8f004,anchor_x); word(x86,0x8f006,anchor_y);
        word(x86,0x8f008,0); word(x86,0x8f00a,0x6000);
        word(x86,0x8f00c,0); word(x86,0x8f00e,0x5000);
        word(x86,0x8f010,flags); word(x86,0x8f012,0);
        check(uc_reg_write(x86,UC_X86_REG_CS,&cs)); check(uc_reg_write(x86,UC_X86_REG_DS,&ds));
        check(uc_reg_write(x86,UC_X86_REG_SS,&ss)); check(uc_reg_write(x86,UC_X86_REG_SP,&sp));
        writes=0; plane=99;
        check(uc_emu_start(x86,0x301ab,0x70000,0,1000000));
        check(uc_reg_read(x86,UC_X86_REG_IP,&ip)); check(uc_reg_read(x86,UC_X86_REG_SP,&sp));
        check(uc_reg_read(x86,UC_X86_REG_AX,&ax));
        if(ip || sp!=0xf004) return 1;
        uint32_t values[]={anchor_x,anchor_y,flags,0x33334400|(unsigned char)adjustments[adjustment],
            10,palette?217:0,palette?0xff02:0x01ff,0x77778888,
            0x100000,0x50000,0x60000,0x33334444,0x44445555,0x55556666,0x66667777};
        uint32_t stack=0x300000,sr=0,pc,value;
        unsigned char return_address[]={0,0x38,0,0};
        check(uc_reg_write(m68k,UC_M68K_REG_SR,&sr));
        check(uc_reg_write(m68k,UC_M68K_REG_A7,&stack));
        check(uc_mem_write(m68k,stack,return_address,4));
        if(abi) {
            uint32_t args[]={0x380000,0x100000,0x50000,0x60000,anchor_x,anchor_y,flags,palette?217:0};
            unsigned char packed[32];
            for(unsigned i=0;i<8;++i) for(unsigned j=0;j<4;++j) packed[4*i+j]=(unsigned char)(args[i]>>(24-8*j));
            check(uc_mem_write(m68k,stack,packed,sizeof packed));
        }
        for(unsigned r=0;r<15;++r) check(uc_reg_write(m68k,registers[r],&values[r]));
        check(uc_emu_start(m68k,abi?records_entry:entry,0x380000,0,1000000));
        check(uc_reg_read(m68k,UC_M68K_REG_PC,&pc));
        check(uc_reg_read(m68k,UC_M68K_REG_A7,&stack));
        check(uc_reg_read(m68k,UC_M68K_REG_D0,&value));
        check(uc_mem_read(m68k,0x100000,native,sizeof native));
        if(pc!=0x380000 || stack!=0x300004 || (value&65535)!=ax || memcmp(native,dos,sizeof dos)) {
            fprintf(stderr,"Font string mismatch string=%u flags=%u spacing=%d palette=%u advance=%u/%u\n",
                string,flags,adjustments[adjustment],palette,value&65535,ax); return 1;
        }
        unsigned char restored[8192];
        check(uc_mem_read(m68k,0x50000,restored,dst));
        if(memcmp(restored,font,dst)) { fputs("Native font palette not restored\n",stderr); return 1; }
        check(uc_mem_read(x86,0x50000,restored,dst));
        if(memcmp(restored,font,dst)) { fputs("DOS font palette not restored\n",stderr); return 1; }
        for(unsigned r=1;r<15;++r) {
            if(abi && (r==1 || r==8 || r==9)) continue;
            check(uc_reg_read(m68k,registers[r],&value));
            if(value!=values[r]) { fprintf(stderr,"String register %u corrupted\n",r); return 1; }
        }
        ++cases;
    }
    printf("Original complete string renderer vs 68020: %u full-frame comparisons, alignment, highlight palette restore, tabs/newlines and return values passed\n",cases);
    uc_close(x86); uc_close(m68k);
    return 0;
}
