#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/ui/saved_file_dialog.h"
static void check(uc_err error) { if(error) { fprintf(stderr,"%s\n",uc_strerror(error)); exit(1); } }
static unsigned word(uc_engine *u,unsigned at)
{ unsigned char b[2]; check(uc_mem_read(u,at,b,2)); return b[0]|b[1]<<8; }
static void put(uc_engine *u,unsigned at,unsigned value)
{ unsigned char b[2]={(unsigned char)value,(unsigned char)(value>>8)}; check(uc_mem_write(u,at,b,2)); }
struct ChoiceTrace { enum SlicksSavedFileAction action; short index; unsigned char answer; };
static void boundary(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size; struct ChoiceTrace *t=context;
    uint16_t ss,sp;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=16U*ss+sp;
    if(address==0x347da) {
        t->action=SLICKS_SAVED_FILE_DELETE;
        uint16_t ax=t->answer,ip=word(u,stack),cs=word(u,stack+2); sp+=4;
        check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_CS,&cs));
        check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp)); return;
    }
    if(address==0x307b6) {
        if(word(u,stack+4)!=100 || word(u,stack+6)!=65 || word(u,stack+16)!=8 || word(u,stack+22)!=0x1b) abort();
        t->action=SLICKS_SAVED_FILE_NAME;
    } else if(address==0x1100c) {
        unsigned offset=word(u,stack+8);
        if(offset%9 || word(u,stack+10)!=0x5000) abort();
        t->index=(short)(offset/9);
        if(t->action!=SLICKS_SAVED_FILE_DELETE) t->action=SLICKS_SAVED_FILE_SELECT;
    } else abort();
    check(uc_emu_stop(u));
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t size=fread(runtime,1,sizeof runtime,f); fclose(f);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,size)); struct ChoiceTrace trace;
    const unsigned hooks[]={0x347da,0x307b6,0x1100c};
    for(unsigned i=0;i<3;++i) { uc_hook h; check(uc_hook_add(u,&h,UC_HOOK_CODE,boundary,&trace,hooks[i],hooks[i])); }
    unsigned cases=0;
    for(unsigned result=0;result<65536;++result) for(unsigned saving=0;saving<2;++saving) for(unsigned yes=0;yes<2;++yes) {
        trace=(struct ChoiceTrace){SLICKS_SAVED_FILE_CANCEL,-1,(unsigned char)(yes?0x15:1)};
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,bp=0xf000,sp=0xeb00,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_BP,&bp)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        put(u,0x8eff8,result); put(u,0x8effc,0); put(u,0x8effe,0x5000); put(u,0x8f00e,saving);
        put(u,0x8f006,0); put(u,0x8f008,0x6500);
        check(uc_emu_start(u,0x1d823,0x1d913,0,10000)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        struct SlicksSavedFileChoice expected=slicks_saved_file_choice((short)result,(unsigned char)saving);
        int expects_index=expected.action==SLICKS_SAVED_FILE_SELECT ||
            (expected.action==SLICKS_SAVED_FILE_DELETE && slicks_saved_file_delete_accepted(trace.answer));
        if(trace.action!=expected.action || (expects_index && trace.index!=expected.index) ||
           (!expects_index && trace.index!=-1) ||
           (ip!=0x347da-0x2e0f0 && ip!=0x307b6-0x2e0f0 && ip!=0x1100c-0x10100 && ip!=0x1d913-0x19870)) {
            fprintf(stderr,"Saved-file caller mismatch result=%d saving=%u yes=%u action=%u/%u index=%d/%d ip=%x\n",
                (short)result,saving,yes,trace.action,expected.action,trace.index,expected.index,ip); return 1;
        }
        ++cases;
    }
    uc_close(u); printf("Original saved-file caller: %u save/load/result/confirmation/action/index comparisons pass\n",cases);
    return 0;
}
