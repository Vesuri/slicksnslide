#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/ui/intermission_menu.h"

static unsigned action;
static void check(uc_err e)
{ if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void modal(uc_engine *u,uint64_t address,uint32_t size,void *opaque)
{
    (void)size; (void)opaque;
    if(address!=0x247fb && address!=0x1d975) return;
    action=address==0x247fb?SLICKS_INTERMISSION_CHANGE_CARS:SLICKS_INTERMISSION_SAVE_GAME;
    uint16_t sp,ip=address==0x247fb?0xaee0:0xaef8;
    check(uc_reg_read(u,UC_X86_REG_SP,&sp)); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    check(uc_reg_write(u,UC_X86_REG_IP,&ip));
}
int main(void)
{
    unsigned char runtime[300000];
    FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f);
    if(n<200000 || n==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n));
    uc_hook hook; check(uc_hook_add(u,&hook,UC_HOOK_CODE,modal,NULL,1,0));
    /* Check initialization directly, not by accepting the translated defaults. */
    uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,bp=0xf000;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
    check(uc_emu_start(u,0x24527,0x24537,0,4));
    unsigned char initial[4]; check(uc_mem_read(u,0x8eff2,initial,4));
    struct SlicksIntermissionMenu start; slicks_intermission_init(&start);
    if((signed char)initial[0]!=start.exit_code || (signed char)initial[1]!=start.cars_redraw ||
       (signed char)initial[2]!=start.redraw || (signed char)initial[3]!=start.selected) return 1;
    for(unsigned option=0;option<65536;++option) {
        unsigned char word[2]={(unsigned char)option,(unsigned char)(option>>8)},first;
        check(uc_mem_write(u,0x3cbf0+0x3030,word,2));
        check(uc_emu_start(u,0x245bb,0x245c8,0,4));
        check(uc_mem_read(u,0x8efef,&first,1));
        if(first!=2) { fprintf(stderr,"intermission navigation origin mismatch %u\n",option); return 1; }
    }
    unsigned cases=0;
    for(unsigned key=0;key<256;++key)
    for(int selected=0;selected<4;++selected)
    for(int dirty=-1;dirty<=1;++dirty) {
        struct SlicksIntermissionMenu m={(signed char)selected,(signed char)dirty,0,0};
        unsigned expected=slicks_intermission_key(&m,(unsigned char)key);
        unsigned char locals[32]={0};
        locals[0x0f]=2; locals[0x13]=0; locals[0x14]=(unsigned char)dirty;
        locals[0x15]=(unsigned char)selected;
        check(uc_mem_write(u,0x8efe0,locals,sizeof locals));
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xe000,bp=0xf000;
        uint16_t ax=(uint16_t)(int16_t)(int8_t)key,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp)); check(uc_reg_write(u,UC_X86_REG_AX,&ax));
        action=0;
        check(uc_emu_start(u,0x2470e,0x2479f,0,200));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        check(uc_mem_read(u,0x8efe0,locals,sizeof locals));
        if(ip!=0xaf2f || action!=expected || (signed char)locals[0x15]!=m.selected ||
           (signed char)locals[0x14]!=m.redraw || (signed char)locals[0x13]!=m.cars_redraw ||
           (signed char)locals[0x12]!=m.exit_code) {
            fprintf(stderr,"intermission mismatch key=%u selected=%d dirty=%d ip=%x action=%u/%u\n",
                key,selected,dirty,ip,action,expected); return 1;
        }
        ++cases;
    }
    check(uc_close(u));
    printf("Original intermission dispatcher: initialization, 65536 Change Cars words, %u key/selection/redraw cases pass\n",cases);
    return 0;
}
