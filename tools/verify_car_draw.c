#define main surface_verifier_main
#include "verify_surface_effects.c"
#undef main
#include <unicorn/unicorn.h>
#include <unicorn/m68k.h>
#include <stdint.h>
static void ck(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e));exit(1); } }
static void be32(unsigned char *p,uint32_t v) { p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v; }
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    unsigned char code[4096];FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    size_t size=fread(code,1,sizeof code,f);fclose(f);
    uint32_t restore=0x10000+((uint32_t)code[size-4]<<24)+((uint32_t)code[size-3]<<16)+((uint32_t)code[size-2]<<8)+code[size-1];
    uc_engine *u;ck(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    ck(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));ck(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    ck(uc_mem_write(u,0x10000,code,size));
    static struct SlicksRaceRuntime race;
    static unsigned char pixels[64000],before[64000],actual[64000];
    const int preserved[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,
        UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,UC_M68K_REG_A3,UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6};
    for(unsigned trial=0;trial<1024;++trial) {
        memset(&race,0,sizeof race);race.chunky=pixels;
        struct SlicksRaceCar *car=&race.cars[0];
        unsigned rotation=trial&3,base=(trial>>2)&3;
        car->x=10000;car->y=10000;car->style=(trial>>4)&7;
        if(trial&512) { car->x=(trial&4)?31500:500;car->y=(trial&8)?18500:500; }
        car->heading=(rotation*4+base)*SLICKS_HEADING_STEP;car->actor_layer=(trial>>7)&1;
        struct SlicksCarSprite *sprite=&race.sprites[0][base];
        sprite->width=(trial&256)?10:7;sprite->height=(trial&256)?10:9;
        for(unsigned i=0;i<100;++i)sprite->pixels[i]=(unsigned char)((trial&512)?i*37+trial:(i+trial)%13);
        for(unsigned i=0;i<64000;++i) {
            pixels[i]=before[i]=(unsigned char)(i*13+trial);
            if(i<sizeof race.material_map) {
                race.material_map[i]=(i+trial)%5;race.surface_map[i]=(i+trial)%32;
            }
        }
        draw_car(&race,0,0);
        unsigned width=car->old_width,height=car->old_height,offset=car->old_y*320+car->old_x;
        int dx=1,dy=sprite->width;unsigned source=0;
        if(rotation==1) { source=(sprite->height-1)*sprite->width;dx=-sprite->width;dy=1; }
        if(rotation==2) { source=sprite->width*sprite->height-1;dx=-1;dy=-sprite->width; }
        if(rotation==3) { source=sprite->width-1;dx=sprite->width;dy=-1; }
        ck(uc_mem_write(u,0x20000,sprite->pixels,100));ck(uc_mem_write(u,0x30000,before,sizeof before));
        ck(uc_mem_write(u,0x40000,race.material_map,sizeof race.material_map));
        ck(uc_mem_write(u,0x50000,race.surface_map,sizeof race.surface_map));
        unsigned char saved[100],stack[48];memset(saved,0xaa,sizeof saved);ck(uc_mem_write(u,0x60000,saved,100));
        const uint32_t args[]={0x30000+offset,0x20000+source,0x60000,0x40000+offset,
            0x50000+offset,width,height,(uint32_t)dx,(uint32_t)dy,car->style*5U,car->actor_layer*15U};
        be32(stack,0x18000);for(unsigned i=0;i<11;++i)be32(stack+4+i*4,args[i]);
        ck(uc_mem_write(u,0x90000,stack,sizeof stack));uint32_t sp=0x90000;
        ck(uc_reg_write(u,UC_M68K_REG_A7,&sp));
        for(unsigned i=0;i<sizeof preserved/sizeof *preserved;++i) {
            uint32_t v=0x12345600+i;ck(uc_reg_write(u,preserved[i],&v));
        }
        ck(uc_emu_start(u,0x10000,0x18000,0,100000));
        ck(uc_mem_read(u,0x30000,actual,sizeof actual));ck(uc_mem_read(u,0x60000,saved,100));
        if(memcmp(pixels,actual,sizeof pixels)||memcmp(saved,car->saved_under,width*height)) {
            fprintf(stderr,"Native car draw mismatch trial=%u\n",trial);return 1;
        }
        for(unsigned i=width*height;i<100;++i)if(saved[i]!=0xaa)fail("car saved buffer overrun");
        ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));if(sp!=0x90004)fail("car draw stack");
        be32(stack+4,0x30000+offset);be32(stack+8,0x60000);be32(stack+12,width);be32(stack+16,height);
        ck(uc_mem_write(u,0x90000,stack,20));sp=0x90000;ck(uc_reg_write(u,UC_M68K_REG_A7,&sp));
        ck(uc_emu_start(u,restore,0x18000,0,100000));
        ck(uc_mem_read(u,0x30000,actual,sizeof actual));
        if(memcmp(before,actual,sizeof before))fail("native car restoration");
        ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));if(sp!=0x90004)fail("car restore stack");
        for(unsigned i=0;i<sizeof preserved/sizeof *preserved;++i) {
            uint32_t v;ck(uc_reg_read(u,preserved[i],&v));
            if(v!=0x12345600+i)fail("car preserved register");
        }
    }
    for(unsigned width=1;width<=16;++width)
    for(unsigned height=1;height<=8;++height)
    for(unsigned align=0;align<4;++align) {
        unsigned char saved[132],stack[20];
        for(unsigned i=0;i<sizeof saved;++i)saved[i]=(unsigned char)(i*53+width);
        memset(before,0xa5,sizeof before);memcpy(pixels,before,sizeof pixels);
        unsigned offset=100*320+100+align;
        for(unsigned y=0;y<height;++y)
            memcpy(pixels+offset+y*320,saved+align+y*width,width);
        ck(uc_mem_write(u,0x30000,before,sizeof before));
        ck(uc_mem_write(u,0x60000,saved,sizeof saved));
        be32(stack,0x18000);be32(stack+4,0x30000+offset);
        be32(stack+8,0x60000+align);be32(stack+12,width);be32(stack+16,height);
        ck(uc_mem_write(u,0x90000,stack,sizeof stack));
        uint32_t sp=0x90000;ck(uc_reg_write(u,UC_M68K_REG_A7,&sp));
        ck(uc_emu_start(u,restore,0x18000,0,100000));
        ck(uc_mem_read(u,0x30000,actual,sizeof actual));
        if(memcmp(pixels,actual,sizeof pixels))fail("restore width/tail/alignment");
        ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));if(sp!=0x90004)fail("restore tail stack");
    }
    /* Exercise every byte-sized mask, especially equal material with surface
     * just above/below the residual threshold. Both contiguous and rotated /
     * recoloured entry paths must keep the original combined comparison. */
    for(unsigned mask=0;mask<256;++mask)
    for(unsigned mode=0;mode<3;++mode) {
        unsigned char source[64],saved[64],stack[48];
        unsigned offset=100*320+100;
        memset(before,0xa5,sizeof before);memcpy(pixels,before,sizeof pixels);
        memset(&race,0,sizeof race);
        for(unsigned i=0;i<64;++i)source[i]=(i%9)?(unsigned char)(i%13):0;
        for(unsigned i=0;i<64;++i) {
            int material=(int)(mask>>3)+(int)((i/8)%5)-2;
            if(material<0)material=0;
            unsigned at=offset+(i/8)*320+i%8;
            race.material_map[at]=(unsigned char)material;
            race.surface_map[at]=(unsigned char)((i%8)|0xf8);
            unsigned pixel=source[mode==2?63-i:i];
            if(pixel && (!mask || (((unsigned)material<<3)|(race.surface_map[at]&7))<=mask)) {
                if(mode && pixel<=5)pixel+=5;
                pixels[at]=(unsigned char)pixel;
            }
        }
        ck(uc_mem_write(u,0x20000,source,sizeof source));
        ck(uc_mem_write(u,0x30000,before,sizeof before));
        ck(uc_mem_write(u,0x40000,race.material_map,sizeof race.material_map));
        ck(uc_mem_write(u,0x50000,race.surface_map,sizeof race.surface_map));
        const uint32_t args[]={0x30000+offset,0x20000+(mode==2?63:0),0x60000,
            0x40000+offset,0x50000+offset,8,8,mode==2?(uint32_t)-1:1,
            mode==2?(uint32_t)-8:8,mode?5:0,mask};
        be32(stack,0x18000);for(unsigned i=0;i<11;++i)be32(stack+4+i*4,args[i]);
        ck(uc_mem_write(u,0x90000,stack,sizeof stack));uint32_t sp=0x90000;
        ck(uc_reg_write(u,UC_M68K_REG_A7,&sp));
        ck(uc_emu_start(u,0x10000,0x18000,0,100000));
        ck(uc_mem_read(u,0x30000,actual,sizeof actual));
        ck(uc_mem_read(u,0x60000,saved,sizeof saved));
        if(memcmp(actual,pixels,sizeof pixels))fail("native material/residual mask boundary");
        for(unsigned i=0;i<64;++i)if(saved[i]!=0xa5)fail("native mask saved background");
        ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));if(sp!=0x90004)fail("mask boundary stack");
    }
    uc_close(u);puts("68020 car draw/restore: 1024 full-frame rotation/ramp/mask/ABI + 512 restore width/tail/alignment + 768 all-mask boundary comparisons pass");return 0;
}
