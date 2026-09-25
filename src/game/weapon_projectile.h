#ifndef SLICKS_WEAPON_PROJECTILE_H
#define SLICKS_WEAPON_PROJECTILE_H

/* Original race-local projectile arrays; actor handles belong to the shared
 * actor allocator, not a private particle pool. Slot zero is reserved by the
 * original free-slot search, although the update loop visits it. */
#define SLICKS_WEAPON_PROJECTILES 30
struct SlicksWeaponProjectile {
    short handle,x,y,vx,vy,lifetime;
    signed char type,layer;
};

/* 20855..20890: last free slot, not first free. Nonpositive handles are free. */
static inline unsigned slicks_weapon_free_slot(const struct SlicksWeaponProjectile pool[30])
{
    unsigned slot=0;
    for(unsigned i=0;i<30;++i) if(pool[i].handle<=0) slot=i;
    return slot;
}

/* 1ea80..1eb48: signed low-word Manhattan distance, strict comparison keeps
 * the first equal-distance target. The original fallback is driver zero. */
static inline unsigned char slicks_weapon_nearest(unsigned driver,
    const int x[4],const int y[4],const signed char roles[4])
{
    short best=30000; unsigned char target=0;
    for(unsigned i=0;i<4;++i) if(i!=driver && roles[i]) {
        int dx=(int)((unsigned int)x[i]-(unsigned int)x[driver])/100;
        int dy=(int)((unsigned int)y[i]-(unsigned int)y[driver])/100;
        short distance=(short)((dx<0?-dx:dx)+(dy<0?-dy:dy));
        if(distance<best) { best=distance; target=(unsigned char)i; }
    }
    return target;
}

/* 20951..20b88: initialize even if the shared actor allocation returned zero.
 * Homing uses vx/vy as heading/target and consumes no random draws. All other
 * types consume two draws, including weapons with zero spread. */
static inline void slicks_weapon_projectile_init(struct SlicksWeaponProjectile *p,
    unsigned driver,const int x[4],const int y[4],const signed char roles[4],
    short heading,signed char type,signed char layer,short lifetime,
    signed char speed,signed char spread,const signed char dirx[16],
    const signed char diry[16],unsigned long *random_state)
{
    p->x=(short)((unsigned int)(x[driver]/100)<<4);
    p->y=(short)((unsigned int)(y[driver]/100)<<4);
    p->type=type; p->layer=layer; p->lifetime=lifetime;
    int sector=heading/1200;
    if(type==5) {
        p->vx=(short)(sector*7);
        p->vy=slicks_weapon_nearest(driver,x,y,roles);
    } else {
        *random_state=(*random_state*0x015a4e35UL+1UL)&0xffffffffUL;
        int rx=(int)((*random_state>>16)&0x7fffUL);
        p->vx=(short)((short)(dirx[sector]*speed)/30+rx*spread/32768-spread/2);
        *random_state=(*random_state*0x015a4e35UL+1UL)&0xffffffffUL;
        int ry=(int)((*random_state>>16)&0x7fffUL);
        p->vy=(short)((short)(diry[sector]*speed)/30+ry*spread/32768-spread/2);
    }
}

/* 21753..2184b: homing changes heading by one of 112 steps per update,
 * independent of elapsed ticks. target_direction is the original 3631b
 * result for (projectile position - target position), with signed words. */
static inline unsigned char slicks_weapon_home(struct SlicksWeaponProjectile *p,
    unsigned char target_direction)
{
    signed char turn=(signed char)(p->vx/7-target_direction+4);
    if(turn<-8) turn=(signed char)(turn+16);
    if(turn>8) turn=(signed char)(turn-16);
    if(turn) {
        p->vx=(short)(p->vx+(turn<0?-1:1));
        if(p->vx>=112) p->vx-=112;
        if(p->vx<0) p->vx+=112;
    }
    return (unsigned char)(p->vx/7);
}

/* 2184b..21965, excluding the homing sprite-pointer update. Keep candidate
 * positions separate: the collision probe receives the old position first. */
static inline void slicks_weapon_move(const struct SlicksWeaponProjectile *p,
    short ticks,const signed char dirx[16],const signed char diry[16],
    short *next_x,short *next_y)
{
    int dx=p->vx,dy=p->vy;
    if(p->type==5) { dx=dirx[p->vx/7]/7; dy=diry[p->vx/7]/7; }
    *next_x=(short)(p->x+dx*ticks);
    *next_y=(short)(p->y+dy*ticks);
}

enum SlicksWeaponExpiry { SLICKS_WEAPON_LIVE, SLICKS_WEAPON_RETIRE,
    SLICKS_WEAPON_DETONATE };
/* 21965..21b47. Return the actor-side expiry action; the caller must perform
 * that action before probing movement, even when the handle was retired.
 * A mine switches from excluding its owner to probing every car at expiry.
 * Homing excludes its owner only while its remaining lifetime is >=800. */
static inline enum SlicksWeaponExpiry slicks_weapon_age(
    struct SlicksWeaponProjectile *p,unsigned driver,short ticks,
    unsigned char *excluded_driver)
{
    *excluded_driver=(unsigned char)(driver+1);
    p->lifetime=(short)((unsigned short)p->lifetime-(unsigned short)ticks);
    if(p->type==5 && p->lifetime<800) *excluded_driver=5;
    if(p->lifetime>0) return SLICKS_WEAPON_LIVE;
    p->lifetime=0;
    switch(p->type) {
    case 0: case 1: case 2: case 4: case 5: case 6:
        return SLICKS_WEAPON_RETIRE;
    case 7: return SLICKS_WEAPON_DETONATE;
    case 3: *excluded_driver=5; break;
    }
    return SLICKS_WEAPON_LIVE;
}
#endif
