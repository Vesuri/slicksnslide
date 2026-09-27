/* Included by race_runtime.c after weapon_actors.inc.
 *
 * Unchanged track-sprite retention. The original restores every saved
 * actor in reverse order and redraws them all. For an unchanged, isolated
 * track sprite whose rectangle no actor drawn before it touches, restoring
 * then redrawing leaves identical chunky pixels and an identical saved
 * background, so both steps may be skipped ("kept"). Actors drawn after the
 * sprite are unaffected: their saved backgrounds already contain its pixels.
 *
 * actor->retain bits:
 *   RETAIN_NEXT     the next restoration may keep this sprite;
 *   RETAIN_KEPT     restoration was skipped this update (pixels remain);
 *   RETAIN_ELIGIBLE set before drawing: an unchanged draw may retain it.
 * The native restoration chain keeps RETAIN_NEXT sprites, writing only
 * their pre-simulation description. After simulation, the preparation pass
 * late-restores any kept sprite that changed or that a lower actor will
 * touch, in the normal deferred-dirty way; the drawing chain skips sprites
 * still kept and grants RETAIN_NEXT only after an unchanged native draw.
 *
 * A lower actor is anything drawn before the sprite: points, cars, shadows,
 * weapon sprites or other track sprites. Track actors hold permanent handles
 * below every point handle, so a point is lower only with lower priority.
 * Overlapping candidates form connected groups: all members must earn NEXT,
 * and any late invalidation restores the entire group in reverse draw order.
 * Candidates must end above the HUD rows. Every drawn point and
 * car rectangle touching a candidate clears its eligibility, which also
 * covers that actor's next restoration and any permanent mark it bakes
 * there. Shadows, weapon sprites, Arcade mode and the countdown disable
 * retention; so does a pause handoff (slicks_race_invalidate_retention).
 */
#define RETAIN_NEXT 1
#define RETAIN_KEPT 2
#define RETAIN_ELIGIBLE 8
#define RETAIN_CELLS_X 40
#define RETENTION_CANDIDATE 1
#define RETENTION_CONFLICT 2
#define RETENTION_MOVED 4    /* conflict source until the next rebuild */
#define RETENTION_SETTLE 16  /* motionless updates before rebuilding */
#define RETENTION_MIN_CANDIDATES 8 /* fewer cannot repay the per-update pass */

#if defined(__m68k__)
_Static_assert(sizeof(struct SlicksRetentionEntry)==20,"native retention layout");
#endif
_Static_assert(SLICKS_POINT_HEIGHT==184,"retention row map");
struct SlicksRetentionState slicks_retention;
unsigned char slicks_race_disable_retention; /* diagnostic reference steps */

void slicks_retention_late(struct SlicksRaceRuntime *,unsigned,unsigned);
void slicks_retention_release(struct SlicksRaceRuntime *);

static void retention_release_group(struct SlicksRaceRuntime *race,unsigned group)
{
    struct SlicksRetentionState *s=&slicks_retention;
    s->group_releasing=1;
    for(unsigned h=s->group_head[group];h;h=s->group_next[h]) {
        struct SlicksWeaponActor *a=&race->weapons.actors[h];
        if(a->retain&RETAIN_KEPT) slicks_retention_late(race,h,0);
        else a->retain=0;
    }
    s->group_releasing=0;
}

/* Native unchanged drawing is the only path that grants NEXT. A general
 * draw, clipping, animation or conflict on any member prevents a partial
 * group from being kept on the following restoration pass. */
static void retention_finish_groups(struct SlicksRaceRuntime *race)
{
    struct SlicksRetentionState *s=&slicks_retention;
    for(unsigned g=1;g<=s->group_count;++g) {
        unsigned all=1;
        for(unsigned h=s->group_head[g];h;h=s->group_next[h])
            if(!(race->weapons.actors[h].retain&RETAIN_NEXT)) { all=0;break; }
        if(!all) for(unsigned h=s->group_head[g];h;h=s->group_next[h])
            race->weapons.actors[h].retain&=(unsigned char)~RETAIN_NEXT;
    }
}

static unsigned retention_root(const unsigned char *parent,unsigned n)
{
    while(parent[n]!=n) n=parent[n];
    return n;
}

