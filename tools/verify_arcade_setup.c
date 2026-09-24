#include <stdio.h>
#include <stdlib.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/arcade_setup.h"
#include "../src/game/race_runtime.c"
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned at,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(u,at,b,2)); }
static unsigned get(uc_engine *u,unsigned at)
{ unsigned char b[2]; check(uc_mem_read(u,at,b,2)); return b[0]|b[1]<<8; }
static uint32_t run(uc_engine *u,unsigned address)
{
    uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000,ip,ax,dx;
    word(u,0x8f000,0); word(u,0x8f002,0x9000);
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    check(uc_emu_start(u,address,0x90000,0,100000));
    check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    check(uc_reg_read(u,UC_X86_REG_AX,&ax)); check(uc_reg_read(u,UC_X86_REG_DX,&dx));
    if(ip || sp!=0xf004) { fprintf(stderr,"Arcade return failed at %x\n",address); exit(1); }
    return ax|((uint32_t)dx<<16);
}
struct SequenceCapture { unsigned count; short tracks[8]; };
static void sequence_boundary(uc_engine *u,uint64_t address,uint32_t size,void *opaque)
{
    (void)size; struct SequenceCapture *c=opaque;
    uint16_t ss,sp,ip,cs,zero=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    if(address==0x24b7d) {
        if(c->count==8) abort();
        c->tracks[c->count++]=(short)get(u,stack+8);
    }
    /* Track-name resolution and the race itself are boundaries; playlist
     * indexing, count policy, loop increment/reset execute original code. */
    check(uc_reg_write(u,UC_X86_REG_AX,&zero)); check(uc_reg_write(u,UC_X86_REG_DX,&zero));
    ip=get(u,stack); cs=get(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f); if(!n || n==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n));
    const short modes[]={-32768,-1,0,1,2,3,4,5,6,32767};
    const short times[]={-32768,-1,0,1,30,99,32767};
    const uint32_t ticks[]={0,1,89,90,91,2699,2700,2701,65535,2147483,2147484,4294967,4294968,0x7fffffff,0xffffffff};
    const short limits[]={-32768,-1,0,1,9998,9999,10000,32767};
    unsigned cases=0;
    for(unsigned m=0;m<sizeof modes/sizeof modes[0];++m)
    for(unsigned t=0;t<sizeof times/sizeof times[0];++t)
    for(unsigned k=0;k<sizeof ticks/sizeof ticks[0];++k) {
        word(u,0x3cbf0+0x92,modes[m]); word(u,0x3cbf0+0xfa,times[t]);
        word(u,0x3cbf0+0x74bc,ticks[k]); word(u,0x3cbf0+0x74be,ticks[k]>>16);
        if((int32_t)run(u,0x19894)!=slicks_arcade_elapsed_ms(ticks[k]) ||
            (int32_t)run(u,0x198c9)!=slicks_arcade_remaining_ms(modes[m],times[t],ticks[k]) ||
            (run(u,0x198fe)&255)!=slicks_arcade_time_finished(modes[m],times[t],ticks[k])) {
            fprintf(stderr,"Arcade clock mismatch mode=%d seconds=%d ticks=%u\n",modes[m],times[t],ticks[k]); return 1;
        }
        for(unsigned l=0;l<sizeof limits/sizeof limits[0];++l) {
            short laps[4]={(short)(l-3),(short)(l+1),l&1?32767:0,l&2?-32768:12};
            word(u,0x3cbf0+0x4c18,limits[l]);
            for(unsigned i=0;i<4;++i) word(u,0x3cbf0+0x4bfe + 2*i,laps[i]);
            short expected=slicks_arcade_lap_limit(modes[m],times[t],ticks[k],limits[l],laps);
            static struct SlicksRaceRuntime race;
            race.started=0;
            slicks_race_set_mode(&race,modes[m],times[t]);
            race.game_clock_ticks=ticks[k];
            race.laps_to_run=(unsigned short)limits[l];
            for(unsigned i=0;i<4;++i) race.cars[i].lap=(unsigned short)(laps[i]+1U);
            if((short)race_lap_limit(&race)!=expected) {
                fputs("Arcade runtime lap mapping mismatch\n",stderr); return 1;
            }
            if((short)run(u,0x1991f)!=expected || (short)get(u,0x3cbf0+0x4c18)!=expected) {
                fprintf(stderr,"Arcade finish mismatch mode=%d seconds=%d ticks=%u limit=%d\n",modes[m],times[t],ticks[k],limits[l]); return 1;
            }
            ++cases;
        }
    }
    unsigned tracks=0;
    for(unsigned m=0;m<sizeof modes/sizeof modes[0];++m)
    for(unsigned a=0;a<sizeof limits/sizeof limits[0];++a)
    for(unsigned b=0;b<sizeof limits/sizeof limits[0];++b) {
        word(u,0x3cbf0+0x92,modes[m]); word(u,0x3cbf0+0x102,limits[a]); word(u,0x3cbf0+0x90,limits[b]);
        if((short)run(u,0x241cc)!=slicks_arcade_track_count(modes[m],limits[a],limits[b]) ||
            (short)get(u,0x3cbf0+0x90)!=limits[b]) { fputs("Arcade track-count mismatch\n",stderr); return 1; }
        ++tracks;
    }
    unsigned finishes=0;
    const unsigned clocks[]={0,1,450,27000,0x7fffffff,0xffffff00};
    const unsigned deadlines[]={0,1,500,30000,0x7fffffff,0xffffffff};
    for(unsigned mode=0;mode<6;++mode)
    for(unsigned c=0;c<6;++c) for(unsigned d=0;d<6;++d) for(unsigned mask=0;mask<16;++mask)
    for(unsigned unfinished_mask=0;unfinished_mask<16;unfinished_mask+=5) {
        word(u,0x3cbf0+0x92,mode);
        word(u,0x3cbf0+0x685e,clocks[c]); word(u,0x3cbf0+0x6860,clocks[c]>>16);
        word(u,0x3cbf0+0x6862,deadlines[d]); word(u,0x3cbf0+0x6864,deadlines[d]>>16);
        for(unsigned i=0;i<4;++i) {
            signed char role=(mask>>i)&1 ? (i&1?-1:1) : 0;
            unsigned char unfinished=(unfinished_mask>>i)&1?0xff:1;
            check(uc_mem_write(u,0x3cbf0+0x4bc6+i,&role,1));
            check(uc_mem_write(u,0x3cbf0+0x4bce + i,&unfinished,1));
        }
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,bp=0xf000,sp=0xef00;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x22c4d,0x22cda,0,10000));
        unsigned actual=get(u,0x3cbf0+0x6862)|(get(u,0x3cbf0+0x6864)<<16);
        if(actual!=slicks_finish_deadline(mode,clocks[c],deadlines[d],(mask&unfinished_mask)!=0)) {
            fputs("Finish deadline mismatch\n",stderr); return 1;
        }
        ++finishes;
    }
    unsigned gates=0;
    const unsigned boundary[]={0,1,269,270,271,1000,1269,1270,1271,0x7fffffff,0x80000000,0xffffffff};
    const signed char ranks[]={-128,-4,-1,0,1,4,127};
    for(unsigned c=0;c<12;++c) for(unsigned d=0;d<12;++d) for(unsigned r=0;r<7;++r) {
        word(u,0x3cbf0+0x685e,boundary[c]); word(u,0x3cbf0+0x6860,boundary[c]>>16);
        word(u,0x3cbf0+0x6862,boundary[d]); word(u,0x3cbf0+0x6864,boundary[d]>>16);
        check(uc_mem_write(u,0x3cbf0+0x4bce,&ranks[r],1));
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,bp=0xf000,sp=0xef00;
        word(u,0x8f000-0x68,0); word(u,0x8f000-0x37,0);
        word(u,0x3cbf0+0x5344,0x0101);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x2037d,0x203f2,0,10000));
        unsigned suppressed=get(u,0x8f000-0x6b)&255,expired=get(u,0x8f000-0x37)&255;
        if(suppressed!=slicks_finish_controls_suppressed(boundary[c],boundary[d],ranks[r]) ||
            expired!=slicks_finish_expired(boundary[c],boundary[d]) ||
            get(u,0x3cbf0+0x5344)!=(suppressed?0:0x0101)) {
            fprintf(stderr,"Finish gate mismatch current=%u deadline=%u rank=%d\n",boundary[c],boundary[d],ranks[r]); return 1;
        }
        ++gates;
    }
    struct SequenceCapture sequence;
    uc_hook h1,h2;
    check(uc_hook_add(u,&h1,UC_HOOK_CODE,sequence_boundary,&sequence,0x35e63,0x35e63));
    check(uc_hook_add(u,&h2,UC_HOOK_CODE,sequence_boundary,&sequence,0x24b7d,0x24b7d));
    const short selection[]={3,1,3,0},caps[]={-1,0,1,2,4,10};
    unsigned sequences=0;
    for(unsigned mode=0;mode<6;++mode) for(unsigned count=0;count<=4;++count)
    for(unsigned cap=0;cap<6;++cap) for(unsigned start=0;start<=4;start+=2) {
        sequence.count=0;
        word(u,0x3cbf0+0x92,mode); word(u,0x3cbf0+0x102,caps[cap]);
        word(u,0x3cbf0+0x90,count); word(u,0x3cbf0+0x628,start);
        word(u,0x3cbf0+0x62a,0); word(u,0x3cbf0+0x62c,0x5000);
        for(unsigned i=0;i<4;++i) word(u,0x50000+2*i,selection[i]);
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x26429,0x26439,0,100000));
        int total=slicks_arcade_track_count(mode,caps[cap],count);
        unsigned expected=(int)start<total?(unsigned)total-start:0;
        if(sequence.count!=expected || get(u,0x3cbf0+0x628) || get(u,0x3cbf0+0x90)!=count) {
            fputs("Track sequence state mismatch\n",stderr); return 1;
        }
        for(unsigned i=0;i<expected;++i) if(sequence.tracks[i]!=selection[start+i]) {
            fputs("Track sequence order mismatch\n",stderr); return 1;
        }
        ++sequences;
    }
    check(uc_close(u)); printf("Original Arcade policy: %u lap limits, %u track counts, %u deadlines, %u control/expiry gates, %u track sequences pass\n",cases,tracks,finishes,gates,sequences); return 0;
}
