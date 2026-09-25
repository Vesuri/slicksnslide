#define main palette_verifier_main
#include "verify_palette_remap.c"
#undef main
#include "../src/ui/track_records_renderer.h"
#include "../src/ui/track_info_renderer.h"
#include "../src/ui/speed_dialog.h"
#include "../src/ui/race_menu_renderer.h"
#include "../src/ui/language_table.h"
#include "../src/ui/championship_standings_draw.h"

static unsigned char cup_nearest(void *p,unsigned char r,unsigned char g,unsigned char b)
{ return slicks_ui_nearest(&((struct SlicksRecordsRenderer *)p)->ui,r,g,b); }
static void cup_colour(void *p,unsigned char index,unsigned char value)
{ ((struct SlicksRecordsRenderer *)p)->fonts[0][6+index]=value; }
static void cup_rectangle(void *p,short l,short t,short r,short b,unsigned char colour)
{ slicks_ui_rectangle(&((struct SlicksRecordsRenderer *)p)->ui,l,t,r,b,colour); }
static void cup_text(void *p,const unsigned char *text,short x,short y,unsigned char flags)
{
    struct SlicksRecordsRenderer *r=p;
    if(r->text(r->context,&r->ui,r->fonts[0],text,x,y,flags,cup_nearest(p,10,10,10))) abort();
}
static void cup_number(void *p,short value,short x,short y,unsigned char flags)
{ unsigned char s[7]; slicks_records_decimal(value,s); cup_text(p,s,x,y,flags); }
static void verify_cup_pixels(uc_engine *u,struct SlicksRecordsRenderer *r,struct Vga *v)
{
    unsigned char asset[64003],palette[768],colours[4][6];
    if(host_archive_load("ref/SLICKS.000","sskuppi.@I",asset,sizeof asset)!=64003 ||
       host_archive_load("ref/SLICKS.000","sskuppi.@p",palette,sizeof palette)!=768) abort();
    r->ui.palette=palette;
    check(uc_mem_write(u,0x67000,palette,768));
    word(u,0x3cbf0+0x71b8,0); word(u,0x3cbf0+0x71ba,0x6700);
    word(u,0x3cbf0+0x68b2,0); word(u,0x3cbf0+0x68b4,0x6700);
    word(u,0x3cbf0+0x1600,slicks_ui_nearest(&r->ui,10,10,10));
    word(u,0x3cbf0+0x1602,0x0101);
    const unsigned char *names[4]={(const unsigned char *)"FIRST DRIVER",(const unsigned char *)"SECOND",
        (const unsigned char *)"THIRD DRIVER",(const unsigned char *)"FOURTH"};
    const struct SlicksStandingsDrawOps ops={cup_nearest,cup_colour,cup_rectangle,cup_text,cup_number,r};
    for(unsigned variant=0;variant<32;++variant) {
        memcpy(r->ui.pixels,asset+3,64000); memcpy(v->pixels,asset+3,64000);
        short points[4]; signed char roles[4];
        for(unsigned i=0;i<4;++i) {
            points[i]=(short)(variant&16?10:(i*7+variant)%24); roles[i]=(variant&(1U<<i))?1:0;
            for(unsigned c=0;c<6;++c) colours[i][c]=(unsigned char)((i*17+c*9+variant)%64);
            word(u,0x3cbf0+0x44c+2*i,i);
            check(uc_mem_write(u,0x3cbf0+0x36aa+21*i,names[i],strlen((const char *)names[i])+1));
        }
        check(uc_mem_write(u,0x3cbf0+0x310c,colours,24));
        struct SlicksChampionshipStandings table; slicks_championship_standings(&table,points,roles);
        for(unsigned i=0;i<4;++i) word(u,0x8eff4+2*i,table.points[i]);
        check(uc_mem_write(u,0x8effc,table.driver,4));
        unsigned char zero=0; check(uc_mem_write(u,0x8eff3,&zero,1));
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xe000,bp=0xf000;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_emu_start(u,0x2a7a1,0x2aa77,0,30000000));
        slicks_draw_championship_standings(&table,colours,names,&ops);
        if(memcmp(r->ui.pixels,v->pixels,64000)) {
            for(unsigned i=0;i<64000;++i) if(r->ui.pixels[i]!=v->pixels[i]) {
                fprintf(stderr,"Cup pixel mismatch variant=%u at %u,%u native=%u DOS=%u\n",variant,i%320,i/320,r->ui.pixels[i],v->pixels[i]); break;
            }
            exit(1);
        }
    }
    puts("Original championship cup: 32 full-frame comparisons with original artwork/font, native 68020 text bridge, all activity masks and tied ranks pass");
}

