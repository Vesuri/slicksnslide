#define main track_info_verifier_main
#include "verify_track_info.c"
#undef main
#include "host_archive.h"
#include "../src/ui/language_table.h"
static const unsigned char *source;
static unsigned source_size,source_at;
static void language_boundary(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size; (void)context;
    uint16_t ss,sp,cs,ip,ax=0,dx=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    if(address==0x373a7) { ax=1; source_at=0; }
    else if(address==0x13b07) { REQUIRE(get(u,stack+4)==1000); dx=0x7000; }
    else if(address==0x11b53) {
        REQUIRE(get(u,stack+8)==199);
        if(source_at<source_size) {
            unsigned start=source_at;
            while(source_at<source_size && source_at-start<198)
                if(source[source_at++]=='\n') break;
            ax=get(u,stack+4); dx=get(u,stack+6);
            unsigned dest=dx*16U+ax; unsigned char zero=0;
            check(uc_mem_write(u,dest,source+start,source_at-start));
            check(uc_mem_write(u,dest+source_at-start,&zero,1));
        }
    } else REQUIRE(address==0x119c2);
    ip=get(u,stack); cs=get(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void run_language(uc_engine *u,unsigned entry,const unsigned *args,unsigned count)
{
    uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
    word(u,0x8f000,0); word(u,0x8f002,0x9000);
    for(unsigned i=0;i<count;++i) word(u,0x8f004+i*2,args[i]);
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    check(uc_emu_start(u,entry,0x90000,0,3000000));
    check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    REQUIRE(ip==0 && sp==0xf004);
}
int main(void)
{
    unsigned char runtime[300000],resource[2000],native[2000],actual[2000];
    FILE *f=fopen("disasm/runtime.bin","rb"); REQUIRE(f);
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f); REQUIRE(n && n<sizeof runtime);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,n));
    const unsigned addresses[]={0x373a7,0x13b07,0x11b53,0x119c2};
    for(unsigned i=0;i<4;++i) { uc_hook h;
        check(uc_hook_add(u,&h,UC_HOOK_CODE,language_boundary,0,addresses[i],addresses[i])); }
    unsigned cases=0;
    for(unsigned language=1;language<=8;++language) {
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xf000,bp;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        word(u,0x8f004,language);
        check(uc_emu_start(u,0x2b70a,0x2b72e,0,10000));
        check(uc_reg_read(u,UC_X86_REG_BP,&bp));
        char filename[11],native_name[10];
        check(uc_mem_read(u,0x80000U+bp-12,filename,sizeof filename));
        REQUIRE(!slicks_language_resource(native_name,language));
        REQUIRE(filename[0]=='/' && !strcmp(filename+1,native_name));
    }
    puts("Original persisted language selector: all eight resource names match");
    static const unsigned char synthetic[]="Title\r\nfoo=first\r\nfoo=second\r\nfoobar=long\r\nempty=\r\nmultiline=one\xaftwo\r\n.\r\nignored=value\r\n";
    const char *keys[]={"back","help","controllers","speed","nexttrack","mainmenu",
        "tracks","menu1","menu7","missing","MAINMENU","main","","foo","foobar","empty","multiline","ignored"};
    for(unsigned variant=0;variant<9;++variant) {
        if(variant<8) {
            char name[16]; snprintf(name,sizeof name,"lang%u.txt",variant+1);
            long size=host_archive_load("ref/SLICKS.000",name,resource,sizeof resource); REQUIRE(size>0);
            source=resource; source_size=(unsigned)size;
        } else { source=synthetic; source_size=sizeof synthetic-1; }
        unsigned used=0;
        REQUIRE(!slicks_language_table_load(source,source_size,native,sizeof native,&used));
        const unsigned args[]={0,0x6000,0,0x6100};
        run_language(u,0x3601a,args,4);
        uint16_t ax,dx; check(uc_reg_read(u,UC_X86_REG_AX,&ax)); REQUIRE(!(ax&255));
        REQUIRE(get(u,0x61000)==0 && get(u,0x61002)==0x7000);
        check(uc_mem_read(u,0x70000,actual,used)); REQUIRE(!memcmp(native,actual,used));
        for(unsigned k=0;k<sizeof keys/sizeof keys[0];++k) {
            check(uc_mem_write(u,0x62000,keys[k],strlen(keys[k])+1));
            const unsigned lookup[]={0,0x7000,0,0x6200};
            run_language(u,0x36182,lookup,4);
            check(uc_reg_read(u,UC_X86_REG_AX,&ax)); check(uc_reg_read(u,UC_X86_REG_DX,&dx));
            const unsigned char *value=slicks_language_lookup(native,used,(const unsigned char *)keys[k],0);
            REQUIRE(value?dx*16U+ax==0x70000+(unsigned)(value-native):!ax && !dx);
            /* 36227 uses the key itself if the table is missing or lookup
             * fails. Execute the real wrapper rather than infer fallback. */
            for(unsigned enabled=0;enabled<2;++enabled) {
                word(u,0x3cbf0+0x1722,0); word(u,0x3cbf0+0x1724,enabled?0x7000:0);
                const unsigned wrapper[]={0,0x6200};
                run_language(u,0x36227,wrapper,2);
                check(uc_reg_read(u,UC_X86_REG_AX,&ax)); check(uc_reg_read(u,UC_X86_REG_DX,&dx));
                const unsigned char *fallback=(const unsigned char *)keys[k];
                const unsigned char *resolved=slicks_language_lookup(enabled?native:0,used,fallback,fallback);
                REQUIRE(dx*16U+ax==(resolved==fallback?0x62000:0x70000+(unsigned)(resolved-native)));
            }
            ++cases;
        }
        unsigned char sentinel[2000]; memset(sentinel,0xa5,sizeof sentinel); unsigned untouched=0x1234;
        REQUIRE(slicks_language_table_load(source,source_size,sentinel,used-1,&untouched)==-1);
        REQUIRE(untouched==0x1234);
        for(unsigned i=0;i<sizeof sentinel;++i) REQUIRE(sentinel[i]==0xa5);
    }
    check(uc_close(u)); printf("Original language loader: 8 archive languages + duplicate/empty/multiline/terminator fixture; %u lookups, %u fallback wrappers and atomic capacity failures pass\n",cases,cases*2);
    return 0;
}
