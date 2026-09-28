/* Included by race_runtime.c: actors share its authoritative chunky surface,
 * producer-side dirty bounds and foreground-mask predicate. */
static unsigned weapon_bucket(unsigned priority)
{ return priority==0?0:priority==3?1:priority==5?2:3; }

static void reset_weapon_actor(struct SlicksWeaponActor *actor)
{
    /* saved=0 invalidates the old background. Drawing saves every visible
     * pixel before restoration can read it; off-screen cells are never read.
     * Point actors use their separate one-byte saved background. */
#if defined(__m68k__)
    _Static_assert(__builtin_offsetof(struct SlicksWeaponActor,saved_under)==36 &&
        sizeof(struct SlicksWeaponActor)==164,
        "actor reset ABI");
    extern void slicks_actor_reset_native(struct SlicksWeaponActor *);
    slicks_actor_reset_native(actor);
#else
    __builtin_memset(actor,0,__builtin_offsetof(struct SlicksWeaponActor,saved_under));
#endif
}

static void restore_weapon_actor(struct SlicksRaceRuntime *race,unsigned handle)
{
    struct SlicksWeaponActor *a=&race->weapons.actors[handle];
#if defined(__m68k__)
    __asm__("" : "+a"(a));
#endif
    a->retain=0; /* only the native restoration chain keeps sprites */
    if(!a->saved) return;
#if defined(__m68k__) || defined(SLICKS_NATIVE_SPRITE_TEST)
    if(race->sprite_dirty_deferred) {
        extern int slicks_restore_actor_sprite(struct SlicksWeaponActor *,void *,unsigned char *);
        if(slicks_restore_actor_sprite(a,&race->sprite_dirty_previous[handle],race->chunky)) {
            race->sprite_dirty_handles[race->sprite_dirty_count++]=(unsigned char)handle;
            return;
        }
    }
#endif
    int left=a->old_x,top=a->old_y,width=a->old_width;
    int x0=left<0?-left:0,x1=320-left;
    int y0=top<0?-top:0,y1=200-top;
    if(x1>width) x1=width;
    if(y1>a->old_height) y1=a->old_height;
#if defined(__m68k__) || defined(SLICKS_NATIVE_SPRITE_TEST)
    if(left>=0 && top>=0 && left+width<=320 && top+a->old_height<=200) {
        extern void slicks_restore_car_chunky(unsigned char *,const unsigned char *,unsigned,unsigned);
        slicks_restore_car_chunky(race->chunky+mult320[top]+left,a->saved_under,width,a->old_height);
    } else
#endif
    if(x0<x1) for(int y=y0;y<y1;++y) {
        unsigned char *dst=race->chunky+mult320[top+y]+left+x0;
        const unsigned char *src=a->saved_under+y*width+x0;
        for(int n=x1-x0;n;--n) *dst++=*src++;
    }
    if(race->sprite_dirty_deferred) {
        /* Keep this separate from the reusable actor slot: it may be freed
         * and replaced by a point or another sprite before drawing. */
        __typeof__(race->sprite_dirty_previous[0]) *previous=&race->sprite_dirty_previous[handle];
#if defined(__m68k__)
        __asm__("" : "+a"(previous));
#endif
        previous->x=a->old_x;previous->y=a->old_y;
        previous->width=a->old_width;previous->height=a->old_height;
        previous->kind=a->kind;previous->asset=a->asset;
        previous->frame=a->motion.frame;previous->colour=a->colour;
        previous->priority=a->priority;previous->occlusion=a->occlusion;
        race->sprite_dirty_handles[race->sprite_dirty_count++]=(unsigned char)handle;
    } else mark_dirty_rect(race,a->old_x,a->old_y,a->old_x+a->old_width,a->old_y+a->old_height);
    a->saved=0;
}

