#define SLICKS_MENU_TRACE_CAPACITY 256
#define main profile_setup_verifier_main
#include "verify_profile_setup.c"
#undef main
#include "../src/ui/options_menu_renderer.h"
#include "../src/gen/setup_defaults.h"

static unsigned char measure(void *p,const unsigned char *text)
{
    struct MenuDrawCall *c=menu_call(p,8); unsigned n=0;
    do { if(n>=64) abort(); c->text[n]=text[n]; } while(text[n++]);
    return (unsigned char)((n-1)*3);
}
static void pattern(void *p,short l,short t,short r,short b)
{ struct MenuDrawCall *c=menu_call(p,9); short a[]={l,t,r,b}; memcpy(c->args,a,sizeof a); }
static void options_boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    if(address!=0x300e5 && address!=0x34599) { editor_draw_boundary(u,address,size,p); return; }
    uint16_t ss,sp,cs,ip,ax;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp; short a[9];
    for(unsigned i=0;i<9;++i) a[i]=(short)readword(u,stack+4+i*2);
    if(address==0x300e5) {
        unsigned char text[64];
        check(uc_mem_read(u,(unsigned short)a[0]+16U*(unsigned short)a[1],text,sizeof text));
        ax=measure(p,text); check(uc_reg_write(u,UC_X86_REG_AX,&ax));
    } else pattern(p,a[0],a[1],a[2],a[3]);
    ip=readword(u,stack); cs=readword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    unsigned char runtime[300000]; size_t n=fread(runtime,1,sizeof runtime,f);
    if(ferror(f) || n<200000 || n==sizeof runtime) return 2;
    fclose(f);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,n));
    const unsigned addresses[]={0x3b9de,0x309cf,0x301ab,0x2fe63,0x39ed8,0x300e5,0x34599};
    uc_hook hooks[7]; struct MenuDrawTrace dos;
    for(unsigned i=0;i<7;++i) check(uc_hook_add(u,&hooks[i],UC_HOOK_CODE,options_boundary,&dos,addresses[i],addresses[i]));
    const unsigned char *off=runtime+0x3cbf0-0x10100+0x113c;
    struct SlicksOptionsLabels labels={slicks_original_option_labels,slicks_original_option_suffixes,
        slicks_original_mode_labels,off,off+4};
    const unsigned char colours[]={11,23,37,49};
    unsigned cases=0;
    for(unsigned mode=0;mode<6;++mode) for(unsigned row=0;row<18;++row)
    for(unsigned variant=0;variant<6;++variant) {
        struct SlicksConfiguration c=slicks_original_configuration; c.options[0]=(short)mode;
        for(unsigned i=1;i<15;++i) c.options[i]=variant==0?slicks_original_option_specs[i].minimum:
            variant==1?slicks_original_option_specs[i].maximum:c.options[i];
        for(unsigned i=0;i<15;++i) word(u,0x3cbf0+0x92+i*8,(unsigned short)c.options[i]);
        word(u,0x3cbf0+0x680,0x100); word(u,0x3cbf0+0x682,0x6000);
        unsigned char locals[0x130]={0};
        locals[0x12f]=colours[0]; locals[0x12e]=colours[1]; locals[0x12d]=colours[2]; locals[0x12c]=colours[3];
        locals[0x12b]=(unsigned char)row;
        signed char redraw=variant<2?-1:variant==2?111:variant==3?123:variant==4?(signed char)row:(signed char)(row?row-1:0);
        locals[0x12a]=(unsigned char)redraw;
        check(uc_mem_write(u,0x8eed0,locals,sizeof locals));
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        memset(&dos,0,sizeof dos); check(uc_emu_start(u,0x28f9d,0x293ef,0,100000));
        struct MenuDrawTrace native={0};
        struct SlicksOptionsDrawOps ops={{menu_restore,menu_bevel,menu_sprite,menu_text,&native},
            editor_colour,measure,pattern,editor_rectangle};
        struct SlicksOptionsMenu m={(unsigned char)row,0,0,redraw};
        if(slicks_draw_options_menu(&m,&c,slicks_original_option_specs,&labels,colours,&ops) || memcmp(&dos,&native,sizeof dos)) {
            fprintf(stderr,"Options drawing differs mode=%u row=%u variant=%u calls=%u/%u\n",mode,row,variant,dos.count,native.count);
            for(unsigned i=0;i<dos.count || i<native.count;++i) if(memcmp(&dos.calls[i],&native.calls[i],sizeof dos.calls[i])) {
                fprintf(stderr,"First difference call=%u kinds=%u/%u texts=%s/%s\n",i,dos.calls[i].kind,native.calls[i].kind,dos.calls[i].text,native.calls[i].text); break;
            }
            return 1;
        }
        ++cases;
    }
    check(uc_close(u)); printf("Original options drawing: %u command/text/order comparisons pass\n",cases);
}
