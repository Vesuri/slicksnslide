#define main options_verifier_main
#include "verify_options_menu.c"
#undef main
#include "host_archive.h"
#include "../src/ui/help_line.h"
struct Event { int kind,a,b,c,d,e; unsigned char text[512]; };
struct Events { struct Event event[128]; unsigned count; };
static struct Events native_events,dos_events;
static struct Event *event(struct Events *events,int kind)
{
    if(events->count==128) abort();
    struct Event *e=&events->event[events->count++]; memset(e,0,sizeof *e); e->kind=kind; return e;
}
static void colour(void *context,unsigned char c) { event(context,1)->a=c; }
static unsigned char nearest(void *context,unsigned char r,unsigned char g,unsigned char b)
{ struct Event *e=event(context,2); e->a=r; e->b=g; e->c=b; return (unsigned char)(r+3*g+7*b); }
static void rectangle(void *context,short x,short y,short right,short bottom,unsigned char c)
{ struct Event *e=event(context,3); e->a=x; e->b=y; e->c=right; e->d=bottom; e->e=c; }
static short measure(void *context,const unsigned char *s,signed char spacing)
{
    struct Event *e=event(context,4); e->a=spacing; strcpy((char *)e->text,(const char *)s);
    int width=0; while(*s) width+=3+(*s++%4)+spacing; return (short)width;
}
static short text_draw(void *context,const unsigned char *s,short x,short y,signed char spacing)
{
    struct Event *e=event(context,5); e->a=x; e->b=y; e->c=spacing; strcpy((char *)e->text,(const char *)s);
    int width=0; while(*s) width+=3+(*s++%4)+spacing; return (short)width;
}
static void draw_service(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size; (void)context;
    uint16_t ss,sp,ip,cs,ax=0; unsigned char spacing,text[512]={0};
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp; unsigned a[8]; for(unsigned i=0;i<8;++i) a[i]=readword(u,stack+4+2*i);
    check(uc_mem_read(u,0x3cbf0+0x1604,&spacing,1));
    if(address==0x2fe63) colour(&dos_events,(unsigned char)a[0]);
    else if(address==0x36fae) ax=nearest(&dos_events,a[0],a[1],a[2]);
    else if(address==0x39ed8) rectangle(&dos_events,(short)a[0],(short)a[1],(short)a[2],(short)a[3],a[4]);
    else if(address==0x300e5) {
        check(uc_mem_read(u,a[0]+16*a[1],text,sizeof text)); text[511]=0;
        ax=(unsigned short)measure(&dos_events,text,(signed char)spacing);
    } else if(address==0x301ab) {
        check(uc_mem_read(u,a[2]+16*a[3],text,sizeof text)); text[511]=0;
        ax=(unsigned short)text_draw(&dos_events,text,(short)a[0],(short)a[1],(signed char)spacing);
    }
    ip=readword(u,stack); cs=readword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void byte(uc_engine *u,unsigned at,unsigned char value) { check(uc_mem_write(u,at,&value,1)); }
static void compare(uc_engine *u,const unsigned char *source,unsigned length,unsigned variant,unsigned number)
{
    unsigned char line[512]={0}; if(length>400) abort(); memcpy(line,source,length);
    if(slicks_help_preprocess(line,sizeof line)) abort();
    struct SlicksHelpStyle s={0}; s.left=50; s.top=10; s.right=270; s.bottom=190;
    s.height=7; s.distance=2; s.spacing=1; s.links=(short)(3*(variant/4));
    s.selected=(short)((variant%4)+s.links-1); s.country=variant%4==2?358:1;
    s.centred=variant%4==3; s.total_links=11;
    for(unsigned i=0;i<4;++i) s.colours[i]=20+10*i;
    unsigned base=0x3cbf0;
    word(u,base+0x169c,s.left); word(u,base+0x169e,s.top); word(u,base+0x16a0,s.right); word(u,base+0x16a2,s.bottom);
    word(u,base+0x6f8c,s.selected); word(u,base+0x6ff6,s.links); word(u,base+0x6ff4,s.total_links); word(u,base+0x1720,s.country);
    check(uc_mem_write(u,base+0x6f20,s.colours,4)); byte(u,base+0x6f26,s.distance); byte(u,base+0x1604,s.spacing);
    byte(u,base+0x6f8e,s.centred); byte(u,base+0x6f8f,0); byte(u,base+0x6f8b,0);
    check(uc_mem_write(u,base+0x16a8,s.prefix,6)); check(uc_mem_write(u,base+0x6f27,s.next,21));
    check(uc_mem_write(u,base+0x6f45,s.previous,21)); check(uc_mem_write(u,base+0x6f63,s.target,21));
    word(u,base+0x16a4,0); word(u,base+0x16a6,0x6000); byte(u,0x60002,s.height);
    check(uc_mem_write(u,0x70000,line,sizeof line));
    uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,ax;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    word(u,0x8f000,0); word(u,0x8f002,0x9000); word(u,0x8f004,0); word(u,0x8f006,0x7000); word(u,0x8f008,37);
    native_events.count=dos_events.count=0;
    check(uc_emu_start(u,0x31b53,0x90000,0,1000000)); check(uc_reg_read(u,UC_X86_REG_AX,&ax));
    const struct SlicksHelpDrawOps ops={&native_events,colour,nearest,rectangle,measure,text_draw}; short y=37;
    if(slicks_help_draw_line(&s,line,&y,&ops)) abort();
    if(native_events.count!=dos_events.count || memcmp(native_events.event,dos_events.event,dos_events.count*sizeof(struct Event)) || (unsigned short)y!=ax) {
        fprintf(stderr,"Help line %u variant %u draw mismatch counts %u/%u y %d/%d: %s\n",number,variant,native_events.count,dos_events.count,y,(short)ax,source);
        for(unsigned i=0;i<native_events.count || i<dos_events.count;++i) {
            struct Event *a=&native_events.event[i],*b=&dos_events.event[i];
            fprintf(stderr,"%u native %d %d %d %d %d %d %s / DOS %d %d %d %d %d %d %s\n",i,a->kind,a->a,a->b,a->c,a->d,a->e,a->text,b->kind,b->a,b->b,b->c,b->d,b->e,b->text);
        } exit(1);
    }
    unsigned addresses[]={0x169c,0x169e,0x16a0,0x16a2,0x6ff6,0x6ff4};
    short values[]={s.left,s.top,s.right,s.bottom,s.links,s.total_links};
    for(unsigned i=0;i<6;++i) if(readword(u,base+addresses[i])!=(unsigned short)values[i]) abort();
    unsigned byte_addresses[]={0x6f20,0x6f21,0x6f22,0x6f23,0x6f26,0x1604,0x6f8e,0x6f8f,0x6f8b};
    unsigned char values8[]={s.colours[0],s.colours[1],s.colours[2],s.colours[3],s.distance,s.spacing,s.centred,s.link_mode,s.link_type};
    for(unsigned i=0;i<9;++i) { unsigned char actual; check(uc_mem_read(u,base+byte_addresses[i],&actual,1));
        if(actual!=values8[i]) { fprintf(stderr,"Help state %x mismatch line %u variant %u %u/%u\n",byte_addresses[i],number,variant,values8[i],actual); exit(1); }
    }
    unsigned text_addresses[]={0x16a8,0x6f27,0x6f45,0x6f63},sizes[]={6,21,21,21};
    const unsigned char *texts[]={s.prefix,s.next,s.previous,s.target};
    for(unsigned i=0;i<4;++i) { unsigned char actual[21]; check(uc_mem_read(u,base+text_addresses[i],actual,sizes[i])); if(memcmp(actual,texts[i],sizes[i])) abort(); }
}
int main(void)
{
    unsigned char runtime[300000],help[65536]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,n));
    const unsigned addresses[]={0x2fe63,0x36fae,0x39ed8,0x300e5,0x301ab};
    for(unsigned i=0;i<5;++i) { uc_hook h; check(uc_hook_add(u,&h,UC_HOOK_CODE,draw_service,0,addresses[i],addresses[i])); }
    long loaded=host_archive_load("ref/SLICKS.000","HELP.TXT",help,sizeof help); if(loaded<0) return 2;
    unsigned cases=0;
    for(unsigned at=0;at<(unsigned)loaded;) {
        unsigned start=at; while(at<(unsigned)loaded && help[at]!='\n') ++at;
        if(at<(unsigned)loaded) ++at;
        if(help[start]=='!') ++start;
        for(unsigned variant=0;variant<8;++variant) compare(u,help+start,at-start,variant,cases++);
    }
    const char *extra[]={"<window 20 30 290 180>","<next main><prev options>","<dist -2><distx 3>words",
        "<center><a main>one</a> <a options>two</a></center>","<hr 0>","<hr 11>","<color 3 45 0 45>test", "<ai Note>info</a>"};
    for(unsigned i=0;i<sizeof extra/sizeof extra[0];++i) for(unsigned variant=0;variant<8;++variant)
        compare(u,(const unsigned char *)extra[i],(unsigned)strlen(extra[i]),variant,cases++);
    check(uc_close(u)); printf("Original help line renderer: %u drawing-order, text-position and formatting-state comparisons pass\n",cases); return 0;
}
