#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/race_rewards.h"
#include "../src/game/setup_session.h"
#include "../src/game/finish_rank.h"
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned at,unsigned value)
{ unsigned char b[2]={value,value>>8}; check(uc_mem_write(u,at,b,2)); }
static unsigned get(uc_engine *u,unsigned at)
{ unsigned char b[2]; check(uc_mem_read(u,at,b,2)); return b[0]|b[1]<<8; }
static void run(uc_engine *u,unsigned start,unsigned end,unsigned driver)
{
    uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xe000,bp=0xf000,bx=driver,ip;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    check(uc_reg_write(u,UC_X86_REG_BP,&bp)); check(uc_reg_write(u,UC_X86_REG_BX,&bx));
    check(uc_emu_start(u,start,end,0,10000));
    check(uc_reg_read(u,UC_X86_REG_IP,&ip));
    if(ip!=end-0x19870) abort();
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); int error=ferror(f); fclose(f);
    if(error || n<200000 || n==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n));
    const signed char rank_values[]={-128,-5,-2,-1,0,1,2,3,4,127};
    unsigned rank_cases=0,lap_cases=0;
    for(unsigned pattern=0;pattern<10000;++pattern) for(unsigned driver=0;driver<4;++driver) {
        signed char ranks[4],actual[4]; unsigned digits=pattern;
        for(unsigned i=0;i<4;++i) { ranks[i]=rank_values[digits%10]; digits/=10; }
        check(uc_mem_write(u,0x3cbf0+0x4bce,ranks,4)); word(u,0x8ef98,driver);
        run(u,0x22b89,0x22bd6,driver);
        (void)slicks_assign_finish_rank(ranks,driver);
        check(uc_mem_read(u,0x3cbf0+0x4bce,actual,4));
        for(unsigned i=0;i<4;++i) if(ranks[i]!=actual[i]) abort();
        ++rank_cases;
    }
    const short lap_values[]={-32768,-1,0,1,3,4,100,32767};
    for(unsigned pattern=0;pattern<4096;++pattern) for(unsigned r=0;r<10;++r) {
        signed char ranks[4],actual[4]; short laps[4]; unsigned digits=pattern;
        for(unsigned i=0;i<4;++i) {
            ranks[i]=rank_values[(r+i)%10]; laps[i]=lap_values[digits%8]; digits/=8;
            word(u,0x3cbf0+0x4bfe +2*i,(unsigned short)laps[i]);
        }
        check(uc_mem_write(u,0x3cbf0+0x4bce,ranks,4));
        run(u,0x22bfd,0x22c4d,0);
        slicks_adjust_finish_laps(ranks,laps);
        check(uc_mem_read(u,0x3cbf0+0x4bce,actual,4));
        for(unsigned i=0;i<4;++i) if(ranks[i]!=actual[i]) abort();
        ++lap_cases;
    }
    const short amounts[]={-32768,-1,0,1,20,50,32767};
    const signed int times[]={-2147483647-1,-65537,-32769,-32768,-1,0,17,29998,29999,30000,32767,32768,65535,2147483647};
    unsigned finishes=0,tracks=0;
    /* Original reset loop initializes the lap sentinel even for absent
     * drivers; they must not become zero-time fastest-lap candidates. */
    for(unsigned pattern=0;pattern<81;++pattern) {
        unsigned digits=pattern;
        for(unsigned i=0;i<4;++i) {
            signed char role=(signed char)(digits%3)-1; digits/=3;
            check(uc_mem_write(u,0x3cbf0+0x4bc6+i,&role,1));
            word(u,0x3cbf0+0x4c06+4*i,0); word(u,0x3cbf0+0x4c08+4*i,0x1234);
        }
        run(u,0x1c111,0x1c241,0);
        for(unsigned i=0;i<4;++i)
            if(get(u,0x3cbf0+0x4c06+4*i)!=30000 || get(u,0x3cbf0+0x4c08+4*i)) abort();
    }
    for(unsigned driver=0;driver<4;++driver) for(unsigned rank=1;rank<=4;++rank)
    for(unsigned a=0;a<7;++a) for(unsigned b=0;b<256;++b) {
        unsigned char rank_byte=rank,points_byte=b,multiplier=(unsigned char)(b*37);
        short cash=(short)(unsigned short)(b*517),points=(short)(unsigned short)(b*131);
        struct SlicksSetupSession session={0};
        unsigned char points_by_rank[4]={b,b,b,b};
        session.players.count=(unsigned char)(multiplier+rank);
        session.options.field_302e=amounts[a];
        session.cash[driver]=cash; session.points[driver]=points;
        check(uc_mem_write(u,0x3cbf0+0x4c16,&session.players.count,1));
        word(u,0x8ef98,driver); word(u,0x3cbf0+0x302e,(unsigned short)amounts[a]);
        word(u,0x3cbf0+0x4bf6+2*driver,(unsigned short)cash);
        word(u,0x3cbf0+0x6826+2*driver,(unsigned short)points);
        check(uc_mem_write(u,0x3cbf0+0x4bce +driver,&rank_byte,1));
        check(uc_mem_write(u,0x3cbf0+0x453+rank,&points_byte,1));
        check(uc_mem_write(u,0x8ef86,&multiplier,1));
        run(u,0x22cda,0x22d1b,driver);
        slicks_setup_finish_reward(&session,driver,(signed char)rank,points_by_rank);
        cash=session.cash[driver]; points=session.points[driver];
        if(get(u,0x3cbf0+0x4bf6+2*driver)!=(unsigned short)cash || get(u,0x3cbf0+0x6826+2*driver)!=(unsigned short)points) abort();
        ++finishes;
    }
    for(unsigned pattern=0;pattern<196;++pattern) for(unsigned a=0;a<7;++a) for(unsigned b=0;b<7;++b) {
        signed int laps[4]={times[pattern%14],times[pattern/14],times[pattern%14],30000};
        short cash[4],points[4]; unsigned char fastest=(unsigned char)(pattern*31);
        struct SlicksSetupSession session={0};
        session.options.field_302c=amounts[a]; session.options.field_302e=amounts[b];
        word(u,0x3cbf0+0x302c,(unsigned short)amounts[a]);
        word(u,0x3cbf0+0x302e,(unsigned short)amounts[b]);
        check(uc_mem_write(u,0x3cbf0+0x458,&fastest,1));
        for(unsigned i=0;i<4;++i) {
            cash[i]=(short)(unsigned short)(pattern*997+i*9000); points[i]=(short)(unsigned short)(pattern*317+i*7000);
            session.cash[i]=cash[i]; session.points[i]=points[i];
            word(u,0x3cbf0+0x4bf6+2*i,(unsigned short)cash[i]); word(u,0x3cbf0+0x6826+2*i,(unsigned short)points[i]);
            word(u,0x3cbf0+0x4c06+4*i,(uint32_t)laps[i]); word(u,0x3cbf0+0x4c08+4*i,(uint32_t)laps[i]>>16);
        }
        unsigned char normal=0; check(uc_mem_write(u,0x3cbf0+0x459,&normal,1));
        run(u,0x25552,0x255ff,0);
        slicks_setup_track_reward(&session,laps,fastest);
        for(unsigned i=0;i<4;++i)
            if(get(u,0x3cbf0+0x4bf6+2*i)!=(unsigned short)session.cash[i] || get(u,0x3cbf0+0x6826+2*i)!=(unsigned short)session.points[i]) abort();
        ++tracks;
    }
    /* Audit the actual Change Car consumer, not its menu label: this
     * executable overwrites 1-setting with 2 before entering intermission. */
    for(unsigned setting=0;setting<65536;++setting) {
        word(u,0x3cbf0+0x3030,setting);
        word(u,0x8efef,0xa5a5);
        run(u,0x245bb,0x245c8,0);
        if((get(u,0x8efef)&255)!=2) abort();
    }
    check(uc_close(u));
    printf("Original race rewards: %u composed finish awards (rank/count/options/session) and %u track/tied-fastest-lap cases pass, including signed wrap and sentinel behavior\n",finishes,tracks);
    printf("Original finish ranking: %u assignments and %u lap adjustments pass\n",rank_cases,lap_cases);
    puts("Original lap initialization: all 81 human/computer/inactive combinations initialize all four best laps to 30000");
    puts("Original Change Car consumer: all 65536 setting words produce initial intermission selection 2");
    return 0;
}
