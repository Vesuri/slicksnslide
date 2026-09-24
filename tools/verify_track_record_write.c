#define main options_verifier_main
#include "verify_options_menu.c"
#undef main
#include "../src/game/track_records.h"
struct RecordFile { unsigned char bytes[400]; unsigned at,opened; };
static void record_io(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size; struct RecordFile *f=context;
    uint16_t sp,ss,ax=0,dx=0;
    check(uc_reg_read(u,UC_X86_REG_SP,&sp)); check(uc_reg_read(u,UC_X86_REG_SS,&ss));
    unsigned stack=(unsigned)ss*16+sp;
    if(address==0x11eaf) { if(f->opened) dx=0x6000; }
    else if(address==0x1219b) {
        unsigned offset=readword(u,stack+8)|(readword(u,stack+10)<<16);
        unsigned origin=readword(u,stack+12);
        if(origin>1) abort();
        f->at=offset+(origin?f->at:0); if(f->at>=sizeof f->bytes) abort();
    } else if(address==0x12d8a) {
        if(f->at>=sizeof f->bytes) abort(); ax=f->bytes[f->at++];
    } else if(address==0x123ce) {
        if(f->at>=sizeof f->bytes) abort();
        ax=readword(u,stack+4); f->bytes[f->at++]=(unsigned char)ax;
    } else if(address!=0x119c2) abort();
    uint16_t ip=readword(u,stack),cs=readword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
}
int main(void)
{
    unsigned char runtime[300000]; FILE *file=fopen("disasm/runtime.bin","rb"); if(!file) return 2;
    size_t n=fread(runtime,1,sizeof runtime,file); fclose(file);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,n)); struct RecordFile f;
    const unsigned hooks[]={0x11eaf,0x1219b,0x12d8a,0x123ce,0x119c2};
    for(unsigned i=0;i<5;++i) { uc_hook h; check(uc_hook_add(u,&h,UC_HOOK_CODE,record_io,&f,hooks[i],hooks[i])); }
    const short times[]={-32768,-1,0,1,2,3,100,32767}; unsigned cases=0;
    for(unsigned pattern=0;pattern<32;++pattern) for(unsigned a=0;a<8;++a) for(unsigned b=0;b<8;++b) {
        struct SlicksTrackRecords records,initial;
        for(unsigned i=0;i<11;++i) for(unsigned j=0;j<29;++j) records.entries[i][j]=(unsigned char)(pattern*17+i*31+j);
        records.entries[0][20]=(unsigned char)times[a]; records.entries[0][21]=(unsigned char)((unsigned short)times[a]>>8);
        records.entries[1][20]=(unsigned char)times[b]; records.entries[1][21]=(unsigned char)((unsigned short)times[b]>>8);
        records.trailer=(unsigned short)(pattern*2099); initial=records;
        unsigned char expected[400]; for(unsigned i=0;i<400;++i) expected[i]=f.bytes[i]=(unsigned char)(i*11+pattern);
        f.bytes[5]=expected[5]=pattern==31?1:2; f.at=0; f.opened=1;
        check(uc_mem_write(u,0x3cbf0+0x693a,records.entries,319)); word(u,0x3cbf0+0x4db2,records.trailer);
        uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000,ax,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        word(u,0x8f000,0); word(u,0x8f002,0x9000); word(u,0x8f004,0); word(u,0x8f006,0x7000);
        check(uc_emu_start(u,0x1a7ea,0x90000,0,1000000)); check(uc_reg_read(u,UC_X86_REG_AX,&ax)); check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        int result=slicks_write_track_records(expected,sizeof expected,&records);
        unsigned char original[319]; check(uc_mem_read(u,0x3cbf0+0x693a,original,sizeof original));
        if(ip || (ax&255)!=1 || result!=(pattern!=31) || memcmp(expected,f.bytes,sizeof expected) || memcmp(records.entries,original,sizeof original)) {
            fprintf(stderr,"Track writer mismatch pattern=%u first=%d second=%d\n",pattern,times[a],times[b]); return 1;
        }
        if(!pattern && !a && !b) for(unsigned capacity=0;capacity<363;++capacity) {
            unsigned char guarded[400],before[400]; memset(guarded,0xa5,sizeof guarded); guarded[5]=2; memcpy(before,guarded,400);
            records=initial;
            if(slicks_write_track_records(guarded,capacity,&records)!=-1 || memcmp(guarded,before,400) || memcmp(&records,&initial,sizeof records)) abort();
        }
        ++cases;
    }
    struct SlicksTrackRecords cleared; memset(&cleared,0xa5,sizeof cleared);
    check(uc_mem_write(u,0x3cbf0+0x693a,cleared.entries,319)); word(u,0x3cbf0+0x4db2,cleared.trailer);
    uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000;
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    check(uc_emu_start(u,0x1a96b,0x1a99f,0,10000));
    slicks_clear_track_records(&cleared);
    unsigned char original_clear[319]; check(uc_mem_read(u,0x3cbf0+0x693a,original_clear,sizeof original_clear));
    if(memcmp(cleared.entries,original_clear,sizeof original_clear) || cleared.trailer!=readword(u,0x3cbf0+0x4db2)) abort();
    unsigned char blank[400]; memset(blank,0xa5,sizeof blank); blank[5]=2;
    if(slicks_write_track_records(blank,sizeof blank,&cleared)!=1) abort();
    struct SlicksTrackRecords decoded;
    if(slicks_track_records(blank,sizeof blank,&decoded)!=1 ||
       memcmp(decoded.entries,cleared.entries,sizeof decoded.entries) || decoded.trailer) abort();
    for(unsigned i=0;i<8;++i) if(blank[i]!=(i==5?2:0xa5)) abort();
    for(unsigned i=363;i<sizeof blank;++i) if(blank[i]!=0xa5) abort();
    puts("Original pre-confirmation clear matches; cleared records decode with valid checksums and untouched track data");
    check(uc_close(u)); printf("Original track record writer: %u full-file/working-table comparisons; short buffers unchanged\n",cases);
    return 0;
}
