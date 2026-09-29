#define main options_verifier_main
#include "verify_options_menu.c"
#undef main

static unsigned calls,image_handle,view;
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
        check(uc_mem_write(u,0x3cbf0,expected,sizeof expected));
        if(flag) expected_reset(expected);
        check(uc_emu_start(u,0x25552,0x25a05,0,4000));
        check(uc_mem_read(u,0x3cbf0,actual,sizeof actual));
        for(unsigned i=0;i<sizeof actual;++i) if(actual[i]!=expected[i]) {
            fprintf(stderr,"flag %u pattern %u DS:%04x got %u expected %u\n",flag,pattern,i,actual[i],expected[i]); abort();
        }
        check(uc_reg_read(u,UC_X86_REG_CS,&cs)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        check(uc_reg_read(u,UC_X86_REG_AX,&ax));
        if(cs*16U+ip!=(flag?0x25a05U:0x2555cU) || calls!=(flag?2U:0U) || (flag && (ax&255))) abort();
        ++cases;
    }
    check(uc_close(u));
    printf("Original demo return: %u flag/target cases verify full DS reset, saved-image restore arguments and branch/return value\n",cases);
    return 0;
}
