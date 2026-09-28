#define SLICKS_CHAIN_HELPERS_ONLY 1
#include "verify_sprite_restore_chain.c"

static unsigned offset(const char *name)
{
    FILE *f=fopen("build/offsets/race_offsets.i","r");char line[256];
    if(!f)exit(2);
    while(fgets(line,sizeof line,f))if(!strncmp(line,name,strlen(name)) && line[strlen(name)]==' ') {
        unsigned value=strtoul(strstr(line,"equ")+3,0,0);fclose(f);return value;
    }
    fail("missing offset");return 0;
}
static void dirty_image(unsigned char *image,const struct SlicksRaceRuntime *r,unsigned rows,unsigned count)
{
    image[count]=r->dirty_row_count;
    for(unsigned i=0;i<16;++i) {
        const struct SlicksDirtyRows *d=&r->dirty_rows[i];
        be16(image+rows+i*8,d->left);be16(image+rows+i*8+2,d->top);
        be16(image+rows+i*8+4,d->right);be16(image+rows+i*8+6,d->bottom);
    }
}
int main(void)
{
    static struct SlicksRaceRuntime race,initial;
    static unsigned char pixels[64000],before[64000],actual[64000],expected[64000];
    static unsigned char araw[32800],praw[2400],state[16384],packets[64*300];
    unsigned char code[8192],rows[1024];
    FILE *f=fopen("build/sprite_opaque.bin","rb");if(!f)return 2;
    size_t n=fread(code,1,sizeof code,f);fclose(f);
    unsigned entry=0x12000+read32(code+n-20);
    unsigned rows_at=offset("RACE_DIRTY_ROWS"),count_at=offset("RACE_DIRTY_ROW_COUNT");
    if(rows_at+128>sizeof state || count_at>=sizeof state)fail("state test bounds");
    uc_engine *u;check(uc_open(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,&u));
    check(uc_ctl_set_cpu_model(u,UC_CPU_M68K_M68020));
    check(uc_mem_map(u,0,sizeof allowed,UC_PROT_ALL));check(uc_mem_write(u,0x12000,code,n));
    f=fopen("build/dirty_rect.bin","rb");if(!f)return 2;
    n=fread(code,1,sizeof code,f);fclose(f);check(uc_mem_write(u,0x1a000,code,n));
    for(unsigned y=0;y<256;++y)be32(rows+4*y,y*320);
    check(uc_mem_write(u,0x80000,rows,sizeof rows));
    uc_hook hook;check(uc_hook_add(u,&hook,UC_HOOK_MEM_WRITE,writes,0,1,0));
    const int regs[]={UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,
        UC_M68K_REG_D5,UC_M68K_REG_D6,UC_M68K_REG_D7,UC_M68K_REG_A2,
        UC_M68K_REG_A3,UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6};
    static const unsigned map[4]={0,5,6,7};
    unsigned changes=0,unchanged=0,kept=0,boundaries=0;
    for(unsigned trial=0;trial<768;++trial) {
        memset(&race,0,sizeof race);race.chunky=pixels;
        race.track_actors_ready=race.sprite_dirty_deferred=1;
        race.dirty_row_count=trial%17;
        for(unsigned i=0;i<16;++i)race.dirty_rows[i]=(struct SlicksDirtyRows){
            (i%10)*32,(i*13)%180,(i%10)*32+32,(i*13)%180+20};
        for(unsigned i=0;i<64000;++i)pixels[i]=(i*37+trial*11)^(i>>4);
        for(unsigned i=0;i<14;++i) {
            struct SlicksTrackActorAsset *s=&race.track_actor_assets[i];
            s->width=1+(i+trial)%12;s->height=1+(i+trial/12)%10;s->opacity_ready=1;
            for(unsigned p=0;p<128;++p){s->pixels[p]=(p+trial+i)%4?1+(p*11+i)%255:0;s->opacity[p]=s->pixels[p]?0:255;}
        }
        unsigned h=1+trial%64,stop=0;
        unsigned char next[200]={0},trail[400];memset(trail,255,sizeof trail);
        for(unsigned j=0;j<3;++j) {
            unsigned id=h+j*64,role=(trial+j)%3,frame=(trial+j)%4;
            struct SlicksWeaponActor *a=&race.weapons.actors[id];
            __typeof__(race.sprite_dirty_previous[0]) *p=&race.sprite_dirty_previous[id];
            a->kind=p->kind=3;a->asset=p->asset=0;
            a->motion.frame=frame;p->frame=role==0?(frame+1)%4:frame;
            const struct SlicksTrackActorAsset *old=&race.track_actor_assets[map[p->frame]];
            a->old_width=p->width=old->width;a->old_height=p->height=old->height;
            a->old_x=p->x=20+(trial%20)+j;a->old_y=p->y=30+(trial%40)+j;
            a->motion.x=a->old_x*64+(trial&63);a->motion.y=a->old_y*64;
            a->priority=p->priority=6;a->colour=p->colour=trial;
            a->saved=role==2;a->retain=role==2?2:8;race.weapons.slots.state[id]=1;
            for(unsigned p=0;p<128;++p)a->saved_under[p]=p*17+id;
            if(j<2)next[id]=id+64;
        }
        unsigned last=h+128;
        if(trial%4==1){be16(trail+last*2,0);stop=last;}
        if(trial%4==2){race.weapons.actors[last].motion.x+=64;race.weapons.actors[last].retain=0;stop=last;}
        if(trial%4==3){race.weapons.actors[last].motion.x=-1;race.weapons.actors[last].retain=0;stop=last;}
        initial=race;memcpy(before,pixels,64000);
        memset(packets,0xcc,sizeof packets);
        for(unsigned i=0;i<64;++i)packets[i*300+33]=0;
        check(uc_mem_write(u,0xd0000,packets,sizeof packets));
        for(unsigned pass=0;pass<3;++pass) {
            race=initial;memcpy(pixels,before,64000);
            actors(araw,&race);previous(praw,&race);
            check(uc_mem_write(u,0xa0000,araw,sizeof araw));check(uc_mem_write(u,0xb0000,praw,sizeof praw));
            check(uc_mem_write(u,0x30000,pixels,64000));
            check(uc_mem_write(u,0xc0000,race.track_actor_assets,sizeof race.track_actor_assets));
            check(uc_mem_write(u,0xb1000,next,sizeof next));check(uc_mem_write(u,0xb2000,trail,sizeof trail));
            memset(state,0xa5,sizeof state);dirty_image(state,&race,rows_at,count_at);
            check(uc_mem_write(u,0x50000,state,sizeof state));
            memset(allowed,0,sizeof allowed);memset(allowed+0x8ff00,1,256);
            for(unsigned id=h;id && id!=stop;id=next[id]) {
                struct SlicksWeaponActor *a=&race.weapons.actors[id];
                __typeof__(race.sprite_dirty_previous[0]) *p=&race.sprite_dirty_previous[id];
                allowed[0xa0000+id*164+33]=1;
                if(a->retain&2){a->retain=1;++kept;continue;}
                unsigned changed=a->motion.frame!=p->frame;
                const struct SlicksTrackActorAsset *s=&race.track_actor_assets[map[(unsigned char)a->motion.frame]];
                for(unsigned y=0;y<s->height;++y)memset(allowed+0x30000+(p->y+y)*320+p->x,1,s->width);
                memset(allowed+0xa0000+id*164+30,1,4);
                memset(allowed+0xa0000+id*164+36,1,s->width*s->height);
                allowed[0xb0000+id*12+6]=1;
                memset(allowed+0xd0000+(id&63)*300,1,300);
                if(changed){memset(allowed+0x50000+rows_at,1,128);allowed[0x50000+count_at]=1;++changes;}
                else ++unchanged;
                draw_weapon_actor_general(&race,id);
                if(!changed)a->retain=1;
            }
            if(stop)++boundaries;
            memcpy(expected,pixels,64000);actors(araw,&race);previous(praw,&race);dirty_image(state,&race,rows_at,count_at);
            unsigned args[]={0x18000,0xa0000,0xb0000,0xc2000,0xc0000,0x30000,0xb1000,0xb2000,h,0xd0000,0x50000};
            unsigned char stack[44];for(unsigned i=0;i<11;++i)be32(stack+4*i,args[i]);
            check(uc_mem_write(u,0x90000,stack,sizeof stack));
            uint32_t sp=0x90000,pc,result;check(uc_reg_write(u,UC_M68K_REG_A7,&sp));
            for(unsigned i=0;i<11;++i){uint32_t v=0x34560000+i;check(uc_reg_write(u,regs[i],&v));}
            check(uc_emu_start(u,entry,0x18000,0,1000000));
            check(uc_reg_read(u,UC_M68K_REG_PC,&pc));check(uc_reg_read(u,UC_M68K_REG_A7,&sp));check(uc_reg_read(u,UC_M68K_REG_D0,&result));
            if(pc!=0x18000 || sp!=0x90004 || result!=stop){fprintf(stderr,"animation chain trial=%u pass=%u result=%u expected=%u pc=%x\n",trial,pass,result,stop,pc);return 1;}
            for(unsigned i=0;i<11;++i){uint32_t v;check(uc_reg_read(u,regs[i],&v));if(v!=0x34560000+i)fail("animation chain ABI");}
            check(uc_mem_read(u,0x30000,actual,64000));if(memcmp(actual,expected,64000))fail("animation chain pixels");
            check(uc_mem_read(u,0xa0000,actual,sizeof araw));if(memcmp(actual,araw,sizeof araw))fail("animation chain actors");
            check(uc_mem_read(u,0xb0000,actual,sizeof praw));if(memcmp(actual,praw,sizeof praw))fail("animation chain previous");
            check(uc_mem_read(u,0x50000,actual,sizeof state));if(memcmp(actual,state,sizeof state))fail("animation chain dirty state");
        }
    }
    if(!changes || !unchanged || !kept || !boundaries)fail("animation chain coverage");
    printf("Animation chain: 2304 cold/repeated aliased chains; %u changed, %u unchanged, %u kept; %u point/moving/clipped boundaries. Full pixels, sprite/dirty state, bounds and ABI passed.\n",changes,unchanged,kept,boundaries);
    check(uc_close(u));return 0;
}
