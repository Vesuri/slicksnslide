#define main options_verifier_main
#include "verify_options_menu.c"
#undef main
#include "../src/ui/title_demo.h"

static void key_boundary(uc_engine *u,uint64_t address,uint32_t size,void *opaque)
{
    (void)size;(void)opaque;
    /* Stop at pixel diagnostic, other-mode delay, normal dispatch or exit.
     * No callback is executed/replaced; these are classification boundaries. */
    if(address==0x23f87 || address==0x23ff1 || address==0x23ff9 || address==0x240fc)
        check(uc_emu_stop(u));
}

struct DemoBranch { unsigned start,taken,fallthrough,predicate; };
static void demo_branch_stop(uc_engine *u,uint64_t address,uint32_t size,void *opaque)
{
    (void)size;
    const struct DemoBranch *b=opaque;
    if(address==b->taken || address==b->fallthrough) check(uc_emu_stop(u));
}
/* Reader inventory from the original instruction image. Stop before either
 * branch's side effects: this proves flag classification, not those callees. */
static unsigned verify_demo_consumers(uc_engine *u)
{
    static const struct DemoBranch branches[]={
        {0x19db0,0x19dbd,0x19db8,0}, {0x1b540,0x1b562,0x1b547,1},
        {0x1b562,0x1b58f,0x1b56a,0}, {0x25067,0x25074,0x2506f,2},
        {0x25316,0x2532a,0x2531d,1}, {0x25552,0x2555c,0x25559,3},
        {0x25c05,0x25c11,0x25c0c,1}, {0x25d76,0x25d8e,0x25d7d,1},
        {0x25eaa,0x25eb4,0x25eb1,3}, {0x26175,0x26185,0x2617c,1},
        {0x26304,0x26348,0x2630c,4}
    };
    unsigned cases=0;
    for(unsigned i=0;i<sizeof branches/sizeof branches[0];++i) {
        const struct DemoBranch *b=&branches[i]; uc_hook hook;
        check(uc_hook_add(u,&hook,UC_HOOK_CODE,demo_branch_stop,(void *)b,1,0));
        for(unsigned flag=0;flag<256;++flag) {
            regs(u,0); uint16_t cs=0x1987,ip;
            check(uc_reg_write(u,UC_X86_REG_CS,&cs));
            unsigned char byte=(unsigned char)flag;
            check(uc_mem_write(u,0x3cbf0+0x459,&byte,1));
            check(uc_emu_start(u,b->start,0x90000,0,20));
            check(uc_reg_read(u,UC_X86_REG_CS,&cs));
            check(uc_reg_read(u,UC_X86_REG_IP,&ip));
            int signed_flag=(signed char)flag;
            int taken=b->predicate==0?signed_flag>=0:b->predicate==1?flag!=0:
                b->predicate==2?signed_flag>0:b->predicate==3?flag==0:signed_flag<=0;
            unsigned expected=taken?b->taken:b->fallthrough;
            if(16U*cs+ip!=expected) {
                fprintf(stderr,"Demo consumer %x flag=%u reached %x expected %x\n",
                    b->start,flag,16U*cs+ip,expected);abort();
            }
            ++cases;
        }
        check(uc_hook_del(u,hook));
    }
    return cases;
}

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
    uc_hook hook;check(uc_hook_add(u,&hook,UC_HOOK_CODE,key_boundary,0,0x23f65,0x240fc));
    unsigned keys=0;
    for(unsigned flag=0;flag<256;++flag)for(unsigned scan=0;scan<256;++scan) {
        regs(u,0);uint16_t cs=0x1987;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));
        word(u,0x3cbf0+0x459,flag);word(u,0x8ef8c,scan);
        unsigned char result=0;check(uc_mem_write(u,0x8efc9,&result,1));
        check(uc_emu_start(u,0x23f65,0x24100,0,100));
        check(uc_mem_read(u,0x8efc9,&result,1));
        if(result!=slicks_title_demo_exit_key((signed char)flag,(short)scan))abort();
        ++keys;
    }
    const unsigned long times[]={0,19999,20000,20001,20999,21000,
        65535,65536,0x7fffffffUL,0x80000000UL,0xffff0000UL,0xffffffffUL};
    unsigned timer_cases=0;
    for(unsigned start=0;start<12;++start)for(unsigned delta=0;delta<12;++delta)
    for(unsigned scan=0;scan<256;++scan) {
        unsigned long now=(times[start]+times[delta])&0xffffffffUL;
        regs(u,0);uint16_t ax=(uint16_t)now,dx=(uint16_t)(now>>16);
        check(uc_reg_write(u,UC_X86_REG_AX,&ax));check(uc_reg_write(u,UC_X86_REG_DX,&dx));
        word(u,0x8eff8,times[start]);word(u,0x8effa,times[start]>>16);
        word(u,0x8eff2,scan);
        /* Clock acquisition is the boundary; the original arithmetic and
         * conditional scan replacement execute without a model stub. */
        check(uc_emu_start(u,0x2a387,0x2a39d,0,100));
        if(readword(u,0x8eff2)!=slicks_title_demo_scan(scan,now,times[start]))abort();
        ++timer_cases;
    }
    unsigned consumers=verify_demo_consumers(u);
    check(uc_close(u));printf("Original demo setup/restore: %u state pairs; atomic guards; %u key cases; %u timer cases; %u consumer branches pass\n",cases,keys,timer_cases,consumers);
    return 0;
}