#if defined(__m68k__)
__attribute__((optimize("O2"),noinline))
#endif
static void draw_weapon_actor_general(struct SlicksRaceRuntime *race,unsigned handle)
{
    struct SlicksWeaponActor *a=&race->weapons.actors[handle];
#if defined(__m68k__)
    /* Materialize these reusable bases once instead of repeating full
     * indexed addressing through the large race structure for each field. */
    __asm__("" : "+a"(a));
#endif
    a->retain=0; /* only native unchanged chain draws may retain */
    if(!a->kind || race->weapons.slots.state[handle]<=0) return;
    /* Per-sprite probes are intentionally opt-in: four callbacks for every
     * tiny sprite measurably perturb a chip-RAM-only 68020. */
#ifdef SLICKS_PROFILE_SPRITES
    int profile=profile_scope(race,7);
#else
    const int profile=0;
#endif
    if(profile)race->profile_marker(60);
    unsigned width=1,height=1;
    const unsigned char *pixels=0;
#if defined(__m68k__) || defined(SLICKS_NATIVE_SPRITE_TEST)
    const unsigned char *opacity=0;
#endif
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
#if defined(__m68k__) || defined(SLICKS_NATIVE_SPRITE_TEST)
        if(asset->opacity_ready)opacity=asset->opacity;
#endif
    }
    short left=a->motion.x>>6,top=a->motion.y>>6;
    /* The unmasked primitive suppresses a negative origin, including the
     * setup flag's x=-1 sentinel. The masked primitive clips separately. */
    if(pixels && !a->occlusion && left<0) { a->saved=0;return; }
    if(left>=320 || top>=(pixels?200:SLICKS_POINT_HEIGHT) ||
       left+(int)width<=0 || top+(int)height<=0) { a->saved=0;return; }
    int x0=left<0?-left:0,x1=320-left;
    int y0=top<0?-top:0,y1=(pixels?200:SLICKS_POINT_HEIGHT)-top;
    if(x1>(int)width) x1=width;
    if(y1>(int)height) y1=height;
    if(profile)race->profile_marker(61);
#if defined(__m68k__) || defined(SLICKS_NATIVE_SPRITE_TEST)
    if(opacity && (!a->occlusion || top>=190) && left>=0 && top>=0 &&
       left+(int)width<=320 && top+(int)height<=200) {
        extern void slicks_draw_sprite_opaque(unsigned char *,const unsigned char *,
            unsigned char *,const unsigned char *,unsigned,unsigned);
        slicks_draw_sprite_opaque(race->chunky+mult320[top]+left,pixels,a->saved_under,opacity,width,height);
    } else if(opacity && left>=0 && top>=0 && left+(int)width<=320 && top+(int)height<=200) {
        __typeof__(race->track_sprite_visibility[0]) *cache=&race->track_sprite_visibility[handle&63];
#if defined(__m68k__)
        __asm__("" : "+a"(cache));
#endif
        if(!cache->valid || cache->x!=left || cache->y!=top || cache->asset!=a->asset ||
           cache->frame!=(unsigned char)a->motion.frame || cache->occlusion!=a->occlusion) {
            unsigned i=0;
            for(unsigned y=0;y<height;++y) {
                unsigned row=mult320[top+y]+left;
                for(unsigned x=0;x<width;++x,++i)
                    cache->write_mask[i]=pixels[i] && (top+y>=190 ||
                        ((race->material_map[row+x]<<3)|(race->surface_map[row+x]&7))<=a->occlusion)?255:0;
            }
            cache->x=left;cache->y=top;cache->asset=a->asset;
            cache->frame=a->motion.frame;cache->occlusion=a->occlusion;cache->valid=1;
        }
        extern void slicks_draw_sprite_visible(unsigned char *,const unsigned char *,
            unsigned char *,const unsigned char *,unsigned,unsigned);
        slicks_draw_sprite_visible(race->chunky+mult320[top]+left,pixels,a->saved_under,
            cache->write_mask,width,height);
    } else if(pixels && left>=0 && top>=0 && left+(int)width<=320 && top+(int)height<=200) {
        extern void slicks_draw_car_chunky(unsigned char *,const unsigned char *,unsigned char *,
            const unsigned char *,const unsigned char *,unsigned,unsigned,int,int,unsigned,unsigned);
        unsigned masked=a->occlusion && top<190 ? (unsigned)(190-top) : 0;
        if(masked>height)masked=height;
        unsigned row=mult320[top]+left;
        if(masked) slicks_draw_car_chunky(race->chunky+row,pixels,a->saved_under,
            race->material_map+row,race->surface_map+row,width,masked,1,width,0,a->occlusion);
        if(masked<height) slicks_draw_car_chunky(race->chunky+row+mult320[masked],
            pixels+masked*width,a->saved_under+masked*width,
            race->material_map,race->surface_map,width,height-masked,1,width,0,0);
    } else
