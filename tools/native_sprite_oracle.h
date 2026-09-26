/* Run the production sprite fast-path assembly inside the existing original
 * DOS actor comparisons. The scalar executable remains a separate test. */
#include <unicorn/m68k.h>
static uc_engine *sprite_cpu;
static uint32_t sprite_restore,sprite_visible,sprite_fast,sprite_restore_fast;
static unsigned sprite_fast_hits,sprite_restore_hits;
static unsigned sprite_fast_active;
static unsigned sprite_at,sprite_width,sprite_height,sprite_saved;
static void sprite_store(uc_engine *u,uc_mem_type type,uint64_t addr,int size,
    int64_t value,void *context)
{
    (void)type;(void)value;(void)context;
    for(int i=0;i<size;++i) {
        unsigned a=(unsigned)addr+i;
        if(sprite_fast_active && !(a>=0x30000 && a<0x40000) &&
           a!=0x70020 && !(a>=0x70022 && a<0x70022+sprite_width*sprite_height) &&
           !(sprite_fast_active==2 ? (a>=0x71000 && a<0x7100c) : a==0x71006) &&
           !(a>=0x8ff00 && a<0x90004)) { uc_emu_stop(u);abort(); }
        if(a>=0x30000 && a<0x40000) {
            if(a<0x30000+sprite_at || (a-0x30000-sprite_at)/320>=sprite_height ||
                (a-0x30000-sprite_at)%320>=sprite_width) { uc_emu_stop(u);abort(); }
        } else if(a>=0x60000 && a<0x61000 && a-0x60000>=sprite_saved) {
            uc_emu_stop(u);abort();
        }
    }
}
static void sprite_long(unsigned char *p,uint32_t n)
{ p[0]=n>>24;p[1]=n>>16;p[2]=n>>8;p[3]=n; }
static void sprite_init(void)
{
    if(sprite_cpu)return;
    unsigned char code[4096];FILE *f=fopen("build/car_draw.bin","rb");if(!f)abort();
    size_t n=fread(code,1,sizeof code,f);fclose(f);
    sprite_restore=0x10000+((uint32_t)code[n-4]<<24)+((uint32_t)code[n-3]<<16)+
        ((uint32_t)code[n-2]<<8)+code[n-1];
    check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&sprite_cpu));
    check(uc_ctl_set_cpu_model(sprite_cpu,UC_CPU_M68K_M68020));
    check(uc_mem_map(sprite_cpu,0,0x100000,UC_PROT_ALL));
    check(uc_mem_write(sprite_cpu,0x10000,code,n));
    f=fopen("build/sprite_opaque.bin","rb");if(!f)abort();
    n=fread(code,1,sizeof code,f);fclose(f);
    check(uc_mem_write(sprite_cpu,0x12000,code,n));
    sprite_visible=0x12000+((uint32_t)code[n-4]<<24)+((uint32_t)code[n-3]<<16)+
        ((uint32_t)code[n-2]<<8)+code[n-1];
    sprite_fast=0x12000+((uint32_t)code[n-8]<<24)+((uint32_t)code[n-7]<<16)+
        ((uint32_t)code[n-6]<<8)+code[n-5];
    sprite_restore_fast=0x12000+((uint32_t)code[n-12]<<24)+((uint32_t)code[n-11]<<16)+
        ((uint32_t)code[n-10]<<8)+code[n-9];
    unsigned char rows[1024];for(unsigned y=0;y<256;++y)sprite_long(rows+4*y,y*320);
    check(uc_mem_write(sprite_cpu,0x80000,rows,sizeof rows));
    uc_hook h;check(uc_hook_add(sprite_cpu,&h,UC_HOOK_MEM_WRITE,sprite_store,0,1,0));
}
static void sprite_run(unsigned entry,const unsigned *args,unsigned count)
{
    unsigned char stack[48];sprite_long(stack,0x18000);
    for(unsigned i=0;i<count;++i)sprite_long(stack+4+i*4,args[i]);
    check(uc_mem_write(sprite_cpu,0x90000,stack,4+count*4));
    uint32_t sp=0x90000,pc;
    check(uc_reg_write(sprite_cpu,UC_M68K_REG_A7,&sp));
    const int preserved[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,
        UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,UC_M68K_REG_A3,UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6};
    for(unsigned i=0;i<sizeof preserved/sizeof *preserved;++i){uint32_t v=0x34560000+i;check(uc_reg_write(sprite_cpu,preserved[i],&v));}
    check(uc_emu_start(sprite_cpu,entry,0x18000,0,100000));
    check(uc_reg_read(sprite_cpu,UC_M68K_REG_A7,&sp));
    check(uc_reg_read(sprite_cpu,UC_M68K_REG_PC,&pc));
    if(sp!=0x90004 || pc!=0x18000)abort();
    for(unsigned i=0;i<sizeof preserved/sizeof *preserved;++i){uint32_t v;check(uc_reg_read(sprite_cpu,preserved[i],&v));if(v!=0x34560000+i)abort();}
}
void slicks_draw_car_chunky(unsigned char *dst,const unsigned char *src,unsigned char *saved,
    const unsigned char *material,const unsigned char *surface,unsigned width,unsigned height,
    int dx,int dy,unsigned ramp,unsigned limit)
{
    sprite_init();if(dx!=1 || dy!=(int)width || ramp)abort();
    sprite_at=(unsigned)(dst-native);sprite_width=width;sprite_height=height;
    sprite_saved=width*height;
    unsigned span=(height-1)*320+width;
    check(uc_mem_write(sprite_cpu,0x20000,src,sprite_saved));
    check(uc_mem_write(sprite_cpu,0x30000+sprite_at,dst,span));
    check(uc_mem_write(sprite_cpu,0x60000,saved,sprite_saved));
    if(limit) {
        check(uc_mem_write(sprite_cpu,0x40000,material,span));
        check(uc_mem_write(sprite_cpu,0x50000,surface,span));
    }
    unsigned args[]={0x30000+sprite_at,0x20000,0x60000,0x40000,0x50000,
        width,height,1,width,0,limit};
    sprite_run(0x10000,args,11);
    check(uc_mem_read(sprite_cpu,0x30000+sprite_at,dst,span));
    check(uc_mem_read(sprite_cpu,0x60000,saved,sprite_saved));
}
void slicks_restore_car_chunky(unsigned char *dst,const unsigned char *saved,unsigned width,unsigned height)
{
    sprite_init();sprite_at=(unsigned)(dst-native);sprite_width=width;sprite_height=height;
    sprite_saved=width*height;
    unsigned span=(height-1)*320+width;
    check(uc_mem_write(sprite_cpu,0x30000+sprite_at,dst,span));
    check(uc_mem_write(sprite_cpu,0x60000,saved,sprite_saved));
    unsigned args[]={0x30000+sprite_at,0x60000,width,height};
    sprite_run(sprite_restore,args,4);
    check(uc_mem_read(sprite_cpu,0x30000+sprite_at,dst,span));
}

