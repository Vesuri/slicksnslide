#define main options_verifier_main
#include "verify_options_menu.c"
#undef main
#include "../src/ui/help_index.h"
#include "host_archive.h"
static const unsigned char index_data[]={
    1,0x34,0x12,0,0,'M','A','I','N',0,0,'I','N','D','E','X',0,
    2,0x56,0x12,0,0,'O','P','T','I','O','N','S',0,
    2,0x78,0x12,0,0,'O','P','T','I','O','N','S',0,
    1,0x45,0x23,1,0,'F','I','_','O','P','T','I','O','N','S',0,
    0,'P','L','R','_','M','E','N','U',0,2,0x89,0x23,1,
    0,'F','I','_','P','L','R','_','M','E','N','U',0,
    1,0x67,0x45,2,0,'O','P','T','I','O','N','S',0,1,0xff,0xff,2};
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n)); check(uc_mem_write(u,0x50000,index_data,sizeof index_data));
    unsigned char help[16384],real_index[8192]; struct SlicksHelpIndexInfo info;
    long help_size=host_archive_load("ref/SLICKS.000","HELP.TXT",help,sizeof help);
    if(help_size<0 || slicks_help_build_index(help,help_size,real_index,sizeof real_index,&info)) return 2;
    const char *topics[]={"main","INDEX","Options","plr_menu","missing","","fi_options","reg"};
    const char *prefixes[]={"","FI_","xx_","fi_"}; unsigned cases=0;
    for(unsigned dataset=0;dataset<2;++dataset) {
    const unsigned char *index=dataset?real_index:index_data;
    unsigned index_size=dataset?info.size:sizeof index_data;
    check(uc_mem_write(u,0x50000,index,index_size));
    word(u,0x3cbf0+0x6f90,0); word(u,0x3cbf0+0x6f92,0x5000); word(u,0x3cbf0+0x6f96,index_size);
    for(unsigned p=0;p<4;++p) for(unsigned t=0;t<8;++t) {
        check(uc_mem_write(u,0x3cbf0+0x16a8,prefixes[p],strlen(prefixes[p])+1));
        check(uc_mem_write(u,0x60000,topics[t],strlen(topics[t])+1));
        unsigned char initial[8]={0xef,0xbe,0xad,0xde,0x76,0x98,0x54,0x32};
        check(uc_mem_write(u,0x61000,initial,8));
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,ax;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        word(u,0x8f000,0); word(u,0x8f002,0x9000);
        unsigned args[]={0,0x6000,0,0x6100,4,0x6100,6,0x6100};
        for(unsigned i=0;i<8;++i) word(u,0x8f004+2*i,args[i]);
        check(uc_emu_start(u,0x32631,0x90000,0,100000)); check(uc_reg_read(u,UC_X86_REG_AX,&ax));
        struct SlicksHelpLocation out={0xdeadbeef,0x9876,0x3254};
        int result=slicks_help_find(index,index_size,(const unsigned char *)prefixes[p],(const unsigned char *)topics[t],&out);
        unsigned char actual[8]; check(uc_mem_read(u,0x61000,actual,8));
        unsigned long chapter=actual[0]|(unsigned long)actual[1]<<8|(unsigned long)actual[2]<<16|(unsigned long)actual[3]<<24;
        if(result!=(ax&255) || out.chapter!=chapter || out.page!=(actual[4]|actual[5]<<8) || out.anchor!=(actual[6]|actual[7]<<8)) {
            fprintf(stderr,"Help lookup mismatch prefix=%s topic=%s result=%d/%u location=%lx/%lx\n",prefixes[p],topics[t],result,ax&255,out.chapter,chapter); return 1;
        }
        ++cases;
        if(dataset && !p && (t==5 || t==7)) printf("Real help topic '%s': result=%d chapter=%lu page=%u\n",topics[t],result,chapter,out.page);
    }
    }
    check(uc_close(u)); printf("Original help lookup: %u prefix/fallback/page/duplicate/missing comparisons pass\n",cases); return 0;
}