#endif
    for(int y=y0;y<y1;++y) {
        unsigned long at=mult320[top+y]+left+x0;
        unsigned i=y*width+x0;
        unsigned char *dst=race->chunky+at,*saved=a->saved_under+i;
        const unsigned char *src=pixels?pixels+i:0;
        /* The original raw mask's 190..199 tail is explicitly cleared. */
        if(!a->occlusion || top+y>=190) {
            for(int n=x1-x0;n;--n) {
                unsigned char pixel=src?*src++:a->colour;
                *saved++=*dst;
                if(!pixels || pixel) *dst=pixel;
                ++dst;
            }
        } else {
            const unsigned char *material=race->material_map+at;
            const unsigned char *surface=race->surface_map+at;
            for(int n=x1-x0;n;--n) {
                unsigned char pixel=src?*src++:a->colour;
                *saved++=*dst;
                if((!pixels || pixel) && ((*material<<3)|(*surface&7))<=a->occlusion)
                    *dst=pixel;
                ++dst;++material;++surface;
            }
        }
    }
    if(profile)race->profile_marker(62);
    a->old_x=left;a->old_y=top;a->old_width=width;a->old_height=height;a->saved=1;
    __typeof__(race->sprite_dirty_previous[0]) *previous=&race->sprite_dirty_previous[handle];
#if defined(__m68k__)
    __asm__("" : "+a"(previous));
#endif
    unsigned unchanged=race->sprite_dirty_deferred && previous->kind &&
        previous->x==left && previous->y==top && previous->width==width && previous->height==height &&
        previous->kind==a->kind && previous->asset==a->asset && previous->frame==(unsigned char)a->motion.frame &&
        previous->colour==a->colour && previous->priority==a->priority && previous->occlusion==a->occlusion;
    if(!unchanged) {
        if(race->sprite_dirty_deferred && previous->kind)
            mark_dirty_rect(race,previous->x,previous->y,previous->x+previous->width,previous->y+previous->height);
        mark_dirty_rect(race,left,top,left+width,top+height);
    }
    previous->kind=0;
    if(profile)race->profile_marker(63);
}

#if defined(__m68k__)
__attribute__((noinline))
#endif
static void draw_weapon_actor(struct SlicksRaceRuntime *race,unsigned handle)
{
#if (defined(__m68k__) || defined(SLICKS_NATIVE_SPRITE_TEST)) && !defined(SLICKS_PROFILE_SPRITES) && !defined(SLICKS_TRACK_FAST_CONTROL)
    /* Detailed per-sprite probes select the general path. The production
     * path keeps unchanged track geometry/style checks and painting native. */
    struct SlicksWeaponActor *a=&race->weapons.actors[handle];
    if(a->kind==3 && race->weapons.slots.state[handle]>0 && race->track_actors_ready &&
       race->sprite_dirty_deferred && race->sprite_dirty_previous[handle].kind==3) {
        _Static_assert(__builtin_offsetof(struct SlicksWeaponActor,asset)==20 &&
            __builtin_offsetof(struct SlicksWeaponActor,kind)==21 &&
            __builtin_offsetof(struct SlicksWeaponActor,priority)==22 &&
            __builtin_offsetof(struct SlicksWeaponActor,occlusion)==23 &&
            __builtin_offsetof(struct SlicksWeaponActor,colour)==24 &&
            __builtin_offsetof(struct SlicksWeaponActor,old_x)==26 &&
            __builtin_offsetof(struct SlicksWeaponActor,old_width)==30 &&
            __builtin_offsetof(struct SlicksWeaponActor,saved_under)==36 &&
            __builtin_offsetof(struct SlicksActorMotion,frame)==16,"native track actor ABI");
        _Static_assert(sizeof(struct SlicksTrackActorAsset)==259 &&
            __builtin_offsetof(struct SlicksTrackActorAsset,width)==128 &&
            __builtin_offsetof(struct SlicksTrackActorAsset,opacity)==130 &&
            __builtin_offsetof(struct SlicksTrackActorAsset,opacity_ready)==258,"native track asset ABI");
        _Static_assert(sizeof race->sprite_dirty_previous[0]==12 &&
            __builtin_offsetof(__typeof__(race->sprite_dirty_previous[0]),kind)==6 &&
            __builtin_offsetof(__typeof__(race->sprite_dirty_previous[0]),priority)==10 &&
            sizeof race->track_sprite_visibility[0]==136 &&
            __builtin_offsetof(__typeof__(race->track_sprite_visibility[0]),write_mask)==8,"native track cache ABI");
        extern int slicks_draw_unchanged_track_sprite(struct SlicksWeaponActor *,void *,void *,
            const struct SlicksTrackActorAsset *,unsigned char *,unsigned);
        if(slicks_draw_unchanged_track_sprite(a,&race->sprite_dirty_previous[handle],
            &race->track_sprite_visibility[handle&63],race->track_actor_assets,race->chunky,1))return;
    }
#endif
    draw_weapon_actor_general(race,handle);
}

