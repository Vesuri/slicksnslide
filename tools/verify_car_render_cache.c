#include <unicorn/unicorn.h>
#define SLICKS_NATIVE_CAR_CACHE_TEST 1
#define main surface_verifier_main
#include "verify_surface_effects.c"
#undef main
static struct SlicksRaceRuntime race;
static unsigned char native[64000],before[64000],expected[64000];
static void check(uc_err e) { if(e) { fprintf(stderr,"%s\n",uc_strerror(e));exit(1); } }
#define slicks_draw_sprite_opaque oracle_draw_sprite_opaque
#include "native_sprite_oracle.h"
#undef slicks_draw_sprite_opaque
static unsigned hits;
void slicks_draw_sprite_opaque(unsigned char *d,const unsigned char *s,unsigned char *b,
    const unsigned char *m,unsigned w,unsigned h)
{ ++hits;oracle_draw_sprite_opaque(d,s,b,m,w,h); }

int main(void)
{
    unsigned cases=0;
    for(unsigned direction=0;direction<16;++direction)
    for(unsigned trial=0;trial<256;++trial) {
        memset(&race,0,sizeof race);race.chunky=native;
        unsigned w=1+trial%10,h=1+(trial/10)%10;
        for(unsigned base=0;base<4;++base) {
            struct SlicksCarSprite *s=&race.sprites[0][base];
            s->ready=1;s->width=w;s->height=h;
            for(unsigned i=0;i<w*h;++i)s->pixels[i]=(i+base+trial)%9;
        }
        struct SlicksRaceCar *car=&race.cars[0];
        car->x=(10+trial%4)*100;car->y=(10+trial%8)*100;
        if(trial%8==0){car->x=31000;car->y=18000;}
        if(trial%16==15){car->x=0;car->y=19000;}
        car->heading=direction*SLICKS_HEADING_STEP;car->style=trial*17;
        car->actor_layer=trial%3?1:0;
        for(unsigned i=0;i<64000;++i)before[i]=(unsigned char)(i*19+trial);
        if(trial%4==1)memset(race.material_map,255,sizeof race.material_map);
        if(trial%4==2) {
            for(unsigned i=0;i<60800;++i) {
                race.material_map[i]=(i%17==0)?2:1;race.surface_map[i]=i%8;
            }
        }
        slicks_race_prepare_car_render_cache(&race);
        if(trial%7==0)++car->style; /* Stale key must fall back. */
        struct SlicksRaceCar initial=*car;
        memcpy(native,before,sizeof native);race.car_render_cache.ready=0;
        draw_car(&race,0,0);memcpy(expected,native,sizeof expected);
        struct SlicksRaceCar result=*car;
        *car=initial;memcpy(native,before,sizeof native);race.car_render_cache.ready=1;
        draw_car(&race,0,0);
        if(memcmp(native,expected,sizeof native) || memcmp(car,&result,sizeof result)) {
            fprintf(stderr,"cache differs direction=%u trial=%u\n",direction,trial);return 1;
        }
        /* A failed replacement can have partially decoded the source. It
         * must invalidate an atlas too, not only successful asset loads. */
        if(slicks_race_add_car_sprite(&race,0,0,0,0)!=-1 ||
           race.car_render_cache.cars[0].ready)fail("sprite reload invalidation");
        ++cases;
    }
    if(hits<500)fail("cache path was not exercised");
    printf("car render cache: %u comparisons, %u native fast-path calls; exact pixels, saved backgrounds and ABI passed\n",cases,hits);
    return 0;
}
