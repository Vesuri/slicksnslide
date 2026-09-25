#define main records_draw_verifier_main
#include "verify_track_records_draw.c"
#undef main
#include "../src/ui/championship_standings_draw.h"
static unsigned char nearest(void *p,unsigned char r,unsigned char g,unsigned char b)
{ short a[8]={r,g,b}; return (unsigned char)emit(p,SLICKS_RECORDS_NEAREST,a,0); }
static void colour(void *p,unsigned char index,unsigned char value)
{ short a[8]={index,value}; (void)emit(p,SLICKS_RECORDS_COLOUR,a,0); }
static void rectangle(void *p,short x,short y,short right,short bottom,unsigned char value)
{ short a[8]={x,y,right,bottom,(signed char)value}; (void)emit(p,SLICKS_RECORDS_RECTANGLE,a,0); }
static void text_row(void *p,const unsigned char *s,short x,short y,unsigned char flags)
{ short a[8]={x,y,0,flags}; (void)emit(p,SLICKS_RECORDS_TEXT,a,s); }
static void number(void *p,short value,short x,short y,unsigned char flags)
{ short a[8]={x,y,value,0,flags}; (void)emit(p,SLICKS_RECORDS_NUMBER,a,0); }
static void font_colour(uc_engine *u,uint64_t at,uint32_t size,void *p)
{
    (void)at; (void)size;
    uint16_t sp,ss,cs,ip; check(uc_reg_read(u,UC_X86_REG_SP,&sp)); check(uc_reg_read(u,UC_X86_REG_SS,&ss));
    unsigned stack=ss*16U+sp;
    colour(p,(unsigned char)getword(u,stack+10),(unsigned char)getword(u,stack+4));
    ip=getword(u,stack); cs=getword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f); if(n<200000 || n==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n));
    const unsigned addresses[]={0x36fae,0x39ed8,0x301ab,0x302b6};
    struct Trace dos; uc_hook hook;
    for(unsigned i=0;i<4;++i) check(uc_hook_add(u,&hook,UC_HOOK_CODE,boundary,&dos,addresses[i],addresses[i]));
    check(uc_hook_add(u,&hook,UC_HOOK_CODE,font_colour,&dos,0x2fe63,0x2fe63));
    word(u,0x3cbf0+0x680,0); word(u,0x3cbf0+0x682,0x6000);
    const short values[]={-32768,-1,0,1,10,32767}; unsigned cases=0;
    for(unsigned mask=0;mask<16;++mask) for(unsigned pattern=0;pattern<1296;++pattern) {
        short points[4]; signed char roles[4]; unsigned char colours[4][6];
        const unsigned char *names[4]={(const unsigned char *)"FIRST",(const unsigned char *)"SECOND",(const unsigned char *)"THIRD",(const unsigned char *)"FOURTH"};
        unsigned digits=pattern;
        for(unsigned i=0;i<4;++i) {
            points[i]=values[digits%6]; digits/=6; roles[i]=(mask&(1U<<i))?1:0;
            for(unsigned j=0;j<6;++j) colours[i][j]=(unsigned char)(pattern+i*41+j*17);
            word(u,0x3cbf0+0x44c+2*i,i);
            check(uc_mem_write(u,0x3cbf0+0x36aa+21*i,names[i],strlen((const char *)names[i])+1));
        }
        check(uc_mem_write(u,0x3cbf0+0x310c,colours,24));
        struct SlicksChampionshipStandings table; slicks_championship_standings(&table,points,roles);
        for(unsigned i=0;i<4;++i) word(u,0x8eff4+2*i,table.points[i]);
        check(uc_mem_write(u,0x8effc,table.driver,4));
        unsigned char zero=0; check(uc_mem_write(u,0x8eff3,&zero,1));
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,bp=0xf000,sp=0xe000;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_BP,&bp)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        memset(&dos,0,sizeof dos); check(uc_emu_start(u,0x2a7a1,0x2aa77,0,100000));
        struct Trace native={0};
        const struct SlicksStandingsDrawOps ops={nearest,colour,rectangle,text_row,number,&native};
        slicks_draw_championship_standings(&table,colours,names,&ops);
        if(memcmp(&dos,&native,sizeof dos)) {
            fprintf(stderr,"Standings drawing mismatch mask=%u pattern=%u calls=%u/%u\n",mask,pattern,dos.count,native.count);
            for(unsigned i=0;i<dos.count && i<native.count;++i) if(memcmp(&dos.calls[i],&native.calls[i],sizeof dos.calls[i])) {
                fprintf(stderr,"call %u kinds=%u/%u\n",i,dos.calls[i].kind,native.calls[i].kind);
                for(unsigned a=0;a<8;++a) fprintf(stderr,"arg%u=%d/%d ",a,dos.calls[i].args[a],native.calls[i].args[a]);
                fputc('\n',stderr); break;
            }
            return 1;
        }
        ++cases;
    }
    check(uc_close(u)); printf("Original championship drawing: %u complete gradient/colour/name/points/tied-rank command traces pass\n",cases); return 0;
}
