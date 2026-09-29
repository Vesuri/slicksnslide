/* Independent original-instruction command oracle; no captured title state. */
#define main title_menu_verifier_main
#include "verify_title_menu.c"
#undef main
#include "../src/ui/title_status.h"
struct StatusTrace { struct SlicksTitleStatusCommand commands[9]; unsigned count; };
static void status_hook(uc_engine *u,uint64_t address,uint32_t size,void *opaque)
{
    (void)size;struct StatusTrace *t=opaque;
    uint16_t sp,ss,cs,ip;
    ck(uc_reg_read(u,UC_X86_REG_SP,&sp));ck(uc_reg_read(u,UC_X86_REG_SS,&ss));
    unsigned stack=16U*ss+sp;
    if(t->count>=9) abort();
    struct SlicksTitleStatusCommand *c=&t->commands[t->count++];
    c->x=(short)rw(u,stack+4);c->y=(short)rw(u,stack+6);
    if(address==0x2e2d2) {
        c->kind=0;c->value=(short)rw(u,stack+8);c->flags=0;
    } else {
        c->kind=1;c->value=(short)rw(u,stack+8);c->flags=(short)rw(u,stack+14);
    }
    ip=rw(u,stack);cs=rw(u,stack+2);sp+=4;
    ck(uc_reg_write(u,UC_X86_REG_CS,&cs));ck(uc_reg_write(u,UC_X86_REG_IP,&ip));
    ck(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    unsigned char runtime[300000];FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f);fclose(f);
    uc_engine *u;ck(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    ck(uc_mem_map(u,0,0x100000,UC_PROT_ALL));ck(uc_mem_write(u,0x10100,runtime,bytes));
    struct StatusTrace actual;uc_hook hook;
    ck(uc_hook_add(u,&hook,UC_HOOK_CODE,status_hook,&actual,0x2e2d2,0x2e2d2));
    ck(uc_hook_add(u,&hook,UC_HOOK_CODE,status_hook,&actual,0x302b6,0x302b6));
    const unsigned pointers[]={0x4c24,0x4c28,0x2fc4,0x2fc8,0x2fcc};
    for(unsigned i=0;i<5;++i) {ww(u,0x3cbf0+pointers[i],i);ww(u,0x3cbf0+pointers[i]+2,0x7000);}
    const short values[]={0,1,9,10,99,100,195,255,4095,32767};unsigned cases=0;
    for(unsigned roles=0;roles<81;++roles) for(unsigned flags=0;flags<8;++flags)
    for(unsigned v=0;v<10;++v) {
        signed char participation[4];unsigned code=roles;
        for(unsigned i=0;i<4;++i) {participation[i]=(signed char)((int)(code%3)-1);code/=3;}
        short selected=values[v],total=values[(v+3)%10],inventory=flags&1?-1:0;
        short weapons=flags&2?2:0,mode=flags&4?4:5;
        ck(uc_mem_write(u,0x3cbf0+0x4bc6,participation,4));
        ww(u,0x3cbf0+0x90,selected);ww(u,0x3cbf0+0x4da8,total);
        ww(u,0x3cbf0+0x3022,inventory);ww(u,0x3cbf0+0x3020,weapons);ww(u,0x3cbf0+0x92,mode);
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xe000,bp=0xf000;
        ck(uc_reg_write(u,UC_X86_REG_CS,&cs));ck(uc_reg_write(u,UC_X86_REG_DS,&ds));
        ck(uc_reg_write(u,UC_X86_REG_SS,&ss));ck(uc_reg_write(u,UC_X86_REG_SP,&sp));ck(uc_reg_write(u,UC_X86_REG_BP,&bp));
        actual=(struct StatusTrace){0};struct SlicksTitleStatusCommand expected[9]={0};
        unsigned count=slicks_title_status_commands(expected,participation,selected,total,inventory,weapons,mode);
        ck(uc_emu_start(u,0x2995f,0x29af6,0,3000));
        if(actual.count!=count || memcmp(actual.commands,expected,sizeof expected)) {
            fprintf(stderr,"Title status mismatch roles=%u flags=%u counts=%u\n",roles,flags,v);return 1;
        }
        ++cases;
    }
    uc_close(u);printf("Original title status: %u role/count/badge draw-command comparisons pass\n",cases);return 0;
}
