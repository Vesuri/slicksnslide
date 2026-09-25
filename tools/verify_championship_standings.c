#define main records_verifier_main
#include "verify_post_race_records.c"
#undef main
#include "../src/game/championship_standings.h"
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f); if(n<200000 || n==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n));
    const short values[]={-32768,-2,-1,0,1,10,32766,32767}; unsigned cases=0;
    for(unsigned mask=0;mask<16;++mask) for(unsigned pattern=0;pattern<4096;++pattern) {
        short points[4],selected[4]; signed char roles[4]; unsigned digits=pattern;
        static struct SlicksPlayerProfiles profiles; memset(&profiles,0,sizeof profiles); profiles.count=4;
        for(unsigned i=0;i<4;++i) {
            points[i]=values[digits%8]; digits/=8; roles[i]=mask&(1U<<i)?(i&1?-1:1):0;
            selected[i]=(short)(pattern&1?0:i);
            word(u,0x65000+2*i,points[i]); word(u,0x3cbf0+0x44c+2*i,selected[i]);
            profiles.statistics[i][2]=32767; profiles.statistics[i][3]=-1;
            word(u,0x3cbf0+0x400e +20*i,32767); word(u,0x3cbf0+0x4010+20*i,65535);
        }
        check(uc_mem_write(u,0x3cbf0+0x4bc6,roles,4));
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,bp=0xf000,sp=0xe000,ip;
        word(u,0x8f006,0); word(u,0x8f008,0x6500);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_BP,&bp)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x2a66c,0x2a74d,0,10000)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        if(ip!=0x2a74d-0x266c0) abort();
        struct SlicksChampionshipStandings table; slicks_championship_standings(&table,points,roles);
        unsigned char order[4]; check(uc_mem_read(u,0x8effc,order,4));
        if(memcmp(order,table.driver,4)) abort();
        unsigned char place=0;
        for(unsigned row=0;row<4;++row) {
            short score=(short)get(u,0x8eff4+2*row);
            if(score!=table.points[row]) abort();
            if(!row || score!=(short)get(u,0x8eff4+2*(row-1))) place=(unsigned char)(row+1);
            if(place!=table.place[row]) abort();
            word(u,0x8efde,row); check(uc_mem_write(u,0x8eff3,&place,1));
            check(uc_emu_start(u,0x2aa0b,0x2aa6b,0,1000));
        }
        if(slicks_championship_statistics(&profiles,selected,&table)) abort();
        for(unsigned i=0;i<4;++i)
            if(get(u,0x3cbf0+0x400e +20*i)!=(unsigned short)profiles.statistics[i][2] ||
               get(u,0x3cbf0+0x4010+20*i)!=(unsigned short)profiles.statistics[i][3]) abort();
        ++cases;
    }
    for(unsigned selected=0;selected<4;++selected) for(unsigned rank=1;rank<=4;++rank)
    for(unsigned initial=0;initial<65536;initial+=257) {
        struct SlicksPlayerProfiles profiles={.count=4};
        profiles.statistics[selected][0]=profiles.statistics[selected][1]=(short)initial;
        word(u,0x3cbf0+0x44c,selected);
        word(u,0x3cbf0+0x400a+20*selected,initial); word(u,0x3cbf0+0x400c+20*selected,initial);
        uint16_t cs=0x1987,ds=0x3cbf,bx=0;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_BX,&bx));
        check(uc_emu_start(u,0x22b7c,0x22b89,0,100));
        if(rank==1) { check(uc_reg_write(u,UC_X86_REG_BX,&bx)); check(uc_emu_start(u,0x22bdf,0x22bec,0,100)); }
        if(slicks_finish_statistics(&profiles,(short)selected,(signed char)rank) ||
           get(u,0x3cbf0+0x400a+20*selected)!=(unsigned short)profiles.statistics[selected][0] ||
           get(u,0x3cbf0+0x400c+20*selected)!=(unsigned short)profiles.statistics[selected][1]) abort();
    }
    check(uc_close(u));
    puts("Original finish statistics: 4096 profile/rank/wrapping-word cases pass");
    printf("Original championship standings: %u signed-score/order/tie/shared-profile/statistic-wrap cases pass\n",cases);
    return 0;
}
