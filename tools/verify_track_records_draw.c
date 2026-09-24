#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/ui/track_records_draw.h"
#include "../src/ui/track_records_renderer.h"
static unsigned expanded;
static struct SlicksChunkyUi oracle_ui;
static unsigned char renderer_fonts[2][8];
struct Call { unsigned kind; short args[8]; unsigned char text[21]; };
struct Trace { struct Call calls[256]; unsigned count; };
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned at,unsigned value)
{ unsigned char b[2]={value,value>>8}; check(uc_mem_write(u,at,b,2)); }
static unsigned getword(uc_engine *u,unsigned at)
{ unsigned char b[2]; check(uc_mem_read(u,at,b,2)); return b[0]|b[1]<<8; }
static short emit(void *context,enum SlicksRecordsDrawCommand kind,const short *args,const unsigned char *text)
{
    static const unsigned counts[]={3,1,2,5,5,4,6,5,3};
    struct Trace *t=context; if(t->count>=256) abort();
    struct Call *c=&t->calls[t->count++]; c->kind=kind;
    memcpy(c->args,args,counts[kind]*sizeof *args);
    if(text) { unsigned i=0; while(i<20 && text[i]) { c->text[i]=text[i]; ++i; } }
    return kind==SLICKS_RECORDS_NEAREST?(short)((args[0]*3+args[1]*5+args[2]*7)&255):0;
}
static void boundary(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size;
    /* In the composition pass, run real integer/time wrappers.
     * Only the final font/icon/rectangle/palette boundary is substituted. */
    if(expanded && (address==0x302b6 || address==0x2aceb)) return;
    if(address==0x1aaea) {
        short a[8]={(short)getword(u,0x3cbf0+0x1600)};
        if(!expanded) emit(context,SLICKS_RECORDS_HIGHLIGHT,a,0); return;
    }
    uint16_t ss,sp,cs,ip,ax;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=16U*ss+sp; short a[8],b[8]={0};
    for(unsigned i=0;i<8;++i) a[i]=(short)getword(u,stack+4+2*i);
    enum SlicksRecordsDrawCommand kind; unsigned char text[21],*string=0;
    if(address==0x36fae) { kind=SLICKS_RECORDS_NEAREST; for(unsigned i=0;i<3;++i) b[i]=a[i]&255; }
    else if(address==0x2fe63) { kind=SLICKS_RECORDS_COLOUR; b[0]=a[1]?1:0; b[1]=a[0]&255; }
    else if(address==0x39ed8) { kind=SLICKS_RECORDS_RECTANGLE; memcpy(b,a,10); }
    else if(address==0x301ab) {
        kind=SLICKS_RECORDS_TEXT; b[0]=a[0]; b[1]=a[1]; b[2]=a[4]?1:0; b[3]=a[6]&255;
        check(uc_mem_read(u,(unsigned short)a[2]+16U*(unsigned short)a[3],text,sizeof text)); string=text;
    } else if(address==0x2e2d2) {
        kind=SLICKS_RECORDS_ICON; b[0]=a[0]; b[1]=a[1]; b[2]=a[2]==0x300?-1:(a[2]-0x400)/4;
    } else {
        kind=address==0x302b6?SLICKS_RECORDS_NUMBER:address==0x3000f?SLICKS_RECORDS_CHARACTER:SLICKS_RECORDS_TIME;
        b[0]=a[0]; b[1]=a[1]; b[2]=a[2]; b[3]=a[3]?1:0; b[4]=a[5];
        if(kind==SLICKS_RECORDS_CHARACTER) b[2]&=255;
        if(kind==SLICKS_RECORDS_TIME) b[5]=a[6];
    }
    if(expanded && kind==SLICKS_RECORDS_CHARACTER) {
        text[0]=(unsigned char)b[2]; text[1]=0; string=text;
        b[2]=b[3]; b[3]=b[4]; b[4]=0; kind=SLICKS_RECORDS_TEXT;
    }
    if(expanded && kind==SLICKS_RECORDS_NEAREST) ax=slicks_ui_nearest(&oracle_ui,b[0],b[1],b[2]);
    else if(expanded && kind==SLICKS_RECORDS_COLOUR) {
        unsigned char colour=(unsigned char)b[1];
        check(uc_mem_write(u,0x60006+0x100*b[0],&colour,1)); ax=0;
    } else if(expanded && kind==SLICKS_RECORDS_RECTANGLE) {
        slicks_ui_rectangle(&oracle_ui,b[0],b[1],b[2],b[3],b[4]); ax=0;
    } else {
        ax=(uint16_t)emit(context,kind,b,string);
        if(expanded && kind==SLICKS_RECORDS_TEXT) {
            struct Trace *t=context;
            t->calls[t->count-1].args[4]=(short)getword(u,0x3cbf0+0x1600);
            unsigned char colour; check(uc_mem_read(u,0x60006+0x100*b[2],&colour,1));
            t->calls[t->count-1].args[5]=colour;
        }
    }
    if(kind==SLICKS_RECORDS_NEAREST) check(uc_reg_write(u,UC_X86_REG_AX,&ax));
    ip=getword(u,stack); cs=getword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static int render_text(void *context,struct SlicksChunkyUi *ui,unsigned char *font,
    const unsigned char *text,short x,short y,unsigned char flags,unsigned char highlight)
{
    (void)ui; short a[8]={x,y,font==renderer_fonts[1]?1:0,flags};
    emit(context,SLICKS_RECORDS_TEXT,a,text); struct Trace *t=context;
    t->calls[t->count-1].args[4]=highlight; t->calls[t->count-1].args[5]=font[6]; return 0;
}
static int render_icon(void *context,struct SlicksChunkyUi *ui,short icon,short x,short y)
{ (void)ui; short a[8]={x,y,icon}; emit(context,SLICKS_RECORDS_ICON,a,0); return 0; }
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f); fclose(f); if(bytes<200000 || bytes==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,bytes));
    const unsigned addresses[]={0x36fae,0x1aaea,0x2fe63,0x39ed8,0x301ab,0x302b6,0x3000f,0x2aceb,0x2e2d2};
    uc_hook hooks[9]; struct Trace dos;
    for(unsigned i=0;i<9;++i) check(uc_hook_add(u,&hooks[i],UC_HOOK_CODE,boundary,&dos,addresses[i],addresses[i]));
    word(u,0x3cbf0+0x680,0); word(u,0x3cbf0+0x682,0x6000);
    word(u,0x3cbf0+0x684,0x100); word(u,0x3cbf0+0x686,0x6000);
    word(u,0x3cbf0+0x4c2c,0x300); word(u,0x3cbf0+0x4c2e,0x6100);
    for(unsigned i=0;i<128;++i) { word(u,0x3cbf0+0x4e40+4*i,0x400+4*i); word(u,0x3cbf0+0x4e42+4*i,0x6100); }
    unsigned cases=0; unsigned char pixels[64000],oracle_pixels[64000],palette[768];
    for(unsigned i=0;i<768;++i) palette[i]=(unsigned char)((i*13+i/7)%64);
    oracle_ui=(struct SlicksChunkyUi){oracle_pixels,palette,0,0};
    for(expanded=0;expanded<2;++expanded)
    for(unsigned variant=0;variant<12;++variant) for(int order=-1;order<=1;++order)
    for(unsigned position=0;position<3;++position) {
        struct SlicksTrackRecords records={0}; signed char ranks[4];
        const short times[]={-1,0,1,2,3,29999,30000,32767};
        for(unsigned i=0;i<11;++i) {
            unsigned char *r=records.entries[i]; snprintf((char *)r,20,"DRIVER %u",i);
            short time=times[(i+variant)%8]; r[20]=time; r[21]=(unsigned short)time>>8;
            r[22]=variant%3?(variant<6?24:128):0; r[23]=variant<6?9:255;
            const unsigned years[]={0,2026,32767,32768,65535}; unsigned year=years[variant%5];
            r[24]=(unsigned char)year; r[25]=(unsigned char)(year>>8);
            r[26]=variant%2?i:0; r[27]=variant%4;
        }
        const signed char ranked[]={-1,0,1,10,11,127};
        for(unsigned i=0;i<4;++i) ranks[i]=ranked[(i+variant)%6];
        check(uc_mem_write(u,0x3cbf0+0x693a,records.entries,sizeof records.entries));
        check(uc_mem_write(u,0x3cbf0+0x4bce,ranks,4));
        unsigned char date[2]={variant&1?'/':'.',(unsigned char)order};
        check(uc_mem_write(u,0x3cbf0+0x171e,date,2));
        short x=position==0?80:position==1?0:37,y=position==0?40:position==1?0:17;
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
        word(u,0x8f000,0); word(u,0x8f002,0x9000); word(u,0x8f004,x); word(u,0x8f006,y);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        memset(pixels,0x37,sizeof pixels); memcpy(oracle_pixels,pixels,sizeof pixels);
        memset(&dos,0,sizeof dos); check(uc_emu_start(u,0x1aabc,0x90000,0,1000000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp)); if(ip || sp!=0xf004) abort();
        struct Trace native={0}; const struct SlicksRecordsDrawOps ops={emit,&native};
        if(expanded) {
            struct SlicksRecordsRenderer renderer={
                .ui={pixels,palette,0,0},.fonts={renderer_fonts[0],renderer_fonts[1]},
                .text=render_text,.icon=render_icon,.context=&native};
            if(slicks_records_renderer_draw(&renderer,&records,ranks,x,y,date[0],(signed char)date[1])) abort();
            if(memcmp(pixels,oracle_pixels,sizeof pixels)) { fputs("Records highlight rectangles differ\n",stderr); return 1; }
        } else slicks_draw_track_records(&records,ranks,x,y,date[0],(signed char)date[1],&ops);
        if(memcmp(&native,&dos,sizeof dos)) {
            fprintf(stderr,"Records draw mismatch expanded=%u variant=%u order=%d position=%u calls=%u/%u\n",expanded,variant,order,position,dos.count,native.count);
            for(unsigned i=0;i<dos.count || i<native.count;++i) if(memcmp(&dos.calls[i],&native.calls[i],sizeof dos.calls[i])) {
                fprintf(stderr,"call %u kind %u/%u text=%s/%s\n",i,dos.calls[i].kind,native.calls[i].kind,dos.calls[i].text,native.calls[i].text);
                for(unsigned j=0;j<8;++j) fprintf(stderr," %d/%d",dos.calls[i].args[j],native.calls[i].args[j]);
                fputc('\n',stderr); break;
            }
            return 1;
        }
        ++cases;
    }
    check(uc_close(u)); printf("Original track records: %u complete ordered draw traces pass\n",cases); return 0;
}
