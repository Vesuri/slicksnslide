#define main options_verifier_main
#include "verify_options_menu.c"
#undef main
#include "../src/ui/demo_overlay.h"

static unsigned event,shade;
struct DrawEvent { unsigned kind,a,b,c,d; };
static struct DrawEvent original[6],native[6];
static unsigned native_count;
static const unsigned char *native_label;
static void emit(unsigned kind,unsigned a,unsigned b,unsigned c,unsigned d)
{
    if(native_count>=6) abort();
    native[native_count++]=(struct DrawEvent){kind,a,b,c,d};
}
static unsigned char nearest(void *p,unsigned char r,unsigned char g,unsigned char b)
{
    (void)p; unsigned pass=native_count/3;
    emit(0,r,g,b,0); return (unsigned char)(shade+pass);
}
static void colour(void *p,unsigned char index,unsigned char value)
{ (void)p; emit(1,index,value,0,0); }
static void text(void *p,const unsigned char *label,short x,short y,unsigned char flags)
{
    (void)p; if(label!=native_label) abort();
    emit(2,(unsigned short)x,(unsigned short)y,flags,0xbff);
}
static void boundary(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size;(void)context;
    if(address==0x1f901) { check(uc_emu_stop(u)); return; }
    if(address!=0x36fae && address!=0x2fe63 && address!=0x301ab) return;
    uint16_t sp,ss; check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); unsigned at=16U*ss+sp;
    unsigned pass=event/3,kind=event%3;
    if(event>=6) abort();
    if(kind==0) {
        unsigned rgb=pass?50:0;
        if(address!=0x36fae || readword(u,at+4)!=rgb ||
            readword(u,at+6)!=rgb || readword(u,at+8)!=rgb) abort();
        uint16_t ax=(uint16_t)(shade+pass);
        check(uc_reg_write(u,UC_X86_REG_AX,&ax));
        original[event]=(struct DrawEvent){0,readword(u,at+4),readword(u,at+6),readword(u,at+8),0};
    } else if(kind==1) {
        if(address!=0x2fe63 || readword(u,at+4)!=shade+pass ||
            readword(u,at+6)!=0x1234 || readword(u,at+8)!=0x7000 || readword(u,at+10)) abort();
        original[event]=(struct DrawEvent){1,readword(u,at+10),readword(u,at+4),0,0};
    } else {
        if(address!=0x301ab || readword(u,at+4)!=(pass?10:11) ||
            readword(u,at+6)!=(pass?10:11) || readword(u,at+8)!=0xbff ||
            readword(u,at+10)!=0x3cbf || readword(u,at+12)!=0x1234 ||
            readword(u,at+14)!=0x7000 || readword(u,at+16)) abort();
        original[event]=(struct DrawEvent){2,readword(u,at+4),readword(u,at+6),readword(u,at+16),readword(u,at+8)};
    }
    ++event;
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f)return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f); if(n<200000 || n==sizeof runtime)return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,n));
    /* Intercept only renderer service boundaries. The actual signed-flag
     * helper, complete overlay branch and all call arguments execute. */
    unsigned char retf=0xcb;
    check(uc_mem_write(u,0x36fae,&retf,1));
    check(uc_mem_write(u,0x2fe63,&retf,1));
    check(uc_mem_write(u,0x301ab,&retf,1));
    uc_hook hook; check(uc_hook_add(u,&hook,UC_HOOK_CODE,boundary,0,1,0));
    word(u,0x3cbf0+0x680,0x1234); word(u,0x3cbf0+0x682,0x7000);
    native_label=runtime+0x3cbf0-0x10100+0xbff;
    const struct SlicksDemoOverlayOps ops={nearest,colour,text,0};
    unsigned cases=0;
    for(unsigned flag=0;flag<256;++flag) for(shade=0;shade<255;shade+=127) {
        regs(u,0); uint16_t cs=0x1987,ip; check(uc_reg_write(u,UC_X86_REG_CS,&cs));
        unsigned char value=(unsigned char)flag;
        check(uc_mem_write(u,0x3cbf0+0x459,&value,1)); event=0;
        check(uc_emu_start(u,0x1f84d,0x1fb46,0,1000));
        check(uc_reg_read(u,UC_X86_REG_CS,&cs)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        unsigned negative=flag>=128;
        if(event!=(negative?6U:0U) || cs*16U+ip!=(negative?0x1fb46U:0x1f901U)) abort();
        native_count=0;
        if(slicks_demo_overlay((signed char)flag,native_label,&ops)!=(int)negative ||
            native_count!=event || memcmp(original,native,event*sizeof *original)) abort();
        ++cases;
    }
    check(uc_close(u));
    printf("Original/native demo overlay: %u complete ordered call comparisons, signed flags and normal-owner bypass pass\n",cases);
    return 0;
}
