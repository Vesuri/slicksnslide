#include <unicorn/unicorn.h>
#include <unicorn/m68k.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t source,destination,length;
static int copying;
static void fail(const char *s) { fprintf(stderr,"memory verifier: %s\n",s);exit(1); }
static void ck(uc_err e) { if(e)fail(uc_strerror(e)); }
static void be32(unsigned char *p,uint32_t v) { p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v; }
static int inside(uint64_t at,int size,uint32_t base,uint32_t bytes)
{ return at>=base && at+(unsigned)size<=(uint64_t)base+bytes; }
static void access_memory(uc_engine *u,uc_mem_type type,uint64_t at,int size,int64_t value,void *data)
{
    (void)u;(void)value;(void)data;
    if(type==UC_MEM_WRITE) {
        if(!inside(at,size,destination,length))fail("write outside destination");
    } else if(!inside(at,size,0xf0000,16) &&
              !(copying && inside(at,size,source,length))) fail("read outside source/arguments");
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    unsigned char code[4096];FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    size_t bytes=fread(code,1,sizeof code,f);fclose(f);if(bytes<4)return 2;
    uint32_t set_entry=0x10000+((uint32_t)code[bytes-4]<<24)+((uint32_t)code[bytes-3]<<16)+
        ((uint32_t)code[bytes-2]<<8)+code[bytes-1];
    uc_engine *u;ck(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    ck(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));
    ck(uc_mem_map(u,0,0x100000,UC_PROT_ALL));ck(uc_mem_write(u,0x10000,code,bytes));
    uc_hook hook;ck(uc_hook_add(u,&hook,UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,access_memory,0,1,0));
    const int preserved[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,
        UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,UC_M68K_REG_A3,UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6};
    static unsigned char original[131104],expected[131104],actual[131104];
    for(unsigned i=0;i<sizeof original;++i)original[i]=(unsigned char)(i*37+(i>>7));
    ck(uc_mem_write(u,0x20000,original,sizeof original));
    unsigned cases=0;
    for(unsigned n=0;n<519;++n)
    for(unsigned src_align=0;src_align<4;++src_align)
    for(unsigned dst_align=0;dst_align<4;++dst_align)
    for(unsigned operation=0;operation<3;++operation) {
        static const unsigned big[]={1023,4096,65535,65536,65537,131071};
        length=n<=512?n:big[n-513];source=0x20000+src_align;
        destination=0x60010+dst_align;copying=operation==0;
        unsigned value=operation==1?0:0xabcdef00U+(n*17U+src_align*31U+dst_align);
        unsigned count=length+32;
        memset(expected,0xa5,count);ck(uc_mem_write(u,0x60000,expected,count));
        if(copying)memcpy(expected+16+dst_align,original+src_align,length);
        else memset(expected+16+dst_align,(int)value,length);
        unsigned char stack[16];be32(stack,0x18000);be32(stack+4,destination);
        be32(stack+8,copying?source:value);be32(stack+12,length);
        ck(uc_mem_write(u,0xf0000,stack,sizeof stack));uint32_t sp=0xf0000;
        ck(uc_reg_write(u,UC_M68K_REG_A7,&sp));
        for(unsigned i=0;i<sizeof preserved/sizeof *preserved;++i) {
            uint32_t v=0x12345600+i;ck(uc_reg_write(u,preserved[i],&v));
        }
        ck(uc_emu_start(u,copying?0x10000:set_entry,0x18000,0,1000000));
        uint32_t result,pc;ck(uc_reg_read(u,UC_M68K_REG_D0,&result));
        ck(uc_reg_read(u,UC_M68K_REG_PC,&pc));ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));
        if(result!=destination || pc!=0x18000 || sp!=0xf0004)fail("return/stack/termination");
        ck(uc_mem_read(u,0x60000,actual,count));if(memcmp(actual,expected,count))fail("bytes/canaries");
        for(unsigned i=0;i<sizeof preserved/sizeof *preserved;++i) {
            ck(uc_reg_read(u,preserved[i],&result));if(result!=0x12345600+i)fail("callee-saved register");
        }
        ++cases;
    }
    uc_close(u);printf("Native memory: %u copy/clear/fill lengths, alignments, exact read/write bounds, bytes and ABI cases pass\n",cases);
    return 0;
}
