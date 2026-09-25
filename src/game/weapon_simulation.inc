/* Included after the race's RNG and damage helpers. The original firing
 * transaction and projectile arithmetic are independently oracle-tested. */
static void weapon_coordinates(const struct SlicksRaceRuntime *race,
    int x[4],int y[4],signed char roles[4])
{
    for(unsigned d=0;d<4;++d) {
        x[d]=(int)race->cars[d].x;y[d]=(int)race->cars[d].y;roles[d]=driver_role(race,d);
    }
}
static void weapon_ai_request(struct SlicksRaceRuntime *race,unsigned driver)
{
    struct SlicksWeaponControl *control=&race->weapons.controls[driver];
    if(!race->weapons.ready || !race->weapons_enabled || race->cars[driver].ai_service_state) {
        control->request=0;return;
    }
    int x[4],y[4];signed char roles[4];weapon_coordinates(race,x,y,roles);
    control->request=slicks_weapon_ai_request(&control->probe_counter,driver,x,y,
        race->cars[driver].heading,roles,race->selected_weapon[driver],
        race->weapon_inventory[driver],race->game_clock_ticks,direction_x,direction_y,
        race->weapons.rules.ranges);
}
static void weapon_flash(void *context,unsigned driver,signed char scale)
{
    struct SlicksRaceRuntime *race=context;
    short h=allocate_weapon_actor(race,18,1,0);
    struct SlicksRaceCar *car=&race->cars[driver];
    unsigned sector=car->heading/1200;
    configure_weapon_actor(race,h,(short)(car->x/100),(short)(car->y/100),
        (short)(direction_x[sector]*scale*2),(short)(direction_y[sector]*scale*2),4,5,0);
}
static void weapon_sound(void *context,unsigned char sample,unsigned char flags,unsigned char priority)
{ emit_sound_event(context,sample,flags,priority); }
static short weapon_allocate(void *context,signed char type)
{
    struct SlicksRaceRuntime *race=context;
    int asset=type==3?0:type==7?1:type==6?2:type==5?3:-1;
    ++race->weapons.shots;
    return allocate_weapon_actor(race,asset,type==5?8:1,race->weapons.bullet_colour);
}
static void weapon_fire_driver(struct SlicksRaceRuntime *race,unsigned driver,unsigned char *controls)
{
    if(!race->weapons.ready) return;
    int x[4],y[4];signed char roles[4];weapon_coordinates(race,x,y,roles);
    const struct SlicksWeaponFireOps ops={weapon_flash,weapon_sound,weapon_allocate,race};
    race->selected_weapon[driver]=slicks_weapon_fire(&race->weapons.controls[driver],
        race->weapon_inventory[driver],race->selected_weapon[driver],controls,driver,
        race->game_clock_ticks,race->weapons.projectiles[driver],x,y,roles,
        race->cars[driver].heading,race->cars[driver].actor_layer,direction_x,direction_y,
        &race->random_state,&race->weapons.rules,&ops,&race->track_actor_scratch);
}
static int weapon_track_probe(void *context,short x,short y,signed char sampling,signed char layer)
{
    struct SlicksRaceRuntime *race=context;(void)sampling;
    int result=slicks_track_projectile_sample(race->material_map,race->surface_map,x,y,layer,race->boundary_level);
    /* A ray can overshoot the original allocation before the caller retires
     * its off-track projectile. Such DOS reads depend on unrelated heap
     * bytes, not track data. Define that external boundary as empty space;
     * never read beyond native maps or alias arbitrary native allocations.
     * The sampler still reports -1 to its oracle for unretained addresses. */
    return result<0?0:result;
}
static signed char weapon_car_probe(void *context,short x,short y,signed char exclude,signed char layer)
{
    struct SlicksRaceRuntime *race=context;
    int cx[4],cy[4];signed char roles[4];unsigned char layers[4];
    weapon_coordinates(race,cx,cy,roles);
    for(unsigned d=0;d<4;++d) layers[d]=race->cars[d].actor_layer;
    return slicks_probe_cars(x,y,exclude,layer,cx,cy,roles,layers,
        exclude>=0 && exclude<4?race->cars[(unsigned)exclude].special_drive_state:0);
}
/* 1c64b exits immediately for a negative sampling owner. */
static void weapon_wall_probe(void *context,short x,short y,signed char sampling)
{ (void)context;(void)x;(void)y;(void)sampling; }

