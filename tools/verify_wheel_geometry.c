#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "../src/game/race_runtime.c"
#include "../src/game/wheel_geometry.h"
#include "host_archive.h"
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e)); exit(1); } }
static void word(uc_engine *u,unsigned a,unsigned v)
{ unsigned char b[2]={v,v>>8}; check(uc_mem_write(u,a,b,2)); }
static int compare_rotation(uc_engine *u,const struct SlicksCarSprite *sprite,
                            unsigned rotation,unsigned char *pixels,unsigned width,unsigned height)
{
    unsigned stride=(sprite->width+3U)/4;
    unsigned char source[1024]={0},expected[1024],actual[1024];
    source[0]=stride; source[1]=sprite->height;
    for(unsigned y=0;y<sprite->height;++y) for(unsigned x=0;x<sprite->width;++x)
        source[2+(x&3)*stride*sprite->height+y*stride+x/4]=sprite->pixels[y*sprite->width+x];
    source[2+4*stride*sprite->height]=4*stride-sprite->width;
    memset(expected,0xa5,sizeof expected); check(uc_mem_write(u,0x91000,expected,sizeof expected));
    check(uc_mem_write(u,0x90000,source,sizeof source));
    uint16_t cs=0x2e0f,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
    word(u,0x8f000,0); word(u,0x8f002,0x7000);
    word(u,0x8f004,0); word(u,0x8f006,0x9000);
    word(u,0x8f008,0); word(u,0x8f00a,0x9100); word(u,0x8f00c,rotation);
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    check(uc_emu_start(u,0x3595b,0x70000,0,100000));
    check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    check(uc_mem_read(u,0x91000,actual,sizeof actual));
    unsigned char visible_width,visible_height;
    rotated_size(sprite,rotation,&visible_width,&visible_height);
    if(height!=visible_height) return 1;
    expected[0]=width/4; expected[1]=height;
    for(unsigned y=0;y<height;++y) for(unsigned x=0;x<width;++x)
        expected[2+(x&3)*(width/4)*height+y*(width/4)+x/4]=pixels[y*width+x];
    expected[2+width*height]=width-visible_width;
    if(ip || sp!=0xf004 || memcmp(expected,actual,sizeof expected)) {
        fprintf(stderr,"Rotation mismatch %ux%u turn=%u\n",sprite->width,sprite->height,rotation); return 1;
    }
    return 0;
}
static int compare(uc_engine *u,unsigned char *pixels,unsigned width,unsigned height,
                   unsigned driver,unsigned heading)
{
    unsigned stride=width/4;
    unsigned char planar[1024]={0},actual[1024];
    planar[0]=stride; planar[1]=height;
    for(unsigned y=0;y<height;++y) for(unsigned x=0;x<width;++x)
        planar[2+(x&3)*stride*height+y*stride+x/4]=pixels[y*width+x];
    check(uc_mem_write(u,0x90000,planar,sizeof planar));
    unsigned at=driver*32+heading;
    signed char xs[2]={42,43},ys[2]={44,45};
    for(unsigned i=0;i<2;++i) {
        check(uc_mem_write(u,0x3cbf0+0x4ca4+at+16*i,&xs[i],1));
        check(uc_mem_write(u,0x3cbf0+0x4d24+at+16*i,&ys[i],1));
    }
    uint16_t cs=0x1987,ds=0x3cbf,ss=0x8000,sp=0xf000,ip;
    word(u,0x8f000,0); word(u,0x8f002,0x7000); word(u,0x8f004,driver);
    word(u,0x8f006,heading); word(u,0x8f008,0); word(u,0x8f00a,0x9000);
    check(uc_reg_write(u,UC_X86_REG_CS,&cs)); check(uc_reg_write(u,UC_X86_REG_DS,&ds));
    check(uc_reg_write(u,UC_X86_REG_SS,&ss)); check(uc_reg_write(u,UC_X86_REG_SP,&sp));
    check(uc_emu_start(u,0x1c2e9,0x70000,0,100000));
    check(uc_reg_read(u,UC_X86_REG_IP,&ip)); check(uc_reg_read(u,UC_X86_REG_SP,&sp));
    if(ip || sp!=0xf004 || slicks_extract_wheels(pixels,width,height,xs,ys)) return 1;
    check(uc_mem_read(u,0x90000,actual,sizeof actual));
    for(unsigned y=0;y<height;++y) for(unsigned x=0;x<width;++x)
        planar[2+(x&3)*stride*height+y*stride+x/4]=pixels[y*width+x];
    if(memcmp(planar,actual,sizeof planar)) return 1;
    for(unsigned i=0;i<2;++i) {
        signed char x,y;
        check(uc_mem_read(u,0x3cbf0+0x4ca4+at+16*i,&x,1));
        check(uc_mem_read(u,0x3cbf0+0x4d24+at+16*i,&y,1));
        if(x!=xs[i] || y!=ys[i]) return 1;
    }
    return 0;
}
int main(void)
{
    unsigned char runtime[300000]; FILE *f=fopen("disasm/runtime.bin","rb"); if(!f) return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f); int error=ferror(f); fclose(f);
    if(error || bytes<200000 || bytes==sizeof runtime) return 2;
    uc_engine *u; check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));
    check(uc_mem_map(u,0,0x100000,UC_PROT_ALL)); check(uc_mem_write(u,0x10100,runtime,bytes));
    unsigned cases=0;
    static struct SlicksRaceRuntime race;
    for(unsigned vehicle=0;vehicle<10;++vehicle)
    for(unsigned heading=0;heading<16;++heading)
    for(unsigned driver=0;driver<4;++driver) {
        unsigned char resource[128],pixels[256]={0},w,h;
        char name[16]; struct SlicksCarSprite sprite;
        snprintf(name,sizeof name,"auto%02u.%03u",vehicle,heading&3);
        if(vehicle==9 && !(heading&3)) strcpy(name,"car9");
        long size=host_archive_load("ref/SLICKS.000",name,resource,sizeof resource);
        if(size<0 || decode_sprite(&sprite,resource,size)) {
            fprintf(stderr,"Cannot decode %s (%ld bytes)\n",name,size); return 2;
        }
        rotated_size(&sprite,heading>>2,&w,&h);
        unsigned padded=(w+3U)&~3U;
        for(unsigned y=0;y<h;++y) for(unsigned x=0;x<w;++x)
            pixels[y*padded+x]=sprite_pixel(&sprite,heading>>2,x,y);
        if(compare_rotation(u,&sprite,heading>>2,pixels,padded,h)) return 1;
        if(compare(u,pixels,padded,h,driver,heading)) {
            fprintf(stderr,"Wheel mismatch vehicle=%u heading=%u driver=%u\n",vehicle,heading,driver); return 1;
        }
        struct SlicksCarSprite *loaded=&race.sprites[vehicle][heading&3];
        for(unsigned r=0;r<4;++r) { loaded->wheel_y[r][0]=44; loaded->wheel_y[r][1]=45; }
        if(slicks_race_add_car_sprite(&race,vehicle,heading&3,resource,size)) return 1;
        for(unsigned wheel=0;wheel<2;++wheel) {
            signed char x,y; unsigned at=driver*32+heading+wheel*16;
            check(uc_mem_read(u,0x3cbf0+0x4ca4+at,&x,1));
            check(uc_mem_read(u,0x3cbf0+0x4d24+at,&y,1));
            if(loaded->wheel_x[heading>>2][wheel]!=x || loaded->wheel_y[heading>>2][wheel]!=y) return 1;
        }
        for(unsigned y=0;y<h;++y) for(unsigned x=0;x<w;++x)
            if(sprite_pixel(loaded,heading>>2,x,y)!=pixels[y*padded+x]) return 1;
        ++cases;
    }
    /* No markers, one marker, column-first ordering, FE saturation and
     * mixed FF/FE markers. Compare complete modified sprite bytes too. */
    for(unsigned pattern=0;pattern<8;++pattern) for(unsigned driver=0;driver<4;++driver) {
        unsigned char pixels[64]={0};
        if(pattern&1) pixels[7*8]=255;
        if(pattern&2) pixels[7]=254;
        if(pattern&4) { pixels[2*8+2]=254; pixels[3*8+3]=254; }
        if(compare(u,pixels,8,8,driver,15)) return 1;
        ++cases;
    }
    check(uc_close(u));
    printf("DOS wheel extraction: %u coordinate/full-sprite comparisons, 640 original rotation/padding comparisons and 640 production loader comparisons pass\n",cases);
    return 0;
}