static void finish_sprite_dirty_batch(struct SlicksRaceRuntime *race)
{
    for(unsigned i=0;i<race->sprite_dirty_count;++i) {
        unsigned h=race->sprite_dirty_handles[i];
        __typeof__(race->sprite_dirty_previous[0]) *previous=&race->sprite_dirty_previous[h];
        /* No successful redraw: expired, hidden, or reused as a point. */
        if(previous->kind)
            mark_dirty_rect(race,previous->x,previous->y,previous->x+previous->width,previous->y+previous->height);
        previous->kind=0;
    }
    race->sprite_dirty_count=0;race->sprite_dirty_deferred=0;
}

static short allocate_weapon_actor(struct SlicksRaceRuntime *race,int asset,
    unsigned frames,unsigned char colour)
{
    short h=slicks_actor_allocate(&race->weapons.slots,1);
    if(!h) return 0;
    slicks_retention.geometry_dirty=1;
    struct SlicksWeaponActor *a=&race->weapons.actors[h];
    reset_weapon_actor(a);
    a->kind=asset<0?2:1;a->asset=asset<0?0:(unsigned char)asset;
    a->motion.frames=(unsigned char)frames;a->colour=colour;
    race->weapons.trail_index[h]=-1;
    if(race->weapons.sprite_high_water && h>=race->weapons.sprite_high_water)
        race->weapons.sprite_high_water=(unsigned char)(h+1);
    return h;
}

static void configure_weapon_actor(struct SlicksRaceRuntime *race,short handle,
    short x,short y,short vx,short vy,short life,unsigned priority,unsigned mask)
{
    if(handle<=0 || handle>=SLICKS_ACTOR_CAPACITY) return;
    slicks_retention.geometry_dirty=1;
    struct SlicksWeaponActor *a=&race->weapons.actors[handle];
    a->motion.x=(short)((unsigned short)x<<6);a->motion.y=(short)((unsigned short)y<<6);
    a->motion.vx=vx;a->motion.vy=vy;a->motion.ax=a->motion.ay=0;
    a->motion.lifetime=life;a->motion.period=a->motion.frame=0;a->motion.age=0;
    a->priority=(unsigned char)priority;a->occlusion=(unsigned char)mask;
    race->weapons.slots.state[handle]=1;
}

/* Verified reference; shared-pool m68k builds use track_motion.s. */
static __attribute__((unused)) void advance_weapon_actors_reference(struct SlicksRaceRuntime *race)
{
    for(unsigned h=1;h<race->weapons.slots.high_water;++h) {
        if(race->weapons.trail_index[h]>=0) continue;
        if(!race->weapons.slots.state[h]) continue;
        struct SlicksWeaponActor *a=&race->weapons.actors[h];
        signed char *state=&race->weapons.slots.state[h];
        /* The diagnostic/reference path may invalidate conservatively.
         * Native cache-hit correctness is audited without shadow execution. */
        if(a->kind==3) slicks_retention.geometry_dirty=1;
        /* Reserved car/shadow slots are advanced by their existing owners. */
        if(!a->kind && !a->motion.lifetime) continue;
        if(*state==-2) { *state=0;a->kind=0;continue; }
        slicks_actor_advance(&a->motion,state,race->actor_page);
        if(*state==-1) *state=-2;
    }
}

static void advance_weapon_actors(struct SlicksRaceRuntime *race)
{
    if(!shared_actor_pool(race)) return;
#if defined(__m68k__) && !defined(SLICKS_REFERENCE_ACTOR_ADVANCE)
    extern void slicks_advance_weapon_actors(struct SlicksRaceRuntime *);
    SLICKS_SHADOW_CALL(4,advance_weapon_actors_reference(race),
        slicks_advance_weapon_actors(race));
#else
    advance_weapon_actors_reference(race);
#endif
}

static void initialize_weapon_actors(struct SlicksRaceRuntime *race)
{
    race->actor_order_drawn=0;
    __builtin_memset(race->track_stationary_ready,0,sizeof race->track_stationary_ready);
    struct SlicksWeaponRuntime *w=&race->weapons;
    w->sprite_high_water=0;
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
    /* Includes inert/reserved slots and intro/notice lifetime-only actors.
     * Track configuration always reuses these handles. Later sprite creation
     * goes through allocate_weapon_actor; point allocation never raises it. */
    w->sprite_high_water=(unsigned char)w->slots.high_water;
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
