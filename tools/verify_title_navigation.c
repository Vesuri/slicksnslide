#define main options_verifier_main
#include "verify_options_menu.c"
#undef main
#include "../src/ui/title_navigation.h"

int main(void)
{
    unsigned char runtime[300000];
    FILE *file=fopen("disasm/runtime.bin","rb"); if(!file) return 2;
    size_t size=fread(runtime,1,sizeof runtime,file); fclose(file);
    if(size<200000 || size==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,size));
    const short edges[]={-32768,-1,0,1,4,5,6,194,195,32767};
    unsigned cases=0;
    for(unsigned key=0;key<256;++key) for(short row=0;row<7;++row)
    for(unsigned pattern=0;pattern<10;++pattern) {
        short selected=row,count=edges[pattern],mode=edges[(pattern+3)%10];
        short total=edges[(pattern+5)%10]; unsigned char refresh=71;
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xf000;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        word(u,0x8f000,0); word(u,0x8f002,0x9000);
        word(u,0x8f004,key); word(u,0x8f006,0); word(u,0x8f008,0x7000);
        word(u,0x70000,row); word(u,0x3cbf0+0x90,(unsigned short)count);
        word(u,0x3cbf0+0x92,(unsigned short)mode); word(u,0x3cbf0+0x4da8,(unsigned short)total);
        check(uc_mem_write(u,0x3cbf0+0x1146,&refresh,1));
        check(uc_emu_start(u,0x2a0cb,0x90000,0,300));
        uint16_t ip; check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        slicks_title_navigation(&selected,&count,total,&mode,&refresh,key);
        unsigned char actual; check(uc_mem_read(u,0x3cbf0+0x1146,&actual,1));
        if(ip || readword(u,0x70000)!=(unsigned short)selected ||
           readword(u,0x3cbf0+0x90)!=(unsigned short)count ||
           readword(u,0x3cbf0+0x92)!=(unsigned short)mode || actual!=refresh) {
            fprintf(stderr,"Title navigation mismatch key=%u row=%d pattern=%u\n",key,row,pattern); return 1;
        }
        ++cases;
    }
    uc_close(u);
    printf("Original title navigation: %u input/selection/count/mode/refresh comparisons pass\n",cases);
    return 0;
}
