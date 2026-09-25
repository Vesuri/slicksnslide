#define main records_verifier_main
#include "verify_post_race_records.c"
#undef main
#include "../src/ui/result_wait.h"
struct WaitTest { unsigned polls,delays,clears,release,press,button_at; };
static short key(void *p)
{
    struct WaitTest *t=p; ++t->polls;
    return t->polls<=t->release?28:t->polls>=t->press?57:156;
}
static unsigned char button(void *p)
{ struct WaitTest *t=p; return t->polls>=t->button_at; }
static void delay(void *p,unsigned short ms)
{ if(ms!=100) abort(); ++((struct WaitTest *)p)->delays; }
static void clear(void *p) { ++((struct WaitTest *)p)->clears; }
static void boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)size; uint16_t ss,sp,cs,ip,ax=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    if(address==0x36c97) ax=(uint16_t)key(p);
    else if(address==0x36c70) ax=button(p);
    else if(address==0x19878) delay(p,(unsigned short)get(u,stack+4));
    else if(address==0x36d65) clear(p);
    else abort();
    ip=get(u,stack); cs=get(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_CS,&cs));
    check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n)); struct WaitTest dos; uc_hook hook;
    const unsigned addresses[]={0x36c97,0x36c70,0x19878,0x36d65};
    for(unsigned i=0;i<4;++i) check(uc_hook_add(u,&hook,UC_HOOK_CODE,boundary,&dos,addresses[i],addresses[i]));
    const short limits[]={-1,0,1,30,300,1200}; unsigned cases=0;
    for(unsigned demo=0;demo<2;++demo) for(unsigned l=0;l<6;++l)
    for(unsigned release=0;release<4;++release) for(unsigned input=0;input<4;++input) {
        dos=(struct WaitTest){.release=release,.press=input==0?release+2:input==1?release+18:2000,
            .button_at=input==2?release+7:4000};
        struct WaitTest native=dos;
        word(u,0x3cbf0+0x622,demo);
        word(u,0x8f000,0); word(u,0x8f002,0x9000); word(u,0x8f004,limits[l]);
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xf000,ax;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x2b73b,0x90000,0,300000)); check(uc_reg_read(u,UC_X86_REG_AX,&ax));
        const struct SlicksResultWaitOps ops={key,button,delay,clear,&native};
        short result=slicks_result_wait(limits[l],(unsigned char)demo,&ops);
        if(ax!=(unsigned short)result || memcmp(&native,&dos,sizeof dos)) abort();
        ++cases;
    }
    check(uc_close(u)); printf("Original result waits: %u release/key/button/timeout/demo cases match poll, delay and clear calls\n",cases); return 0;
}