static int retention_listed(const struct SlicksRaceRuntime *race,unsigned h)
{
    const struct SlicksWeaponActor *a=&race->weapons.actors[h];
    return h && h<SLICKS_ACTOR_CAPACITY && a->kind==3 && race->weapons.slots.state[h]>0 &&
        a->asset<5 && (unsigned char)a->motion.frame<4;
}

static int retention_overlap(const struct SlicksRetentionEntry *a,short left,short top,short right,short bottom)
{
    return a->left<right && left<a->right && a->top<bottom && top<a->bottom;
}

/* Entries hold handle/key/asset/frame for listed sprites in source order.
 * Derive rectangles, isolation and the point-row/cell maps from them. */
void slicks_retention_rebuild(struct SlicksRaceRuntime *race,unsigned count);
void slicks_retention_rebuild(struct SlicksRaceRuntime *race,unsigned count)
{
    static const unsigned char frames[5][4]={{0,5,6,7},{1,1,1,1},{2,8,9,10},{3,3,3,3},{4,11,12,13}};
    struct SlicksRetentionState *s=&slicks_retention;
    unsigned candidates=0,max_priority=0;
    /* Old groups may have a different order/geometry. Release them before
     * replacing their lists. Old isolated sprites have disjoint saved
     * backgrounds and can stay kept unless they join a new group below. */
    for(unsigned g=1;g<=s->group_count;++g) retention_release_group(race,g);
    s->group_count=s->group_releasing=0;
    __builtin_memset(s->group_head,0,sizeof s->group_head);
    __builtin_memset(s->group_of_handle,0,sizeof s->group_of_handle);
    __builtin_memset(s->group_next,0,sizeof s->group_next);
    unsigned char parent[SLICKS_TRACK_ACTOR_MAX],valid[SLICKS_TRACK_ACTOR_MAX],
        members[SLICKS_TRACK_ACTOR_MAX],group[SLICKS_TRACK_ACTOR_MAX];
    for(unsigned c=0;c<count;++c) {parent[c]=(unsigned char)c;valid[c]=1;members[c]=group[c]=0;}
    unsigned char *width=s->kind_width,*height=s->kind_height;
    /* Each kind's union over its animation frames keeps the rectangles
     * stable while animating; frame changes still redraw normally. */
    for(unsigned k=0;k<5;++k) {
        width[k]=height[k]=0;
        for(unsigned f=0;f<4;++f) {
            const struct SlicksTrackActorAsset *asset=&race->track_actor_assets[frames[k][f]];
            if(asset->width>width[k]) width[k]=asset->width;
            if(asset->height>height[k]) height[k]=asset->height;
        }
    }
    s->count=(unsigned char)count; s->valid=1; s->rebuild_pending=0;
    for(unsigned c=0;c<count;++c) {
        struct SlicksRetentionEntry *e=&s->entries[c];
        e->left=(short)((short)(e->key>>16)>>6); e->top=(short)((short)e->key>>6);
        e->right=(short)(e->left+width[e->asset]); e->bottom=(short)(e->top+height[e->asset]);
        e->flags=0;
    }
    for(unsigned c=0;c<count;++c)
        for(unsigned o=0;o<c;++o)
            if(retention_overlap(&s->entries[o],s->entries[c].left,s->entries[c].top,
                                 s->entries[c].right,s->entries[c].bottom)) {
                unsigned a=retention_root(parent,c),b=retention_root(parent,o);
                if(a!=b) parent[a]=(unsigned char)b;
            }
    for(unsigned c=0;c<count;++c) {
        struct SlicksRetentionEntry *e=&s->entries[c];
        unsigned root=retention_root(parent,c);
        ++members[root];
        if(e->left<0 || e->top<0 || e->right>320 || e->bottom>SLICKS_POINT_HEIGHT ||
           e->right<=e->left || e->bottom<=e->top) valid[root]=0;
    }
    for(unsigned c=0;c<count;++c) {
        struct SlicksRetentionEntry *e=&s->entries[c];
        unsigned root=retention_root(parent,c);
        if(valid[root]) {
            e->flags=RETENTION_CANDIDATE; ++candidates;
            if(e->priority>max_priority) max_priority=e->priority;
            if(members[root]>1) {
                if(!group[root]) group[root]=++s->group_count;
                unsigned g=group[root],h=e->handle;
                s->group_of_handle[h]=(unsigned char)g;
                unsigned char *link=&s->group_head[g];
                while(*link && (race->weapons.actors[*link].priority>e->priority ||
                    (race->weapons.actors[*link].priority==e->priority && *link>h)))
                    link=&s->group_next[*link];
                s->group_next[h]=*link;*link=(unsigned char)h;
            }
        }
    }
    /* Any old isolated sprite joining a group must be restored too: the
     * new component must not mix kept and already-restored members. Old
     * isolated saved rectangles cannot overlap any old group/member, so
     * these remaining restores are independent of the releases above. */
    for(unsigned g=1;g<=s->group_count;++g) retention_release_group(race,g);
    if(candidates<RETENTION_MIN_CANDIDATES) {
        for(unsigned c=0;c<count;++c) s->entries[c].flags=0;
        candidates=max_priority=0;
        s->group_count=0;
        __builtin_memset(s->group_of_handle,0,sizeof s->group_of_handle);
    }
    s->candidates=(unsigned char)(candidates>255?255:candidates);
    s->max_priority=(unsigned char)max_priority;
    __builtin_memset(s->rows,0,sizeof s->rows);
    __builtin_memset(s->cells,0,sizeof s->cells);
    for(unsigned c=0;c<count;++c) {
        const struct SlicksRetentionEntry *e=&s->entries[c];
        if(!(e->flags&RETENTION_CANDIDATE)) continue;
        for(int y=e->top;y<e->bottom;++y) s->rows[y]=1;
        for(int y=e->top>>3;y<=(e->bottom-1)>>3;++y)
            for(int x=e->left>>3;x<=(e->right-1)>>3;++x) {
                unsigned char *cell=&s->cells[y*RETAIN_CELLS_X+x];
                *cell=*cell?255:(unsigned char)(c+1);
            }
    }
}

