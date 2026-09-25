/* Real DOS point draw, including its VGA plotter, versus native chunky draw.
 * Only the VGA sequencer/window is modelled; no game drawing call is stubbed. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/race_runtime.c"

static struct SlicksRaceRuntime race;
static unsigned char native[64000], dos[64000], runtime[300000];
static unsigned plane, writes;
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *uc,unsigned a,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(uc,a,b,2)); }
static void port(uc_engine *uc,uint32_t address,int size,uint32_t value,void *user)
{
    (void)uc; (void)user;
    if(address!=0x3c4 || size!=2 || (value&255)!=2) abort();
    for(plane=0;plane<4;++plane) if((value>>8)==(1U<<plane)) return;
    abort();
}
static void pixel(uc_engine *uc,uc_mem_type type,uint64_t a,int size,int64_t value,void *user)
{
    unsigned offset=(unsigned)a-0xa0000;
    unsigned x=(offset%100)*4+plane,y=offset/100;
    (void)uc; (void)type; (void)user;
    if(size!=1 || x>=320 || y>=190) abort();
    dos[y*320+x]=(unsigned char)value; ++writes;
}
int main(int argc,char **argv)
{
    static const short positions[][2]={{-1,0},{0,-1},{0,0},{1,1},{2,1},{3,1},
        {319,183},{319,184},{319,189},{320,189},{319,190},{101,83},{-512,100},{511,189}};
    FILE *f;
    uc_engine *uc;
    uc_hook output,write;
    unsigned cases=0;
    if(argc!=2 || !(f=fopen(argv[1],"rb"))) return 1;
    size_t size=fread(runtime,1,sizeof runtime,f);
    if(ferror(f) || !feof(f)) return 1;
    fclose(f);
    check(uc_open(UC_ARCH_X86,UC_MODE_16,&uc));
    check(uc_mem_map(uc,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(uc,0x10100,runtime,size));
    check(uc_hook_add(uc,&output,UC_HOOK_INSN,port,NULL,1,0,UC_X86_INS_OUT));
    check(uc_hook_add(uc,&write,UC_HOOK_MEM_WRITE,pixel,NULL,0xa0000,0xaffff));
    word(uc,0x616ce,0); word(uc,0x616d0,0x9000);
    word(uc,0x616ca,0); word(uc,0x616cc,0x8000);
    word(uc,0x616c7,1); word(uc,0x616c4,184);
    word(uc,0x3cbf0+0x1d7d,320); word(uc,0x3cbf0+0x1d89,0);
    word(uc,0x61d7b,100);
    for(unsigned raw=0;raw<256;++raw)
    for(unsigned layer=0;layer<2;++layer)
    for(unsigned page=0;page<2;++page)
    for(unsigned p=0;p<sizeof positions/sizeof positions[0];++p) {
        uint16_t cs=0x2e0f,ds=0x6000,ss=0x7000,sp=0xf000,ip;
        unsigned char actor[64]={0},mask=raw;
        short x=positions[p][0],y=positions[p][1];
        unsigned limit=layer*15,colour=(raw*53+17)&255;
        int visible=x>=0 && x<320 && y>=0 && y<184;
        memset(native,91,sizeof native); memset(dos,91,sizeof dos);
        memset(&race,0,sizeof race); race.chunky=native;
        if(visible) {
            unsigned at=y*320+x;
            race.material_map[at]=raw>>3; race.surface_map[at]=raw&7;
            check(uc_mem_write(uc,0x80000+at,&mask,1));
        }
        actor[0x26]=colour; actor[0x3b]=limit;
        check(uc_mem_write(uc,0x90040,actor,sizeof actor));
        word(uc,0x90044+page*2,(uint16_t)x);
        word(uc,0x90048+page*2,(uint16_t)y);
        word(uc,0x616c6,page); /* Restore packed map mode after adjacent word. */
        word(uc,0x616c7,1);
        word(uc,0x7f000,0); word(uc,0x7f002,0x5000); word(uc,0x7f004,1);
        check(uc_reg_write(uc,UC_X86_REG_CS,&cs));
        check(uc_reg_write(uc,UC_X86_REG_DS,&ds));
        check(uc_reg_write(uc,UC_X86_REG_SS,&ss));
        check(uc_reg_write(uc,UC_X86_REG_SP,&sp));
        writes=0; plane=99;
        check(uc_emu_start(uc,0x33673,0x50000,0,1000));
        check(uc_reg_read(uc,UC_X86_REG_IP,&ip));
        check(uc_reg_read(uc,UC_X86_REG_SP,&sp));
        add_trail_component(&race,x,y,colour,3,0,0,35);
        race.trail_particles[0].occlusion_limit=limit;
        draw_trail_particles(&race,1);
        if(ip || sp!=0xf004 || memcmp(native,dos,sizeof native) ||
           writes!=(unsigned)(visible && (!limit || raw<=limit))) {
            fprintf(stderr,"point draw mismatch raw=%u layer=%u page=%u x=%d y=%d writes=%u ip=%x sp=%x\n",
                raw,layer,page,x,y,writes,ip,sp); return 1;
        }
        ++cases;
    }
    check(uc_close(uc));
    printf("DOS/native point raster: %u cases passed; raw mask bytes, layer limits, pages, colour bytes, clipping and VGA planes (retirement not tested)\n",cases);
    return 0;
}
