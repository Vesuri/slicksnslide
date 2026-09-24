#define main options_verifier_main
#include "verify_options_menu.c"
#undef main
#include "../src/ui/controllers_dialog.h"
static void capture_boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{ (void)address;(void)size; *(unsigned *)p=1; check(uc_emu_stop(u)); }
int main(void)
{
    FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    unsigned char runtime[300000]; size_t n=fread(runtime,1,sizeof runtime,f);
    if(ferror(f) || n<200000 || n==sizeof runtime) return 2;
    fclose(f); const unsigned char *ds=runtime+0x3cbf0-0x10100;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,n));
    uc_hook hook; unsigned captured=0;
    check(uc_hook_add(u,&hook,UC_HOOK_CODE,capture_boundary,&captured,0x2dece,0x2dece));
    const unsigned char keys[]={0,1,0x43,0x44,0x48,0x50,0x4b,0x4d,0x1c,0x1d,0x39};
    const unsigned char devices[]={0,1,2,3,126,127,128,255};
    unsigned cases=0;
    for(unsigned row=0;row<5;++row) for(unsigned col=0;col<6;++col)
    for(unsigned k=0;k<sizeof keys;++k) for(unsigned v=0;v<sizeof devices;++v) {
        struct SlicksConfiguration c={0};
        memset(c.keys,0x57,20); memset(c.player_input,devices[v],4);
        check(uc_mem_write(u,0x3cbf0+0x5358,c.keys,20));
        check(uc_mem_write(u,0x3cbf0+0x5e2,c.player_input,4));
        unsigned char locals[48]={0}; locals[48-13]=(unsigned char)row;
        locals[48-14]=(unsigned char)col; locals[48-16]=111;
        check(uc_mem_write(u,0x8efd0,locals,sizeof locals)); word(u,0x8efdc,keys[k]);
        regs(u,keys[k]); captured=0;
        check(uc_emu_start(u,0x2dd91,0x2df4b,0,1000));
        check(uc_mem_read(u,0x8efd0,locals,sizeof locals));
        struct SlicksControllersDialog d={(unsigned char)row,(unsigned char)col,0,0,111};
        if(slicks_controllers_key(&d,&c,ds+0x6ac,keys[k]) || d.row!=locals[48-13] ||
            d.column!=locals[48-14] || d.done!=locals[48-15] ||
            (unsigned char)d.redraw!=locals[48-16] || d.capturing!=captured) {
            fprintf(stderr,"Controllers dispatch mismatch row=%u col=%u key=%x device=%u\n",row,col,keys[k],devices[v]); return 1;
        }
        unsigned char actual[20]; check(uc_mem_read(u,0x3cbf0+0x5358,actual,20));
        if(memcmp(actual,c.keys,20)) return 1;
        check(uc_mem_read(u,0x3cbf0+0x5e2,actual,4)); if(memcmp(actual,c.player_input,4)) return 1;
        ++cases;
    }
    check(uc_hook_del(u,hook)); check(uc_ctl_remove_cache(u,0,0x100000));
    unsigned captures=0;
    for(unsigned row=0;row<4;++row) for(unsigned col=1;col<=5;++col)
    for(unsigned scan=0;scan<512;++scan) {
        struct SlicksConfiguration c={0}; memset(c.keys,0x57,20);
        check(uc_mem_write(u,0x3cbf0+0x5358,c.keys,20));
        unsigned char state[2]={(unsigned char)col,(unsigned char)row};
        check(uc_mem_write(u,0x8eff2,state,2)); word(u,0x8efda,scan);
        regs(u,scan); check(uc_emu_start(u,0x2df1b,0x2df4b,0,1000));
        struct SlicksControllersDialog d={(unsigned char)row,(unsigned char)col,0,1,(signed char)row};
        if(slicks_controllers_capture(&d,&c,ds+0x45a,(short)scan) || d.capturing) return 1;
        unsigned char actual[20]; check(uc_mem_read(u,0x3cbf0+0x5358,actual,20));
        if(memcmp(actual,c.keys,20)) {
            fprintf(stderr,"Controllers capture mismatch row=%u col=%u scan=%x\n",row,col,scan); return 1;
        }
        ++captures;
    }
    check(uc_close(u));
    printf("Original Controllers: %u navigation/device/default/exit and %u key-capture comparisons pass\n",cases,captures);
    return 0;
}
