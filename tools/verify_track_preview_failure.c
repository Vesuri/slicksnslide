/* Execute the original failing resource lookup and its preview caller.
 * The captured DS supplies actual initialized resource bounds, not artwork.
 * Two valid synthetic allocation contents expose stale-buffer drawing. */
#define main track_info_verifier_main
#include "verify_track_info.c"
#undef main
static unsigned allocations,frees,failures,pixels,expected_colour;
static void failure_boundary(uc_engine *u,uint64_t a,uint32_t size,void *p)
{
    (void)size; (void)p;
    uint16_t ax=0,dx=0,ss,sp,cs,ip;
    if(a==0x35324) {
        check(uc_reg_read(u,UC_X86_REG_AX,&ax));
        REQUIRE((ax&255)==1); ++failures;
        return; /* Execute the real resource-loader epilogue and return. */
    }
    check(uc_reg_read(u,UC_X86_REG_SS,&ss));
    check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=ss*16U+sp;
    if(a==0x13b07) {
        REQUIRE(get(u,stack+4)==30000); dx=0x6000; ++allocations;
    } else if(a==0x139fd) {
        REQUIRE(get(u,stack+4)==0 && get(u,stack+6)==0x6000); ++frees;
    } else if(a==0x3b55e) {
        REQUIRE((get(u,stack+8)&255)==expected_colour); ++pixels;
    } else REQUIRE(0);
    ip=get(u,stack); cs=get(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(int argc,char **argv)
{
    unsigned char runtime[300000],data[28672];
    FILE *f=fopen("disasm/runtime.bin","rb"); REQUIRE(f);
    size_t n=fread(runtime,1,sizeof runtime,f); fclose(f); REQUIRE(n && n<sizeof runtime);
    f=fopen(argc>1?argv[1]:"tmp/pc-fixed/slicks-race-data.bin","rb"); REQUIRE(f);
    REQUIRE(fread(data,1,sizeof data,f)==sizeof data && fgetc(f)==EOF); fclose(f);
    unsigned count=data[0x1706]|data[0x1707]<<8;
    unsigned base=data[0x53ac]|data[0x53ad]<<8;
    unsigned types=data[0x53b0]|data[0x53b1]<<8;
    REQUIRE(count==886 && base==6 && types==110);
    f=fopen("ref/TRACKS/RAILROAD.SS","rb"); REQUIRE(f);
    length=(unsigned)fread(bytes,1,sizeof bytes,f); fclose(f); REQUIRE(length<sizeof bytes);
    REQUIRE(!slicks_track_preview_open(&view,bytes,length));
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n));
    word(u,0x3cbf0+0x1706,count); word(u,0x3cbf0+0x53ac,base); word(u,0x3cbf0+0x53b0,types);
    const unsigned hooks[]={0x13b07,0x139fd,0x35324,0x3b55e};
    for(unsigned i=0;i<sizeof hooks/sizeof hooks[0];++i) {
        uc_hook h; check(uc_hook_add(u,&h,UC_HOOK_CODE,failure_boundary,0,hooks[i],hooks[i]));
    }
    unsigned cases=0;
    for(unsigned i=0;i<view.count;++i) {
        unsigned type=view.objects[5*i+3],rotation=view.objects[5*i+4];
        if(type>=types || rotation<=3) continue;
        int resource=(short)(base+type+(signed char)rotation*types);
        REQUIRE(resource>0 && (unsigned)resource>count);
        for(unsigned pattern=1;pattern<=2;++pattern) {
            unsigned char scratch[30000],after[30000];
            memset(scratch,0,sizeof scratch); scratch[0]=2; scratch[1]=10;
            expected_colour=pattern*31; memset(scratch+2,expected_colour,80);
            check(uc_mem_write(u,0x60000,scratch,sizeof scratch));
            word(u,0x8f000,0); word(u,0x8f002,0x9000);
            word(u,0x8f004,100); word(u,0x8f006,100);
            word(u,0x8f008,type); word(u,0x8f00a,rotation);
            uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
            check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
            check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
            allocations=frees=failures=pixels=0;
            check(uc_emu_start(u,0x1a1d4,0x90000,0,1000000));
            check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
            REQUIRE(!ip && sp==0xf004 && allocations==1 && frees==1 && failures==1 && pixels>0);
            check(uc_mem_read(u,0x60000,after,sizeof after)); REQUIRE(!memcmp(scratch,after,sizeof scratch));
            ++cases;
        }
        printf("RAILROAD object %u type=%u bank=%u rejected resource=%d > %u; caller draws stale allocation\n",i,type,rotation,resource,count);
    }
    REQUIRE(cases==10); check(uc_close(u));
    printf("Original preview resource-failure: %u real-selector/scratch-content cases pass\n",cases);
    return 0;
}
