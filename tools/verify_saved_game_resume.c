#define main configuration_verifier_main
#include "verify_configuration.c"
#undef main
#include <assert.h>
#include "../src/game/saved_game_resume.h"

static const unsigned char *name_at(void *p,unsigned i) { return ((unsigned char (*)[9])p)[i]; }
static void boundary(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size; (void)context;
    if(address!=0x35e63) { check(uc_emu_stop(u)); return; }
    uint16_t ss,sp;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp,index=readword(u,stack+8);
    uint16_t ip=readword(u,stack),cs=readword(u,stack+2),ax=(uint16_t)(index*9),dx=0x7000;
    sp+=4;
    check(uc_reg_write(u,UC_X86_REG_IP,&ip)); check(uc_reg_write(u,UC_X86_REG_CS,&cs));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_AX,&ax));
    check(uc_reg_write(u,UC_X86_REG_DX,&dx));
}
static void registers(uc_engine *u)
{
    uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,bp=0xf000,sp=0xeb00;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); assert(f);
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,n));
    const unsigned addresses[]={0x35e63,0x1d33d,0x1d4ec,0x1d55b};
    for(unsigned i=0;i<4;++i) { uc_hook h; check(uc_hook_add(u,&h,UC_HOOK_CODE,boundary,0,addresses[i],addresses[i])); }
    static struct SlicksPlayerProfiles profiles;
    profiles.count=5;
    unsigned char names[5][9]={{0}},tracks[1][8]={{'T','E','S','T',0}};
    struct SlicksSavedGame game={.track_count=1,.tracks=tracks};
    struct SlicksSavedGameResolved out,before;
    unsigned cases=0;
    for(unsigned mask=0;mask<32;++mask) {
        for(unsigned j=0;j<5;++j) memcpy(names[j],(mask&(1U<<j))?(j&1?"test":"TEST"):"MISS",5);
        registers(u); check(uc_mem_write(u,0x70000,names,sizeof names));
        word(u,0x3cbf0+0x90,0); word(u,0x3cbf0+0x62a,0); word(u,0x3cbf0+0x62c,0x7100);
        word(u,0x3cbf0+0x4da8,5); check(uc_mem_write(u,0x8f000-0x12,tracks[0],8));
        unsigned char zero=0; check(uc_mem_write(u,0x8f000-0xa,&zero,1));
        check(uc_emu_start(u,0x1d2d4,0x90000,0,10000));
        short original=(short)readword(u,0x71000);
        memset(&out,0xa5,sizeof out); before=out;
        int result=slicks_resolve_saved_game(&out,&game,5,name_at,names,&profiles,10);
        if(result!=(original<0?SLICKS_RESUME_MISSING_TRACK:SLICKS_RESUME_READY)) {
            fprintf(stderr,"Track mask=%u original=%d result=%d\n",mask,original,result); return 1;
        }
        if(original<0) assert(!memcmp(&out,&before,sizeof out)); else assert(out.tracks[0]==original);
        ++cases;
    }
    game.track_count=0;
    for(unsigned length=0;length<=20;++length) for(unsigned mask=0;mask<32;++mask)
    for(int participation=-2;participation<=2;++participation) {
        memset(game.names[0],'A',21); game.names[0][length]=0;
        for(unsigned j=0;j<5;++j) {
            memset(profiles.names[j],(mask&(1U<<j))?(j&1?'a':'A'):'Z',21);
            profiles.names[j][length]=0;
        }
        game.participation[0]=(signed char)participation;
        registers(u); word(u,0x8f000-8,0); word(u,0x3cbf0+0x36a8,5);
        check(uc_mem_write(u,0x3cbf0+0x36aa,profiles.names,5*21));
        check(uc_mem_write(u,0x3cbf0+0x4bc6,game.participation,4));
        check(uc_mem_write(u,0x8f000-0x2c,game.names[0],21));
        check(uc_emu_start(u,0x1d48b,0x90000,0,10000));
        short original=(short)readword(u,0x3cbf0+0x44c);
        memset(&out,0xa5,sizeof out); before=out;
        int result=slicks_resolve_saved_game(&out,&game,5,name_at,names,&profiles,10);
        int missing=participation<0 && original==-2;
        assert(result==(missing?SLICKS_RESUME_MISSING_PROFILE:SLICKS_RESUME_READY));
        if(missing) assert(!memcmp(&out,&before,sizeof out)); else assert(out.profiles[0]==original);
        ++cases;
    }
    unsigned vehicle_cases=0;
    game.participation[0]=0;
    for(unsigned total=1;total<=10;++total) for(unsigned value=0;value<256;++value) {
        game.vehicles[0]=(signed char)value;
        registers(u); uint16_t bx=0;
        check(uc_reg_write(u,UC_X86_REG_BX,&bx));
        check(uc_mem_write(u,0x3cbf0+0x4bc2,game.vehicles,4));
        word(u,0x8f000-8,0); word(u,0x3cbf0+0x4c6e,total);
        check(uc_emu_start(u,0x1d3f6,0x1d412,0,100));
        unsigned char original;
        check(uc_mem_read(u,0x3cbf0+0x4bc2,&original,1));
        assert(slicks_resolve_saved_game(&out,&game,5,name_at,names,&profiles,total)==SLICKS_RESUME_READY);
        assert(out.vehicles[0]==original); ++vehicle_cases;
    }
    check(uc_close(u));
    printf("Saved-game resume: %u original track/profile matching comparisons pass, including duplicates, case, empty/max names and player roles\n",cases);
    printf("Saved-game resume: %u original vehicle-fallback comparisons pass\n",vehicle_cases);
    return 0;
}
