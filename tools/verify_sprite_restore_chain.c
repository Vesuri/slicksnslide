#include <unicorn/unicorn.h>
#include <unicorn/m68k.h>
#define main surface_verifier_main
#include "verify_surface_effects.c"
#undef main
static unsigned char allowed[0x100000];
static void check(uc_err e) { if(e){fprintf(stderr,"%s\n",uc_strerror(e));exit(1);} }
static void be16(unsigned char *p,unsigned v) {p[0]=v>>8;p[1]=v;}
static void be32(unsigned char *p,unsigned v) {p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static unsigned read32(const unsigned char *p) {return (unsigned)p[0]<<24|p[1]<<16|p[2]<<8|p[3];}
static void writes(uc_engine *u,uc_mem_type t,uint64_t a,int n,int64_t v,void *c)
{
    (void)u;(void)t;(void)v;(void)c;
    for(int i=0;i<n;++i)if(a+i>=sizeof allowed || !allowed[a+i])fail("chain wrote outside its exact destinations");
}
static void actors(unsigned char *out,const struct SlicksRaceRuntime *r)
{
    for(unsigned i=0;i<200;++i) {
        const struct SlicksWeaponActor *a=&r->weapons.actors[i];
        memcpy(out+i*164,a,164);be16(out+i*164,a->motion.x);be16(out+i*164+2,a->motion.y);
        be16(out+i*164+26,a->old_x);be16(out+i*164+28,a->old_y);
    }
}
static void previous(unsigned char *out,const struct SlicksRaceRuntime *r)
{
    for(unsigned i=0;i<200;++i) {
        memcpy(out+i*12,&r->sprite_dirty_previous[i],12);
        be16(out+i*12,r->sprite_dirty_previous[i].x);be16(out+i*12+2,r->sprite_dirty_previous[i].y);
    }
}
#ifndef SLICKS_CHAIN_HELPERS_ONLY
int main(void)
{
    _Static_assert(sizeof(struct SlicksWeaponActor)==164,"host actor serialization");
    static struct SlicksRaceRuntime race;
    static unsigned char pixels[64000],expected[64000],araw[32800],praw[2400],actual[64000];
    unsigned char code[8192],rows[1024];
    FILE *f=fopen("build/sprite_opaque.bin","rb");if(!f)return 2;
    size_t bytes=fread(code,1,sizeof code,f);fclose(f);
    unsigned entry=0x12000+read32(code+bytes-16);
    uc_engine *u;check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    check(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));
    check(uc_mem_map(u,0,sizeof allowed,UC_PROT_ALL));check(uc_mem_write(u,0x12000,code,bytes));
    for(unsigned y=0;y<256;++y)be32(rows+4*y,y*320);
    check(uc_mem_write(u,0x80000,rows,sizeof rows));
    uc_hook hook;check(uc_hook_add(u,&hook,UC_HOOK_MEM_WRITE,writes,0,1,0));
    unsigned restored=0;
    for(unsigned trial=0;trial<384;++trial) {
        memset(&race,0,sizeof race);race.chunky=pixels;race.sprite_dirty_deferred=1;
        memset(race.sprite_dirty_previous,0xcc,sizeof race.sprite_dirty_previous);
        memset(race.sprite_dirty_handles,0xcc,sizeof race.sprite_dirty_handles);
        race.sprite_dirty_count=trial%5;
        unsigned initial_count=race.sprite_dirty_count;
        for(unsigned i=0;i<64000;++i)pixels[i]=(unsigned char)(i*19+trial);
        unsigned char next[200]={0},trail[400];
        memset(trail,255,sizeof trail);
        unsigned head=0,last=0;
        for(unsigned i=0;i<31;++i) {
            unsigned h=1+(i*37+trial)%199;
            if(last)next[last]=h;else head=h;last=h;
            struct SlicksWeaponActor *a=&race.weapons.actors[h];
            a->old_x=5+(trial+i*13)%300;a->old_y=(trial+i*7)%180;
            a->old_width=1+(trial+i)%12;a->old_height=1+(trial/12+i)%10;
            a->kind=1+i%3;a->asset=i;a->motion.frame=i%4;
            a->priority=i*3;a->occlusion=i*7;a->colour=trial+i;a->saved=1;
            for(unsigned p=0;p<128;++p)a->saved_under[p]=p*23+h;
            if(i==trial%32) switch(trial%6) {
            case 0: be16(trail+2*h,0);break;
            case 1: a->saved=0;break;
            case 2: a->old_x=-1;break;
            case 3: a->old_y=199;break;
            case 4: a->old_width=0;break;
            default: break;
            }
        }
        if(trial%31==0)head=0;
        actors(araw,&race);previous(praw,&race);
        check(uc_mem_write(u,0x30000,pixels,sizeof pixels));
        check(uc_mem_write(u,0xa0000,araw,sizeof araw));check(uc_mem_write(u,0xb0000,praw,sizeof praw));
        check(uc_mem_write(u,0xb1000,next,sizeof next));check(uc_mem_write(u,0xb2000,trail,sizeof trail));
        check(uc_mem_write(u,0xb3000,race.sprite_dirty_handles,200));
        unsigned char count[2];be16(count,initial_count);check(uc_mem_write(u,0xb4000,count,2));
        memset(allowed,0,sizeof allowed);memset(allowed+0x8ff00,1,0x124);
        unsigned stop=head;
        while(stop && (short)((trail[stop*2]<<8)|trail[stop*2+1])<0) {
            struct SlicksWeaponActor *a=&race.weapons.actors[stop];
            if(!a->saved || a->old_x<0 || a->old_y<0 || !a->old_width || !a->old_height ||
               a->old_x+a->old_width>320 || a->old_y+a->old_height>200)break;
            for(unsigned y=0;y<a->old_height;++y)
                memset(allowed+0x30000+(a->old_y+y)*320+a->old_x,1,a->old_width);
            allowed[0xa0000+stop*164+32]=1;memset(allowed+0xb0000+stop*12,1,12);
            allowed[0xb3000+race.sprite_dirty_count]=1;memset(allowed+0xb4000,1,2);
            restore_weapon_actor(&race,stop);++restored;stop=next[stop];
        }
        memcpy(expected,pixels,sizeof expected);actors(araw,&race);previous(praw,&race);
        unsigned args[]={0x18000,0xa0000,0xb0000,0x30000,0xb1000,0xb2000,head,0xb3000,0xb4000};
        unsigned char stack[36];for(unsigned i=0;i<9;++i)be32(stack+4*i,args[i]);
        check(uc_mem_write(u,0x90000,stack,sizeof stack));
        uint32_t sp=0x90000,pc,result;check(uc_reg_write(u,UC_M68K_REG_A7,&sp));
        const int regs[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,
            UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,UC_M68K_REG_A3,UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6};
        for(unsigned i=0;i<11;++i){uint32_t v=0x34560000+i;check(uc_reg_write(u,regs[i],&v));}
        check(uc_emu_start(u,entry,0x18000,0,1000000));
        check(uc_reg_read(u,UC_M68K_REG_PC,&pc));check(uc_reg_read(u,UC_M68K_REG_A7,&sp));
        check(uc_reg_read(u,UC_M68K_REG_D0,&result));if(pc!=0x18000 || sp!=0x90004 || result!=stop)fail("chain return/stack");
        for(unsigned i=0;i<11;++i){uint32_t v;check(uc_reg_read(u,regs[i],&v));if(v!=0x34560000+i)fail("chain register ABI");}
        check(uc_mem_read(u,0x30000,actual,64000));if(memcmp(actual,expected,64000))fail("chain pixels");
        check(uc_mem_read(u,0xa0000,actual,32800));if(memcmp(actual,araw,32800))fail("chain actor state");
        check(uc_mem_read(u,0xb0000,actual,2400));if(memcmp(actual,praw,2400))fail("chain descriptors");
        check(uc_mem_read(u,0xb3000,actual,200));if(memcmp(actual,race.sprite_dirty_handles,200))fail("chain dirty handles");
        check(uc_mem_read(u,0xb4000,count,2));if(((count[0]<<8)|count[1])!=race.sprite_dirty_count)fail("chain count");
    }
    printf("Native sprite restoration chain: 384 cases, %u restorations, exact state/write bounds/fallback/ABI passed\n",restored);
}
#endif
