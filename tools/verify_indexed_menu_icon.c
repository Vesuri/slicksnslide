#define main verify_rgb_icons_main
#include "verify_menu_icon.c"
#undef main
#define abort() do { fprintf(stderr,"indexed test failure at line %d (cursor=%u length=%u allocations=%u)\n",__LINE__,cursor,length,allocations); exit(1); } while(0)

static void verify_indexed(uc_engine *u,const char *name)
{
    unsigned char pixels[2048],original[2051]; unsigned short w=0,h=0;
    if(slicks_decode_indexed_menu_icon(source,length,pixels,sizeof pixels,&w,&h)) abort();
    cursor=allocations=0;
    word(u,0x8f004,0); word(u,0x8f006,0x9000);
    word(u,0x8f008,0); word(u,0x8f00a,0x6000); word(u,0x8f00c,2);
    start(u,0x2e51a);
    unsigned stride=(w+3)/4,plane=stride*h;
    if(cursor!=length || allocations!=1 || getword(u,0x60002)!=0x5000) abort();
    check(uc_mem_read(u,0x50000,original,4*plane+3));
    if(original[0]!=stride || original[1]!=h || original[4*plane+2]!=stride*4-w) abort();
    for(unsigned y=0;y<h;++y) for(unsigned x=0;x<stride*4;++x) {
        unsigned char expected=x<w?pixels[y*w+x]:0;
        if(original[2+(x&3)*plane+y*stride+x/4]!=expected) {
            fprintf(stderr,"Indexed icon mismatch %s x=%u y=%u\n",name,x,y); exit(1);
        }
    }
    /* Original 3aa7c skips index zero. Exercise all source plane rotations
     * with patterned backgrounds, including padded source columns. */
    if(w<=300) for(unsigned alignment=0;alignment<4;++alignment) {
        unsigned char expected[64000]; unsigned left=9+alignment,top=31;
        for(unsigned i=0;i<64000;++i) dos[i]=expected[i]=(unsigned char)(i*19+alignment);
        for(unsigned y=0;y<h;++y) for(unsigned x=0;x<w;++x)
            if(pixels[y*w+x]) expected[(top+y)*320+left+x]=pixels[y*w+x];
        word(u,0x3cbf0+0x1d7b,100);
        word(u,0x8f004,left); word(u,0x8f006,top);
        word(u,0x8f008,0); word(u,0x8f00a,0x5000); word(u,0x8f00c,0);
        start(u,0x3aa7c);
        if(memcmp(dos,expected,sizeof dos)) { fprintf(stderr,"Indexed draw mismatch %s alignment=%u\n",name,alignment); exit(1); }
    }
    for(unsigned cut=0;cut<length;++cut) {
        memset(pixels,0xa5,sizeof pixels); w=0x1234; h=0x5678;
        if(slicks_decode_indexed_menu_icon(source,cut,pixels,sizeof pixels,&w,&h)!=-1 ||
           w!=0x1234 || h!=0x5678) abort();
        for(unsigned i=0;i<sizeof pixels;++i) if(pixels[i]!=0xa5) abort();
    }
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f); fclose(f);
    if(bytes<200000 || bytes==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,bytes));
    const unsigned addresses[]={0x373a7,0x317af,0x12d8a,0x1221d,0x13b07,0x119c2,0x36243};
    for(unsigned i=0;i<sizeof addresses/sizeof addresses[0];++i) {
        uc_hook hook; check(uc_hook_add(u,&hook,UC_HOOK_CODE,services,0,addresses[i],addresses[i]));
    }
    uc_hook hook;
    check(uc_hook_add(u,&hook,UC_HOOK_INSN,port,0,1,0,UC_X86_INS_OUT));
    check(uc_hook_add(u,&hook,UC_HOOK_MEM_WRITE,pixel,0,0xa0000,0xaffff));
    const char *names[]={"val1.@I","val2.@I","pel_on.@I","pel_ei.@I","pel_t.@I",
        "ohj_key.@I","ohj_joy.@I","ohj_lptc.@I",
        "miina.ase","aikabomb.ase","flam_raj.@I",
        "ohjus.1","ohjus.2","ohjus.3","ohjus.4","ohjus.5","ohjus.6","ohjus.7","ohjus.8",
        "savu.1","savu.2","savu.3","rajahdys.1","rajahdys.2","rajahdys.3","rajahdys.4","flash.@I"};
    for(unsigned i=0;i<sizeof names/sizeof names[0];++i) {
        long n=host_archive_load("ref/SLICKS.000",names[i],source,sizeof source); if(n<0) return 2;
        length=(unsigned)n; verify_indexed(u,names[i]);
    }
    /* Synthetic streams exercise raw pixels, a literal escape, cross-row runs
     * and the ninth width bit against the actual original decoder. */
    for(unsigned format=0;format<4;++format) {
        unsigned w=(format&1)?257:17,h=3; length=0;
        source[length++]=format; source[length++]=w; source[length++]=h;
        if(format&2) source[length++]=1;
        for(unsigned i=0;i<w*h;++i) {
            unsigned char value=(unsigned char)(i*37);
            source[length++]=value;
            if((format&2) && value==1) source[length++]=0;
        }
        verify_indexed(u,"synthetic");
    }
    check(uc_close(u));
    puts("Indexed title/controller/weapon assets: 27 resources and four format variants match original loader pixels, padding and dimensions; 124 transparent full-frame draws match across all alignments; every truncation rejected atomically");
    return 0;
}
