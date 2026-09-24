#define main track_info_verifier_main
#include "verify_track_info.c"
#undef main
#include "../src/ui/race_menu.h"
static enum SlicksRaceMenuAction called;
static void menu_boundary(uc_engine *u,uint64_t a,uint32_t size,void *context)
{
    (void)size; (void)context;
    uint16_t ss,sp,cs,ip;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    if(a==0x327dc) {
        REQUIRE(called==SLICKS_RACE_MENU_NONE && get(u,stack+4)==0x7fa && get(u,stack+6)==0x3cbf);
        called=SLICKS_RACE_MENU_HELP;
    } else if(a==0x2d8ac) {
        REQUIRE(called==SLICKS_RACE_MENU_NONE && get(u,stack+4)==45 && get(u,stack+6)==65);
        called=SLICKS_RACE_MENU_CONTROLLERS;
    } else if(a==0x1e00a) {
        REQUIRE(called==SLICKS_RACE_MENU_NONE); called=SLICKS_RACE_MENU_SPEED;
    } else if(a==0x3aaf2) REQUIRE(called==SLICKS_RACE_MENU_SPEED);
    else REQUIRE(0);
    ip=get(u,stack); cs=get(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); REQUIRE(f);
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f); REQUIRE(n && n<sizeof runtime);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n));
    const unsigned points[]={0x327dc,0x2d8ac,0x1e00a,0x3aaf2};
    for(unsigned i=0;i<4;++i) {
        uc_hook hook; check(uc_hook_add(u,&hook,UC_HOOK_CODE,menu_boundary,0,points[i],points[i]));
    }
    const unsigned char rows[]={0,1,2,3,4,5,6,127,128,255};
    const unsigned char counts[]={0,1,4,6,127,128,255};
    unsigned cases=0;
    for(unsigned r=0;r<sizeof rows;++r) for(unsigned c=0;c<sizeof counts;++c)
    for(unsigned key=0;key<256;++key) {
        struct SlicksRaceMenu menu={rows[r],counts[c],0,0};
        word(u,0x8f006,menu.row); word(u,0x8f008,menu.count);
        word(u,0x8eff8,0);
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xe000,bp=0xf000,ax=key,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp)); check(uc_reg_write(u,UC_X86_REG_AX,&ax));
        called=SLICKS_RACE_MENU_NONE;
        check(uc_emu_start(u,0x1e664,0x1e718,0,10000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        enum SlicksRaceMenuAction action=slicks_race_menu_key(&menu,(unsigned char)key);
        unsigned char redraw,result;
        check(uc_mem_read(u,0x8eff9,&redraw,1)); check(uc_mem_read(u,0x8eff8,&result,1));
        if(ip!=0x1e718-0x19870 || sp!=0xe000 || get(u,0x8f006)!=menu.row ||
           redraw!=(unsigned char)menu.redraw || result!=(unsigned char)menu.result || called!=action) {
            fprintf(stderr,"Race menu mismatch row=%u count=%u key=%u DOS=%u/%u/%u/%u native=%u/%u/%u/%u\n",
                rows[r],counts[c],key,get(u,0x8f006),redraw,result,called,menu.row,(unsigned char)menu.redraw,(unsigned char)menu.result,action);
            return 1;
        }
        ++cases;
    }
    check(uc_close(u)); printf("Original in-race menu: %u key/navigation/result/modal comparisons pass\n",cases); return 0;
}