static void pause_boundary(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size; (void)context;
    uint16_t ss,sp,cs,ip,ax=0,dx=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    if(address==0x13b07) {
        if(getword(u,stack+4)!=7686) abort();
        dx=0x7000;
    } else if(address!=0x36ca5) abort();
    ip=getword(u,stack); cs=getword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void verify_pause_pixels(uc_engine *u,struct SlicksRecordsRenderer *r,
    struct Vga *v,const unsigned char *base,unsigned font_size)
{
    uc_hook hooks[2]; const unsigned addresses[]={0x13b07,0x36ca5};
    for(unsigned i=0;i<2;++i)
        check(uc_hook_add(u,&hooks[i],UC_HOOK_CODE,pause_boundary,0,addresses[i],addresses[i]));
    const unsigned char *labels[6];
    unsigned frames=0;
    for(unsigned variant=0;variant<24;++variant) {
        unsigned char resource[1000],table[1000]; unsigned used=0;
        char name[16]; snprintf(name,sizeof name,"lang%u.txt",variant%8+1);
        long size=host_archive_load("ref/SLICKS.000",name,resource,sizeof resource);
        if(size<0 || slicks_language_table_load(resource,(unsigned)size,table,sizeof table,&used)) abort();
        check(uc_mem_write(u,0x73000,table,used));
        word(u,0x3cbf0+0x1722,0); word(u,0x3cbf0+0x1724,0x7300);
        const char *keys[]={"back","help","controllers","speed","nexttrack","mainmenu"};
        for(unsigned i=0;i<6;++i) labels[i]=slicks_language_lookup(table,used,
            (const unsigned char *)keys[i],(const unsigned char *)keys[i]);
        memcpy(r->ui.pixels,base,64000); memcpy(v->pixels,base,64000);
        struct SlicksRaceMenu m={variant%6,6,-1,0};
        struct SlicksRaceMenuRenderer d; unsigned char storage[7500];
        unsigned char percent=(unsigned char)((variant/8)*50);
        if(slicks_race_menu_render_open(&d,r,&m,percent,storage,sizeof storage)) abort();
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xef00,bp=0xf000,ip;
        word(u,0x8f006,m.row); word(u,0x8f008,m.count); word(u,0x3cbf0+0x16fd,percent);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_emu_start(u,0x1e481,0x1e544,0,30000000));
        if(memcmp(r->ui.pixels,v->pixels,64000)) { fprintf(stderr,"Pause opening differs %u\n",variant); exit(1); }
        for(unsigned step=0;step<15;++step) {
            if(step) slicks_race_menu_key(&m,step<8?0x50:0x48);
            unsigned char redraw=(unsigned char)m.redraw;
            word(u,0x8f006,m.row); check(uc_mem_write(u,0x8eff9,&redraw,1));
            check(uc_emu_start(u,0x1e54f,0x1e65c,0,30000000));
            check(uc_reg_read(u,UC_X86_REG_IP,&ip));
            if(ip!=0x1e65c-0x19870 || slicks_race_menu_render_draw(&d,r,&m,labels)) abort();
            if(memcmp(r->ui.pixels,v->pixels,64000)) {
                for(unsigned p=0;p<64000;++p) if(r->ui.pixels[p]!=v->pixels[p]) {
                    fprintf(stderr,"Pause variant=%u step=%u at %u,%u native=%u DOS=%u\n",
                        variant,step,p%320,p/320,r->ui.pixels[p],v->pixels[p]); break;
                }
                exit(1);
            }
            ++frames;
        }
        unsigned char old; check(uc_mem_read(u,0x8efff,&old,1));
        slicks_race_menu_render_close(&d,r);
        if(r->fonts[0][6]!=old) abort();
        check(uc_mem_write(u,0x50006,&old,1));
        unsigned char font[8192]; check(uc_mem_read(u,0x50000,font,font_size));
        if(memcmp(font,r->fonts[0],font_size)) abort();
    }
    for(unsigned i=0;i<2;++i) check(uc_hook_del(u,hooks[i]));
    word(u,0x3cbf0+0x1722,0); word(u,0x3cbf0+0x1724,0);
    printf("Original pause menu: 24 openings and %u full-frame redraw comparisons pass with all 8 original languages\n",frames);
}

