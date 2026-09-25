/* Included by race_runtime.c: actors share its authoritative chunky surface,
 * producer-side dirty bounds and foreground-mask predicate. */
static unsigned weapon_bucket(unsigned priority)
{ return priority==0?0:priority==3?1:priority==5?2:3; }

static void restore_weapon_actor(struct SlicksRaceRuntime *race,unsigned handle)
{
    struct SlicksWeaponActor *a=&race->weapons.actors[handle];
    if(!a->saved) return;
    for(unsigned y=0;y<a->old_height;++y) for(unsigned x=0;x<a->old_width;++x) {
        int px=a->old_x+x,py=a->old_y+y;
        if(px>=0 && px<320 && py>=0 && py<200)
            race->chunky[mult320[py]+px]=a->saved_under[y*a->old_width+x];
    }
    mark_dirty_rect(race,a->old_x,a->old_y,a->old_x+a->old_width,a->old_y+a->old_height);
    a->saved=0;
}

static void draw_weapon_actor(struct SlicksRaceRuntime *race,unsigned handle)
{
    struct SlicksWeaponActor *a=&race->weapons.actors[handle];
    if(!a->kind || race->weapons.slots.state[handle]<=0) return;
    unsigned width=1,height=1;
    const unsigned char *pixels=0;
    if(a->kind==1) {
        unsigned frame=a->asset+(unsigned char)a->motion.frame;
        if(frame>=SLICKS_WEAPON_ASSET_COUNT || !race->weapons.assets[frame].ready) {
            race->collision_error=1;return;
        }
        const struct SlicksWeaponAsset *asset=&race->weapons.assets[frame];
        pixels=asset->pixels;width=asset->width;height=asset->height;
    } else if(a->kind==3) {
        static const unsigned char frames[5][4]={{0,5,6,7},{1,1,1,1},{2,8,9,10},{3,3,3,3},{4,11,12,13}};
        if(a->asset>=5 || (unsigned char)a->motion.frame>=4 || !race->track_actors_ready) {
            race->collision_error=1;return;
        }
        const struct SlicksTrackActorAsset *asset=&race->track_actor_assets[frames[a->asset][(unsigned char)a->motion.frame]];
        pixels=asset->pixels;width=asset->width;height=asset->height;
    }
    short left=a->motion.x>>6,top=a->motion.y>>6;
    /* The unmasked primitive suppresses a negative origin, including the
     * setup flag's x=-1 sentinel. The masked primitive clips separately. */
    if(pixels && !a->occlusion && left<0) { a->saved=0;return; }
    if(left>=320 || top>=(pixels?200:SLICKS_POINT_HEIGHT) ||
       left+(int)width<=0 || top+(int)height<=0) { a->saved=0;return; }
    for(unsigned y=0;y<height;++y) for(unsigned x=0;x<width;++x) {
        int px=left+(int)x,py=top+(int)y;
        if(px<0 || px>=320 || py<0 || py>=(pixels?200:SLICKS_POINT_HEIGHT)) continue;
        unsigned long at=mult320[py]+px;
        unsigned i=y*width+x;
        a->saved_under[i]=race->chunky[at];
        unsigned char pixel=pixels?pixels[i]:a->colour;
        /* The original raw mask's 190..199 tail is explicitly cleared. */
        if((!pixels || pixel) && (py>=190 || actor_pixel_visible(race,at,a->occlusion)))
            race->chunky[at]=pixel;
    }
    a->old_x=left;a->old_y=top;a->old_width=width;a->old_height=height;a->saved=1;
    mark_dirty_rect(race,left,top,left+width,top+height);
}

static short allocate_weapon_actor(struct SlicksRaceRuntime *race,int asset,
    unsigned frames,unsigned char colour)
{
    short h=slicks_actor_allocate(&race->weapons.slots,1);
    if(!h) return 0;
    struct SlicksWeaponActor *a=&race->weapons.actors[h];
    *a=(struct SlicksWeaponActor){0};
    a->kind=asset<0?2:1;a->asset=asset<0?0:(unsigned char)asset;
    a->motion.frames=(unsigned char)frames;a->colour=colour;
    race->weapons.trail_index[h]=-1;
    return h;
}

