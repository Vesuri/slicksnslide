#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/ui/text_entry.h"
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned a,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(u,a,b,2)); }
static unsigned getword(uc_engine *u,unsigned a)
{ unsigned char b[2]; check(uc_mem_read(u,a,b,2)); return b[0]|b[1]<<8; }
static void boundary(uc_engine *u,uint64_t address,uint32_t size,void *opaque)
{
    (void)size;
    if(address==0x2fb58) { *(unsigned *)opaque=1; check(uc_emu_stop(u)); return; }
    /* VGA saved-under restore: no text-state effects. */
    uint16_t ss,sp,cs,ip;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    ip=getword(u,ss*16U+sp); cs=getword(u,ss*16U+sp+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_CS,&cs));
    check(uc_reg_write(u,UC_X86_REG_IP,&ip));
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f); int error=ferror(f); fclose(f);
    if(error || bytes<200000 || bytes==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,bytes));
    unsigned stopped=0,cases=0; uc_hook h;
    check(uc_hook_add(u,&h,UC_HOOK_CODE,boundary,&stopped,0x2fb58,0x2fb58));
    check(uc_hook_add(u,&h,UC_HOOK_CODE,boundary,&stopped,0x3aaf2,0x3aaf2));
    for(unsigned length=0;length<=20;++length) for(unsigned preserve=0;preserve<2;++preserve) {
        unsigned char text[24],actual[24]; memset(text,'A',sizeof text); text[length]=0;
        check(uc_mem_write(u,0x60000,text,sizeof text));
        word(u,0x8effc,0); word(u,0x8f00a,0); word(u,0x8f00c,0x6000);
        word(u,0x8f014,preserve?512:0);
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_emu_start(u,0x2f753,0x2f784,0,1000)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        struct SlicksTextEntry state;
        if(slicks_text_entry_begin(&state,text,sizeof text,preserve?512:0)) abort();
        check(uc_mem_read(u,0x60000,actual,sizeof actual));
        if(ip!=0x2f784-0x2e0f0 || getword(u,0x8effc)!=state.position || memcmp(text,actual,sizeof text)) abort();
    }
    puts("DOS text-entry initialization: 42 clear/preserve comparisons pass, including retained-name append position");
    const unsigned positions[]={0,1,19,20};
    for(unsigned variant=0;variant<3;++variant) for(unsigned flags=0;flags<64;++flags)
    for(unsigned pos=0;pos<4;++pos) for(unsigned key=0;key<256;++key) {
        unsigned char font[300]={0}; font[0]=128; font[5]=4;
        for(unsigned i=0;i<128;++i) font[10+i]=(unsigned char)(variant==0?i:variant==1?i+128:i*2);
        unsigned char text[24],actual[24]; memset(text,0xa5,sizeof text);
        for(unsigned i=0;i<positions[pos];++i) text[i]=(unsigned char)('A'+i);
        text[positions[pos]]=0;
        check(uc_mem_write(u,0x60000,text,sizeof text)); check(uc_mem_write(u,0x60100,font,sizeof font));
        word(u,0x8effc,positions[pos]); word(u,0x8eff9,key);
        word(u,0x8f00a,0); word(u,0x8f00c,0x6000); word(u,0x8f00e,20);
        word(u,0x8f010,0x100); word(u,0x8f012,0x6000); word(u,0x8f014,flags);
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp)); stopped=0;
        check(uc_emu_start(u,0x2f92d,0x2faa7,0,10000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        struct SlicksTextEntry state={(unsigned short)positions[pos],0};
        unsigned result=slicks_text_entry_key(&state,text,20,flags,font,(unsigned char)key);
        unsigned char character; check(uc_mem_read(u,0x8eff9,&character,1));
        check(uc_mem_read(u,0x60000,actual,sizeof actual));
        if(ip!=(stopped?0x2fb58:0x2faa7)-0x2e0f0 || sp!=0xeb00 ||
            stopped!=(result!=0) || character!=state.character ||
            getword(u,0x8effc)!=state.position || memcmp(text,actual,sizeof text)) {
            fprintf(stderr,"Text input mismatch font=%u flags=%u position=%u key=%u pc=%04x\n",variant,flags,positions[pos],key,ip); return 1;
        }
        ++cases;
    }
    check(uc_close(u)); printf("DOS text input: %u character/filter/buffer-state comparisons pass\n",cases); return 0;
}
