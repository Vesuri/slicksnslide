#define main options_verifier_main
#include "verify_options_menu.c"
#undef main
#include "../src/game/race_return.h"

static unsigned calls,image_handle,view;
static unsigned getword(const unsigned char *p,unsigned at)
{ return p[at]|(unsigned)p[at+1]<<8; }
static int32_t getlong(const unsigned char *p,unsigned at)
{ return (int32_t)(getword(p,at)|(uint32_t)getword(p,at+2)<<16); }
/* Map only original reset fields and their native derived views. Everything
 * else is deliberately filled with a sentinel and must remain byte-exact. */
static void map_reset_state(struct SlicksRaceRuntime *r,const unsigned char *p)
{
    r->finished_count=0;r->finish_ranks_ready=1;r->participation_ready=1;
    for(unsigned d=0;d<4;++d) {
        unsigned at=54*d;struct SlicksRaceCar *c=&r->cars[d];
        r->participation[d]=(signed char)p[0x4bc6+d];
        c->pending_damage_impact=getlong(p,0x304b+at);
        c->damage_turn_sign=(signed char)p[0x3057+at];c->service_flags=p[0x305e +at];
        c->special_drive_target=(short)getword(p,0x305a+at);
        c->special_drive_state=(short)getword(p,0x3058+at);
        c->speed_fixed=getlong(p,0x303f+at);c->speed=(short)(c->speed_fixed/100);
        c->last_lap_time_units=(unsigned int)getlong(p,0x3037+at);
        unsigned word=(unsigned short)c->last_lap_time_units;
        c->last_lap_centiseconds=(unsigned short)((word>17999?17999:word)*5/9);
        c->checkpoint=(unsigned short)getword(p,0x305c+at);
        for(unsigned j=0;j<4;++j)c->damage[j]=(short)getword(p,0x304f+at+2*j);
        r->finish_ranks[d]=(signed char)p[0x4bce +d];
        c->finished=(unsigned char)(r->finish_ranks[d]>0);
        c->finish_position=c->finished?(unsigned char)r->finish_ranks[d]:0;
        r->finished_count+=c->finished;
        c->ai_contact_threshold=(unsigned short)getword(p,0x2fa4+2*d);
        c->old_x=(unsigned short)getword(p,0x53b6+2*d);c->old_y=(unsigned char)getword(p,0x53be +2*d);
        c->velocity_x=getlong(p,0x682e +4*d);c->velocity_y=getlong(p,0x683e +4*d);
        c->ai_contact_ticks=(unsigned short)getword(p,0x53c6+2*d);
        c->lap=(unsigned short)(getword(p,0x4bfe +2*d)+1);
        c->previous_actor_contact=p[0x5370+d];c->actor_contact=p[0x536c+d];
        c->best_lap_time_units=(unsigned int)getlong(p,0x4c06+4*d);
        word=(unsigned short)c->best_lap_time_units;
        c->best_lap_centiseconds=(unsigned short)((word>17999?17999:word)*5/9);
        c->oil_turn_sign=(signed char)p[0x4c78+d];
        c->steering_scale=(short)getword(p,0x4c3a+2*d);
        c->touching_car=p[0x4dae +d];c->forward_drive_latch=p[0x5374+d];
        c->maximum_speed=(unsigned short)getword(p,0x4c42+2*d);
        c->collision_sampling=p[0x4daa+d];
        r->car_display[d].direction=(signed char)p[0x3068+at];r->car_display[d].ticks=p[0x3069+at];
    }
    r->boundary_level=(short)getword(p,0x4c6c);
}
static void putword(unsigned char *data,unsigned at,unsigned value)
{ data[at]=value; data[at+1]=value>>8; }
static void expected_reset(unsigned char *data)
{
    static const unsigned zeros[]={0x304d,0x304b,0x305a,0x3041,0x303f,
        0x3058,0x3039,0x3037,0x305c,0x304f,0x3051,0x3053,0x3055};
    for(unsigned d=0;d<4;++d) {
        for(unsigned j=0;j<sizeof zeros/sizeof *zeros;++j)
            putword(data,zeros[j]+d*0x36,0);
        data[0x3057+d*0x36]=data[0x305e +d*0x36]=data[0x3069+d*0x36]=0;
        data[0x3068+d*0x36]=255;
        data[0x4bce +d]=data[0x4bc6+d]?255:0;
        putword(data,0x2fa4+2*d,350);
        putword(data,0x53b6+2*d,0); putword(data,0x53be +2*d,0);
        memset(data+0x682e +4*d,0,4); memset(data+0x683e +4*d,0,4);
        putword(data,0x53c6+2*d,60000); putword(data,0x4bfe +2*d,0);
        data[0x5370+d]=data[0x536c+d]=data[0x4c78+d]=0;
        putword(data,0x4c06+4*d,30000); putword(data,0x4c08+4*d,0);
        putword(data,0x4c3a+2*d,1000); putword(data,0x4c42+2*d,100);
        data[0x4dae +d]=data[0x5374+d]=data[0x4daa+d]=0;
    }
    putword(data,0x4c6c,5);
}
static void return_boundary(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)size;(void)p;
    if(address==0x2555c) { check(uc_emu_stop(u)); return; }
    if(address==0x1c10b) { if(calls++) abort(); return; }
    if(address!=0x34f15) return;
    if(calls++!=1) abort();
    uint16_t ss,sp; check(uc_reg_read(u,UC_X86_REG_SS,&ss));
    check(uc_reg_read(u,UC_X86_REG_SP,&sp)); unsigned at=16U*ss+sp;
    if(readword(u,at+4)!=image_handle || readword(u,at+18)!=view) abort();
    for(unsigned i=1;i<7;++i) if(readword(u,at+4+2*i)) abort();
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f)return 2;
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f); if(n<200000 || n==sizeof runtime)return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,n));
    /* Execute the complete race-state reset. Only the saved-image device
     * restore is stubbed; its ordered call and geometry are checked. */
    unsigned char retf=0xcb;
    check(uc_mem_write(u,0x34f15,&retf,1));
    uc_hook hook; check(uc_hook_add(u,&hook,UC_HOOK_CODE,return_boundary,0,1,0));
    unsigned cases=0;
    for(unsigned flag=0;flag<256;++flag) for(unsigned pattern=0;pattern<4;++pattern) {
        regs(u,0); uint16_t cs=0x1987,ip,ax=0x1234;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_AX,&ax));
        unsigned char expected[65536],actual[65536];
        for(unsigned i=0;i<sizeof expected;++i) expected[i]=(i*73+pattern*31+flag)^ (i>>8);
        expected[0x459]=(unsigned char)flag;
        for(unsigned d=0;d<4;++d) expected[0x4bc6+d]=(unsigned char[]){0,1,255,127}[(d+pattern)%4];
        image_handle=0x4567+pattern*991; view=0xa000+pattern*127;
        putword(expected,0x4c1c,image_handle); putword(expected,0x1d87,view); calls=0;
        static struct SlicksRaceRuntime native,want;
        memset(&native,0xa5,sizeof native);map_reset_state(&native,expected);want=native;
        check(uc_mem_write(u,0x3cbf0,expected,sizeof expected));
        if(flag) expected_reset(expected);
        check(uc_emu_start(u,0x25552,0x25a05,0,4000));
        check(uc_mem_read(u,0x3cbf0,actual,sizeof actual));
        if(flag) slicks_race_reset_return(&native);
        map_reset_state(&want,actual);
        if(memcmp(&native,&want,sizeof native)) {
            fprintf(stderr,"Native return reset mismatch flag=%u pattern=%u\n",flag,pattern);abort();
        }
        for(unsigned i=0;i<sizeof actual;++i) if(actual[i]!=expected[i]) {
            fprintf(stderr,"flag %u pattern %u DS:%04x got %u expected %u\n",flag,pattern,i,actual[i],expected[i]); abort();
        }
        check(uc_reg_read(u,UC_X86_REG_CS,&cs)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        check(uc_reg_read(u,UC_X86_REG_AX,&ax));
        if(cs*16U+ip!=(flag?0x25a05U:0x2555cU) || calls!=(flag?2U:0U) || (flag && (ax&255))) abort();
        ++cases;
    }
    check(uc_close(u));
    printf("Original demo return: %u flag/target cases verify full DS reset, native reset and preservation of unrelated state, saved-image restore arguments and branch/return value\n",cases);
    return 0;
}