/* Mark candidates touched by a lower actor's half-open rectangle; a point
 * passes its priority, cars and other always-lower actors pass -1. */
void slicks_retention_touch(int left,int top,int right,int bottom,int priority);
void slicks_retention_touch(int left,int top,int right,int bottom,int priority)
{
    struct SlicksRetentionState *s=&slicks_retention;
    if(left<0) left=0;
    if(top<0) top=0;
    if(right>320) right=320;
    if(bottom>SLICKS_POINT_HEIGHT) bottom=SLICKS_POINT_HEIGHT;
    if(left>=right || top>=bottom) return;
    for(int y=top>>3;y<=(bottom-1)>>3;++y)
        for(int x=left>>3;x<=(right-1)>>3;++x) {
            unsigned cell=s->cells[y*RETAIN_CELLS_X+x];
            if(!cell) continue;
            for(unsigned c=cell==255?0:cell-1;c<(cell==255?s->count:cell);++c) {
                struct SlicksRetentionEntry *e=&s->entries[c];
                if((e->flags&RETENTION_CANDIDATE) && (int)e->priority>priority &&
                   retention_overlap(e,(short)left,(short)top,(short)right,(short)bottom))
                    e->flags|=RETENTION_CONFLICT;
            }
        }
}

/* Cars: last drawn rectangles and a conservative square around each new
 * centre (half-extent at least the larger sprite dimension). */
void slicks_retention_cars(struct SlicksRaceRuntime *race);
void slicks_retention_cars(struct SlicksRaceRuntime *race)
{
    for(unsigned d=0;d<SLICKS_RACE_CAR_COUNT;++d) {
        const struct SlicksRaceCar *car=&race->cars[d];
        if(car->saved_valid)
            slicks_retention_touch(car->old_x,car->old_y,car->old_x+car->old_width,
                car->old_y+car->old_height,-1);
        if(!driver_role(race,d)) continue;
        const struct SlicksCarSprite *sprite=
            &race->sprites[car->vehicle][((unsigned short)car->heading/SLICKS_HEADING_STEP)&3];
        int extent=sprite->width>sprite->height?sprite->width:sprite->height;
        int x=(short)(car->x/100),y=(short)(car->y/100);
        slicks_retention_touch(x-extent,y-extent,x+extent+1,y+extent+1,-1);
    }
}

/* Restore a kept sprite's saved background now, as its skipped restoration
 * would have. Keep the pre-simulation description recorded when it was
 * kept: restore_weapon_actor() would replace it with post-simulation state,
 * hiding changes (such as an animation frame) from the dirty tracking. */
