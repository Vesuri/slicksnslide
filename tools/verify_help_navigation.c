#define main options_verifier_main
#include "verify_options_menu.c"
#undef main
#include "../src/ui/help_navigation.h"
static const unsigned char topics[]={1,0x34,0x12,0,0,'M','A','I','N',0,
    2,0x78,0x12,0,0,'N','E','X','T',0,0,'P','R','E','V',0,
    1,0x56,0x34,0,0,'F','I','_','N','E','X','T',0,1,0x78,0x56,0};
static void longword(uc_engine *u,unsigned at,unsigned long value)
{ word(u,at,value); word(u,at+2,value>>16); }
static unsigned long readlong(uc_engine *u,unsigned at)
{ return readword(u,at)|((unsigned long)readword(u,at+2)<<16); }
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n)); check(uc_mem_write(u,0x60000,topics,sizeof topics));
    unsigned dsbase=0x3cbf0,bpbase=0x8f000;
    word(u,dsbase+0x6f90,0); word(u,dsbase+0x6f92,0x6000); word(u,dsbase+0x6f96,sizeof topics);
    longword(u,dsbase+0x6fee,0x9876);
    const unsigned char keys[][2]={{27,0},{0,0x44},{0,0x3b},{0,0x49},{0,0x51},{0,0x48},
        {'K',0},{0,0x50},{0,0x4d},{9,0},{8,0},{'b',0},{13,0},{32,0},{0,0x4b},{'B',0},{'x',0}};
    const char *targets[]={"next","missing","$save","inline"}; unsigned cases=0;
    for(unsigned k=0;k<sizeof keys/sizeof keys[0];++k)
    for(unsigned variant=0;variant<12;++variant) for(unsigned target=0;target<4;++target) {
        struct SlicksHelpNavigation nav; slicks_help_navigation_init(&nav);
        struct SlicksHelpStyle style={0}; nav.chapter=0x112233; nav.page=variant%3;
        nav.next_page=(short)(variant%4); nav.redraw=0;
        style.total_links=(short)(variant%4); style.selected=(short)(variant%3);
        style.link_type=target==3?0:-73;
        strcpy((char *)style.target,targets[target]); strcpy((char *)style.next,"next"); strcpy((char *)style.previous,"prev");
        if(variant&1) strcpy((char *)style.prefix,"FI_");
        if(variant>=6) for(unsigned i=0;i<101;++i) {
            nav.chapters[i]=0x556677+i; nav.pages[i]=(signed char)(i+1); nav.selections[i]=(signed char)(i%5);
        }
        longword(u,bpbase-0x26,nav.chapter); word(u,bpbase-0x28,nav.page);
        word(u,dsbase+0x6ff2,nav.next_page); word(u,dsbase+0x6ff4,style.total_links); word(u,dsbase+0x6f8c,style.selected);
        unsigned char control[4]={keys[k][1],keys[k][0],0,0}; check(uc_mem_write(u,bpbase-0x38,control,4));
        unsigned char redraw=0; check(uc_mem_write(u,bpbase-0x31,&redraw,1));
        check(uc_mem_write(u,dsbase+0x6f8b,&style.link_type,1)); check(uc_mem_write(u,dsbase+0x6f63,style.target,21));
        check(uc_mem_write(u,dsbase+0x6f27,style.next,21)); check(uc_mem_write(u,dsbase+0x6f45,style.previous,21));
        check(uc_mem_write(u,dsbase+0x16a8,style.prefix,6));
        word(u,bpbase-0xc,0); word(u,bpbase-0xa,0x5000);
        word(u,bpbase-8,0); word(u,bpbase-6,0x5100); word(u,bpbase-4,0); word(u,bpbase-2,0x5200);
        for(unsigned i=0;i<101;++i) longword(u,0x50000+4*i,nav.chapters[i]);
        check(uc_mem_write(u,0x51000,nav.pages,101)); check(uc_mem_write(u,0x52000,nav.selections,101));
        uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_emu_start(u,0x32b81,0x32e20,0,1000000)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        if(ip!=0x4d30) abort();
        if(slicks_help_navigation_key(&nav,&style,topics,sizeof topics,0x9876,keys[k][0],keys[k][1])) abort();
        unsigned char actual_redraw,done; check(uc_mem_read(u,bpbase-0x31,&actual_redraw,1)); check(uc_mem_read(u,bpbase-0x36,&done,1));
        if(nav.chapter!=readlong(u,bpbase-0x26) || (unsigned short)nav.page!=readword(u,bpbase-0x28) ||
           (unsigned short)style.selected!=readword(u,dsbase+0x6f8c) || (unsigned char)nav.redraw!=actual_redraw || nav.done!=done) {
            fprintf(stderr,"Help navigation mismatch key=%u variant=%u target=%u chapter=%lx/%lx page=%d/%d selection=%d/%d\n",k,variant,target,
                nav.chapter,readlong(u,bpbase-0x26),nav.page,(short)readword(u,bpbase-0x28),style.selected,(short)readword(u,dsbase+0x6f8c)); return 1;
        }
        signed char pages[101],selections[101]; check(uc_mem_read(u,0x51000,pages,101)); check(uc_mem_read(u,0x52000,selections,101));
        if(memcmp(pages,nav.pages,101) || memcmp(selections,nav.selections,101)) abort();
        for(unsigned i=0;i<101;++i) if(nav.chapters[i]!=readlong(u,0x50000+4*i)) abort();
        ++cases;
    }
    check(uc_close(u)); printf("Original help navigation: %u key/page/link/history/fallback comparisons pass\n",cases); return 0;
}
