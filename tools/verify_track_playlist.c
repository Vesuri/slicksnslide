#define main options_verifier_main
#include "verify_options_menu.c"
#undef main
#include "../src/game/track_playlist.h"
#include "../src/ui/title_start.h"
static void name_boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)address; (void)size; (void)p;
    uint16_t sp,ss; check(uc_reg_read(u,UC_X86_REG_SP,&sp)); check(uc_reg_read(u,UC_X86_REG_SS,&ss));
    uint16_t ip=readword(u,ss*16U+sp),cs=readword(u,ss*16U+sp+2),ax=0,dx=0x6000; sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,n));
    uc_hook h; check(uc_hook_add(u,&h,UC_HOOK_CODE,name_boundary,0,0x35e63,0x35e63));
    const unsigned counts[]={0,1,2,22,195,256,257,300}; unsigned cases=0;
    for(unsigned op=0;op<5;++op) for(unsigned c=0;c<8;++c) for(unsigned pattern=0;pattern<16;++pattern) {
        unsigned total=counts[c],initial=total/2;
        if(op==4) initial=total;
        short list[512]; for(unsigned i=0;i<512;++i) list[i]=(short)(i<initial?initial-1-i:0x7777);
        for(unsigned i=0;i<512;++i) word(u,0x70000+2*i,(unsigned short)list[i]);
        unsigned long seed=(pattern*0x9e3779b9UL+c)&0xffffffffUL;
        word(u,0x3cbf0+0x2aaa,seed); word(u,0x3cbf0+0x2aac,seed>>16);
        word(u,0x3cbf0+0x62a,0); word(u,0x3cbf0+0x62c,0x7000);
        word(u,0x3cbf0+0x90,initial); word(u,0x3cbf0+0x4da8,total);
        unsigned wanted=pattern&1?total:total/2; word(u,0x3cbf0+0x626,wanted);
        short track=(short)(pattern% (total?total:1)); word(u,0x3cbf0+0x10b2,track);
        unsigned char name=(pattern&2)?0:'T'; check(uc_mem_write(u,0x60000,&name,1));
        regs(u,0);
        struct SlicksTrackPlaylist p={list,(unsigned short)initial,512}; int result;
        unsigned start=0,end=0x278f1;
        if(op==0) { start=0x27583; result=slicks_track_playlist_toggle(&p,track,name); }
        else if(op==1) { start=0x276bb; result=slicks_track_playlist_all(&p,total); }
        else if(op==2) { start=0x27705; result=slicks_track_playlist_all(&p,0); }
        else if(op==3) { start=0x2771c; result=slicks_track_playlist_random(&p,total,wanted,&seed); }
        else {
            start=0x26d34; end=0x90000; uint16_t sp=0xf000;
            check(uc_reg_write(u,UC_X86_REG_SP,&sp)); word(u,0x8f000,0); word(u,0x8f002,0x9000);
            result=slicks_track_playlist_shuffle(&p,&seed);
        }
        check(uc_emu_start(u,start,end,0,30000000));
        uint16_t stopped_cs,stopped_ip;
        check(uc_reg_read(u,UC_X86_REG_CS,&stopped_cs));check(uc_reg_read(u,UC_X86_REG_IP,&stopped_ip));
        if(16U*stopped_cs+stopped_ip!=end) { fprintf(stderr,"Playlist oracle instruction limit\n");return 1; }
        unsigned long actual_seed=readword(u,0x3cbf0+0x2aaa)|((unsigned long)readword(u,0x3cbf0+0x2aac)<<16);
        if(result || p.count!=readword(u,0x3cbf0+0x90) || seed!=actual_seed) {
            fprintf(stderr,"Playlist state mismatch op=%u total=%u pattern=%u count=%u/%u seed=%lx/%lx\n",op,total,pattern,p.count,readword(u,0x3cbf0+0x90),seed,actual_seed); return 1;
        }
        for(unsigned i=0;i<512;++i) if((unsigned short)list[i]!=readword(u,0x70000+2*i)) {
            fprintf(stderr,"Playlist data mismatch op=%u total=%u pattern=%u index=%u\n",op,total,pattern,i); return 1;
        }
        ++cases;
    }
    unsigned starts=0;
    for(unsigned action=2;action<=4;action+=2) for(unsigned flag=0;flag<3;++flag)
    for(unsigned c=0;c<8;++c) for(unsigned pattern=0;pattern<16;++pattern) {
        unsigned count=counts[c];
        short list[512];
        for(unsigned i=0;i<512;++i) {
            list[i]=(short)(i<count?count-1-i:0x7777);
            word(u,0x70000+2*i,(unsigned short)list[i]);
        }
        unsigned long seed=(pattern*0x9e3779b9UL+c)&0xffffffffUL;
        word(u,0x3cbf0+0x2aaa,seed);word(u,0x3cbf0+0x2aac,seed>>16);
        word(u,0x3cbf0+0x62a,0);word(u,0x3cbf0+0x62c,0x7000);
        word(u,0x3cbf0+0x90,count);word(u,0x3cbf0+0x624,flag==2?255:flag);
        regs(u,0);word(u,0x8effe,0);
        check(uc_emu_start(u,action==2?0x2a4e7:0x2a4c5,
            action==2?0x2a537:0x2a566,0,5000000));
        struct SlicksTrackPlaylist p={list,(unsigned short)count,512};
        if(slicks_title_start_shuffle(&p,action,flag==2?255:flag,&seed) ||
            seed!=(readword(u,0x3cbf0+0x2aaa)|((unsigned long)readword(u,0x3cbf0+0x2aac)<<16)) ||
            readword(u,0x3cbf0+0x90)!=count || (readword(u,0x8effe)>>8)!=99) abort();
        for(unsigned i=0;i<512;++i) if((unsigned short)list[i]!=readword(u,0x70000+2*i)) abort();
        ++starts;
    }
    printf("Original GO/F9 caller: %u playlist/RNG/return comparisons pass\n",starts);
    /* Native capacity guards must reject before changing list, count or RNG. */
    short guarded[3]={0,1,123},before[3]; memcpy(before,guarded,sizeof guarded);
    struct SlicksTrackPlaylist full={guarded,2,2}; unsigned long seed=1234;
    if(slicks_track_playlist_toggle(&full,2,1)!=-1 ||
       slicks_track_playlist_all(&full,3)!=-1 ||
       slicks_track_playlist_random(&full,2,3,&seed)!=-1 ||
       slicks_track_playlist_random(&full,3,3,&seed)!=-1 ||
       full.count!=2 || seed!=1234 || memcmp(before,guarded,sizeof guarded)) abort();
    full.count=3;
    if(slicks_track_playlist_shuffle(&full,&seed)!=-1 || seed!=1234 ||
       full.count!=3 || memcmp(before,guarded,sizeof guarded)) abort();
    check(uc_close(u)); printf("Original track playlists: %u complete-array/count/RNG comparisons for toggle, All, None, unique random and full-list shuffle; capacity guards atomic\n",cases);
    return 0;
}
