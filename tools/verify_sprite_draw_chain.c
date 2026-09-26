#define SLICKS_CHAIN_HELPERS_ONLY 1
#include "verify_sprite_restore_chain.c"
int main(void)
{
    static struct SlicksRaceRuntime race;
    static struct SlicksRaceRuntime initial_race;
    static unsigned char before[64000],packets[64*300];
    static unsigned char pixels[64000],expected[64000],araw[32800],praw[2400],actual[64000],cache[64*136];
    unsigned char code[8192],rows[1024];
    FILE *f=fopen("build/sprite_opaque.bin","rb");if(!f)return 2;
    size_t bytes=fread(code,1,sizeof code,f);fclose(f);
    unsigned entry=0x12000+read32(code+bytes-20);
    uc_engine *u;check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    check(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));
    check(uc_mem_map(u,0,sizeof allowed,UC_PROT_ALL));check(uc_mem_write(u,0x12000,code,bytes));
    for(unsigned y=0;y<256;++y)be32(rows+4*y,y*320);
    check(uc_mem_write(u,0x80000,rows,sizeof rows));
    uc_hook hook;check(uc_hook_add(u,&hook,UC_HOOK_MEM_WRITE,writes,0,1,0));
    unsigned drawn=0;
    for(unsigned trial=0;trial<384;++trial) {
        memset(&race,0,sizeof race);race.chunky=pixels;race.sprite_dirty_deferred=1;race.track_actors_ready=1;
        memset(race.sprite_dirty_previous,0xcc,sizeof race.sprite_dirty_previous);
        for(unsigned i=0;i<64000;++i) {
            pixels[i]=(unsigned char)(i*19+trial);
            if(i<60800){race.material_map[i]=(i%23)>>3;race.surface_map[i]=(i%23)&7;}
        }
        for(unsigned i=0;i<14;++i) {
            struct SlicksTrackActorAsset *a=&race.track_actor_assets[i];
            a->width=1+(i+trial)%12;a->height=1+(trial/12+i)%10;a->opacity_ready=1;
            for(unsigned p=0;p<(unsigned)a->width*a->height;++p) {
                unsigned v=(p*13+i*7+trial)%17;a->pixels[p]=v<4?0:v;
                a->opacity[p]=a->pixels[p]?0:255;
            }
        }
        unsigned char next[200]={0},trail[400];memset(trail,255,sizeof trail);
        unsigned head=trial<128?1:trial<256?65:169;
        unsigned stop=0,first=head;
        for(unsigned i=0;i<31;++i) {
            unsigned h=first+i;if(i<30)next[h]=h+1;
            struct SlicksWeaponActor *a=&race.weapons.actors[h];
            a->kind=3;a->asset=i%5;a->motion.frame=(trial+i)%4;
            static const unsigned frames[5][4]={{0,5,6,7},{1,1,1,1},{2,8,9,10},{3,3,3,3},{4,11,12,13}};
            const struct SlicksTrackActorAsset *asset=&race.track_actor_assets[frames[a->asset][(unsigned char)a->motion.frame]];
            a->old_width=asset->width;a->old_height=asset->height;
            a->old_x=(trial+i*7)%(321-a->old_width);a->old_y=(trial+i*3)%(191-a->old_height);
            if(trial%11==0)a->old_y=190;
            else if(trial%13==0)a->old_y=188; /* Cross the foreground-map bottom. */
            a->motion.x=a->old_x*64;a->motion.y=a->old_y*64;
            a->priority=i*3;a->occlusion=trial%3?15:0;a->colour=trial+i;
            for(unsigned p=0;p<128;++p)a->saved_under[p]=p*23+h;
            race.weapons.slots.state[h]=1;
            __typeof__(race.sprite_dirty_previous[0]) *previous=&race.sprite_dirty_previous[h];
            previous->x=a->old_x;previous->y=a->old_y;previous->width=a->old_width;previous->height=a->old_height;
            previous->kind=3;previous->asset=a->asset;previous->frame=a->motion.frame;
            previous->colour=a->colour;previous->priority=a->priority;previous->occlusion=a->occlusion;
            __typeof__(race.track_sprite_visibility[0]) *v=&race.track_sprite_visibility[h&63];
            v->x=a->old_x;v->y=a->old_y;v->asset=a->asset;v->frame=a->motion.frame;v->occlusion=a->occlusion;v->valid=1;
            for(unsigned y=0;y<a->old_height;++y)for(unsigned x=0;x<a->old_width;++x) {
                unsigned p=y*a->old_width+x,at=(a->old_y+y)*320+a->old_x+x;
                v->write_mask[p]=asset->pixels[p] && (a->old_y+y>=190 || actor_pixel_visible(&race,at,a->occlusion))?255:0;
            }
            if(i==trial%32 && trial%7!=6) {
                stop=h;
                switch(trial%7) {
                case 0:be16(trail+2*h,0);break;
                case 1:previous->kind=0;break;
                case 2:a->motion.x+=64;break;
                case 3:previous->colour^=1;break;
                case 4:a->old_x=previous->x=-1;a->motion.x=-64;break;
                case 5:previous->frame^=1;break;
                }
            }
        }
        if(trial%31==0)head=stop=0;
        initial_race=race;memcpy(before,pixels,sizeof before);
        unsigned original_stop=stop;
        memset(packets,0,sizeof packets);check(uc_mem_write(u,0xd0000,packets,sizeof packets));
        for(unsigned pass=0;pass<5;++pass) {
        race=initial_race;memcpy(pixels,before,sizeof pixels);stop=original_stop;
        if(pass==2)for(unsigned h=head;h && h!=stop;h=next[h]) {
            if(race.weapons.actors[h].occlusion && race.weapons.actors[h].old_y<190) {
                race.track_sprite_visibility[h&63].valid=0;stop=h;break;
            }
        }
        if(pass==3 && head && head!=stop) {
            race.weapons.actors[head].motion.frame^=1;stop=head;
        }
        if(pass==4)for(unsigned h=head;h && h!=stop;h=next[h])
            ++race.weapons.actors[h].motion.x;
        actors(araw,&race);previous(praw,&race);
        for(unsigned i=0;i<64;++i) {
            memcpy(cache+136*i,&race.track_sprite_visibility[i],136);
            be16(cache+136*i,race.track_sprite_visibility[i].x);be16(cache+136*i+2,race.track_sprite_visibility[i].y);
        }
        check(uc_mem_write(u,0x30000,pixels,sizeof pixels));
        check(uc_mem_write(u,0xa0000,araw,sizeof araw));check(uc_mem_write(u,0xb0000,praw,sizeof praw));
        check(uc_mem_write(u,0xb1000,next,sizeof next));check(uc_mem_write(u,0xb2000,trail,sizeof trail));
        check(uc_mem_write(u,0xc0000,race.track_actor_assets,sizeof race.track_actor_assets));
        check(uc_mem_write(u,0xc2000,cache,sizeof cache));
        memset(allowed,0,sizeof allowed);memset(allowed+0x8ff00,1,0x128);
        unsigned pass_draws=0;
        for(unsigned h=head;h && h!=stop;h=next[h]) {
            struct SlicksWeaponActor *a=&race.weapons.actors[h];
            for(unsigned y=0;y<a->old_height;++y)memset(allowed+0x30000+(a->old_y+y)*320+a->old_x,1,a->old_width);
            allowed[0xa0000+h*164+32]=1;memset(allowed+0xa0000+h*164+36,1,a->old_width*a->old_height);
            allowed[0xb0000+h*12+6]=1;
            memset(allowed+0xd0000+(h&63)*300,1,300);
            draw_weapon_actor_general(&race,h);++drawn;++pass_draws;
        }
        memcpy(expected,pixels,sizeof expected);actors(araw,&race);previous(praw,&race);
        unsigned args[]={0x18000,0xa0000,0xb0000,0xc2000,0xc0000,0x30000,0xb1000,0xb2000,head,0xd0000};
        unsigned char stack[40];for(unsigned i=0;i<10;++i)be32(stack+4*i,args[i]);
        check(uc_mem_write(u,0x90000,stack,sizeof stack));
        uint32_t sp=0x90000,pc,result;check(uc_reg_write(u,UC_M68K_REG_A7,&sp));
        const int regs[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,
            UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,UC_M68K_REG_A3,UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6};
        for(unsigned i=0;i<11;++i){uint32_t v=0x34560000+i;check(uc_reg_write(u,regs[i],&v));}
        packet_writes=0;check(uc_emu_start(u,entry,0x18000,0,1000000));
        check(uc_reg_read(u,UC_M68K_REG_PC,&pc));check(uc_reg_read(u,UC_M68K_REG_A7,&sp));
        check(uc_reg_read(u,UC_M68K_REG_D0,&result));
        if(pc!=0x18000 || sp!=0x90004 || result!=stop){fprintf(stderr,"draw chain trial %u returned %u expected %u\n",trial,result,stop);return 1;}
        for(unsigned i=0;i<11;++i){uint32_t v;check(uc_reg_read(u,regs[i],&v));if(v!=0x34560000+i)fail("draw chain register ABI");}
        check(uc_mem_read(u,0x30000,actual,64000));if(memcmp(actual,expected,64000))fail("draw chain pixels");
        check(uc_mem_read(u,0xa0000,actual,32800));if(memcmp(actual,araw,32800))fail("draw chain actor state");
        check(uc_mem_read(u,0xb0000,actual,2400));if(memcmp(actual,praw,2400))fail("draw chain descriptors");
        if(pass && packet_writes)fail("warm packet was rebuilt instead of reused");
        if(!pass && pass_draws && !packet_writes)fail("cold packets were not populated");
        }
    }
    printf("Native sprite drawing chain: 1920 cold/warm/stale-mask/changed-frame/fractional-position cases, %u draws, exact pixels/state/write bounds/fallback/ABI passed\n",drawn);
}