static void weapon_explosion(struct SlicksRaceRuntime *race,short x,short y,unsigned count)
{
    ++race->weapons.explosions;
    for(unsigned i=0;i<count;++i) {
        short h=allocate_weapon_actor(race,i?11:14,i?3:4,0);
        short vx=0,vy=0;
        if(i) { vx=(short)(random_scaled(race,30)-15);vy=(short)(random_scaled(race,30)-15); }
        if(h) {
            configure_weapon_actor(race,h,x,y,vx,vy,i?45:30,3,0);
            race->weapons.actors[h].motion.period=7;
        }
    }
}
static void weapon_radial_damage(struct SlicksRaceRuntime *race,short x,short y)
{
    for(unsigned d=0;d<4;++d) if(driver_role(race,d)) {
        struct SlicksRaceCar *car=&race->cars[d];
        short dx=(short)(car->x/100-x),dy=(short)(car->y/100-y);
        short distance=(short)((dx<0?-(int)dx:dx)+(dy<0?-(int)dy:dy));
        if(distance>50) continue;
        if(distance<=0) distance=1;
        car->velocity_x=(int)((unsigned int)car->velocity_x+(unsigned int)(500*dx/distance));
        car->velocity_y=(int)((unsigned int)car->velocity_y+(unsigned int)(500*dy/distance));
        apply_damage(race,d,400/distance);
    }
}
static void weapon_impact_point(struct SlicksRaceRuntime *race,short x,short y,
    unsigned char colour,unsigned range,unsigned mask)
{
    short h=allocate_weapon_actor(race,-1,1,colour);
    race->track_actor_scratch=h; /* Shared original BP-3c, read by finish flags. */
    /* The original argument pushes evaluate Y before X, even on failure. */
    short vy=(short)(random_scaled(race,range)-(short)(range/2));
    short vx=(short)(random_scaled(race,range)-(short)(range/2));
    configure_weapon_actor(race,h,x,y,vx,vy,25,3,mask);
}

static void update_weapon_projectiles(struct SlicksRaceRuntime *race,unsigned short ticks)
{
    if(!race->weapons.ready) return;
    const struct SlicksWeaponRules *rules=&race->weapons.rules;
    const struct SlicksMovingProbeOps probe={weapon_track_probe,weapon_car_probe,weapon_wall_probe,race};
    for(unsigned d=0;d<4;++d) {
        slicks_weapon_cooldown(&race->weapons.controls[d],(short)ticks);
        for(unsigned slot=0;slot<30;++slot) {
            struct SlicksWeaponProjectile *p=&race->weapons.projectiles[d][slot];
            if(p->handle<=0) continue;
            unsigned type=(unsigned char)p->type;
            short h=p->handle;
            configure_weapon_actor(race,h,(short)((p->x>>4)-rules->radius[type]),
                (short)((p->y>>4)-rules->radius[type]),0,0,0,3,p->layer*15);
            if(type==5) {
                struct SlicksRaceCar *target=&race->cars[p->vy];
                unsigned sector=slicks_weapon_home(p,(unsigned char)((dos_vector_direction(
                    (short)((p->x>>4)-target->x/100),(short)((p->y>>4)-target->y/100))+4)&15));
                /* 218ad replaces the current resource pointer, not +24's
                 * animation frame. The next configure call still uses 0. */
                race->weapons.actors[h].asset=(unsigned char)(3+sector/2);
            }
            short nx,ny;slicks_weapon_move(p,(short)ticks,direction_x,direction_y,&nx,&ny);
            unsigned char exclude;
            enum SlicksWeaponExpiry expiry=slicks_weapon_age(p,d,(short)ticks,&exclude);
            if(expiry!=SLICKS_WEAPON_LIVE) {
                if(expiry==SLICKS_WEAPON_DETONATE) weapon_radial_damage(race,p->x>>4,p->y>>4);
                race->weapons.actors[h].motion.lifetime=1;p->handle=0;
                if(expiry==SLICKS_WEAPON_DETONATE) weapon_explosion(race,p->x>>4,p->y>>4,4);
            }
            int hit=slicks_moving_probe(p->x>>4,p->y>>4,nx>>4,ny>>4,&p->x,&p->y,
                16,(signed char)exclude,-1,p->layer,&probe);
            if(hit==-2) { race->collision_error=1;return; }
            if(hit || p->x<0 || p->x>5120 || p->y<0 || p->y>2944) {
                if(p->handle>0) race->weapons.actors[p->handle].motion.lifetime=1;
                if(hit>0) {
                    ++race->weapons.hits;
                    emit_sound_event(race,rules->hit_sound[type],2,16);
                    weapon_explosion(race,p->x>>4,p->y>>4,1);
                }
                if(rules->effect[type]==0 || rules->effect[type]==2)
                    weapon_impact_point(race,p->x>>4,p->y>>4,
                        rules->effect[type]?race->collision_colour:race->weapons.impact_colour,60,0);
                else weapon_explosion(race,p->x>>4,p->y>>4,5);
                p->handle=0;
                if(hit>0) {
                    if(type==3) {
                        p->vx=(short)(random_scaled(race,1500)-750);
                        p->vy=(short)(random_scaled(race,1500)-750);
                    }
                    if(type==5) { unsigned sector=p->vx/7;p->vx=direction_x[sector]*10;p->vy=direction_y[sector]*10; }
                    struct SlicksRaceCar *car=&race->cars[hit-1];
                    unsigned mass=race->properties[car->vehicle].collision_weight;
                    car->velocity_x=(int)((unsigned int)car->velocity_x+(unsigned int)((short)(p->vx*rules->force[type])/(int)mass));
                    car->velocity_y=(int)((unsigned int)car->velocity_y+(unsigned int)((short)(p->vy*rules->force[type])/(int)mass));
                    apply_damage(race,(unsigned)hit-1,(unsigned short)rules->damage[type]);
                    for(unsigned i=0;i<4;++i) weapon_impact_point(race,p->x>>4,p->y>>4,race->collision_colour,30,p->layer*15);
                }
            }
            p->x=nx;p->y=ny;
        }
    }
}
