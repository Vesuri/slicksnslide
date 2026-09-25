#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/track_scene.h"
static void ck(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e));exit(1); } }
static void word(uc_engine *u,unsigned a,unsigned v) { unsigned char b[2]={v,v>>8};ck(uc_mem_write(u,a,b,2)); }
static void registers(uc_engine *u)
{
    uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000;
    ck(uc_reg_write(u,UC_X86_REG_CS,&cs));ck(uc_reg_write(u,UC_X86_REG_DS,&ds));
    ck(uc_reg_write(u,UC_X86_REG_SS,&ss));ck(uc_reg_write(u,UC_X86_REG_SP,&sp));
}
int main(void)
{
    static unsigned char bytes[300000],raw[65536],packed[65536],lower[60800],upper[60800];
    FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    size_t n=fread(bytes,1,sizeof bytes,f);fclose(f);
    uc_engine *u;ck(uc_open(UC_ARCH_X86,UC_MODE_16,&u));ck(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    ck(uc_mem_write(u,0x10100,bytes,n));
    word(u,0x3cbf0+0x5b8,0);word(u,0x3cbf0+0x5ba,0xa000);
    word(u,0x3cbf0+0x5bc,0);word(u,0x3cbf0+0x5be,0xb000);
    memset(raw,0xa5,sizeof raw);memset(packed,0x5a,sizeof packed);
    ck(uc_mem_write(u,0xa0000,raw,sizeof raw));ck(uc_mem_write(u,0xb0000,packed,sizeof packed));
    registers(u);ck(uc_emu_start(u,0x1bd30,0x1bd5a,0,1000000));
    ck(uc_mem_read(u,0xa0000,raw,sizeof raw));ck(uc_mem_read(u,0xb0000,packed,sizeof packed));
    for(unsigned i=0;i<65536;++i) if(raw[i]!=(i<64000?0:0xa5) || packed[i]!=(i<65152?0:0x5a)) return 1;
    puts("Original map initialization: exact 64000/65152-byte clears match poisoned tails");
    for(unsigned i=0;i<59200;++i) {
        raw[i]=(unsigned char)((i*73U+i/320U*31U)&255U);
        unsigned high=(i*13U+i/319U)%4;
        packed[i/4]|=(unsigned char)(high<<((i&3)*2));
        lower[i]=raw[i]>>3;upper[i]=(unsigned char)((high<<3)|(raw[i]&7));
    }
    ck(uc_mem_write(u,0xa0000,raw,sizeof raw));ck(uc_mem_write(u,0xb0000,packed,sizeof packed));
    const short xs[]={-321,-320,-319,-5,-4,-3,-2,-1,0,1,2,3,4,159,319,320,321,639,640};
    const short ys[]={-1,0,1,90,184,185,189,190,191,199,200,204,205};
    unsigned checked=0,rejected=0;
    for(unsigned layer=0;layer<2;++layer) for(int level=-1;level<8;++level)
    for(unsigned xi=0;xi<sizeof xs/sizeof *xs;++xi) for(unsigned yi=0;yi<sizeof ys/sizeof *ys;++yi) {
        short x=xs[xi],y=ys[yi];
        int native=slicks_track_projectile_sample(lower,upper,x,y,layer,level);
        if(native<0) { ++rejected;continue; }
        registers(u);word(u,0x8f000,0);word(u,0x8f002,0x7000);
        word(u,0x8f004,x);word(u,0x8f006,y);word(u,0x8f008,0xffff);word(u,0x8f00a,layer);
        word(u,0x3cbf0+0x4c6c,level);
        ck(uc_emu_start(u,0x1c5a0,0x70000,0,10000));uint16_t ax;ck(uc_reg_read(u,UC_X86_REG_AX,&ax));
        if(native!=(unsigned char)ax) {
            fprintf(stderr,"Projectile map mismatch xy=%d,%d layer=%u level=%d native=%d original=%u\n",x,y,layer,level,native,(unsigned char)ax);return 1;
        }
        ++checked;
    }
    printf("Original projectile map: %u signed-coordinate/layer/boundary cases match; %u unretained addresses rejected\n",checked,rejected);
    ck(uc_close(u));return 0;
}
