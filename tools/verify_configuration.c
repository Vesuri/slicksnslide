#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/configuration.h"
#include "../src/gen/setup_defaults.h"
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned a,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(u,a,b,2)); }
static unsigned readword(uc_engine *u,unsigned a)
{ unsigned char b[2]; check(uc_mem_read(u,a,b,2)); return b[0]|b[1]<<8; }
struct Stream { unsigned char data[142]; unsigned at,available; };
static void io(uc_engine *u,uint64_t address,uint32_t size,void *p)
{
    (void)size; struct Stream *s=p; uint16_t ss,sp,cs,ip,ax=0,dx=0;
    check(uc_reg_read(u,UC_X86_REG_SS,&ss)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    unsigned stack=16U*ss+sp;
    if(address==0x11eaf) dx=s->available?0x9100:0;
    else if(address==0x12d8a) { if(s->at>=142) abort(); ax=s->data[s->at++]; }
    else if(address==0x123ce) { if(s->at>=142) abort(); s->data[s->at++]=(unsigned char)readword(u,stack+4); }
    ip=readword(u,stack); cs=readword(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
static void put(unsigned char *data,unsigned offset,unsigned value)
{ data[offset]=value; data[offset+1]=value>>8; }
static void project(unsigned char *data,const struct SlicksConfiguration *c)
{
    data[0x5e1]=c->field_05e1; data[0x62e]=c->field_062e;
    put(data,0x3032,(unsigned short)c->date_code); put(data,0x5de,(unsigned short)c->field_05de);
    put(data,0x626,(unsigned short)c->field_0626);
    memcpy(data+0x5358,c->keys,20); memcpy(data+0x5e2,c->player_input,4);
    memcpy(data+0x5e6,c->bindings,60);
    for(unsigned i=0;i<15;++i) put(data,0x92+8*i,(unsigned short)c->options[i]);
    for(unsigned i=0;i<4;++i) put(data,0x44c+2*i,(unsigned short)c->selected_profile[i]);
    put(data,0x172c,(unsigned short)c->field_172c); put(data,0x172e,(unsigned short)c->field_172e);
    put(data,0x1728,(unsigned short)c->field_1728); put(data,0x172a,(unsigned short)c->field_172a);
    put(data,0x719c,(unsigned short)c->field_719c); put(data,0x719e,(unsigned short)c->field_719e);
    data[0x17c0]=slicks_configuration_volume(c);
}
static void verify_save(uc_engine *u,struct Stream *stream,unsigned char signature)
{
    static unsigned char ds_data[65536];
    for(unsigned pattern=0;pattern<256;++pattern) {
        struct SlicksConfiguration config;
        for(unsigned i=0;i<sizeof config;++i) ((unsigned char *)&config)[i]=(unsigned char)(pattern+i*37);
        project(ds_data,&config); ds_data[0x6c0]=1;
        check(uc_mem_write(u,0x3cbf0,ds_data,sizeof ds_data));
        stream->at=0; stream->available=1;
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x2b097,0x2b2ca,0,100000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip));
        unsigned char encoded[144]; memset(encoded,0xa5,sizeof encoded);
        if(cs*16U+ip!=0x2b2ca || stream->at!=142 ||
           slicks_save_configuration(&config,encoded,142,signature)!=142 ||
           memcmp(encoded,stream->data,142) || encoded[142]!=0xa5 || encoded[143]!=0xa5) {
            fprintf(stderr,"Configuration save mismatch pattern=%u bytes=%u\n",pattern,stream->at); exit(1);
        }
        for(unsigned capacity=0;capacity<142;++capacity) {
            memset(encoded,0xa5,sizeof encoded);
            if(slicks_save_configuration(&config,encoded,capacity,signature)!=-1) abort();
            for(unsigned i=0;i<sizeof encoded;++i) if(encoded[i]!=0xa5) abort();
        }
    }
    puts("DOS configuration save: 256 complete 142-byte streams match original byte/word writers; short output buffers stay untouched");
}

static void verify_defaults(uc_engine *u,const unsigned char *runtime)
{
    static unsigned char seed[65536],expected[65536],actual[65536];
    for(unsigned pattern=0;pattern<256;++pattern) {
        memset(seed,0xa5,sizeof seed);
        for(unsigned i=0;i<0x2fa4;++i)
            seed[i]=runtime[0x3cbf0-0x10100+i]^(unsigned char)pattern;
        memcpy(expected,seed,sizeof seed);
        memset(expected+0x2fa4,0,0x7822-0x2fa4);
        struct SlicksConfiguration c;
        memset(&c,0x55,sizeof c);
        if(slicks_configuration_defaults(&c,seed,0x2fa4)) abort();
        project(expected,&c);
        if(!pattern) {
            /* Compile the generated typed resource, then compare its complete
             * configuration projection to the independently decoded input. */
            memcpy(actual,expected,sizeof actual);
            project(actual,&slicks_original_configuration);
            if(memcmp(actual,expected,sizeof actual)) {
                fputs("Generated setup defaults differ from original data\n",stderr);
                exit(1);
            }
        }
        /* Volume application belongs to the later CFG reader. */
        expected[0x17c0]=seed[0x17c0];
        check(uc_mem_write(u,0x3cbf0,seed,sizeof seed));
        uint16_t cs=0x1010,ds=0x3cbf,ss=0x8000,sp=0xf000,bp=0xf000;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));
        check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss));
        check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_reg_write(u,UC_X86_REG_BP,&bp));
        /* The entry instructions install the data-segment selector used by
         * the clear loop. Skip DOS environment/allocation setup between them. */
        check(uc_emu_start(u,0x10100,0x10108,0,100000));
        check(uc_emu_start(u,0x101ae,0x101c0,0,100000));
        cs=0x2468;
        check(uc_reg_write(u,UC_X86_REG_CS,&cs));
        check(uc_emu_start(u,0x25b2c,0x25b44,0,100000));
        check(uc_emu_start(u,0x25bc9,0x25bfb,0,100000));
        check(uc_mem_read(u,0x3cbf0,actual,sizeof actual));
        if(memcmp(actual,expected,sizeof actual)) {
            for(unsigned i=0;i<sizeof actual;++i) if(actual[i]!=expected[i]) {
                fprintf(stderr,"startup defaults pattern=%u DS:%04x native=%02x DOS=%02x\n",
                    pattern,i,expected[i],actual[i]); break;
            }
            exit(1);
        }
    }
    struct SlicksConfiguration before,after;
    memset(&before,0x55,sizeof before);
    for(unsigned size=0;size<0x2fa4;++size) {
        after=before;
        if(slicks_configuration_defaults(&after,seed,size)!=-1 ||
            memcmp(&before,&after,sizeof before)) abort();
    }
    after=before;
    if(slicks_configuration_defaults(&after,0,0x2fa4)!=-1 ||
        memcmp(&before,&after,sizeof before)) abort();
    puts("DOS configuration defaults: 256 original startup-slice comparisons pass, including poisoned BSS; short inputs rejected atomically");
}

