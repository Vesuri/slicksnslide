/* Original 1ff97..202eb, before any car's motion. The movable objects use
 * wrapping Q4 coordinates and one damping step per update, not per tick. */
static void update_track_actor_motion(struct SlicksRaceRuntime *race)
{
    const struct SlicksMovingProbeOps probe={weapon_track_probe,weapon_car_probe,weapon_wall_probe,race};
    long car_pixel_x[4],car_pixel_y[4];
    unsigned car_pixels_ready=0;
    for(unsigned i=0;i<race->navigation.actor_count;++i) {
        struct SlicksTrackActor *a=&race->navigation.actors[i];
        race->track_actor_scratch=a->kind;
        if(a->kind!=1 && a->kind!=3) continue;
        if(!car_pixels_ready) {
            for(unsigned d=0;d<4;++d) {
                car_pixel_x[d]=race->cars[d].x/100;
                car_pixel_y[d]=race->cars[d].y/100;
            }
            car_pixels_ready=1;
        }
        short nx=(short)(a->x+a->velocity_x),ny=(short)(a->y+a->velocity_y);
        int material=slicks_track_material_sample(race->material_map,race->surface_map,nx/16,ny/16,0);
        if(material<0) { race->collision_error=1;return; }
        if(material==0) a->layer=1;
        else if(material==19) a->layer=0;
        const unsigned moving=a->velocity_x || a->velocity_y;
        /* With zero velocity and exclude=0, the original zero-length
         * probe has no callbacks or writes. Keep material/layer sampling
         * and all car contacts, but bypass motion and zero damping. */
        if(moving) {
            int hit=slicks_moving_probe(a->x>>4,a->y>>4,nx>>4,ny>>4,
                &a->x,&a->y,16,0,-1,a->layer,&probe);
            if(hit==-2) { race->collision_error=1;return; }
            if(!hit) { a->x=nx;a->y=ny; }
        }
        const int actor_pixel_x=a->x/16,actor_pixel_y=a->y/16;
        if(race->track_actors_ready) {
            short h=race->track_actor_handles[i];
            configure_weapon_actor(race,h,(short)(actor_pixel_x-2),(short)(actor_pixel_y-2),
                0,0,0,a->kind==1?1:4,a->layer*15);
        }
        if(moving) {
            a->velocity_x=(short)(a->velocity_x*10)/11;
            a->velocity_y=(short)(a->velocity_y*10)/11;
        }
        /* Contacts change velocities, never positions. Cache the signed
         * pixel divisions once per update with movable track objects. */
        for(unsigned d=0;d<4;++d) {
            struct SlicksRaceCar *car=&race->cars[d];
            if(car->actor_layer!=a->layer) continue;
            long dx=car_pixel_x[d]-actor_pixel_x,dy=car_pixel_y[d]-actor_pixel_y;
            short ax=(short)(dx<0?-dx:dx),ay=(short)(dy<0?-dy:dy);
            if(ax>=4 || ay>=4 || car->measured_speed<=300) continue;
            a->velocity_x=(short)(car->velocity_x/100);
            a->velocity_y=(short)(car->velocity_y/100);
            car->velocity_x=multiply_q15_unsigned(car->velocity_x,0x7cc9);
            car->velocity_y=multiply_q15_unsigned(car->velocity_y,0x7cc9);
        }
    }
}

static void activate_track_flags(struct SlicksRaceRuntime *race)
{
    if(!race->track_actors_ready) return;
    /* 228ba/228f4 really index the shared scratch, not the current kind.
     * A weapon slot/effect handle can replace the last track-object kind.
     * Retain those original adjacent table bytes as typed build-time data. */
    unsigned style=(unsigned short)race->track_actor_scratch;
    if(style>=200) { race->collision_error=1;return; }
    short offset=(signed char)race->track_flag_styles[style];
    unsigned priority=race->track_flag_styles[style+5];
    for(unsigned i=0;i<race->navigation.actor_count;++i) {
        struct SlicksTrackActor *a=&race->navigation.actors[i];
        if(a->kind!=2) continue;
        short h=race->track_actor_handles[i];
        configure_weapon_actor(race,h,(short)(a->x/16+offset),(short)(a->y/16+offset),
            0,0,0,priority,a->layer*15);
        race->weapons.actors[h].motion.period=(signed char)(random_scaled(race,8)+10);
    }
}
