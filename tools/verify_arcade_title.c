#define main arcade_hud_verifier_main
#include "verify_arcade_hud.c"
#undef main
#include "../src/ui/arcade_title_draw.h"
struct Event { short kind,a[6]; };
struct Events { struct Event e[64];unsigned count;unsigned char shadow; };
static void event(struct Events *t,short kind,short a,short b,short c,short d,short e,short f)
{ if(t->count==64) abort();t->e[t->count++]=(struct Event){kind,{a,b,c,d,e,f}}; }
static unsigned char nearest(void *p,unsigned char r,unsigned char g,unsigned char b)
{ event(p,0,r,g,b,0,0,0);return (unsigned char)(r*3+g*5+b*7); }
static void colour(void *p,unsigned font,unsigned char c) {event(p,1,font,c,0,0,0,0);}
static void shadow(void *p,unsigned char c) {((struct Events *)p)->shadow=c;}
static void restore(void *p,short x,short y,short w,short h) {event(p,2,x,y,w,h,0,0);}
static void rectangle(void *p,short l,short t,short r,short b,unsigned char c) {event(p,3,l,t,r,b,c,0);}
static void text_command(void *p,unsigned font,unsigned id,short x,short y,unsigned char flags)
{event(p,4,font,id,x,y,flags,0);}
static void boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)size;uint16_t ss,sp,cs,ip,ax=0,dx=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss));check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned s=16U*ss+sp,a[10];for(unsigned i=0;i<10;++i)a[i]=get(u,s+4+2*i);
    if(address==0x36fae) {
        ax=nearest(p,a[0],a[1],a[2]);check(uc_reg_write(u,UC_X86_REG_AX,&ax));
    } else if(address==0x2fe63) {
        if(a[2]!=0x7000 || a[1]>3 || a[3]) abort();colour(p,a[1],a[0]);
    } else if(address==0x3b9de) {
        if(a[0] || a[1] || a[6]!=0 || a[7]!=0x7100 || a[8]) abort();
        restore(p,a[2],a[3],a[4],a[5]);
    } else if(address==0x39ed8) {
        if(a[5]) abort();rectangle(p,a[0],a[1],a[2],a[3],a[4]);
    } else if(address==0x36227) {
        ax=a[0];dx=a[1];check(uc_reg_write(u,UC_X86_REG_AX,&ax));check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    } else if(address==0x12943) {
        unsigned char id=a[2]==0x13f7?(unsigned char)a[4]:6;
        if(id<1 || id>6) abort();check(uc_mem_write(u,a[0]+16U*a[1],&id,1));
    } else if(address==0x301ab) {
        unsigned char id;
        if(a[3]==0x3cbf && a[2]==0x13ef) id=0;
        else if(a[3]==0x3cbf && a[2]==0x13fb) id=5;
        else check(uc_mem_read(u,a[2]+16U*a[3],&id,1));
        if(a[5]!=0x7000 || a[4]>3 || a[7]) abort();
        text_command(p,a[4],id,a[0],a[1],a[6]);
    } else abort();
    ip=get(u,s);cs=get(u,s+2);sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_IP,&ip));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    for(unsigned i=0;i<65536;++i) {
        short values[2]={(short)i,(short)(65535-i)};unsigned char out[96];char expected[96];
        snprintf(expected,sizeof expected,"%d SECS\n%d TRACKS",values[0],values[1]);
        if(slicks_arcade_title_format(out,sizeof out,(const unsigned char *)"%d SECS\n%d TRACKS",values,2) || strcmp((char *)out,expected)) return 1;
    }
    {unsigned char out[4];short value=1;
        if(!slicks_arcade_title_format(out,sizeof out,(const unsigned char *)"%d",&value,0) ||
           !slicks_arcade_title_format(out,sizeof out,(const unsigned char *)"%s",&value,1) ||
           !slicks_arcade_title_format(out,sizeof out,(const unsigned char *)"1234",&value,1) ||
           slicks_arcade_title_format(out,sizeof out,(const unsigned char *)"%%",&value,0) || strcmp((char *)out,"%")) return 1;
    }
    unsigned char runtime[300000];FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f);fclose(f);
    uc_engine *u;check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,bytes));struct Events actual;
    const unsigned hooks[]={0x36fae,0x2fe63,0x3b9de,0x39ed8,0x36227,0x12943,0x301ab};
    for(unsigned i=0;i<7;++i){uc_hook h;check(uc_hook_add(u,&h,UC_HOOK_CODE,boundary,&actual,hooks[i],hooks[i]));}
    for(unsigned i=0;i<3;++i){word(u,0x3cbf0+0x680+4*i,i);word(u,0x3cbf0+0x682+4*i,0x7000);}
    word(u,0x3cbf0+0x6bd4,3);word(u,0x3cbf0+0x6bd6,0x7000);
    word(u,0x3cbf0+0x5b8,0);word(u,0x3cbf0+0x5ba,0x7100);word(u,0x3cbf0+0x1d89,0);
    unsigned cases=0;const short counts[]={-1,0,1,2,3,4,5,32767};
    for(unsigned initial=0;initial<256;++initial)for(unsigned selected=0;selected<3;++selected)
    for(unsigned k=0;k<8;++k)for(unsigned dirty=0;dirty<2;++dirty){
        signed char colours[4][6];for(unsigned i=0;i<4;++i)for(unsigned j=0;j<6;++j)
            colours[i][j]=(signed char)(initial+39*i+71*j);
        check(uc_mem_write(u,0x3cbf0+0x433,colours,sizeof colours));word(u,0x3cbf0+0xf1a,(unsigned short)counts[k]);
        unsigned char counter=initial,refresh=dirty?2:0;
        check(uc_mem_write(u,0x3cbf0+0x6b4e,&counter,1));check(uc_mem_write(u,0x3cbf0+0x1146,&refresh,1));
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xf000;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        word(u,0x8f000,0);word(u,0x8f002,0x9000);word(u,0x8f004,selected);
        actual=(struct Events){0};check(uc_emu_start(u,0x29afa,0x90000,0,10000));
        actual.shadow=(unsigned char)get(u,0x3cbf0+0x1600);
        struct Events expected={0};const struct SlicksArcadeTitleDrawOps ops={nearest,colour,shadow,restore,rectangle,text_command,&expected};
        slicks_arcade_title_draw(&counter,&refresh,selected,counts[k],colours,&ops);
        unsigned char got_counter,got_refresh;check(uc_mem_read(u,0x3cbf0+0x6b4e,&got_counter,1));check(uc_mem_read(u,0x3cbf0+0x1146,&got_refresh,1));
        if(memcmp(&actual,&expected,sizeof actual) || counter!=got_counter || refresh!=got_refresh){
            fprintf(stderr,"Arcade title differs counter=%u row=%u players=%d dirty=%u calls=%u/%u\n",initial,selected,counts[k],dirty,actual.count,expected.count);
            for(unsigned i=0;i<actual.count;++i)if(memcmp(&actual.e[i],&expected.e[i],sizeof actual.e[i])){
                fprintf(stderr,"event %u kind=%d/%d\n",i,actual.e[i].kind,expected.e[i].kind);return 1;}
            return 1;
        }
        ++cases;
    }
    uc_close(u);printf("Original Arcade title: %u complete colour/crop/font/text/rectangle command comparisons pass\n",cases);return 0;
}
