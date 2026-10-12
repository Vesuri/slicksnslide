/* Execute the original pre-race branch; intercept only graphics, delay and
 * sound-driver boundaries. Compare ordered requests, waits and final tuning. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/audio_start.h"
static void check(uc_err e) { if(e) {fprintf(stderr,"%s\n",uc_strerror(e));exit(1);} }
static unsigned word(uc_engine *u,unsigned at)
{ unsigned char b[2];check(uc_mem_read(u,at,b,2));return b[0]|b[1]<<8; }
struct Event {unsigned time,priority,frequency;};
struct Trace {struct Event events[4];unsigned count,time,waits;};
static struct Trace original,native;
static void play(void *context,unsigned short priority,unsigned short frequency)
{
    struct Trace *t=context;assert(t->count<4);
    t->events[t->count++]=(struct Event){t->time,priority,frequency?frequency:11025};
}
static void delay(void *context,unsigned short milliseconds)
{ struct Trace *t=context;t->time+=milliseconds;++t->waits; }
static void boundary(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size;(void)context;
    if(address!=0x3989b && address!=0x395b8 && address!=0x19878 && address!=0x3aa7c) return;
    uint16_t ss,sp,cs,ip;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss));check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    if(address==0x3989b) {
        assert(word(u,stack+4)==42 && word(u,stack+6)==0);
        play(&original,word(u,stack+8),0);
        uint16_t handle=original.count;check(uc_reg_write(u,UC_X86_REG_AX,&handle));
    } else if(address==0x395b8) {
        unsigned handle=word(u,stack+4);assert(handle && handle<=original.count);
        original.events[handle-1].frequency=word(u,stack+6);
    } else if(address==0x19878) delay(&original,word(u,stack+4));
    ip=word(u,stack);cs=word(u,stack+2);sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    unsigned char runtime[300000];FILE *f=fopen("disasm/runtime.bin","rb");assert(f);
    size_t n=fread(runtime,1,sizeof runtime,f);fclose(f);
    uc_engine *u;check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));check(uc_mem_write(u,0x10100,runtime,n));
    unsigned char handle=42;check(uc_mem_write(u,0x3cbf0+0x4c4b,&handle,1));
    uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,bp=0xf000,sp=0xe000;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_BP,&bp));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    uc_hook hook;check(uc_hook_add(u,&hook,UC_HOOK_CODE,boundary,0,1,0));
    check(uc_emu_start(u,0x25376,0x253f4,0,1000));
    uint16_t ip;check(uc_reg_read(u,UC_X86_REG_IP,&ip));assert(ip==0x253f4-0x19870);
    slicks_audio_start_cues(&native,play,delay);
    assert(original.count==4 && original.waits==2 && original.time==900);
    assert(!memcmp(&native,&original,sizeof native));
    check(uc_close(u));
    puts("Original pre-race audio: four ordered sample-1 requests, 400/500 ms waits, priorities and final 13000 Hz voice match");
    return 0;
}
