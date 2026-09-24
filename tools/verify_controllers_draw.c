#define SLICKS_MENU_TRACE_CAPACITY 128
#define main profile_setup_verifier_main
#include "verify_profile_setup.c"
#undef main
#include "../src/ui/controllers_dialog_draw.h"
#include "../src/gen/setup_defaults.h"
static void controller_boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    if(address!=0x3aa7c) { editor_draw_boundary(u,address,size,p); return; }
    uint16_t ss,sp,cs,ip;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    menu_sprite(p,(short)((readword(u,stack+8)-0x400)/16),
        (short)readword(u,stack+4),(short)readword(u,stack+6));
    ip=readword(u,stack); cs=readword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    unsigned char runtime[300000]; size_t n=fread(runtime,1,sizeof runtime,f);
    if(ferror(f) || n<200000 || n==sizeof runtime) return 2;
    fclose(f); const unsigned char *ds_data=runtime+0x3cbf0-0x10100;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,n));
    struct MenuDrawTrace dos; uc_hook hooks[7];
    const unsigned addresses[]={0x3b9de,0x309cf,0x301ab,0x2fe63,0x302b6,0x36fae,0x3aa7c};
    for(unsigned i=0;i<7;++i) check(uc_hook_add(u,&hooks[i],UC_HOOK_CODE,controller_boundary,&dos,addresses[i],addresses[i]));
    word(u,0x3cbf0+0x680,0x100); word(u,0x3cbf0+0x682,0x6000);
    for(unsigned i=0;i<3;++i) { word(u,0x3cbf0+0x68c+4*i,0x400+16*i); word(u,0x3cbf0+0x68e +4*i,0x6000); }
    struct SlicksControllersLabels labels={slicks_original_controller_key_names,
        slicks_original_controller_scans,ds_data+0x15e7,ds_data+0x1523};
    const unsigned char colours[]={(unsigned char)(55+55*3+50*7),(unsigned char)(38+38*3+38*7)};
    unsigned cases=0;
    for(unsigned row=0;row<5;++row) for(unsigned col=0;col<6;++col)
    for(unsigned variant=0;variant<4;++variant) {
        struct SlicksConfiguration c=slicks_original_configuration;
        for(unsigned i=0;i<4;++i) c.player_input[i]=(unsigned char)((i+variant)%5);
        for(unsigned i=0;i<20;++i) c.keys[i]=variant==3?(unsigned char)(i*13):slicks_original_controller_scans[(i*3+variant)%69];
        check(uc_mem_write(u,0x3cbf0+0x5358,c.keys,20)); check(uc_mem_write(u,0x3cbf0+0x5e2,c.player_input,4));
        unsigned char state[]={variant==0?255:variant==1?111:(unsigned char)row,0,(unsigned char)col,(unsigned char)row};
        check(uc_mem_write(u,0x8eff0,state,sizeof state)); word(u,0x8f006,100); word(u,0x8f008,80);
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        memset(&dos,0,sizeof dos); check(uc_emu_start(u,0x2da76,0x2dd89,0,100000));
        struct MenuDrawTrace native={0};
        struct SlicksControllersDrawOps ops={{menu_restore,menu_bevel,menu_sprite,menu_text,&native},editor_colour,editor_number};
        struct SlicksControllersDialog d={(unsigned char)row,(unsigned char)col,0,0,(signed char)state[0]};
        if(slicks_draw_controllers(&d,&c,100,80,&labels,colours,&ops)) return 1;
        /* The original nearest-colour lookups are palette-boundary calls,
         * while native receives the two cached indices. Remove only those. */
        struct MenuDrawTrace filtered={0};
        for(unsigned i=0;i<dos.count;++i) if(dos.calls[i].kind!=6) filtered.calls[filtered.count++]=dos.calls[i];
        if(memcmp(&filtered,&native,sizeof native)) {
            fprintf(stderr,"Controllers draw mismatch row=%u col=%u variant=%u calls=%u/%u\n",row,col,variant,filtered.count,native.count);
            return 1;
        }
        unsigned char actual; check(uc_mem_read(u,0x8eff2,&actual,1)); if(d.column!=actual) return 1;
        ++cases;
    }
    check(uc_close(u)); printf("Original Controllers drawing: %u command/text/order/selection comparisons pass\n",cases); return 0;
}
