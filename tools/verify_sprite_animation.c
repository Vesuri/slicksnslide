#define SLICKS_CHAIN_HELPERS_ONLY 1
#include "verify_sprite_restore_chain.c"

int main(void)
{
    static struct SlicksRaceRuntime race;
    static unsigned char pixels[64000], expected[64000], actual[64000];
    static unsigned char araw[32800], praw[2400];
    unsigned char code[4096], rows[1024];
    FILE *f=fopen("build/sprite_animation.bin","rb");if(!f)return 2;
    size_t size=fread(code,1,sizeof code,f);fclose(f);
    unsigned entry=0x12000+read32(code+size-4);
    uc_engine *u;check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    check(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));
    check(uc_mem_map(u,0,sizeof allowed,UC_PROT_ALL));
    check(uc_mem_write(u,0x12000,code,size));
    for(unsigned y=0;y<256;++y)be32(rows+4*y,y*320);
    check(uc_mem_write(u,0x80000,rows,sizeof rows));
    uc_hook hook;check(uc_hook_add(u,&hook,UC_HOOK_MEM_WRITE,writes,0,1,0));
    const int regs[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,
        UC_M68K_REG_D5,UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,
        UC_M68K_REG_A3,UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6};
    static const unsigned frame_map[5][4]={{0,5,6,7},{1,1,1,1},{2,8,9,10},{3,3,3,3},{4,11,12,13}};
    unsigned successes=0,rejections=0,resized=0;
    for(unsigned trial=0;trial<4096;++trial) {
        unsigned mode=trial%32, h=1+(trial/32)%199;
        memset(&race,0,sizeof race);race.chunky=pixels;
        race.sprite_dirty_deferred=1;race.track_actors_ready=1;
        for(unsigned i=0;i<64000;++i)pixels[i]=(i*23+trial*17)^(i>>7);
        for(unsigned i=0;i<14;++i) {
            struct SlicksTrackActorAsset *s=&race.track_actor_assets[i];
            s->width=1+(trial/32+i)%12;s->height=1+(trial/384+i)%10;
            s->opacity_ready=1;
            for(unsigned j=0;j<128;++j) {
                s->pixels[j]=(j+trial+i)%5 ? (1+(j*7+i)%255) : 0;
                s->opacity[j]=s->pixels[j]?0:255;
            }
        }
        struct SlicksWeaponActor *a=&race.weapons.actors[h];
        __typeof__(race.sprite_dirty_previous[0]) *p=&race.sprite_dirty_previous[h];
        a->kind=p->kind=3;a->asset=p->asset=(trial/32)%5;
        a->motion.frame=(trial/160)%4;p->frame=(a->motion.frame+1)%4;
        a->colour=p->colour=trial;a->priority=p->priority=trial/5;
        a->retain=mode&1?8:0;a->saved=mode&1;
        struct SlicksTrackActorAsset *s=&race.track_actor_assets[frame_map[a->asset][(unsigned char)a->motion.frame]];
        a->old_width=p->width=1+(trial/11)%12;
        a->old_height=p->height=1+(trial/19)%10;
        a->old_x=p->x=(trial*7)%300;a->old_y=p->y=(trial*11)%180;
        a->motion.x=a->old_x*64+(trial&63);a->motion.y=a->old_y*64+((trial>>2)&63);
        for(unsigned j=0;j<128;++j)a->saved_under[j]=j*37+trial;
        race.weapons.slots.state[h]=1;
        unsigned accept=mode<8;
        switch(mode) {
        case 0: break;
        case 1: a->old_x=p->x=0;a->old_y=p->y=0;a->motion.x=a->motion.y=0;break;
        case 2: a->old_x=p->x=320-s->width;a->motion.x=a->old_x*64;break;
        case 3: a->old_y=p->y=200-s->height;a->motion.y=a->old_y*64;break;
        case 4: s->width=128;s->height=1;a->old_x=p->x=0;a->motion.x=0;break;
        case 5: s->width=1;s->height=128;a->old_y=p->y=0;a->motion.y=0;break;
        case 6: memset(s->pixels,0,128);memset(s->opacity,255,128);break;
        case 7: memset(s->pixels,255,128);memset(s->opacity,0,128);break;
        case 8: a->kind=1;break;
        case 9: p->kind=0;break;
        case 10: a->occlusion=p->occlusion=15;break;
        case 11: a->asset=p->asset=5;break;
        case 12: p->asset^=1;break;
        case 13: p->frame=a->motion.frame;break;
        case 14: a->motion.frame=4;break;
        case 15: a->motion.frame=-1;break;
        case 16: p->colour^=1;break;
        case 17: p->priority^=1;break;
        case 18: p->occlusion=1;break;
        case 19: a->motion.x=-1;break;
        case 20: a->motion.y=-1;break;
        case 21: a->motion.x+=64;break;
        case 22: a->motion.y+=64;break;
        case 23: a->old_x^=1;break;
        case 24: a->old_y^=1;break;
        case 25: a->old_width^=1;break;
        case 26: a->old_height^=1;break;
        case 27: s->opacity_ready=0;break;
        case 28: s->width=0;break;
        case 29: s->height=0;break;
        case 30: s->width=129;s->height=1;break;
        case 31: a->old_x=p->x=320;a->motion.x=320*64;break;
        }
        actors(araw,&race);previous(praw,&race);
        check(uc_mem_write(u,0x30000,pixels,64000));
        check(uc_mem_write(u,0xa0000,araw,sizeof araw));
        check(uc_mem_write(u,0xb0000,praw,sizeof praw));
        check(uc_mem_write(u,0xc0000,race.track_actor_assets,sizeof race.track_actor_assets));
        memset(allowed,0,sizeof allowed);memset(allowed+0x8ffc0,1,64);
        if(accept) {
            for(unsigned y=0;y<s->height;++y)
                memset(allowed+0x30000+(p->y+y)*320+p->x,1,s->width);
            memset(allowed+0xa0000+h*164+30,1,4);
            memset(allowed+0xa0000+h*164+36,1,s->width*s->height);
            allowed[0xb0000+h*12+6]=1;
            resized+=s->width!=p->width || s->height!=p->height;
            draw_weapon_actor_general(&race,h);++successes;
        } else ++rejections;
        memcpy(expected,pixels,64000);actors(araw,&race);previous(praw,&race);
        unsigned args[]={0x18000,0xa0000+h*164,0xb0000+h*12,0xc0000,0x30000};
        unsigned char stack[20];for(unsigned i=0;i<5;++i)be32(stack+4*i,args[i]);
        check(uc_mem_write(u,0x90000,stack,sizeof stack));
        uint32_t sp=0x90000,pc,result;check(uc_reg_write(u,UC_M68K_REG_A7,&sp));
        for(unsigned i=0;i<11;++i){uint32_t v=0x34560000+i;check(uc_reg_write(u,regs[i],&v));}
        check(uc_emu_start(u,entry,0x18000,0,100000));
        check(uc_reg_read(u,UC_M68K_REG_PC,&pc));check(uc_reg_read(u,UC_M68K_REG_A7,&sp));
        check(uc_reg_read(u,UC_M68K_REG_D0,&result));
        if(pc!=0x18000 || sp!=0x90004 || result!=accept) {
            fprintf(stderr,"animation trial=%u mode=%u result=%u expected=%u pc=%x\n",trial,mode,result,accept,pc);return 1;
        }
        for(unsigned i=0;i<11;++i){uint32_t v;check(uc_reg_read(u,regs[i],&v));if(v!=0x34560000+i)fail("animation register ABI");}
        check(uc_mem_read(u,0x30000,actual,64000));if(memcmp(actual,expected,64000))fail("animation pixels");
        check(uc_mem_read(u,0xa0000,actual,sizeof araw));if(memcmp(actual,araw,sizeof araw))fail("animation actors");
        check(uc_mem_read(u,0xb0000,actual,sizeof praw));if(memcmp(actual,praw,sizeof praw))fail("animation previous descriptors");
    }
    if(successes!=1024 || rejections!=3072 || !resized)fail("animation coverage");
    printf("Animation candidate: %u draws (%u resized), %u write-free rejections; exact full pixels/actors/descriptors, write bounds and ABI passed. Dirty publication is not exercised.\n",successes,resized,rejections);
    check(uc_close(u));return 0;
}
