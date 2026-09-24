#define main profile_setup_verifier_main
#include "verify_profile_setup.c"
#undef main
#include "../src/ui/intermission_draw.h"
static void name_row(void *p,const unsigned char *name,short x,short y,unsigned char flags)
{ menu_text(p,0,name,x,y,flags); }
static void fastest_row(void *p,short x,short y) { menu_sprite(p,-1,x,y); }
static short header_total;
static void header_lookup(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)size; (void)p; uint16_t ss,sp,cs,ip,ax,dx=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    if(address==0x35e63) { ax=0; dx=0x6500; } else ax=(unsigned short)header_total;
    ip=readword(u,stack); cs=readword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
}
static void verify_header(uc_engine *u,struct MenuDrawTrace *dos)
{
    uc_hook hooks[3];
    check(uc_hook_add(u,&hooks[0],UC_HOOK_CODE,header_lookup,NULL,0x35e63,0x35e63));
    check(uc_hook_add(u,&hooks[1],UC_HOOK_CODE,header_lookup,NULL,0x241cc,0x241cc));
    check(uc_hook_add(u,&hooks[2],UC_HOOK_CODE,editor_draw_boundary,dos,0x36fae,0x36fae));
    const short indices[]={-32768,-1,0,1,194,32767}; unsigned cases=0;
    const unsigned char name[]="BASIC",slash[]="/";
    check(uc_mem_write(u,0x65000,name,sizeof name));
    /* Lookup inputs remain valid even though catalogue resolution is mocked. */
    word(u,0x3cbf0+0x62a,0); word(u,0x3cbf0+0x62c,0x7000);
    for(unsigned i=0;i<6;++i) for(unsigned j=0;j<6;++j) {
        short index=indices[i]; header_total=indices[j]; word(u,0x3cbf0+0x628,(unsigned short)index);
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        memset(dos,0,sizeof *dos); check(uc_emu_start(u,0x243e9,0x244fb,0,10000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        struct MenuDrawTrace native={0};
        const struct SlicksIntermissionHeaderOps ops={{editor_colour,editor_number,name_row,fastest_row,&native},editor_nearest};
        slicks_intermission_track_header(index,header_total,name,slash,&ops);
        if(ip!=0xac8b || sp!=0xeb00 || memcmp(dos,&native,sizeof native)) {
            fprintf(stderr,"intermission header mismatch index=%d total=%d calls=%u/%u\n",index,header_total,dos->count,native.count);
            for(unsigned k=0;k<dos->count && k<native.count;++k) if(memcmp(&dos->calls[k],&native.calls[k],sizeof dos->calls[k])) {
                fprintf(stderr,"call %u kind %u/%u text %s/%s\n",k,dos->calls[k].kind,native.calls[k].kind,dos->calls[k].text,native.calls[k].text);
                for(unsigned a=0;a<8;++a) fprintf(stderr," arg%u %d/%d",a,dos->calls[k].args[a],native.calls[k].args[a]);
                fputc('\n',stderr);
            }
            exit(1);
        }
        ++cases;
    }
    for(unsigned i=0;i<3;++i) check(uc_hook_del(u,hooks[i]));
    printf("Original intermission header: %u colour/text/number/coordinate comparisons pass\n",cases);
}
static void action_restore(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)address; (void)size; uint16_t ss,sp,cs,ip;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    short x=(short)readword(u,stack+4);
    menu_restore(p,x,(short)readword(u,stack+6),0,0,x==95?8:72,x==95?40:42);
    ip=readword(u,stack); cs=readword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void verify_actions(uc_engine *u,struct MenuDrawTrace *dos,const unsigned char *runtime)
{
    uc_hook restore_hook,bevel_hook;
    check(uc_hook_add(u,&restore_hook,UC_HOOK_CODE,action_restore,dos,0x3aaf2,0x3aaf2));
    check(uc_hook_add(u,&bevel_hook,UC_HOOK_CODE,editor_draw_boundary,dos,0x309cf,0x309cf));
    word(u,0x3cbf0+0x1722,0); word(u,0x3cbf0+0x1724,0);
    const unsigned char *data=runtime+0x3cbf0-0x10100,*labels[4];
    check(uc_mem_write(u,0x8eed8,data+0x7c2,16));
    for(unsigned i=0;i<4;++i) labels[i]=data+(data[0x7c2+4*i]|data[0x7c3+4*i]<<8);
    unsigned cases=0;
    for(unsigned count=1;count<=4;++count) for(unsigned selected=0;selected<4;++selected)
    for(int redraw=-1;redraw<=1;++redraw) for(unsigned saved=0;saved<2;++saved) {
        struct SlicksIntermissionMenu m={(signed char)selected,(signed char)redraw,0,0};
        unsigned char c=(unsigned char)count,locals[2]={(unsigned char)redraw,(unsigned char)selected},first=2;
        check(uc_mem_write(u,0x3cbf0+0x4c16,&c,1));
        check(uc_mem_write(u,0x8eff4,locals,2)); check(uc_mem_write(u,0x8efef,&first,1));
        word(u,0x8effc,0); word(u,0x8effe,saved?0x5000:0);
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        memset(dos,0,sizeof *dos); check(uc_emu_start(u,0x245da,0x24702,0,10000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        unsigned char actual; check(uc_mem_read(u,0x8eff4,&actual,1));
        struct MenuDrawTrace native={0};
        const struct SlicksPlayerMenuDrawOps ops={menu_restore,menu_bevel,menu_sprite,menu_text,&native};
        slicks_intermission_action_rows(&m,c,(unsigned char)saved,labels,&ops);
        if(ip!=0xae92 || sp!=0xeb00 || (signed char)actual!=m.redraw || memcmp(dos,&native,sizeof native)) {
            fprintf(stderr,"intermission actions mismatch count=%u selected=%u redraw=%d saved=%u\n",count,selected,redraw,saved); exit(1);
        }
        ++cases;
    }
    printf("Original intermission action rows: %u restore/highlight/label/redraw comparisons pass\n",cases);
    cases=0;
    for(unsigned role=0;role<81;++role) for(int redraw=-1;redraw<=1;++redraw)
    for(unsigned saved=0;saved<2;++saved) {
        unsigned digits=role; signed char roles[4],vehicles[4];
        for(unsigned i=0;i<4;++i) {
            roles[i]=(signed char)(digits%3)-1; digits/=3;
            vehicles[i]=(signed char)((role+i*3)%10);
        }
        for(unsigned i=0;i<10;++i) {
            word(u,0x3cbf0+0x4e44+4*i,0x400+16*i); word(u,0x3cbf0+0x4e46+4*i,0x6000);
        }
        check(uc_mem_write(u,0x3cbf0+0x4bc6,roles,4)); check(uc_mem_write(u,0x3cbf0+0x4bc2,vehicles,4));
        unsigned char flag=(unsigned char)redraw; check(uc_mem_write(u,0x8eff3,&flag,1));
        word(u,0x8eff8,0); word(u,0x8effa,saved?0x5000:0);
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        memset(dos,0,sizeof *dos); check(uc_emu_start(u,0x2453a,0x245bb,0,10000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        check(uc_mem_read(u,0x8eff3,&flag,1));
        struct MenuDrawTrace native={0};
        const struct SlicksPlayerMenuDrawOps ops={menu_restore,menu_bevel,menu_sprite,menu_text,&native};
        struct SlicksIntermissionMenu m={2,0,(signed char)redraw,0};
        slicks_intermission_car_rows(&m,roles,vehicles,(unsigned char)saved,&ops);
        if(ip!=0xad4b || sp!=0xeb00 || (signed char)flag!=m.cars_redraw || memcmp(dos,&native,sizeof native)) {
            fprintf(stderr,"intermission cars mismatch roles=%u redraw=%d saved=%u\n",role,redraw,saved); exit(1);
        }
        ++cases;
    }
    printf("Original intermission car rows: %u restore/icon/redraw comparisons pass\n",cases);
    check(uc_hook_del(u,restore_hook)); check(uc_hook_del(u,bevel_hook));
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f);
    if(n<200000 || n==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,n));
    struct MenuDrawTrace dos; uc_hook hook;
    const unsigned addresses[]={0x2fe63,0x302b6,0x301ab,0x2e2d2};
    for(unsigned i=0;i<4;++i) check(uc_hook_add(u,&hook,UC_HOOK_CODE,editor_draw_boundary,&dos,addresses[i],addresses[i]));
    word(u,0x3cbf0+0x680,0x100); word(u,0x3cbf0+0x682,0x6000);
    word(u,0x3cbf0+0x4c30,0x300); word(u,0x3cbf0+0x4c32,0x6000);
    const signed int extremes[]={-2147483647-1,-1,0,2147483647}; unsigned cases=0;
    for(unsigned role=0;role<81;++role) for(unsigned ties=0;ties<16;++ties) {
        signed char roles[4]; short points[4]; signed int laps[4],best=extremes[ties%4];
        unsigned char names[4][21]={{0}}; const unsigned char *resolved[4]; unsigned digits=role;
        for(unsigned i=0;i<4;++i) {
            roles[i]=(signed char)(digits%3)-1; digits/=3;
            points[i]=(short)(role*4093+ties*173+i*127);
            laps[i]=(ties&(1U<<i))?best:extremes[(ties+1)%4];
            snprintf((char *)names[i],21,"PROFILE %u",i^1); resolved[i]=names[i];
            word(u,0x3cbf0+0x44c+2*i,i^1);
            check(uc_mem_write(u,0x3cbf0+0x36aa+21*(i^1),names[i],21));
            word(u,0x3cbf0+0x6826+2*i,(unsigned short)points[i]);
            word(u,0x3cbf0+0x4c06+4*i,(uint32_t)laps[i]); word(u,0x3cbf0+0x4c08+4*i,(uint32_t)laps[i]>>16);
        }
        check(uc_mem_write(u,0x3cbf0+0x4bc6,roles,4));
        word(u,0x8f006,(uint32_t)best); word(u,0x8f008,(uint32_t)best>>16);
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xeb00,bp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        memset(&dos,0,sizeof dos); check(uc_emu_start(u,0x242ca,0x243c9,0,10000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        struct MenuDrawTrace native={0};
        const struct SlicksIntermissionRowsOps ops={editor_colour,editor_number,name_row,fastest_row,&native};
        slicks_intermission_driver_rows(roles,points,resolved,laps,best,&ops);
        if(ip!=0xab59 || sp!=0xeb00 || memcmp(&dos,&native,sizeof dos)) {
            fprintf(stderr,"intermission rows mismatch roles=%u ties=%u calls=%u/%u\n",role,ties,dos.count,native.count); return 1;
        }
        ++cases;
    }
    printf("Original intermission driver rows: %u complete text/points/colour/tied-lap marker command comparisons pass\n",cases);
    verify_actions(u,&dos,runtime);
    verify_header(u,&dos);
    check(uc_close(u));
    return 0;
}
