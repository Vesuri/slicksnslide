/* Native optimization regression against the DOS-verified scalar point path.
 * Original x86 pixel/layer comparisons remain in verify-track-actors. */
#define main surface_verifier_main
#include "verify_surface_effects.c"
#undef main
#include <unicorn/unicorn.h>
#include <unicorn/m68k.h>
#include <stdint.h>

static void ck(uc_err e) { if(e) { fprintf(stderr,"Unicorn: %s\n",uc_strerror(e));exit(1); } }
static void be16(unsigned char *p,unsigned n) { p[0]=n>>8;p[1]=n; }
static void be32(unsigned char *p,uint32_t n) { be16(p,n>>16);be16(p+2,n); }
static void packed(unsigned char p[24],const struct SlicksTrailParticle *s)
{
    be32(p,s->x);be32(p+4,s->y);be16(p+8,s->velocity_x);be16(p+10,s->velocity_y);
    be16(p+12,s->old_x);be16(p+14,s->old_y);
    p[16]=s->saved_under;p[17]=s->lifetime;p[18]=s->colour;p[19]=s->priority;
    p[20]=s->saved_valid;p[21]=s->permanent;p[22]=s->occlusion_limit;p[23]=s->state;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    unsigned char code[4096];FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    size_t size=fread(code,1,sizeof code,f);fclose(f);
    uc_engine *u;ck(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    ck(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));ck(uc_mem_map(u,0,0x100000,UC_PROT_ALL));
    ck(uc_mem_write(u,0x10000,code,size));
    unsigned char rows[1024];for(unsigned y=0;y<256;++y)be32(rows+y*4,y*320);
    ck(uc_mem_write(u,0x80000,rows,sizeof rows));
    static struct SlicksRaceRuntime race;
    static unsigned char pixels[64000],before[64000],actual[64000],dirty[2048],got_dirty[2048];
    const unsigned addresses[]={0x20000,0x30000,0x40000,0x50000,0x60000,0x70000,0x80000};
    const int preserved[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,
        UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,UC_M68K_REG_A3,UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6};
    for(unsigned trial=0;trial<2048;++trial) {
        memset(&race,0,sizeof race);race.chunky=pixels;
        for(unsigned i=0;i<64000;++i) {
            pixels[i]=before[i]=(unsigned char)(i*31+trial);
            if(i<sizeof race.material_map) {
                race.material_map[i]=(i+trial)%32;race.surface_map[i]=(i*3+trial)%8;
            }
        }
        struct SlicksTrailParticle p={0};
        p.x=(long)((int)(trial%324)-2)*64+(trial&63);
        p.y=(long)((int)((trial*7)%204)-2)*64+(trial&63);
        if(trial&128) { p.x=6400+(trial&63);p.y=6400+(trial&63); }
        p.old_x=(trial&8)?100:(short)(p.x>>6);p.old_y=(trial&16)?100:(short)(p.y>>6);
        p.saved_valid=trial&3;p.colour=trial;p.saved_under=trial>>1;
        p.occlusion_limit=(trial&4)?15:0;
        p.lifetime=7;p.priority=3;p.permanent=1;p.state=5;
        race.dirty_pixel_count=trial%4==0?510:trial%4==1?511:trial%4==2?512:0;
        unsigned count=race.dirty_pixel_count;
        unsigned char original[24],expected[24],got[24],count_bytes[2],stack[32];
        packed(original,&p);ck(uc_mem_write(u,addresses[0],original,24));
        ck(uc_mem_write(u,addresses[1],pixels,sizeof pixels));
        ck(uc_mem_write(u,addresses[2],race.material_map,sizeof race.material_map));
        ck(uc_mem_write(u,addresses[3],race.surface_map,sizeof race.surface_map));
        memset(dirty,0,sizeof dirty);ck(uc_mem_write(u,addresses[4],dirty,sizeof dirty));
        be16(count_bytes,count);ck(uc_mem_write(u,addresses[5],count_bytes,2));
        be32(stack,0x18000);for(unsigned i=0;i<7;++i)be32(stack+4+i*4,addresses[i]);
        ck(uc_mem_write(u,0x90000,stack,sizeof stack));uint32_t sp=0x90000;
        ck(uc_reg_write(u,UC_M68K_REG_A7,&sp));
        for(unsigned i=0;i<sizeof preserved/sizeof *preserved;++i) {
            uint32_t v=0x11223300+i;ck(uc_reg_write(u,preserved[i],&v));
        }
        ck(uc_emu_start(u,0x10000,0x18000,0,10000));
        uint32_t result;ck(uc_reg_read(u,UC_M68K_REG_D0,&result));
        if(result!=(count>510))fail("native point overflow return");
        if(!result) draw_trail_point(&race,&p);
        packed(expected,&p);ck(uc_mem_read(u,addresses[0],got,24));
        ck(uc_mem_read(u,addresses[1],actual,sizeof actual));
        ck(uc_mem_read(u,addresses[4],got_dirty,sizeof got_dirty));
        ck(uc_mem_read(u,addresses[5],count_bytes,2));
        for(unsigned i=0;i<race.dirty_pixel_count;++i) {
            be16(dirty+i*4,race.dirty_pixels[i].x);dirty[i*4+2]=race.dirty_pixels[i].y;
        }
        if(memcmp(expected,got,24)||memcmp(pixels,actual,sizeof pixels)||
           memcmp(dirty,got_dirty,sizeof dirty)||
           ((count_bytes[0]<<8)|count_bytes[1])!=race.dirty_pixel_count) {
            fprintf(stderr,"native particle mismatch trial=%u\n",trial);return 1;
        }
        ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));if(sp!=0x90004)fail("native point stack");
        for(unsigned i=0;i<sizeof preserved/sizeof *preserved;++i) {
            uint32_t v;ck(uc_reg_read(u,preserved[i],&v));
            if(v!=0x11223300+i)fail("native point preserved register");
        }
    }
    uint32_t batch_entry=0x10000+((uint32_t)code[size-8]<<24)+((uint32_t)code[size-7]<<16)+((uint32_t)code[size-6]<<8)+code[size-5];
    uint32_t chain_entry=0x10000+((uint32_t)code[size-4]<<24)+((uint32_t)code[size-3]<<16)+((uint32_t)code[size-2]<<8)+code[size-1];
    for(unsigned trial=0;trial<512;++trial) {
        unsigned limit=trial<256 || (trial&32)?32:trial%33;
        struct SlicksTrailParticle points[256];
        unsigned char packed_points[256*24],got_points[256*24],order[64],stack[44],count_bytes[2];
        memset(&race,0,sizeof race);race.chunky=pixels;
        memset(pixels,40,sizeof pixels);memset(dirty,0,sizeof dirty);
        memset(race.material_map,1,sizeof race.material_map);
        memset(race.surface_map,trial&7,sizeof race.surface_map);
        for(unsigned i=0;i<256;++i) {
            points[i]=(struct SlicksTrailParticle){.x=6400+(trial&63),.y=6400,
                .old_x=99,.old_y=100,.saved_valid=trial&3,.colour=i,
                .occlusion_limit=(unsigned char)((i+trial)%20)};
            if(i%7==0) points[i].x=-1;
            packed(packed_points+i*24,&points[i]);
        }
        /* Exercise every pool offset, including slot 255, in both walkers. */
        for(unsigned i=0;i<32;++i)be16(order+i*2,(trial+31-i)&255);
        race.dirty_pixel_count=trial%4?0:500+(trial%13);
        be16(count_bytes,race.dirty_pixel_count);
        ck(uc_mem_write(u,0x20000,packed_points,sizeof packed_points));
        ck(uc_mem_write(u,0x30000,pixels,sizeof pixels));
        ck(uc_mem_write(u,0x40000,race.material_map,sizeof race.material_map));
        ck(uc_mem_write(u,0x50000,race.surface_map,sizeof race.surface_map));
        ck(uc_mem_write(u,0x60000,dirty,sizeof dirty));ck(uc_mem_write(u,0x70000,count_bytes,2));
        ck(uc_mem_write(u,0x81000,order,sizeof order));
        be32(stack,0x18000);for(unsigned i=0;i<7;++i)be32(stack+4+i*4,addresses[i]);
        be32(stack+32,0x81000);be32(stack+36,32);
        if(trial>=256) {
            unsigned char next[200]={0},map[400];memset(map,255,sizeof map);
            for(unsigned h=1;h<=32;++h) {
                unsigned handle=1+((h-1)*73)%199;
                next[handle]=h==32?0:1+(h*73)%199;
                if(h<=limit)be16(map+handle*2,(trial+32-h)&255);
            }
            ck(uc_mem_write(u,0x82000,next,sizeof next));
            ck(uc_mem_write(u,0x83000,map,sizeof map));
            be32(stack+32,0x82000);be32(stack+36,0x83000);be32(stack+40,1);
        }
        ck(uc_mem_write(u,0x90000,stack,sizeof stack));uint32_t sp=0x90000;
        ck(uc_reg_write(u,UC_M68K_REG_A7,&sp));
        for(unsigned i=0;i<sizeof preserved/sizeof *preserved;++i) {
            uint32_t v=0x55667700+i;ck(uc_reg_write(u,preserved[i],&v));
        }
        ck(uc_emu_start(u,trial<256?batch_entry:chain_entry,0x18000,0,100000));
        uint32_t result;ck(uc_reg_read(u,UC_M68K_REG_D0,&result));
        unsigned processed=0;
        for(;processed<limit && race.dirty_pixel_count<=510;++processed)
            draw_trail_point(&race,&points[(trial+31-processed)&255]);
        unsigned expected_result=trial<256?processed:processed==32?0:1+(processed*73)%199;
        if(result!=expected_result)fail("batch/chain point processed prefix");
        for(unsigned i=0;i<256;++i)packed(packed_points+i*24,&points[i]);
        for(unsigned i=0;i<race.dirty_pixel_count;++i) {
            be16(dirty+i*4,race.dirty_pixels[i].x);dirty[i*4+2]=race.dirty_pixels[i].y;
        }
        ck(uc_mem_read(u,0x20000,got_points,sizeof got_points));
        ck(uc_mem_read(u,0x30000,actual,sizeof actual));
        ck(uc_mem_read(u,0x60000,got_dirty,sizeof got_dirty));
        ck(uc_mem_read(u,0x70000,count_bytes,2));
        if(memcmp(packed_points,got_points,sizeof got_points)||memcmp(pixels,actual,sizeof pixels)||
           memcmp(dirty,got_dirty,sizeof dirty)||((count_bytes[0]<<8)|count_bytes[1])!=race.dirty_pixel_count)
            fail("batch point state/order mismatch");
        ck(uc_reg_read(u,UC_M68K_REG_A7,&sp));if(sp!=0x90004)fail("batch stack");
        for(unsigned i=0;i<sizeof preserved/sizeof *preserved;++i) {
            uint32_t v;ck(uc_reg_read(u,preserved[i],&v));
            if(v!=0x55667700+i)fail("batch preserved register");
        }
    }
    uc_close(u);puts("68020 point draw: 2048 single + 256 ordered-batch + 256 actor-chain full-frame, metadata, dirty-list, overflow and ABI cases pass");
    return 0;
}
