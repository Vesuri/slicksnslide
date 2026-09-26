#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unicorn/unicorn.h>
#include <unicorn/m68k.h>
#include "../src/game/sprite_opacity.h"
static void check(uc_err e){if(e){fprintf(stderr,"%s\n",uc_strerror(e));exit(1);}}
static void be32(unsigned char *p,uint32_t n){p[0]=n>>24;p[1]=n>>16;p[2]=n>>8;p[3]=n;}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    unsigned char code[4096];FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    size_t size=fread(code,1,sizeof code,f);fclose(f);
    uint32_t visible_entry=0x10000+((uint32_t)code[size-4]<<24)+((uint32_t)code[size-3]<<16)+
        ((uint32_t)code[size-2]<<8)+code[size-1];
    uc_engine *u;check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    check(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));check(uc_mem_write(u,0x10000,code,size));
    const int regs[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,
        UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,UC_M68K_REG_A3,UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6};
    static unsigned char pixels[64000],expected[64000],actual[64000];
    for(unsigned trial=0;trial<4096;++trial) {
        unsigned visible=trial>=2048;
        unsigned width=1+(trial&15),height=1+((trial>>4)&7);
        unsigned x=trial&128?320-width:100+((trial>>8)&3),y=trial&256?200-height:80;
        unsigned source_align=(trial>>7)&3,saved_align=(trial>>9)&3;
        unsigned char source[256],opacity[128],saved[256],expected_saved[256],stack[28];
        memset(source,0xcc,sizeof source);memset(opacity,0xcc,sizeof opacity);
        memset(saved,0xcc,sizeof saved);memset(expected_saved,0xcc,sizeof expected_saved);
        for(unsigned i=0;i<width*height;++i) {
            unsigned pattern=(trial>>9)&3;
            source[source_align+i]=pattern==0?0:pattern==1?43:
                pattern==2?(i&1?0:91):(i%3?(unsigned char)(i*23+trial):0);
        }
        unsigned used=slicks_make_sprite_opacity(source+source_align,width*height,opacity,sizeof opacity);
        if(used!=width*height)return 1;
        if(visible)for(unsigned i=0;i<used;++i)opacity[i]=!opacity[i] && (i+trial)%5?255:0;
        for(unsigned i=0;i<64000;++i)pixels[i]=expected[i]=(unsigned char)(i*13+trial);
        for(unsigned row=0;row<height;++row)for(unsigned col=0;col<width;++col) {
            unsigned at=(y+row)*320+x+col,si=row*width+col;
            expected_saved[saved_align+si]=pixels[at];
            if(visible?opacity[si]!=0:source[source_align+si]!=0)expected[at]=source[source_align+si];
        }
        check(uc_mem_write(u,0x20000,source,sizeof source));check(uc_mem_write(u,0x30000,pixels,sizeof pixels));
        check(uc_mem_write(u,0x50000,saved,sizeof saved));check(uc_mem_write(u,0x60000,opacity,sizeof opacity));
        const unsigned args[]={0x18000,0x30000+y*320+x,0x20000+source_align,0x50000+saved_align,0x60000,width,height};
        for(unsigned i=0;i<7;++i)be32(stack+4*i,args[i]);
        check(uc_mem_write(u,0x90000,stack,sizeof stack));uint32_t sp=0x90000;
        check(uc_reg_write(u,UC_M68K_REG_A7,&sp));
        for(unsigned i=0;i<sizeof regs/sizeof *regs;++i){uint32_t v=0x55667700+i;check(uc_reg_write(u,regs[i],&v));}
        check(uc_emu_start(u,visible?visible_entry:0x10000,0x18000,0,100000));
        check(uc_mem_read(u,0x30000,actual,sizeof actual));check(uc_mem_read(u,0x50000,saved,sizeof saved));
        if(memcmp(actual,expected,sizeof actual)||memcmp(saved,expected_saved,sizeof saved)) {
            fprintf(stderr,"Sprite opacity mismatch trial=%u width=%u height=%u\n",trial,width,height);return 1;
        }
        check(uc_reg_read(u,UC_M68K_REG_A7,&sp));if(sp!=0x90004)return 1;
        for(unsigned i=0;i<sizeof regs/sizeof *regs;++i){uint32_t v;check(uc_reg_read(u,regs[i],&v));if(v!=0x55667700+i)return 1;}
        /* Every truncated metadata destination must fail without overrunning. */
        for(unsigned n=0;n<used;++n) {
            unsigned char small[514];memset(small,0xaa,sizeof small);
            if(slicks_make_sprite_opacity(source+source_align,width*height,small+1,n))return 1;
            if(small[0]!=0xaa)return 1;
            for(unsigned i=n+1;i<sizeof small;++i)if(small[i]!=0xaa)return 1;
        }
    }
    check(uc_close(u));puts("Sprite opacity/visibility: 4096 native full-frame/saved-buffer/ABI cases, edge/alignment/transparency/word tails and truncated metadata capacities pass");
    return 0;
}
