#define main records_verifier_main
#include "verify_post_race_records.c"
#undef main
#include "../src/ui/palette_fade.h"
struct FadeTest { unsigned calls,clock,stride; short start,end,duration,step; unsigned char palette[768]; };
static void fade_boundary(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size; struct FadeTest *t=context;
    uint16_t ss,sp,cs,ip,ax=0,dx=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    if(address==0x3703a) {
        unsigned index=get(u,stack+4); unsigned char expected[768];
        slicks_fade_palette(expected,t->palette,slicks_fade_weight(t->start,t->end,t->duration,t->step));
        if(index!=t->calls%256 || t->step>t->duration) abort();
        for(unsigned c=0;c<3;++c) if((get(u,stack+6+2*c)&255)!=expected[index*3+c]) {
            fprintf(stderr,"Fade colour mismatch step=%d index=%u component=%u\n",t->step,index,c); exit(1);
        }
        ++t->calls;
    } else if(address==0x37b18) {
        t->clock+=t->stride;
        t->step=slicks_fade_advance(t->step,t->duration,(unsigned short)t->stride);
        ax=(uint16_t)t->clock; dx=(uint16_t)(t->clock>>16);
    } else if(address==0x37a8b) { ax=(uint16_t)t->clock; dx=(uint16_t)(t->clock>>16); }
    else if(address!=0x2eae4) abort();
    ip=get(u,stack); cs=get(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n)); struct FadeTest t; uc_hook hook;
    const unsigned addresses[]={0x3703a,0x37b18,0x37a8b,0x2eae4};
    for(unsigned i=0;i<4;++i) check(uc_hook_add(u,&hook,UC_HOOK_CODE,fade_boundary,&t,addresses[i],addresses[i]));
    unsigned cases=0,colours=0;
    const unsigned timers[]={0,250,500,1000};
    for(unsigned timer=0;timer<4;++timer) for(unsigned direction=0;direction<4;++direction)
    for(unsigned duration=2;duration<=6;duration+=2) for(unsigned stride=1;stride<=7;++stride) {
        memset(&t,0,sizeof t); t.start=direction&1?100:0; t.end=direction&2?50:100-t.start;
        t.duration=slicks_fade_duration(timers[timer],(short)duration); t.stride=stride;
        for(unsigned i=0;i<768;++i) t.palette[i]=(unsigned char)((i*17+cases)%64);
        check(uc_mem_write(u,0x60000,t.palette,768));
        word(u,0x3cbf0+0x17b6,timers[timer]);
        word(u,0x8f000,0); word(u,0x8f002,0x9000);
        word(u,0x8f004,t.start); word(u,0x8f006,t.end); word(u,0x8f008,duration);
        word(u,0x8f00a,0); word(u,0x8f00c,0x6000);
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x378a3,0x90000,0,3000000));
        unsigned char saved[768]; check(uc_mem_read(u,0x3cbf0+0x71bc,saved,768));
        if(t.step<=t.duration || !t.calls || t.calls%256 || memcmp(saved,t.palette,768)) abort();
        colours+=t.calls; ++cases;
    }
    check(uc_close(u)); printf("Original palette fade: %u full routines, %u RGB writes, skipped-tick endpoints and preserved base palettes pass\n",cases,colours); return 0;
}