static void sprite_masked_copy(unsigned char *dst,const unsigned char *src,unsigned char *saved,
    const unsigned char *opacity,unsigned width,unsigned height,unsigned visible)
{
    sprite_init();sprite_at=(unsigned)(dst-native);sprite_width=width;sprite_height=height;
    sprite_saved=width*height;
    unsigned span=(height-1)*320+width;
    check(uc_mem_write(sprite_cpu,0x20000,src,sprite_saved));
    check(uc_mem_write(sprite_cpu,0x30000+sprite_at,dst,span));
    check(uc_mem_write(sprite_cpu,0x60000,saved,sprite_saved));
    check(uc_mem_write(sprite_cpu,0x70000,opacity,sprite_saved));
    unsigned args[]={0x30000+sprite_at,0x20000,0x60000,0x70000,width,height};
    sprite_run(visible?sprite_visible:0x12000,args,6);
    check(uc_mem_read(sprite_cpu,0x30000+sprite_at,dst,span));
    check(uc_mem_read(sprite_cpu,0x60000,saved,sprite_saved));
}

void slicks_draw_sprite_opaque(unsigned char *dst,const unsigned char *src,unsigned char *saved,
    const unsigned char *mask,unsigned width,unsigned height)
{ sprite_masked_copy(dst,src,saved,mask,width,height,0); }
void slicks_draw_sprite_visible(unsigned char *dst,const unsigned char *src,unsigned char *saved,
    const unsigned char *mask,unsigned width,unsigned height)
{ sprite_masked_copy(dst,src,saved,mask,width,height,1); }

