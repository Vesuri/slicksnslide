/* Execute the original palette matcher, including signed queries and ties. */
#include <stdio.h>
#include <stdlib.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/ui/arcade_title_painter.h"
static void check(uc_err e)
{ if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned at,unsigned value)
{ unsigned char bytes[2]={value,value>>8}; check(uc_mem_write(u,at,bytes,2)); }
int main(void)
{
    unsigned char runtime[300000],palettes[2][768];
    FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f);fclose(f);
    uc_engine *u;check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));check(uc_mem_write(u,0x10100,runtime,bytes));
    struct SlicksArcadePulseCache cache={0};
    struct SlicksArcadeTitlePainter painter={.cache=&cache};
    unsigned cases=0;
    for(unsigned epoch=0;epoch<4;++epoch) {
        /* Same pointer/full redraw, changed pointer/pulse, changed content
         * at same pointer/full redraw, and the uncached caller. */
        unsigned char *palette=palettes[epoch?1:0];
        for(unsigned i=0;i<768;++i)palette[i]=epoch?(unsigned char)(i*61+epoch*43+(i>>3)*17):0;
        painter.palette=palette;painter.pulse=epoch==1;
        if(epoch==3)painter.cache=0;
        slicks_arcade_paint_cache_begin(&painter);
        if(painter.cache && (cache.valid || cache.nearest_count)){fprintf(stderr,"Cache invalidation failed epoch=%u\n",epoch);return 1;}
        check(uc_mem_write(u,0x68000,palette,768));
        for(unsigned q=0;q<256;++q) {
            unsigned char r=q,g=q*3,b=q*7;
            uint16_t cs=0x36f0,ds=0x3cbf,ss=0x8000,sp=0xf000,ax,ip;
            check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&ds));
            check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
            word(u,0x8f000,0);word(u,0x8f002,0x9000);
            word(u,0x8f004,r);word(u,0x8f006,g);word(u,0x8f008,b);
            word(u,0x8f00a,0);word(u,0x8f00c,0x6800);
            check(uc_emu_start(u,0x36fae,0x90000,0,100000));
            check(uc_reg_read(u,UC_X86_REG_AX,&ax));check(uc_reg_read(u,UC_X86_REG_IP,&ip));
            ax&=255; /* The original returns an unsigned char in AL, not AX. */
            unsigned actual=slicks_arcade_paint_nearest(&painter,r,g,b);
            if(ip || ax!=actual){fprintf(stderr,"Cache mismatch epoch=%u query=%u original=%u native=%u ip=%u\n",epoch,q,ax,actual,ip);return 1;}
            unsigned count=cache.nearest_count;
            if(ax!=slicks_arcade_paint_nearest(&painter,r,g,b) || cache.nearest_count!=count)return 1;
            cases+=2;
        }
        cache.valid=1;
    }
    uc_close(u);
    printf("Arcade palette cache: %u original comparisons; hits, rollover, signed queries, ties, palette replacement/full-redraw invalidation and uncached calls pass\n",cases);
    return 0;
}