static void configure_weapon_actor(struct SlicksRaceRuntime *race,short handle,
    short x,short y,short vx,short vy,short life,unsigned priority,unsigned mask)
{
    if(handle<=0 || handle>=SLICKS_ACTOR_CAPACITY) return;
    struct SlicksWeaponActor *a=&race->weapons.actors[handle];
    a->motion.x=(short)((unsigned short)x<<6);a->motion.y=(short)((unsigned short)y<<6);
    a->motion.vx=vx;a->motion.vy=vy;a->motion.ax=a->motion.ay=0;
    a->motion.lifetime=life;a->motion.period=a->motion.frame=0;a->motion.age=0;
    a->priority=(unsigned char)priority;a->occlusion=(unsigned char)mask;
    race->weapons.slots.state[handle]=1;
}

static void advance_weapon_actors(struct SlicksRaceRuntime *race)
{
    if(!shared_actor_pool(race)) return;
    for(unsigned h=1;h<race->weapons.slots.high_water;++h) {
        if(race->weapons.trail_index[h]>=0) continue;
        struct SlicksWeaponActor *a=&race->weapons.actors[h];
        signed char *state=&race->weapons.slots.state[h];
        /* Reserved car/shadow slots are advanced by their existing owners. */
        if(!a->kind && !a->motion.lifetime) continue;
        if(*state==-2) { *state=0;a->kind=0;continue; }
        slicks_actor_advance(&a->motion,state,race->actor_page);
        if(*state==-1) *state=-2;
    }
}

static void initialize_weapon_actors(struct SlicksRaceRuntime *race)
{
    struct SlicksWeaponRuntime *w=&race->weapons;
    slicks_actor_slots_init(&w->slots);
    for(unsigned h=0;h<SLICKS_ACTOR_CAPACITY;++h) {
        w->trail_index[h]=-1;w->actors[h]=(struct SlicksWeaponActor){0};
    }
    for(unsigned d=0;d<4;++d) {
        w->controls[d]=(struct SlicksWeaponControl){0};
        for(unsigned p=0;p<30;++p) w->projectiles[d][p]=(struct SlicksWeaponProjectile){0};
    }
    /* Cars precede track actors (24f07), then the temporary intro (251f9)
     * and, only with weapons, the notice (25276). HUD text is not an actor. */
    for(unsigned i=0;i<4;++i) (void)slicks_actor_allocate(&w->slots,1);
    for(unsigned i=0;i<race->navigation.actor_count;++i) {
        short h=slicks_actor_allocate(&w->slots,1);
        race->track_actor_handles[i]=(unsigned char)h;
        if(!race->track_actors_ready) continue;
        static const signed char offset[5]={0,-2,0,-2,0};
        static const unsigned char priority[5]={6,1,6,4,6};
        const struct SlicksTrackActor *t=&race->navigation.actors[i];
        struct SlicksWeaponActor *a=&w->actors[h];
        a->kind=3;a->asset=t->kind;a->motion.frames=(t->kind==1 || t->kind==3)?1:4;
        configure_weapon_actor(race,h,t->kind==2?-1:(short)(t->x/16+offset[t->kind]),
            t->kind==2?1:(short)(t->y/16+offset[t->kind]),0,0,0,priority[t->kind],0);
        /* 2501c consumes one draw for every kind, even a hidden flag. */
        a->motion.period=(signed char)(((unsigned long)next_random(race)*8)>>15)+10;
    }
    short intro=slicks_actor_allocate(&w->slots,1);
    if(intro) { w->actors[intro].motion.lifetime=30;w->actors[intro].motion.frames=1; }
    short notice=race->weapons_enabled?slicks_actor_allocate(&w->slots,1):0;
    if(notice) { w->actors[notice].motion.lifetime=200;w->actors[notice].motion.frames=1; }
    for(unsigned d=0;d<4;++d) if(driver_role(race,d)) {
        short h=slicks_actor_allocate(&w->slots,1);
        if(h) w->slots.state[h]=-3;
    }
    w->shots=w->hits=w->explosions=0;
}

int slicks_race_add_weapon_asset(struct SlicksRaceRuntime *race,
    unsigned asset,const unsigned char *resource,unsigned long size)
{
    if(!race || asset>=SLICKS_WEAPON_ASSET_COUNT) return -1;
    struct SlicksWeaponAsset *a=&race->weapons.assets[asset];
    unsigned short width,height;
    if(slicks_decode_indexed_menu_icon(resource,size,a->pixels,sizeof a->pixels,&width,&height)) return -1;
    a->width=(unsigned char)width;a->height=(unsigned char)height;a->ready=1;
    return 0;
}