int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f); int error=ferror(f); fclose(f);
    if(error || bytes<200000 || bytes==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,bytes));
    verify_defaults(u,runtime);
    struct Stream stream; const unsigned hooks[]={0x11eaf,0x12d8a,0x119c2,0x123ce};
    for(unsigned i=0;i<4;++i) { uc_hook h; check(uc_hook_add(u,&h,UC_HOOK_CODE,io,&stream,hooks[i],hooks[i])); }
    /* Execute the real BIOS-date signature routine, not a return-value stub. */
    check(uc_mem_write(u,0xffff5,"09/24/26",8));
    const unsigned char signature=(unsigned char)(9+24*16+26*256);
    static unsigned char expected[65536],actual[65536];
    unsigned cases=0;
    for(unsigned pattern=0;pattern<256;++pattern) for(unsigned mode=0;mode<4;++mode) {
        for(unsigned i=0;i<142;++i) stream.data[i]=(unsigned char)(pattern+37*i);
        stream.data[0]=mode==1?14:15; stream.data[1]=mode==2?signature^1:signature;
        stream.at=0; stream.available=mode!=3;
        struct SlicksConfiguration c; memset(&c,0xa5,sizeof c);
        memset(expected,0xa5,sizeof expected); expected[0x17bd]=0;
        unsigned short first=pattern*257U,second=65535-pattern,third=pattern*131U;
        put(expected,0x4db4,first); put(expected,0x4db6,second); put(expected,0x4db8,third);
        check(uc_mem_write(u,0x3cbf0,expected,sizeof expected));
        int result=slicks_load_configuration(&c,stream.available?stream.data:0,
            stream.available?142:0,signature,first,second,third);
        project(expected,&c);
        uint16_t cs=0x266c,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
        word(u,0x8f000,0); word(u,0x8f002,0x7000);
        check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
        check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
        check(uc_emu_start(u,0x2b486,0x70000,0,100000));
        check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
        check(uc_mem_read(u,0x3cbf0,actual,sizeof actual));
        if(result!=(mode==0) || ip || sp!=0xf004 ||
            stream.at!=(mode==0?142:mode==1?1:mode==2?2:0) || memcmp(expected,actual,sizeof expected)) {
            fprintf(stderr,"CFG mismatch pattern=%u mode=%u bytes=%u\n",pattern,mode,stream.at);
            for(unsigned i=0;i<sizeof actual;++i) if(actual[i]!=expected[i]) {
                fprintf(stderr,"DS:%04x native=%02x DOS=%02x\n",i,expected[i],actual[i]); break;
            }
            return 1;
        }
        ++cases;
    }
    struct SlicksConfiguration before,after; memset(&before,0x55,sizeof before);
    stream.data[0]=15; stream.data[1]=signature;
    for(unsigned size=1;size<142;++size) {
        after=before;
        if(slicks_load_configuration(&after,stream.data,size,signature,1,2,3)!=-1 ||
            memcmp(&after,&before,sizeof before)) return 1;
    }
    verify_save(u,&stream,signature);
    check(uc_close(u));
    printf("DOS configuration: %u full-loader/state comparisons pass; 141 recognized truncated prefixes rejected atomically\n",cases);
    return 0;
}