void slicks_retention_late(struct SlicksRaceRuntime *race,unsigned h,unsigned eligible);
void slicks_retention_late(struct SlicksRaceRuntime *race,unsigned h,unsigned eligible)
{
    struct SlicksRetentionState *s=&slicks_retention;
    if(!s->group_releasing && s->group_of_handle[h]) {
        retention_release_group(race,s->group_of_handle[h]);
        return;
    }
    struct SlicksWeaponActor *a=&race->weapons.actors[h];
    if(a->saved) {
        int left=a->old_x,top=a->old_y,width=a->old_width,height=a->old_height;
        int x0=left<0?-left:0,x1=320-left,y0=top<0?-top:0,y1=200-top;
        if(x1>width) x1=width;
        if(y1>height) y1=height;
#if defined(__m68k__)
        extern void slicks_restore_car_chunky(unsigned char *,const unsigned char *,unsigned,unsigned);
        if(!x0 && !y0 && x1==width && y1==height && width && height)
            slicks_restore_car_chunky(race->chunky+mult320[top]+left,a->saved_under,width,height);
        else
#endif
        for(int y=y0;y<y1;++y) {
            unsigned char *dst=race->chunky+mult320[top+y]+left+x0;
            const unsigned char *src=a->saved_under+y*width+x0;
            for(int n=x1-x0;n>0;--n) *dst++=*src++;
        }
        a->saved=0;
        race->sprite_dirty_handles[race->sprite_dirty_count++]=(unsigned char)h;
    }
    a->retain=(unsigned char)(eligible?RETAIN_ELIGIBLE:0);
}

/* Late-restore every kept sprite; used whenever retention is unavailable. */
void slicks_retention_release(struct SlicksRaceRuntime *race);
void slicks_retention_release(struct SlicksRaceRuntime *race)
{
    for(unsigned g=1;g<=slicks_retention.group_count;++g)
        retention_release_group(race,g);
    for(unsigned i=0;i<race->navigation.actor_count;++i) {
        unsigned h=race->track_actor_handles[i];
        if(!h || h>=SLICKS_ACTOR_CAPACITY) continue;
        struct SlicksWeaponActor *a=&race->weapons.actors[h];
        if(a->retain&RETAIN_KEPT) slicks_retention_late(race,h,0);
        else a->retain=0;
    }
}

int slicks_retention_weapons_live(const struct SlicksRaceRuntime *race);
int slicks_retention_weapons_live(const struct SlicksRaceRuntime *race)
{
    for(unsigned h=1;h<race->weapons.slots.high_water;++h)
        if(race->weapons.trail_index[h]<0 && race->weapons.slots.state[h]>0 &&
           (race->weapons.actors[h].kind==1 || race->weapons.actors[h].kind==2)) return 1;
    return 0;
}

