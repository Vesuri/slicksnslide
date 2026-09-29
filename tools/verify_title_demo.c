#define main options_verifier_main
#include "verify_options_menu.c"
#undef main
#include "../src/ui/title_demo.h"

static void compare(uc_engine *u,const struct SlicksConfiguration *config,
    const struct SlicksTrackPlaylist *playlist,unsigned long seed)
{
    for(unsigned i=0;i<15;++i)
        if((unsigned short)config->options[i]!=readword(u,0x3cbf0+0x92+8*i)) abort();
    for(unsigned i=0;i<4;++i)
        if((unsigned short)config->selected_profile[i]!=readword(u,0x3cbf0+0x44c+2*i)) abort();
    for(unsigned i=0;i<256;++i)
        if((unsigned short)playlist->tracks[i]!=readword(u,0x70000+2*i)) abort();
    if(playlist->count!=readword(u,0x3cbf0+0x90) ||
        seed!=(readword(u,0x3cbf0+0x2aaa)|((unsigned long)readword(u,0x3cbf0+0x2aac)<<16))) abort();
}
int main(void)
{
    unsigned char runtime[300000];FILE *file=fopen("disasm/runtime.bin","rb");if(!file)return 2;
    size_t size=fread(runtime,1,sizeof runtime,file);fclose(file);
    if(size<200000 || size==sizeof runtime)return 2;
    uc_engine *u;check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));check(uc_mem_write(u,0x10100,runtime,size));
    const unsigned totals[]={1,2,22,195,256,32767};unsigned cases=0;
    for(unsigned t=0;t<6;++t)for(unsigned count=0;count<=256;count+=64)
    for(unsigned pattern=0;pattern<32;++pattern) {
        struct SlicksConfiguration config={0},before;
        struct SlicksTitleDemo demo={0};short tracks[256],selection=6;
        struct SlicksTrackPlaylist playlist={tracks,(unsigned short)count,256};
        unsigned long seed=(pattern*0x9e3779b9UL+t)&0xffffffffUL;
        for(unsigned i=0;i<15;++i) {
            config.options[i]=(short)(pattern*2391+i*9013);
            word(u,0x3cbf0+0x92+8*i,(unsigned short)config.options[i]);
        }
        for(unsigned i=0;i<4;++i) {
            config.selected_profile[i]=(short)(pattern*391+i*139);
            word(u,0x3cbf0+0x44c+2*i,(unsigned short)config.selected_profile[i]);
        }
        for(unsigned i=0;i<256;++i) {tracks[i]=(short)(i*131+pattern);word(u,0x70000+2*i,(unsigned short)tracks[i]);}
        before=config;
        word(u,0x3cbf0+0x90,count);word(u,0x3cbf0+0x4da8,totals[t]);
        word(u,0x3cbf0+0x62a,0);word(u,0x3cbf0+0x62c,0x7000);
        word(u,0x3cbf0+0x2aaa,seed);word(u,0x3cbf0+0x2aac,seed>>16);
        regs(u,0);word(u,0x8effc,selection);
        /* Actual memcpy, RNG and integer-runtime helpers execute. */
        check(uc_emu_start(u,0x2a3db,0x2a566,0,10000));
        if(slicks_title_demo_begin(&demo,&config,&playlist,totals[t],&seed,&selection))abort();
        compare(u,&config,&playlist,seed);
        if(selection || readword(u,0x8effc) || !demo.active ||
            (readword(u,0x3cbf0+0x459)&255)!=255)abort();
        /* Restore after intervening edits, including a tail entry. The
         * original restores only entry zero; unrelated state survives. */
        config.options[2]=-19;word(u,0x3cbf0+0xa2,65517);
        config.selected_profile[3]=2;word(u,0x3cbf0+0x452,2);
        tracks[7]=42;word(u,0x7000e,42);
        unsigned char refresh=71;check(uc_mem_write(u,0x3cbf0+0x1146,&refresh,1));
        regs(u,0);check(uc_emu_start(u,0x2a2f4,0x2a344,0,10000));
        slicks_title_demo_restore(&demo,&config,&playlist,&refresh);
        compare(u,&config,&playlist,seed);
        if(memcmp(&config,&before,sizeof config) || demo.active || refresh!=2 ||
            (readword(u,0x3cbf0+0x1148)&255))abort();
        ++cases;
    }
    /* Native invalid-data boundaries must not leave a half-applied demo. */
    struct SlicksConfiguration config={0},before=config;
    struct SlicksTitleDemo demo={0};short tracks[2]={123,456},selection=6;
    struct SlicksTrackPlaylist playlist={tracks,0,2};unsigned long seed=1234;
    if(slicks_title_demo_begin(&demo,&config,&playlist,0,&seed,&selection)!=-1 ||
        slicks_title_demo_begin(&demo,&config,&playlist,32768,&seed,&selection)!=-1 ||
        slicks_title_demo_begin(&demo,&config,&playlist,195,0,&selection)!=-1)abort();
    playlist.capacity=0;
    if(slicks_title_demo_begin(&demo,&config,&playlist,195,&seed,&selection)!=-1)abort();
    playlist.capacity=2;playlist.count=3;
    if(slicks_title_demo_begin(&demo,&config,&playlist,195,&seed,&selection)!=-1)abort();
    playlist.count=0;demo.active=1;
    if(slicks_title_demo_begin(&demo,&config,&playlist,195,&seed,&selection)!=-1)abort();
    demo.active=0;unsigned char refresh=71;
    slicks_title_demo_restore(&demo,&config,&playlist,&refresh);
    if(memcmp(&config,&before,sizeof config) || tracks[0]!=123 || tracks[1]!=456 ||
        selection!=6 || seed!=1234 || playlist.count || refresh!=71 || demo.active)abort();
    check(uc_close(u));printf("Original demo setup/restore: %u complete option/profile/playlist/RNG pairs pass; native guards atomic\n",cases);
    return 0;
}