int slicks_draw_unchanged_track_sprite(struct SlicksWeaponActor *a,void *previous,void *visibility,
    const struct SlicksTrackActorAsset *assets,unsigned char *chunky,unsigned deferred)
{
    sprite_init();
    __typeof__(race.sprite_dirty_previous[0]) *p=previous;
    __typeof__(race.track_sprite_visibility[0]) *c=visibility;
    unsigned char actor[sizeof *a],description[12],cache[136];
    memcpy(actor,a,sizeof actor);memcpy(description,p,sizeof description);memcpy(cache,c,sizeof cache);
    actor[0]=(unsigned short)a->motion.x>>8;actor[1]=a->motion.x;
    actor[2]=(unsigned short)a->motion.y>>8;actor[3]=a->motion.y;
    actor[26]=(unsigned short)a->old_x>>8;actor[27]=a->old_x;
    actor[28]=(unsigned short)a->old_y>>8;actor[29]=a->old_y;
    description[0]=(unsigned short)p->x>>8;description[1]=p->x;
    description[2]=(unsigned short)p->y>>8;description[3]=p->y;
    cache[0]=(unsigned short)c->x>>8;cache[1]=c->x;
    cache[2]=(unsigned short)c->y>>8;cache[3]=c->y;
    sprite_at=(unsigned)(p->y*320+p->x);sprite_width=p->width;sprite_height=p->height;
    sprite_saved=0;
    check(uc_mem_write(sprite_cpu,0x30000,chunky,64000));
    check(uc_mem_write(sprite_cpu,0x70000,actor,sizeof actor));
    check(uc_mem_write(sprite_cpu,0x71000,description,sizeof description));
    check(uc_mem_write(sprite_cpu,0x72000,cache,sizeof cache));
    check(uc_mem_write(sprite_cpu,0x74000,assets,14*sizeof *assets));
    unsigned args[]={0x70000,0x71000,0x72000,0x74000,0x30000,deferred};
    sprite_fast_active=1;sprite_run(sprite_fast,args,6);sprite_fast_active=0;
    uint32_t result;check(uc_reg_read(sprite_cpu,UC_M68K_REG_D0,&result));
    check(uc_mem_read(sprite_cpu,0x30000,chunky,64000));
    check(uc_mem_read(sprite_cpu,0x70000,actor,sizeof actor));
    check(uc_mem_read(sprite_cpu,0x71000,description,sizeof description));
    a->saved=actor[32];memcpy(a->saved_under,actor+34,sizeof a->saved_under);p->kind=description[6];
    if(result)++sprite_fast_hits;
    return (int)result;
}

int slicks_restore_actor_sprite(struct SlicksWeaponActor *a,void *previous,unsigned char *chunky)
{
    sprite_init();
    __typeof__(race.sprite_dirty_previous[0]) *p=previous;
    unsigned char actor[sizeof *a],description[12];
    memcpy(actor,a,sizeof actor);memcpy(description,p,sizeof description);
    actor[26]=(unsigned short)a->old_x>>8;actor[27]=a->old_x;
    actor[28]=(unsigned short)a->old_y>>8;actor[29]=a->old_y;
    description[0]=(unsigned short)p->x>>8;description[1]=p->x;
    description[2]=(unsigned short)p->y>>8;description[3]=p->y;
    sprite_at=(unsigned)(a->old_y*320+a->old_x);
    sprite_width=a->old_width;sprite_height=a->old_height;sprite_saved=0;
    check(uc_mem_write(sprite_cpu,0x30000,chunky,64000));
    check(uc_mem_write(sprite_cpu,0x70000,actor,sizeof actor));
    check(uc_mem_write(sprite_cpu,0x71000,description,sizeof description));
    unsigned args[]={0x70000,0x71000,0x30000};
    sprite_fast_active=2;sprite_run(sprite_restore_fast,args,3);sprite_fast_active=0;
    uint32_t result;check(uc_reg_read(sprite_cpu,UC_M68K_REG_D0,&result));
    check(uc_mem_read(sprite_cpu,0x30000,chunky,64000));
    check(uc_mem_read(sprite_cpu,0x70000,actor,sizeof actor));
    check(uc_mem_read(sprite_cpu,0x71000,description,sizeof description));
    if(memcmp(actor+34,a->saved_under,sizeof a->saved_under))abort();
    a->saved=actor[32];memcpy(p,description,sizeof description);
    p->x=(short)((unsigned)description[0]*256+description[1]);
    p->y=(short)((unsigned)description[2]*256+description[3]);
    if(result)++sprite_restore_hits;
    return (int)result;
}
