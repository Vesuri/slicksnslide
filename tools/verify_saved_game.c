#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/saved_game.h"

static void check(uc_err error)
{ if(error) { fprintf(stderr,"%s\n",uc_strerror(error)); exit(1); } }
static unsigned word(uc_engine *u,unsigned address)
{ unsigned char b[2]; check(uc_mem_read(u,address,b,2)); return b[0]|b[1]<<8; }
static void putword(uc_engine *u,unsigned address,unsigned value)
{ unsigned char b[2]={(unsigned char)value,(unsigned char)(value>>8)}; check(uc_mem_write(u,address,b,2)); }
struct SaveOracle { unsigned char bytes[SLICKS_SAVED_GAME_MAX_BYTES]; unsigned used,closed,opened,catalogue_calls; };
static void io(uc_engine *u,uint64_t address,uint32_t size,void *context)
{
    (void)size; struct SaveOracle *f=context;
    uint16_t sp,ss,ax=0,dx=0;
    check(uc_reg_read(u,UC_X86_REG_SP,&sp)); check(uc_reg_read(u,UC_X86_REG_SS,&ss));
    unsigned stack=ss*16U+sp;
    if(address==0x11eaf) dx=f->opened?0x6000:0;
    else if(address==0x123ce) {
        if(f->used>=sizeof f->bytes) abort();
        ax=word(u,stack+4); f->bytes[f->used++]=(unsigned char)ax;
    } else if(address==0x119c2) ++f->closed;
    else if(address==0x35e63) {
        unsigned index=word(u,stack+8);
        if(index>=256) abort();
        ax=(uint16_t)(index*8); dx=0x7000; ++f->catalogue_calls;
    } else abort();
    uint16_t ip=word(u,stack),cs=word(u,stack+2); sp+=4;
    check(uc_reg_write(u,UC_X86_REG_AX,&ax)); check(uc_reg_write(u,UC_X86_REG_DX,&dx));
    check(uc_reg_write(u,UC_X86_REG_SP,&sp)); check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_IP,&ip));
}
int main(void)
{
    unsigned char runtime[300000]; FILE *file=fopen("disasm/runtime.bin","rb"); if(!file) return 2;
    size_t size=fread(runtime,1,sizeof runtime,file); fclose(file);
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u)); check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(u,0x10100,runtime,size));
    struct SaveOracle oracle;
    const unsigned hooks[]={0x11eaf,0x123ce,0x119c2,0x35e63};
    for(unsigned i=0;i<4;++i) { uc_hook h; check(uc_hook_add(u,&h,UC_HOOK_CODE,io,&oracle,hooks[i],hooks[i])); }
    const short counts[]={0,1,2,64,256}; unsigned cases=0,guards=0;
    for(unsigned count=0;count<5;++count) for(unsigned length=0;length<=20;++length) for(unsigned pattern=0;pattern<4;++pattern) {
        unsigned char tracks[256][8]; struct SlicksSavedGame game={.track_count=counts[count],.next_track=(short)(pattern*21845),.tracks=tracks};
        for(unsigned i=0;i<256;++i) for(unsigned j=0;j<8;++j) tracks[i][j]=(unsigned char)(i*31+j*11+pattern);
        check(uc_mem_write(u,0x70000,tracks,sizeof tracks));
        putword(u,0x3cbf0+0x90,game.track_count);
        putword(u,0x3cbf0+0x628,(unsigned short)(game.next_track-1));
        putword(u,0x3cbf0+0x62a,0); putword(u,0x3cbf0+0x62c,0x7100);
        for(unsigned i=0;i<(unsigned)game.track_count;++i) putword(u,0x71000+i*2,i);
        for(unsigned i=0;i<4;++i) {
            for(unsigned j=0;j<21;++j) game.names[i][j]=(unsigned char)('A'+(i+j)%26);
            unsigned end=(length+i)%21; game.names[i][end]=0;
            if(pattern==3) memset(game.names[i],0xff,sizeof game.names[i]);
            unsigned profile=3-i;
            putword(u,0x3cbf0+0x44c+i*2,profile);
            check(uc_mem_write(u,0x3cbf0+0x36aa+21*profile,game.names[i],21));
            game.vehicles[i]=(signed char)(pattern*83+i);
            game.participation[i]=(signed char)((i+pattern)%3-1);
            game.position_scale[i]=(unsigned char)(pattern*85+i);
            check(uc_mem_write(u,0x3cbf0+0x3067+0x36*i,&game.position_scale[i],1));
            game.points[i]=(short)(pattern*32767+i); game.cash[i]=(short)(pattern*16385-i);
            putword(u,0x3cbf0+0x6826+2*i,(unsigned short)game.points[i]);
            putword(u,0x3cbf0+0x4bf6+2*i,(unsigned short)game.cash[i]);
            for(unsigned j=0;j<13;++j) {
                game.inventory[i][j]=(short)(pattern*17001+i*79+j*32767);
                putword(u,0x3cbf0+0x6a7a+26*i+2*j,(unsigned short)game.inventory[i][j]);
            }
        }
        check(uc_mem_write(u,0x3cbf0+0x4bc2,game.vehicles,4));
        check(uc_mem_write(u,0x3cbf0+0x4bc6,game.participation,4));
        for(unsigned opened=0;opened<2;++opened) {
            oracle=(struct SaveOracle){.opened=opened};
            uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000,ax,ip;
            check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
            check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
            putword(u,0x8f000,0); putword(u,0x8f002,0x9000); putword(u,0x8f004,0); putword(u,0x8f006,0x7200);
            check(uc_emu_start(u,0x1d587,0x90000,0,3000000));
            check(uc_reg_read(u,UC_X86_REG_CS,&cs)); check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_AX,&ax));
            if(cs!=0x9000 || ip || (ax&255)!=!opened || oracle.closed!=opened) abort();
            if(!opened) { if(oracle.used || oracle.catalogue_calls) abort(); continue; }
            unsigned char encoded[SLICKS_SAVED_GAME_MAX_BYTES+1]; memset(encoded,0xa5,sizeof encoded);
            long bytes=slicks_save_game_bytes(&game,encoded,sizeof encoded);
            if(bytes!=(long)oracle.used || bytes!=slicks_saved_game_size(&game) ||
                memcmp(encoded,oracle.bytes,(size_t)bytes) || encoded[bytes]!=0xa5 ||
                oracle.catalogue_calls!=8U*(unsigned)game.track_count) {
                fprintf(stderr,"Saved-game mismatch count=%u length=%u pattern=%u\n",count,length,pattern); return 1;
            }
            struct SlicksSavedGame decoded;
            unsigned char loaded_tracks[256][8],roundtrip[SLICKS_SAVED_GAME_MAX_BYTES];
            if(slicks_load_game_bytes(&decoded,loaded_tracks,256,oracle.bytes,oracle.used) ||
               slicks_save_game_bytes(&decoded,roundtrip,sizeof roundtrip)!=bytes ||
               memcmp(roundtrip,oracle.bytes,oracle.used)) abort();
            /* Every truncated original stream must leave both outputs alone. */
            memset(&decoded,0xa5,sizeof decoded); memset(loaded_tracks,0xa5,sizeof loaded_tracks);
            for(unsigned cut=0;cut<oracle.used;++cut) {
                if(slicks_load_game_bytes(&decoded,loaded_tracks,256,oracle.bytes,cut)!=-1) abort();
                for(unsigned j=0;j<sizeof decoded;++j) if(((unsigned char *)&decoded)[j]!=0xa5) abort();
                for(unsigned j=0;j<sizeof loaded_tracks;++j) if(((unsigned char *)loaded_tracks)[j]!=0xa5) abort();
            }
            encoded[bytes]=0;
            if(slicks_load_game_bytes(&decoded,loaded_tracks,256,encoded,bytes+1)!=-1) abort();
            if(game.track_count && slicks_load_game_bytes(&decoded,loaded_tracks,
               (unsigned)game.track_count-1,oracle.bytes,oracle.used)!=-1) abort();
            encoded[0]=0;
            if(slicks_load_game_bytes(&decoded,loaded_tracks,256,encoded,bytes)!=-1) abort();
            if(length==20 && count==4 && pattern==3) for(unsigned cap=0;cap<(unsigned)bytes;++cap) {
                memset(encoded,0xa5,sizeof encoded);
                if(slicks_save_game_bytes(&game,encoded,cap)!=-1) abort();
                for(unsigned j=0;j<sizeof encoded;++j) if(encoded[j]!=0xa5) abort();
                ++guards;
            }
            ++cases;
        }
    }
    struct SlicksSavedGame invalid={.track_count=-1}; unsigned char byte=0xa5;
    if(slicks_save_game_bytes(&invalid,&byte,1)!=-1 || byte!=0xa5) abort();
    invalid.track_count=257; if(slicks_saved_game_size(&invalid)!=-1) abort();
    invalid.track_count=1; if(slicks_saved_game_size(&invalid)!=-1) abort();
    uc_close(u);
    printf("Original saved-game writer: %u byte-exact streams and open failures; %u unchanged-destination capacity guards pass\n",cases,guards);
    puts("Saved-game decoder: original-writer streams round-trip exactly; all truncated prefixes, extra bytes, bad signatures and undersized track buffers rejected");
    return 0;
}
