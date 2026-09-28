/* Independent explicit-width arithmetic oracle for the native pair loop.
 * No probe cache and no host-long overflow assumptions. This specifies
 * low-dword multiply/add/subtract followed by signed truncating division;
 * it is not an independent execution of the original DOS binary. */
static int32_t pair_s32(uint32_t v)
{ return v<=INT32_MAX?(int32_t)v:(int32_t)((int64_t)v-INT64_C(4294967296)); }
static int32_t pair_add(int32_t a,int32_t b){return pair_s32((uint32_t)a+(uint32_t)b);}
static int32_t pair_sub(int32_t a,int32_t b){return pair_s32((uint32_t)a-(uint32_t)b);}
static int32_t pair_mul(int32_t a,int32_t b){return pair_s32((uint32_t)a*(uint32_t)b);}
static int32_t pair_abs(int32_t a){return a<0?pair_sub(0,a):a;}
static int32_t pair_div(int32_t a,int32_t b){
    if(!b || (a==INT32_MIN && b==-1)){fputs("Invalid division in fixed-width fixture\n",stderr);exit(2);}
    return (int32_t)((int64_t)a/b);
}
static void fixed_pair_reference(struct SlicksRaceRuntime *r,unsigned current){
    if(r->car_collisions_disabled || (r->participation_ready&&!r->participation[current]))return;
    struct SlicksRaceCar *a=&r->cars[current];
    int32_t weight=r->properties[a->vehicle].collision_weight;
    int32_t extent=r->properties[a->vehicle].collision_radius*50;
    int32_t denominator=pair_add(pair_div(pair_add(pair_abs((int32_t)a->velocity_x),pair_abs((int32_t)a->velocity_y)),2),1);
    unsigned hit=0;
    for(unsigned j=0;j<4;++j){
        struct SlicksRaceCar *b=&r->cars[j];
        if(j==current || (r->participation_ready&&!r->participation[j]) || a->actor_layer!=b->actor_layer)continue;
        int32_t px=pair_add((int32_t)a->x,pair_div(pair_mul((int32_t)a->velocity_x,10),denominator));
        int32_t py=pair_add((int32_t)a->y,pair_div(pair_mul((int32_t)a->velocity_y,10),denominator));
        if(px<pair_sub((int32_t)b->x,extent) || px>pair_add((int32_t)b->x,extent) ||
           py<pair_sub((int32_t)b->y,extent) || py>pair_add((int32_t)b->y,extent))continue;
        hit=1;
        if(!a->touching_car){
            a->actor_contact=b->actor_contact=1;
            int32_t dx=pair_sub((int32_t)a->velocity_x,(int32_t)b->velocity_x),dy=pair_sub((int32_t)a->velocity_y,(int32_t)b->velocity_y);
            int32_t magnitude=pair_add(pair_abs(dx),pair_abs(dy));
            int32_t other=r->properties[b->vehicle].collision_weight;
            int32_t ratio=pair_div(pair_mul(other,100),weight);
            a->velocity_x=pair_sub((int32_t)a->velocity_x,pair_div(pair_mul(dx,ratio),100));
            a->velocity_y=pair_sub((int32_t)a->velocity_y,pair_div(pair_mul(dy,ratio),100));
            ratio=pair_div(pair_mul(weight,100),other);
            b->velocity_x=pair_add((int32_t)b->velocity_x,pair_div(pair_mul(dx,ratio),100));
            b->velocity_y=pair_add((int32_t)b->velocity_y,pair_div(pair_mul(dy,ratio),100));
            a->collision_impact=pair_div(pair_div(pair_div(pair_mul(magnitude,other),2),weight),5);
            b->collision_impact=pair_div(pair_div(pair_div(pair_mul(magnitude,weight),2),other),5);
            a->pending_damage_impact=a->collision_impact;b->pending_damage_impact=b->collision_impact;
            if((uint32_t)a->collision_impact>(uint32_t)r->collision_impact)r->collision_impact=(uint32_t)a->collision_impact;
            if((uint32_t)b->collision_impact>(uint32_t)r->collision_impact)r->collision_impact=(uint32_t)b->collision_impact;
            r->collision_count=(uint32_t)r->collision_count+1U;
        }
        a->touching_car=b->touching_car=1;a->collision_partner=(unsigned char)j;
    }
    if(!hit)a->touching_car=0;
}
