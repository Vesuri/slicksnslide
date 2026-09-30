#define main options_verifier_main
#include "verify_options_menu.c"
#undef main

struct Discovery { unsigned count,next,allocations; };
static unsigned farptr(uc_engine *u,unsigned p)
{ return readword(u,p)+16*readword(u,p+2); }
static void endpoint(uc_engine *u,unsigned expected)
{
    uint16_t cs,ip;check(uc_reg_read(u,UC_X86_REG_CS,&cs));
    check(uc_reg_read(u,UC_X86_REG_IP,&ip));if(16U*cs+ip!=expected)abort();
}
/* Substitute only DOS directory enumeration and allocation. The original
 * filename normalization, append loop, count limit and output order execute. */
static void boundary(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size;struct Discovery *d=context;uint16_t ss,sp,ax=0,cs,ip;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss));check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=16*ss+sp;
    if(address==0x35e7c) {
        unsigned out=farptr(u,stack+4);
        word(u,out,0);word(u,out+2,0x6000);++d->allocations;
    } else {
        if(address==0x12c46)d->next=0;
        unsigned out=farptr(u,stack+(address==0x12c46?8:4));
        if(d->next>=d->count) ax=1;
        else {
            char name[13]={0};
            /* Deliberately descending enumeration order, not alphabetical. */
            snprintf(name,sizeof name,"T%07u.SS",d->count-d->next++);
            check(uc_mem_write(u,out+30,name,sizeof name));
        }
    }
    ip=readword(u,stack);cs=readword(u,stack+2);sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax));check(uc_reg_write(u,UC_X86_REG_CS,&cs));
    check(uc_reg_write(u,UC_X86_REG_IP,&ip));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    unsigned char runtime[300000];FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    size_t n=fread(runtime,1,sizeof runtime,f);fclose(f);if(n<200000||n==sizeof runtime)return 2;
    uc_engine *u;check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));check(uc_mem_write(u,0x10100,runtime,n));
    struct Discovery d={0};uc_hook hooks[3];const unsigned addresses[]={0x12c46,0x12c7d,0x35e7c};
    for(unsigned i=0;i<3;++i)check(uc_hook_add(u,&hooks[i],UC_HOOK_CODE,boundary,&d,addresses[i],addresses[i]));
    const unsigned counts[]={0,1,195,256,257,300};
    for(unsigned c=0;c<sizeof counts/sizeof counts[0];++c) {
        d=(struct Discovery){.count=counts[c]};regs(u,0);uint16_t cs=0x2e0f,ax;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));
        word(u,0x8ef04,0);word(u,0x8ef06,0x7000);
        word(u,0x8ef08,0);word(u,0x8ef0a,0x7100);
        word(u,0x8ef0c,1);word(u,0x8ef0e,10000);
        check(uc_mem_write(u,0x70000,"*.SS",5));
        check(uc_emu_start(u,0x35d28,0x35e62,0,1000000));
        endpoint(u,0x35e62);
        check(uc_reg_read(u,UC_X86_REG_AX,&ax));
        if(ax!=d.count || farptr(u,0x71000)!=(d.count?0x60000:0))abort();
        for(unsigned i=0;i<d.count;++i) {
            char expected[13]={0},actual[9];snprintf(expected,sizeof expected,"T%07u",d.count-i);
            check(uc_mem_read(u,0x60000+9*i,actual,9));if(memcmp(actual,expected,9))abort();
        }
        printf("Original discovery count=%u: enumeration order retained, 9-byte stems\n",d.count);
        /* Startup subsequently calls the real catalogue sorter at 2612a. */
        word(u,0x3cbf0+0x4da4,0);word(u,0x3cbf0+0x4da6,0x6000);
        word(u,0x3cbf0+0x4da8,d.count);regs(u,0);
        check(uc_emu_start(u,0x2c301,0x2c41b,0,100000000));
        endpoint(u,0x2c41b);
        for(unsigned i=0;i<d.count;++i) {
            char expected[13]={0},actual[9];snprintf(expected,sizeof expected,"T%07u",i+1);
            check(uc_mem_read(u,0x60000+9*i,actual,9));
            if(memcmp(actual,expected,9)) { fprintf(stderr,"Sort mismatch at %u: %.9s\n",i,actual);abort(); }
        }
        printf("Original startup sorter: %u stems in ascending order\n",d.count);
    }
    check(uc_close(u));return 0;
}