static void info_boundary(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size; (void)context;
    uint16_t ss,sp,cs,ip,ax=0,dx=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    /* The string table and description loader are independently verified.
     * Supply their strings; every drawing routine still executes DOS code. */
    if(address==0x35e63) dx=0x6600;
    else if(address!=0x1a4ef) abort();
    ip=getword(u,stack); cs=getword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}

struct RecordsPixels { struct NativeText native; struct SlicksMenuIcon icons[11]; unsigned char icon_pixels[11][256]; };
static int records_text(void *context,struct SlicksChunkyUi *ui,unsigned char *font,
    const unsigned char *string,short x,short y,unsigned char flags,unsigned char highlight)
{
    struct RecordsPixels *p=context; p->native.highlight=highlight;
    renderer_text(&p->native,ui,font,string,x,y,flags); return 0;
}
static int records_icon(void *context,struct SlicksChunkyUi *ui,short id,short x,short y)
{
    struct RecordsPixels *p=context; unsigned index=id==-1?0:(unsigned)id;
    if(index>=11) return -1;
    renderer_icon(&p->native,ui,&p->icons[index],x,y); return 0;
}
static unsigned long bigword(const unsigned char *p)
{ return (unsigned long)p[0]<<24|(unsigned long)p[1]<<16|(unsigned)p[2]<<8|p[3]; }
struct SpeedPixels {
    struct SlicksRecordsRenderer *renderer;
    struct Vga *vga;
    struct SlicksSpeedDialog dialog;
    short speed;
    unsigned char keys[8];
    unsigned cursor,frames,rate;
};
static void speed_boundary(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size; struct SpeedPixels *s=context;
    uint16_t ss,sp,cs,ip,ax=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    if(address==0x36ce0) {
        if(s->cursor>=sizeof s->keys || getword(u,stack+4)!=2 ||
           slicks_speed_dialog_draw(&s->dialog,s->renderer,s->speed)) abort();
        if(memcmp(s->renderer->ui.pixels,s->vga->pixels,64000)) {
            fprintf(stderr,"Speed pixels differ at key %u, speed %d\n",s->cursor,s->speed); exit(1);
        }
        if(getword(u,0x3cbf0+0x5de)!=(unsigned short)s->speed) abort();
        ++s->frames;
        ax=s->keys[s->cursor++];
        s->speed=slicks_speed_dialog_key(&s->dialog,s->speed,(unsigned char)ax);
    } else if(address==0x37bc2) {
        s->rate=getword(u,stack+4);
        if(!s->dialog.done || s->rate!=(unsigned short)(s->speed*5)) abort();
    } else if(address!=0x36ca5) abort();
    ip=getword(u,stack); cs=getword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax));
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void verify_speed_pixels(uc_engine *u,struct SlicksRecordsRenderer *r,
    struct Vga *v,const unsigned char *base,unsigned font_size)
{
    struct SpeedPixels s={.renderer=r,.vga=v};
    uc_hook hooks[3]; const unsigned addresses[]={0x36ce0,0x36ca5,0x37bc2};
    for(unsigned i=0;i<3;++i)
        check(uc_hook_add(u,&hooks[i],UC_HOOK_CODE,speed_boundary,&s,addresses[i],addresses[i]));
    const short values[]={-32768,-1,0,49,50,51,99,100,199,200,201,32767};
    const unsigned char exits[]={1,28,57,59};
    unsigned frames=0;
    for(unsigned variant=0;variant<sizeof values/sizeof values[0];++variant) {
        memcpy(r->ui.pixels,base,64000); memcpy(v->pixels,base,64000);
        s.speed=values[variant]; s.cursor=s.frames=s.rate=0;
        const unsigned char keys[]={75,77,72,80,71,79,29,1};
        memcpy(s.keys,keys,sizeof keys); s.keys[7]=exits[variant%4];
        word(u,0x3cbf0+0x5de,(unsigned short)s.speed); word(u,0x3cbf0+0x6c0,0);
        if(slicks_speed_dialog_open(&s.dialog,r)) abort();
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
        word(u,0x8f000,0); word(u,0x8f002,0x9000);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x1e00a,0x90000,0,30000000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        if(ip || sp!=0xf004 || s.cursor!=8 || !s.dialog.done || getword(u,0x3cbf0+0x6c0)!=1 ||
           slicks_speed_dialog_close(&s.dialog,r,s.speed)!=s.rate) abort();
        unsigned char font[8192]; check(uc_mem_read(u,0x50000,font,font_size));
        if(memcmp(font,r->fonts[0],font_size)) abort();
        frames+=s.frames;
    }
    for(unsigned i=0;i<3;++i) check(uc_hook_del(u,hooks[i]));
    printf("Original Speed dialog: %u complete-frame comparisons plus close font/dirty/timer-boundary checks pass\n",frames);
}
int main(void)
{
    static unsigned char runtime[300000],source[70000],base[64000],pixels[64000],palette[768],fonts[2][8192];
    FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t size=fread(runtime,1,sizeof runtime,f); fclose(f); if(size<200000 || size==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,size));
    if(host_archive_load("ref/SLICKS.000","trckmenu.@p",palette,sizeof palette)!=768) abort();
    long loaded=host_archive_load("ref/SLICKS.000","trckmenu.@I",source,sizeof source);
    unsigned short width,height;
    if(loaded<0 || slicks_decode_indexed_menu_icon(source,loaded,base,sizeof base,&width,&height) || width!=320 || height!=200) abort();
    struct RecordsPixels p={0}; p.native.pixels=pixels; p.native.spacing=1; p.native.tab=10; p.native.shadow=0x100;
    const char *names[]={"kirj.@f","pieni.@f"};
    for(unsigned i=0;i<2;++i) {
        loaded=host_archive_load("ref/SLICKS.000",names[i],source,sizeof source);
        long bytes=loaded<0?-1:slicks_decode_font_resource(source,loaded,fonts[i],sizeof fonts[i]);
        if(bytes<0) abort(); p.native.fonts[i]=fonts[i]; p.native.sizes[i]=(unsigned)bytes;
        check(uc_mem_write(u,0x50000+i*0x2000,fonts[i],bytes));
        word(u,0x3cbf0+0x680+i*4,0); word(u,0x3cbf0+0x682+i*4,0x5000+i*0x200);
    }
    unsigned char code[8192]; f=fopen("build/font_string_test.bin","rb"); if(!f) abort();
    size=fread(code,1,sizeof code,f); fclose(f); if(size<24 || size==sizeof code) abort();
    check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&p.native.cpu));
    check(uc_ctl_set_cpu_model(p.native.cpu,UC_CPU_M68K_M68020)); check(uc_mem_map(p.native.cpu,0,0x400000,UC_PROT_ALL));
    check(uc_mem_write(p.native.cpu,0,code,size)); p.native.records_bridge=(unsigned)bigword(code+20);
    unsigned standings_bridge=(unsigned)bigword(code+24);
    f=fopen("build/hud_icon_test.bin","rb"); if(!f) abort(); size=fread(code,1,sizeof code,f); fclose(f);
    check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&p.native.icon_cpu));
    check(uc_ctl_set_cpu_model(p.native.icon_cpu,UC_CPU_M68K_M68020)); check(uc_mem_map(p.native.icon_cpu,0,0x400000,UC_PROT_ALL));
    check(uc_mem_write(p.native.icon_cpu,0,code,size));
    for(unsigned i=0;i<11;++i) {
        char name[32]; if(!i) strcpy(name,"top10cc.@16"); else if(i==1) strcpy(name,"carimage16"); else snprintf(name,sizeof name,"auto%02u.@16",i-1);
        unsigned char original[1024]; loaded=host_archive_load("ref/SLICKS.000",name,source,sizeof source);
        if(loaded<8 || loaded>1024 || slicks_decode_menu_icon(source,loaded,palette,p.icon_pixels[i],256,&p.icons[i].width,&p.icons[i].height)) abort();
        p.icons[i].pixels=p.icon_pixels[i];
        original[0]=0; original[1]=0xb1; original[2]=source[5]; original[3]=0; original[4]=source[7]; original[5]=0;
        memcpy(original+6,source+8,(size_t)loaded-8); check(uc_mem_write(u,0x68000+0x800*i,original,loaded-2));
        unsigned pointer=i?0x4e40+4*i:0x4c2c;
        word(u,0x3cbf0+pointer,0x800*i); word(u,0x3cbf0+pointer+2,0x6800);
    }
    check(uc_mem_write(u,0x67000,palette,768)); word(u,0x3cbf0+0x71b8,0); word(u,0x3cbf0+0x71ba,0x6700);
    word(u,0x3cbf0+0x1d7b,100); word(u,0x3cbf0+0x1d87,0);
    word(u,0x3cbf0+0x1d8d,0); word(u,0x3cbf0+0x1d8f,200); word(u,0x3cbf0+0x1d91,0); word(u,0x3cbf0+0x1d93,79);
    struct Vga v={0}; uc_hook hooks[2];
    check(uc_hook_add(u,&hooks[0],UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,vga_access,&v,0xa0000,0xaffff));
    check(uc_hook_add(u,&hooks[1],UC_HOOK_INSN,font_port,&v,1,0,UC_X86_INS_OUT));
    struct SlicksRecordsRenderer renderer={.ui={pixels,palette,0,0},.fonts={fonts[0],fonts[1]},
        .text=records_text,.icon=records_icon,.context=&p};
    unsigned cases=0;
    for(unsigned variant=0;variant<12;++variant) {
        memcpy(pixels,base,sizeof pixels); memcpy(v.pixels,base,sizeof base);
        struct SlicksTrackRecords records={0}; signed char ranks[4]={1,3,7,10};
        const unsigned times[]={0,1,2,3,17999,18000,29999,30000,32768,65535};
        for(unsigned i=0;i<11;++i) {
            unsigned char *r=records.entries[i]; snprintf((char *)r,20,"DRIVER %u",i);
            unsigned t=times[(i+variant)%10]; r[20]=t; r[21]=t>>8;
            r[22]=variant&1?24:0; r[23]=9; r[24]=0xea; r[25]=7;
            r[26]=i; r[27]=variant&2;
        }
        if(variant&4) memset(ranks,0,sizeof ranks);
        unsigned char date[]={variant&1?'/':'.',(unsigned char)(variant%2)};
        check(uc_mem_write(u,0x3cbf0+0x171e,date,2));
        check(uc_mem_write(u,0x3cbf0+0x693a,records.entries,sizeof records.entries));
        check(uc_mem_write(u,0x3cbf0+0x4bce,ranks,sizeof ranks));
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
        word(u,0x8f000,0); word(u,0x8f002,0x9000); word(u,0x8f004,80); word(u,0x8f006,40);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x1aabc,0x90000,0,20000000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp)); if(ip || sp!=0xf004) abort();
        if(slicks_records_renderer_draw(&renderer,&records,ranks,80,40,date[0],(signed char)date[1])) abort();
        if(memcmp(pixels,v.pixels,sizeof pixels)) {
            for(unsigned i=0;i<64000;++i) if(pixels[i]!=v.pixels[i]) { fprintf(stderr,"Records pixel mismatch variant=%u at %u,%u native=%u DOS=%u\n",variant,i%320,i/320,pixels[i],v.pixels[i]); break; }
            return 1;
        }
        for(unsigned i=0;i<2;++i) {
            unsigned char actual[8192]; check(uc_mem_read(u,0x50000+i*0x2000,actual,p.native.sizes[i]));
            if(memcmp(actual,fonts[i],p.native.sizes[i])) { fputs("Records font state mismatch\n",stderr); return 1; }
        }
        ++cases;
    }
    uc_hook info_hooks[2];
    check(uc_hook_add(u,&info_hooks[0],UC_HOOK_CODE,info_boundary,0,0x35e63,0x35e63));
    check(uc_hook_add(u,&info_hooks[1],UC_HOOK_CODE,info_boundary,0,0x1a4ef,0x1a4ef));
    unsigned compositions=0;
    for(unsigned variant=0;variant<12;++variant) {
        const unsigned char *name=(const unsigned char *)(variant&1?"BASIC":"A LONG TRACK NAME");
        const unsigned char *description=(const unsigned char *)(variant&2?"":"Original track description");
        unsigned char percent=(unsigned char[]){0,50,100}[variant%3];
        struct SlicksTrackRecords records={0}; signed char ranks[4]={0};
        if(variant&4) {
            unsigned char *r=records.entries[0];
            memcpy(r,"RECORD HOLDER",14); r[20]=0xb0; r[21]=4;
            r[22]=5; r[23]=9; r[24]=0xea; r[25]=7; r[26]=2; r[27]=1;
        }
        memcpy(pixels,base,sizeof pixels); memcpy(v.pixels,base,sizeof base);
        check(uc_mem_write(u,0x66000,name,strlen((const char *)name)+1));
        check(uc_mem_write(u,0x3cbf0+0x4c7c,description,strlen((const char *)description)+1));
        check(uc_mem_write(u,0x3cbf0+0x693a,records.entries,sizeof records.entries));
        check(uc_mem_write(u,0x3cbf0+0x4bce,ranks,sizeof ranks));
        word(u,0x3cbf0+0x16fd,percent);
        word(u,0x3cbf0+0x171e,'.');
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xe000,bp=0xf000,ip;
        word(u,0x8f006,0);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_emu_start(u,0x2682c,0x269e8,0,30000000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        if(ip!=0x269e8-0x266c0 || sp!=0xe000) abort();
        if(slicks_track_info_render(&renderer,&records,name,description,percent,'.',0)) abort();
        if(memcmp(pixels,v.pixels,sizeof pixels)) {
            for(unsigned i=0;i<64000;++i) if(pixels[i]!=v.pixels[i]) {
                fprintf(stderr,"Info composition variant=%u at %u,%u native=%u DOS=%u\n",variant,i%320,i/320,pixels[i],v.pixels[i]); break;
            }
            return 1;
        }
        for(unsigned i=0;i<2;++i) {
            unsigned char actual[8192]; check(uc_mem_read(u,0x50000+i*0x2000,actual,p.native.sizes[i]));
            if(memcmp(actual,fonts[i],p.native.sizes[i])) abort();
        }
        ++compositions;
    }
    printf("Original track-info surround: %u full-screen/font compositions pass before separately verified preview\n",compositions);
    verify_speed_pixels(u,&renderer,&v,base,p.native.sizes[0]);
    verify_pause_pixels(u,&renderer,&v,base,p.native.sizes[0]);
    p.native.records_bridge=standings_bridge; p.native.shadow=0x0101;
    verify_cup_pixels(u,&renderer,&v);
    check(uc_close(u)); check(uc_close(p.native.cpu)); check(uc_close(p.native.icon_cpu));
    printf("Original records panel: %u full-screen/font comparisons with real assets and 68020 text/icons pass\n",cases); return 0;
}
