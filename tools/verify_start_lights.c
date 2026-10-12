/* Original position calculation and actual VGA light blits, including
 * source transparency, versus the public native constructor/presentation. */
#define main icon_fixture_main
#include "verify_menu_icon.c"
#undef main
#include "../src/game/race_runtime.c"
#include <assert.h>
static struct SlicksRaceRuntime race;
static unsigned char logical[0x40000],chunky[64000],background[64000];
static void longword(uc_engine *u,unsigned at,int value)
{ unsigned char b[4]={value,value>>8,value>>16,value>>24};check(uc_mem_write(u,at,b,4)); }
static void original_position(uc_engine *u,int x,int y,short *left,short *top)
{
    longword(u,0x3cbf0+0x5390,x);longword(u,0x3cbf0+0x53a0,y);
    uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xe000,bp=0xf000,ip;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    check(uc_reg_write(u,UC_X86_REG_BP,&bp));
    check(uc_emu_start(u,0x2519c,0x251f3,0,10000));
    check(uc_reg_read(u,UC_X86_REG_IP,&ip));assert(ip==0x251f3-0x19870);
    *left=(short)getword(u,0x8efd4);*top=(short)getword(u,0x8efd2);
}
int main(void)
{
    unsigned char runtime[300000];FILE *f=fopen("disasm/runtime.bin","rb");assert(f);
    size_t n=fread(runtime,1,sizeof runtime,f);fclose(f);
    uc_engine *u;check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));check(uc_mem_write(u,0x10100,runtime,n));
    const unsigned addresses[]={0x373a7,0x317af,0x12d8a,0x1221d,0x13b07,0x119c2,0x36243};uc_hook h;
    for(unsigned i=0;i<sizeof addresses/sizeof *addresses;++i)
        check(uc_hook_add(u,&h,UC_HOOK_CODE,services,0,addresses[i],addresses[i]));
    check(uc_hook_add(u,&h,UC_HOOK_INSN,port,0,1,0,UC_X86_INS_OUT));
    check(uc_hook_add(u,&h,UC_HOOK_MEM_WRITE,pixel,0,0xa0000,0xaffff));
    for(unsigned light=0;light<4;++light) {
        char name[]="lahto1.@I";name[5]+=(char)light;
        long size=host_archive_load("ref/SLICKS.000",name,source,sizeof source);assert(size>0);
        assert(!slicks_race_add_start_light(&race,light,source,(unsigned long)size));
        length=(unsigned)size;cursor=allocations=0;
        word(u,0x8f004,0);word(u,0x8f006,0x9000);word(u,0x8f008,0);word(u,0x8f00a,0x6000);word(u,0x8f00c,2);
        start(u,0x2e51a);
        unsigned char decoded[1024];check(uc_mem_read(u,0x50000,decoded,sizeof decoded));
        check(uc_mem_write(u,0x51000+light*1024,decoded,sizeof decoded));
    }
    race.navigation.zone_count=1;race.font.ready=1;race.participation_ready=1;
    for(unsigned v=0;v<SLICKS_VEHICLE_COUNT;++v) {
        race.properties[v].ready=1;
        for(unsigned d=0;d<SLICKS_CAR_BASE_DIRECTIONS;++d)race.sprites[v][d].ready=1;
    }
    const int xs[]={-99,0,99,9999,10000,12999,13000,20999,21000,21099,21100,23999,24000,31900};
    const int ys[]={-99,0,4999,5000,12999,18500};unsigned cases=0;
    for(unsigned xi=0;xi<sizeof xs/sizeof *xs;++xi)for(unsigned yi=0;yi<sizeof ys/sizeof *ys;++yi) {
        short left,top;original_position(u,xs[xi],ys[yi],&left,&top);
        for(unsigned y=0;y<200;++y)for(unsigned x=0;x<320;++x) {
            unsigned char c=(unsigned char)((y*320+x)*19+xi+yi);
            dos[y*320+x]=background[y*320+x]=chunky[y*320+x]=c;
            logical[y*100+x/4+(x&3)*0x10000]=c;
        }
        race.cars[0].x=xs[xi];race.cars[0].y=ys[yi];race.chunky_authoritative=0;
        race.start_light_stage_mask=0;
        assert(!slicks_race_start(&race,logical,chunky));
        assert(race.start_light_x==left && race.start_light_y==top);
        slicks_race_use_chunky_surface(&race);
        for(unsigned light=0;light<3;++light) {
            if(light)slicks_race_show_start_light(&race,(unsigned short)light);
            word(u,0x3cbf0+0x1d7b,100);word(u,0x8f004,left);word(u,0x8f006,top);
            word(u,0x8f008,0x1000+light*1024);word(u,0x8f00a,0x5000);word(u,0x8f00c,0);
            start(u,0x3aa7c);
            if(memcmp(dos,chunky,sizeof chunky)) {fprintf(stderr,"Light %u at %d,%d differs\n",light,left,top);return 1;}
            assert(race.countdown_stage==light && race.start_light_stage_mask==((1U<<(light+1))-1));
            ++cases;
        }
        restore_start_light(&race,0);assert(!memcmp(background,chunky,sizeof chunky));
    }
    check(uc_close(u));printf("Start lights: %u original positioned VGA frames and saved-background restorations match\n",cases);
    return 0;
}
