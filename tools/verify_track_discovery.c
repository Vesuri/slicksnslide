#define main options_verifier_main
#include "verify_options_menu.c"
#undef main
#include "../src/game/track_catalogue.h"

struct Discovery { unsigned count,next,allocations; const char *const *names; };
struct StartupDiscovery { unsigned counts[2],calls,reports,exits; char patterns[2][64]; };
static unsigned farptr(uc_engine *u,unsigned p)
{ return readword(u,p)+16*readword(u,p+2); }
static void endpoint(uc_engine *u,unsigned expected)
{
    uint16_t cs,ip;check(uc_reg_read(u,UC_X86_REG_CS,&cs));
    check(uc_reg_read(u,UC_X86_REG_IP,&ip));if(16U*cs+ip!=expected)abort();
}
/* Execute the original startup caller and its path/string helpers. Only
 * discovery's result, diagnostic output and process exit are boundaries. */
static void startup_boundary(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size;struct StartupDiscovery *s=context;
    if(address==0x10d3f) { ++s->exits;check(uc_emu_stop(u));return; }
    uint16_t ss,sp,ax=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss));check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=16U*ss+sp;
    if(address==0x2bfc8) {
        if(s->calls>=2)abort();
        check(uc_mem_read(u,farptr(u,stack+4),s->patterns[s->calls],64));
        if(!memchr(s->patterns[s->calls],0,64))abort();
        ax=(uint16_t)s->counts[s->calls++];
    } else if(address==0x36243)++s->reports;
    else abort();
    uint16_t ip=readword(u,stack),cs=readword(u,stack+2);sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_IP,&ip));
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
            if(d->names) snprintf(name,sizeof name,"%s",d->names[d->next++]);
            else snprintf(name,sizeof name,"T%07u.SS",d->count-d->next++);
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
    const char *const mixed[]={"z.SS","A0.SS","A!.SS","A.SS","AA.SS","a.SS",
        "a!.ss","Z.ss","EIGHT888.SS","DUP.SS","DUP.SS","_ONE.SS"};
    const char *const sorted[]={"A","A!","A0","AA","DUP","DUP","EIGHT888","Z","_ONE","a","a!","z"};
    const unsigned counts[]={0,1,195,256,257,300,sizeof mixed/sizeof mixed[0]};
    for(unsigned c=0;c<sizeof counts/sizeof counts[0];++c) {
        d=(struct Discovery){.count=counts[c]};regs(u,0);uint16_t cs=0x2e0f,ax;
        if(c==sizeof counts/sizeof counts[0]-1)d.names=mixed;
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
            check(uc_mem_read(u,0x60000+9*i,actual,9));
            if(d.names) {
                unsigned j=0;
                while(d.names[i][j] && d.names[i][j]!='.') { expected[j]=d.names[i][j];++j; }
                expected[j]=0;
                if(!memchr(actual,0,sizeof actual) || strcmp(actual,expected))abort();
            } else if(memcmp(actual,expected,9))abort();
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
            if(d.names) {
                if(!memchr(actual,0,sizeof actual) || strcmp(actual,sorted[i]))abort();
                continue;
            }
            if(memcmp(actual,expected,9)) { fprintf(stderr,"Sort mismatch at %u: %.9s\n",i,actual);abort(); }
        }
        printf("Original startup sorter: %u stems in ascending order\n",d.count);
        if(d.names) {
            const char *native[sizeof mixed/sizeof mixed[0]];
            memcpy(native,mixed,sizeof native);
            for(unsigned left=0;left+1<d.count;++left)
                for(unsigned right=left+1;right<d.count;++right)
                    if(slicks_track_stem_compare(native[right],native[left])<0) {
                        const char *swap=native[left];native[left]=native[right];native[right]=swap;
                    }
            for(unsigned i=0;i<d.count;++i) {
                char actual[9];check(uc_mem_read(u,0x60000+9*i,actual,9));
                if(slicks_track_stem_compare(native[i],actual))abort();
            }
            puts("Native stem comparator matches original: prefixes, punctuation, case, duplicate stems and eight-byte names");
        }
    }
    struct StartupDiscovery startup={0};
    const unsigned startup_hooks[]={0x2bfc8,0x36243,0x10d3f};
    for(unsigned i=0;i<3;++i) {
        uc_hook h;check(uc_hook_add(u,&h,UC_HOOK_CODE,startup_boundary,&startup,startup_hooks[i],startup_hooks[i]));
    }
    for(unsigned first=0;first<2;++first)for(unsigned fallback=0;fallback<2;++fallback) {
        startup=(struct StartupDiscovery){.counts={first,fallback}};
        regs(u,0);uint16_t cs=0x1987;check(uc_reg_write(u,UC_X86_REG_CS,&cs));
        check(uc_mem_write(u,0x3cbf0+0x5e6,"TRACKS\\",8));
        check(uc_emu_start(u,0x25db6,0x25e4a,0,100000));
        if(startup.calls!=(first?1U:2U) || strcmp(startup.patterns[0],"TRACKS\\*.SS"))abort();
        if(!first && strcmp(startup.patterns[1],".\\*.SS"))abort();
        if(startup.reports!=(!first&&!fallback) || startup.exits!=(!first&&!fallback))abort();
        endpoint(u,!first&&!fallback?0x10d3f:0x25e4a);
        if(readword(u,0x3cbf0+0x4da8)!=(first?first:fallback))abort();
        printf("Original startup first=%u fallback=%u: searches=%u reports=%u exits=%u\n",
            first,fallback,startup.calls,startup.reports,startup.exits);
    }
    check(uc_close(u));return 0;
}
