#define main options_verifier_main
#include "verify_options_menu.c"
#undef main
#include "../src/ui/help_index.h"
#include "../src/ui/help_text.h"
#include "host_archive.h"
static unsigned char input[65536];
static unsigned input_size,position,allocation;
static void help_io(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size; (void)context;
    uint16_t ss,sp,cs,ip,ax=0,dx=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    if(address==0x373a7) dx=0x9000;
    else if(address==0x12d8a) ax=position<input_size?input[position++]:0xffff;
    else if(address==0x1221d) { ax=position; dx=position>>16; }
    else if(address==0x1219b) {
        position=readword(u,stack+8)|(readword(u,stack+10)<<16);
        if(position>input_size || readword(u,stack+12)) abort();
    }
    else if(address==0x11b53) {
        unsigned dest=readword(u,stack+4)+16*readword(u,stack+6),limit=readword(u,stack+8),n=0;
        while(position<input_size && n+1<limit) {
            unsigned char c=input[position++]; check(uc_mem_write(u,dest+n++,&c,1)); if(c=='\n') break;
        }
        unsigned char zero=0; check(uc_mem_write(u,dest+n,&zero,1));
        if(n) { ax=readword(u,stack+4); dx=readword(u,stack+6); }
    } else if(address==0x13b07) { dx=allocation?0x6000+allocation*0x20:0x5000; ++allocation; }
    else if(address==0x13c6e) dx=0x5000;
    else if(address==0x36243) { fputs("Original help builder error\n",stderr); exit(1); }
    ip=readword(u,stack); cs=readword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void run_text(uc_engine *u,unsigned address)
{
    uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    word(u,0x8f000,0); word(u,0x8f002,0x9000);
    check(uc_emu_start(u,address,0x90000,0,10000000)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
    if(ip) abort();
}
static unsigned chapters_checked,lines_checked;
static void compare_chapter(uc_engine *u,unsigned offset)
{
    unsigned char native[32768],original[32768]; unsigned length;
    memset(native,0xa5,sizeof native); check(uc_mem_write(u,0x70000,native,sizeof native));
    word(u,0x3cbf0+0x6c1c,0); word(u,0x3cbf0+0x6c1e,0x7000);
    word(u,0x3cbf0+0x6fec,32767);
    word(u,0x8f004,0); word(u,0x8f006,0x9000); word(u,0x8f008,offset); word(u,0x8f00a,offset>>16);
    run_text(u,0x32048);
    if(slicks_help_load_chapter(input,input_size,offset,native,sizeof native,&length)) abort();
    check(uc_mem_read(u,0x70000,original,sizeof original));
    if(memcmp(original,native,sizeof native) || strlen((const char *)original)!=length) {
        fprintf(stderr,"Help chapter mismatch at offset %u\n",offset); exit(1);
    }
    memset(native,0xa5,sizeof native);
    if(slicks_help_load_chapter(input,input_size,offset,native,1,&length)!=-1) abort();
    for(unsigned i=0;i<sizeof native;++i) if(native[i]!=0xa5) abort();
    ++chapters_checked;
}
static void compare_line(uc_engine *u,const unsigned char *line,unsigned length)
{
    unsigned char native[512],original[512];
    if(length>=sizeof native) abort();
    memset(native,0xa5,sizeof native); memcpy(native,line,length); native[length]=0;
    check(uc_mem_write(u,0x70000,native,sizeof native)); word(u,0x3cbf0+0x6fec,1000);
    word(u,0x8f004,0); word(u,0x8f006,0x7000); run_text(u,0x31923);
    if(slicks_help_preprocess(native,sizeof native)) abort();
    check(uc_mem_read(u,0x70000,original,sizeof original));
    if(memcmp(original,native,sizeof native)) { fputs("Help preprocessing mismatch\n",stderr); exit(1); }
    ++lines_checked;
}
static void compare(uc_engine *u,unsigned case_number)
{
    position=allocation=0;
    word(u,0x3cbf0+0x6f94,0); word(u,0x3cbf0+0x6f96,0); word(u,0x3cbf0+0x6f98,0);
    word(u,0x3cbf0+0x6fec,1000);
    uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    word(u,0x8f000,0); word(u,0x8f002,0x9000); word(u,0x8f004,0); word(u,0x8f006,0x9000);
    check(uc_emu_start(u,0x32464,0x90000,0,10000000)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
    if(ip) abort();
    unsigned char native[32768],original[32768],headers;
    struct SlicksHelpIndexInfo info;
    if(slicks_help_build_index(input,input_size,native,sizeof native,&info)) abort();
    unsigned count=readword(u,0x3cbf0+0x6f96);
    check(uc_mem_read(u,0x50000,original,count)); check(uc_mem_read(u,0x3cbf0+0x6fea,&headers,1));
    unsigned body=readword(u,0x3cbf0+0x6fee)|(readword(u,0x3cbf0+0x6ff0)<<16);
    if(info.size!=count || memcmp(native,original,count) || info.records!=readword(u,0x3cbf0+0x6f94) ||
       info.chapter_capacity!=readword(u,0x3cbf0+0x6fec) || info.body!=body || info.headers!=headers) {
        fprintf(stderr,"Help builder case %u: size %u/%u records %u/%u capacity %lu/%u body %lu/%u headers %u/%u\n",
            case_number,info.size,count,info.records,readword(u,0x3cbf0+0x6f94),info.chapter_capacity,
            readword(u,0x3cbf0+0x6fec),info.body,body,info.headers,headers); exit(1);
    }
    for(unsigned i=0;i<info.headers;++i)
        compare_line(u,input+info.header_start[i],info.header_length[i]);
    for(unsigned at=0;at<count;) {
        unsigned kind=original[at++];
        if(kind) {
            unsigned offset=original[at]|original[at+1]<<8|original[at+2]<<16;
            at+=3; if(kind==1) compare_chapter(u,offset);
        } else { while(original[at]) ++at; ++at; }
    }
    for(unsigned cap=0;cap<count;++cap) {
        memset(native,0xa5,sizeof native);
        if(slicks_help_build_index(input,input_size,native,cap,&info)!=-1) abort();
        for(unsigned i=0;i<sizeof native;++i) if(native[i]!=0xa5) abort();
    }
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n));
    unsigned addresses[]={0x373a7,0x12d8a,0x1221d,0x1219b,0x11b53,0x13b07,0x13c6e,0x119c2,0x36243};
    for(unsigned i=0;i<sizeof addresses/sizeof addresses[0];++i) {
        uc_hook h; check(uc_hook_add(u,&h,UC_HOOK_CODE,help_io,0,addresses[i],addresses[i]));
    }
    long loaded=host_archive_load("ref/SLICKS.000","HELP.TXT",input,sizeof input); if(loaded<0) return 2;
    input_size=(unsigned)loaded; compare(u,0);
    const char *cases[]={"\r\n<#main>body<end>\r\n<#next><>page\fend\032ignored",
        "!header\r\n\r\n<#abcdefghijklmnopqrstuvw><#under_score><#a1><!stop",
        "\n<#main><E>\n<#next><\ftext<", "\n<#unfinished", "\n<e", "\n", ""};
    unsigned count=1;
    for(unsigned i=0;i<sizeof cases/sizeof cases[0];++i) {
        input_size=(unsigned)strlen(cases[i]); memcpy(input,cases[i],input_size); compare(u,count++);
    }
    for(unsigned headers=19;headers<=21;++headers) {
        input_size=0;
        for(unsigned i=0;i<headers;++i) { memcpy(input+input_size,"!header\n",8); input_size+=8; }
        memcpy(input+input_size,"\n<#main>text",12); input_size+=12; compare(u,count++);
    }
    const char *lines[]={"", "text\r\n", "<>text\r\n", "<<", "<<<>", "<>", "<", "a\nb\nc\n", "<#main><a next>test</a>\r\n"};
    for(unsigned i=0;i<sizeof lines/sizeof lines[0];++i)
        compare_line(u,(const unsigned char *)lines[i],(unsigned)strlen(lines[i]));
    unsigned char unterminated[16]; memset(unterminated,'<',sizeof unterminated);
    if(slicks_help_preprocess(unterminated,sizeof unterminated)!=-1) abort();
    for(unsigned i=0;i<sizeof unterminated;++i) if(unterminated[i]!='<') abort();
    check(uc_close(u)); printf("Original help: %u byte-exact indexes, %u chapters, %u preprocessed lines; undersized buffers rejected atomically\n",count,chapters_checked,lines_checked);
    return 0;
}
