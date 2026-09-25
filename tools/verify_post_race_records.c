#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/post_race_records.h"
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned at,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(u,at,b,2)); }
static unsigned get(uc_engine *u,unsigned at)
{ unsigned char b[2]; check(uc_mem_read(u,at,b,2)); return b[0]|b[1]<<8; }
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); int error=ferror(f); fclose(f);
    if(error || n<200000 || n==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n));
    const signed int times[]={-2147483647-1,-32769,-32768,-1,0,1,2,100,200,29999,30000,32767,65536};
    const short upgrades[]={-32768,-1,0,4,5,32767};
    unsigned cases=0;
    for(unsigned pattern=0;pattern<81;++pattern) for(unsigned t=0;t<13;++t)
    for(unsigned upgrade=0;upgrade<6;++upgrade) for(unsigned seed=0;seed<4;++seed) {
        struct SlicksTrackRecords records;
        for(unsigned i=0;i<sizeof records.entries;++i) ((unsigned char *)records.entries)[i]=(unsigned char)(i*31+seed);
        records.trailer=seed==3?65535:0;
        for(unsigned row=0;row<11;++row) {
            unsigned value=seed==0?0:seed==1?100+row*100:seed==2?100:32768+row;
            records.entries[row][20]=(unsigned char)value; records.entries[row][21]=(unsigned char)(value>>8);
        }
        check(uc_mem_write(u,0x3cbf0+0x693a,records.entries,319)); word(u,0x3cbf0+0x4db2,records.trailer);
        unsigned char names[4][21]; struct SlicksRecordEntrant entrants[4];
        unsigned digits=pattern;
        for(unsigned i=0;i<4;++i) {
            for(unsigned j=0;j<21;++j) names[i][j]=(unsigned char)('A'+(i+j)%26);
            entrants[i]=(struct SlicksRecordEntrant){.role=(signed char)(digits%3)-1,
                .vehicle=(signed char)(i+seed),.setting=(unsigned char)(seed==3?101:seed==2?100:0),
                .name=names[i],.best_lap=times[(t+i)%13],.engine=upgrades[upgrade],.tyres=upgrades[(upgrade+i)%6]};
            digits/=3;
            check(uc_mem_write(u,0x3cbf0+0x4bc6+i,&entrants[i].role,1));
            check(uc_mem_write(u,0x3cbf0+0x4bc2+i,&entrants[i].vehicle,1));
            check(uc_mem_write(u,0x3cbf0+0x3f42+i,&entrants[i].setting,1));
            check(uc_mem_write(u,0x3cbf0+0x36aa+21*i,names[i],21));
            word(u,0x3cbf0+0x44c+2*i,i);
            word(u,0x3cbf0+0x4c06+4*i,(uint32_t)entrants[i].best_lap);
            word(u,0x3cbf0+0x4c08+4*i,(uint32_t)entrants[i].best_lap>>16);
            word(u,0x3cbf0+0x6a7a+26*i,entrants[i].engine);
            word(u,0x3cbf0+0x6a7c+26*i,entrants[i].tyres);
        }
        word(u,0x3cbf0+0x4db4,25); word(u,0x3cbf0+0x4db6,9); word(u,0x3cbf0+0x4db8,2026);
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,bp=0xf000,sp=0xe000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_BP,&bp)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x255ff,0x25803,0,100000)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        if(ip!=0x25803-0x19870) abort();
        struct SlicksRecordOutcome result=slicks_post_race_records(&records,entrants,25,9,2026);
        unsigned char actual[319],ranks[4]; check(uc_mem_read(u,0x3cbf0+0x693a,actual,319));
        check(uc_mem_read(u,0x3cbf0+0x4bce,ranks,4));
        if(memcmp(actual,records.entries,319) || memcmp(ranks,result.ranks,4) ||
           get(u,0x3cbf0+0x4db2)!=records.trailer ||
           !!(get(u,0x8f000-0x2d)&255)!=result.changed || !!(get(u,0x8f000-0x2e)&255)!=result.show) {
            fprintf(stderr,"Post-race record mismatch pattern=%u time=%u upgrade=%u seed=%u\n",pattern,t,upgrade,seed); return 1;
        }
        ++cases;
    }
    check(uc_close(u));
    printf("Original post-race records: %u complete qualification/insertion/rank/date/trailer comparisons pass\n",cases);
    return 0;
}