/* Reference pass (host builds and SLICKS_REFERENCE_RETENTION). */
static __attribute__((unused)) void prepare_sprite_retention_reference(struct SlicksRaceRuntime *race)
{
    struct SlicksRetentionState *s=&slicks_retention;
    if(race->navigation.actor_count<RETENTION_MIN_CANDIDATES) return; /* never retained */
    unsigned enabled=!slicks_race_disable_retention && race->racing && race->track_actors_ready &&
        race->sprite_dirty_deferred && race->race_mode!=5;
    for(unsigned d=0;d<SLICKS_RACE_CAR_COUNT && enabled;++d) {
        const struct SlicksCarShadow *shadow=&race->shadows[d];
        if(shadow->saved_valid || shadow->state>0 || race->cars[d].special_drive_state>500) enabled=0;
    }
    if(enabled && race->weapons.ready && slicks_retention_weapons_live(race)) enabled=0;
    if(!enabled) { slicks_retention_release(race); return; }
    unsigned count=0,changed=!s->valid,moved=0;
    if(s->valid) for(unsigned c=0;c<s->count;++c)
        s->entries[c].flags&=RETENTION_CANDIDATE|RETENTION_MOVED;
    for(unsigned i=0;i<race->navigation.actor_count;++i) {
        unsigned h=race->track_actor_handles[i];
        if(!retention_listed(race,h)) {
            if(h && h<SLICKS_ACTOR_CAPACITY) {
                struct SlicksWeaponActor *a=&race->weapons.actors[h];
                if(a->retain&RETAIN_KEPT) slicks_retention_late(race,h,0);
                else a->retain=0;
            }
            continue;
        }
        const struct SlicksWeaponActor *a=&race->weapons.actors[h];
        struct SlicksRetentionEntry *e=&s->entries[count++];
        unsigned int key=(((unsigned int)(unsigned short)a->motion.x<<16)|
            (unsigned short)a->motion.y)&0xffc0ffc0U;
        if(e->handle!=h || e->asset!=a->asset || e->priority!=a->priority) {
            changed=1;
            e->handle=(unsigned char)h; e->key=key; e->asset=a->asset; e->priority=a->priority;
        } else if(e->key!=key && !changed) {
            /* A moving sprite is no candidate; both rectangles conflict
             * until it settles and the maps are rebuilt. */
            slicks_retention_touch(e->left,e->top,e->right,e->bottom,-1);
            e->key=key; e->left=(short)((short)(key>>16)>>6); e->top=(short)((short)key>>6);
            e->right=(short)(e->left+s->kind_width[e->asset]);
            e->bottom=(short)(e->top+s->kind_height[e->asset]);
            slicks_retention_touch(e->left,e->top,e->right,e->bottom,-1);
            e->flags=RETENTION_MOVED; s->rebuild_pending=RETENTION_SETTLE; moved=1;
        } else e->key=key;
    }
    if(changed || count!=s->count || (s->rebuild_pending && !moved && !--s->rebuild_pending))
        slicks_retention_rebuild(race,count);
    else if(s->rebuild_pending)
        for(unsigned c=0;c<count;++c) {
            const struct SlicksRetentionEntry *e=&s->entries[c];
            if(e->flags&RETENTION_MOVED) slicks_retention_touch(e->left,e->top,e->right,e->bottom,-1);
        }
    if(s->candidates) {
        const struct SlicksTrailParticle *p=race->trail_particles;
        for(unsigned i=0;i<race->trail_particle_count;++i,++p) {
            unsigned short x=(unsigned short)(p->x>>6),y=(unsigned short)(p->y>>6);
            if(p->priority>=s->max_priority || y>=SLICKS_POINT_HEIGHT || !s->rows[y] || x>=320) continue;
            if(s->cells[(y>>3)*RETAIN_CELLS_X+(x>>3)]) slicks_retention_touch(x,y,x+1,y+1,p->priority);
        }
        slicks_retention_cars(race);
    }
    for(unsigned c=0;c<count;++c) {
        const struct SlicksRetentionEntry *e=&s->entries[c];
        struct SlicksWeaponActor *a=&race->weapons.actors[e->handle];
        const __typeof__(race->sprite_dirty_previous[0]) *previous=&race->sprite_dirty_previous[e->handle];
        unsigned eligible=(e->flags&(RETENTION_CANDIDATE|RETENTION_CONFLICT))==RETENTION_CANDIDATE;
        if(a->retain&RETAIN_KEPT) {
            /* Same comparison as the unchanged drawing path. */
            unsigned unchanged=eligible && previous->kind==3 && previous->x==e->left &&
                previous->y==e->top && previous->asset==a->asset &&
                previous->frame==(unsigned char)a->motion.frame && previous->colour==a->colour &&
                previous->priority==a->priority && previous->occlusion==a->occlusion;
            if(!unchanged) { slicks_retention_late(race,e->handle,eligible); continue; }
        }
        a->retain=(unsigned char)((a->retain&(RETAIN_NEXT|RETAIN_KEPT))|(eligible?RETAIN_ELIGIBLE:0));
    }
}

static void prepare_sprite_retention(struct SlicksRaceRuntime *race)
{
#if defined(__m68k__) && !defined(SLICKS_REFERENCE_RETENTION)
    extern void slicks_prepare_sprite_retention(struct SlicksRaceRuntime *);
    slicks_prepare_sprite_retention(race);
#else
    prepare_sprite_retention_reference(race);
#endif
}

/* Platform hook after pause/menu handoffs: never assume retained pixels. */
void slicks_race_invalidate_retention(struct SlicksRaceRuntime *race)
{
    if(!race) return;
    for(unsigned h=0;h<SLICKS_ACTOR_CAPACITY;++h) race->weapons.actors[h].retain=0;
    slicks_retention.valid=0;
    slicks_retention.group_count=0;
    __builtin_memset(slicks_retention.group_of_handle,0,sizeof slicks_retention.group_of_handle);
}
