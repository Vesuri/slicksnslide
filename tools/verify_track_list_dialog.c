#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/ui/track_list_dialog.h"
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static unsigned word(uc_engine *u,unsigned at) { unsigned char b[2]; check(uc_mem_read(u,at,b,2)); return b[0]|b[1]<<8; }
static void put(uc_engine *u,unsigned at,unsigned v) { unsigned char b[2]={v,v>>8}; check(uc_mem_write(u,at,b,2)); }
struct Trace { unsigned names,questions,loads,writes,reloads,frees; short load,remove; unsigned char name_result,answer; };
static void boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)size; struct Trace *t=p; uint16_t ss,sp,cs,ip,ax=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    if(address==0x2d28a) {
        short index=(short)word(u,stack+4);
        if(index<0) ++t->reloads;
        else { ++t->loads; t->load=index; }
    } else if(address==0x2d4db) { ++t->writes; t->remove=(short)word(u,stack+4); }
    else if(address==0x307b6) { ++t->names; ax=t->name_result; }
    else if(address==0x347da) { ++t->questions; ax=t->answer; }
    else ++t->frees;
    ip=word(u,stack); cs=word(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_CS,&cs));
    check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    unsigned char bytes[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t size=fread(bytes,1,sizeof bytes,f); fclose(f);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,bytes,size)); struct Trace trace;
    const unsigned addresses[]={0x2d28a,0x2d4db,0x307b6,0x347da,0x139fd};
    for(unsigned i=0;i<5;++i) { uc_hook h; check(uc_hook_add(u,&h,UC_HOOK_CODE,boundary,&trace,addresses[i],addresses[i])); }
    const short choices[]={-32768,-1,0,1,99,4094,4095,4096,4195,7999,8000,8191,8192,8291,12199,12200,12288,32767};
    const short counts[]={0,1,100}; unsigned cases=0;
    for(unsigned r=0;r<sizeof choices/sizeof choices[0];++r)
    for(unsigned l=0;l<3;++l) for(unsigned s=0;s<3;++s)
    for(unsigned answer=0;answer<4;++answer) {
        memset(&trace,0,sizeof trace); trace.load=trace.remove=-99;
        trace.name_result=(unsigned char)(answer&1); trace.answer=(unsigned char)(answer&2?0x15:1);
        put(u,0x8effa,counts[l]); put(u,0x8eff6,(unsigned short)choices[r]);
        put(u,0x3cbf0+0x90,counts[s]);
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,bp=0xf000,sp=0xeb00,ip,ax=(uint16_t)choices[r];
        check(uc_reg_write(u,UC_X86_REG_AX,&ax));
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_BP,&bp)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x2d7b5,0x2d88f,0,10000)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        struct SlicksTrackListChoice c=slicks_track_list_choice(choices[r],counts[l],counts[s]);
        unsigned write=c.action==SLICKS_TRACK_LIST_NAME?slicks_track_list_name_accepted(trace.name_result):
            c.action==SLICKS_TRACK_LIST_CONFIRM_DELETE?slicks_track_list_delete_accepted(trace.answer):0;
        if(ip!=0x2d88f-0x266c0 || trace.names!=(c.action==SLICKS_TRACK_LIST_NAME) ||
           trace.questions!=(c.action==SLICKS_TRACK_LIST_CONFIRM_DELETE) || trace.loads!=(c.action==SLICKS_TRACK_LIST_LOAD) ||
           trace.writes!=write || trace.reloads!=write || trace.frees!=write ||
           (trace.loads && trace.load!=c.index) || (write && trace.remove!=c.index)) {
            fprintf(stderr,"Lists dispatch mismatch result=%d lists=%d selected=%d answer=%u\n",choices[r],counts[l],counts[s],answer); return 1;
        }
        ++cases;
    }
    check(uc_close(u)); printf("Original Lists caller: %u action/index/name/confirmation/write/reload comparisons pass\n",cases);
    return 0;
}
