#define main menu_icon_verifier_main
#include "verify_menu_icon.c"
#undef main
#include "../src/game/race_runtime.c"
static struct SlicksRaceRuntime race;
static unsigned char native[64000],before[64000];
/* DOS can write the 400-pixel virtual margin; compare the 320-pixel display. */
static void actor_pixel(uc_engine *u,uc_mem_type type,uint64_t address,int size,int64_t value,void *context)
{
    (void)u;(void)type;(void)context;
    unsigned at=(unsigned)(address-0xa0000);
    for(int b=0;b<size;++b)for(unsigned p=0;p<4;++p)if(mask&(1U<<p)) {
        unsigned x=((at+b)%100)*4+p,y=(at+b)/100;
        if(x<320&&y<200)dos[y*320+x]=(unsigned char)((uint64_t)value>>(8*b));
    }
}
int main(void)
{
    unsigned char runtime[300000];FILE *f=fopen("disasm/runtime.bin","rb");if(!f)return 2;
    size_t bytes=fread(runtime,1,sizeof runtime,f);fclose(f);
    uc_engine *u;check(uc_open(UC_ARCH_X86,UC_MODE_16,&u));check(uc_mem_map(u,0,0x100000,UC_PROT_ALL));check(uc_mem_write(u,0x10100,runtime,bytes));
    const unsigned addresses[]={0x373a7,0x317af,0x12d8a,0x1221d,0x13b07,0x119c2,0x36243};uc_hook h;
    for(unsigned i=0;i<sizeof addresses/sizeof *addresses;++i)check(uc_hook_add(u,&h,UC_HOOK_CODE,services,0,addresses[i],addresses[i]));
    check(uc_hook_add(u,&h,UC_HOOK_INSN,port,0,1,0,UC_X86_INS_OUT));
    check(uc_hook_add(u,&h,UC_HOOK_MEM_WRITE,actor_pixel,0,0xa0000,0xaffff));
    const char *names[]={"miina.ase","aikabomb.ase","flam_raj.@I","ohjus.1","ohjus.2","ohjus.3","ohjus.4","ohjus.5","ohjus.6","ohjus.7","ohjus.8",
        "savu.1","savu.2","savu.3","rajahdys.1","rajahdys.2","rajahdys.3","rajahdys.4","flash.@I"};
    unsigned cases=0;
    for(unsigned asset=0;asset<19;++asset) {
        long loaded=host_archive_load("ref/SLICKS.000",names[asset],source,sizeof source);if(loaded<0)return 2;length=loaded;cursor=allocations=0;
        word(u,0x8f004,0);word(u,0x8f006,0x9000);word(u,0x8f008,0);word(u,0x8f00a,0x6000);word(u,0x8f00c,2);start(u,0x2e51a);
        struct SlicksWeaponAsset *image=&race.weapons.assets[asset];unsigned short w,hgt;
        if(slicks_decode_indexed_menu_icon(source,length,image->pixels,sizeof image->pixels,&w,&hgt))abort();
        image->width=w;image->height=hgt;image->ready=1;
        for(unsigned trial=0;trial<128;++trial) {
            unsigned page=trial&1,limit=(trial&2)?15:0,pattern=(trial/4)%8;
            short x=(short)(100+trial%4),y=(short)(50+(trial/32)*45);
            if(y+hgt>200)y=(short)(200-hgt);
            if(trial&16)x=(short)(320-w);
            unsigned char raw[64000];
            for(unsigned i=0;i<64000;++i) {
                before[i]=native[i]=dos[i]=(unsigned char)(i*19+trial);
                raw[i]=i>=60800?0:(unsigned char)((i+pattern*17)%256);
                if(i<60800){race.material_map[i]=raw[i]>>3;race.surface_map[i]=raw[i]&7;}
            }
            check(uc_mem_write(u,0x70000,raw,sizeof raw));
            word(u,0x616ce,0);word(u,0x616d0,0x9000);word(u,0x616ca,0);word(u,0x616cc,0x7000);
            word(u,0x616c6,page);word(u,0x616c7,1);word(u,0x616c4,184);
            word(u,0x3cbf0+0x1d7d,320);word(u,0x3cbf0+0x1d89,0);word(u,0x61d7b,100);
            word(u,0x3cbf0+0x1d7b,100);word(u,0x3cbf0+0x1d8d,0);word(u,0x3cbf0+0x1d8f,200);word(u,0x3cbf0+0x1d91,0);word(u,0x3cbf0+0x1d93,79);
            unsigned char actor[64]={0};actor[0x18]=1;actor[0x3b]=limit;check(uc_mem_write(u,0x90040,actor,sizeof actor));
            word(u,0x90044+page*2,x);word(u,0x90048+page*2,y);word(u,0x9004c,0);word(u,0x9004e,0x5000);
            uint16_t cs=0x2e0f,ds=0x6000,ss=0x8000,sp=0xf000;
            word(u,0x8f000,0);word(u,0x8f002,0x5500);word(u,0x8f004,1);
            check(uc_reg_write(u,UC_X86_REG_CS,&cs));check(uc_reg_write(u,UC_X86_REG_DS,&ds));check(uc_reg_write(u,UC_X86_REG_SS,&ss));check(uc_reg_write(u,UC_X86_REG_SP,&sp));
            check(uc_emu_start(u,0x33673,0x55000,0,100000));
            race.chunky=native;race.weapons.slots.state[1]=1;race.weapons.actors[1]=(struct SlicksWeaponActor){.asset=asset,.kind=1,.occlusion=limit};
            race.weapons.actors[1].motion.x=x*64;race.weapons.actors[1].motion.y=y*64;draw_weapon_actor(&race,1);
            if(memcmp(native,dos,64000)) {
                for(unsigned i=0;i<64000;++i)if(native[i]!=dos[i]){fprintf(stderr,"Weapon actor %s trial=%u mask=%u pixel=%u,%u native=%u DOS=%u\n",names[asset],trial,limit,i%320,i/320,native[i],dos[i]);break;}return 1;
            }
            restore_weapon_actor(&race,1);if(memcmp(native,before,64000)){fputs("Weapon actor restore failed\n",stderr);return 1;}++cases;
        }
    }
    check(uc_close(u));printf("Original weapon actors: %u full-screen sprite/mask/page comparisons and exact restoration checks pass\n",cases);return 0;
}
