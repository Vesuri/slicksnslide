#ifndef SLICKS_ACTOR_SLOTS_H
#define SLICKS_ACTOR_SLOTS_H
#define SLICKS_ACTOR_CAPACITY 200
struct SlicksActorSlots {
    signed char state[SLICKS_ACTOR_CAPACITY];
    unsigned short high_water,capacity;
};
struct SlicksActorMotion {
    short x,y,vx,vy,ax,ay,lifetime,age;
    signed char frame,period;
    unsigned char frames;
};
/* 33918..33a9a: one actor update, NOT elapsed physics ticks. Sprite-table
 * selection belongs to the renderer; frame advancement remains identical.
 * Expiry is deferred on page zero and negates the current state on page one. */
static inline void slicks_actor_advance(struct SlicksActorMotion *a,signed char *state,unsigned page)
{
    if(*state<=0) return;
    if(a->lifetime) {
        a->lifetime=(short)((unsigned short)a->lifetime-1U);
        if(a->lifetime<=0) {
            if(!page) a->lifetime=1;
            else *state=(signed char)-*state;
        }
    }
    if(*state<=0) return;
    a->vx=(short)(a->vx+a->ax);a->vy=(short)(a->vy+a->ay);
    a->x=(short)(a->x+a->vx);a->y=(short)(a->y+a->vy);
    if(a->age>a->period) { a->age=0;a->frame=(signed char)(a->frame+1); }
    if(a->frame>=a->frames) a->frame=0;
    if(a->period) a->age=(short)((unsigned short)a->age+1U);
}
/* Original 330ab selection: lowest zero-state slot, otherwise extend the
 * high-water mark. Negative retirement/reservation states are NOT free.
 * Handle zero is reserved; valid handles are strictly below capacity.
 * A null resource returns a found hole without activating it or extending.
 * The owner supplies resources/storage before entering this allocator. */
static inline short slicks_actor_allocate(struct SlicksActorSlots *pool,unsigned resource_present)
{
#if defined(__m68k__)
    _Static_assert(SLICKS_ACTOR_CAPACITY==200 &&
        __builtin_offsetof(struct SlicksActorSlots,high_water)==200 &&
        __builtin_offsetof(struct SlicksActorSlots,capacity)==202,"actor allocator ABI");
    extern short slicks_actor_allocate_native(struct SlicksActorSlots *,unsigned);
    return slicks_actor_allocate_native(pool,resource_present);
#else
    if(!pool->capacity) return 0;
    unsigned slot=0;
    for(unsigned i=1;i<pool->high_water;++i) if(!pool->state[i]) { slot=i; break; }
    if(pool->high_water>=pool->capacity && !slot) return 0;
    if(!resource_present) return (short)slot;
    if(!slot) slot=pool->high_water++;
    pool->state[slot]=1;
    return (short)slot;
#endif
}
static inline void slicks_actor_slots_init(struct SlicksActorSlots *pool)
{
    for(unsigned i=0;i<SLICKS_ACTOR_CAPACITY;++i) pool->state[i]=0;
    pool->high_water=1; pool->capacity=SLICKS_ACTOR_CAPACITY;
}
/* Allocation-only batch: start cursor at 1 and discard it BEFORE any slot
 * can be released. Other allocations are safe; frees/resets are not. */
static inline short slicks_actor_allocate_batch(struct SlicksActorSlots *pool,
    unsigned short *cursor)
{
    if(!pool->capacity) return 0;
    unsigned short slot=*cursor;
    while(slot<pool->high_water && pool->state[slot]) ++slot;
    *cursor=slot;
    if(slot>=pool->high_water) {
        if(pool->high_water>=pool->capacity) return 0;
        slot=pool->high_water++;
    }
    pool->state[slot]=1;
    *cursor=(unsigned short)(slot+1);
    return (short)slot;
}
#endif
