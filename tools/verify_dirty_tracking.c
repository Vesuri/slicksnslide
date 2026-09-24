#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/game/race_runtime.c"
#include "host_archive.h"

static int covered(const struct SlicksRaceRuntime *race, unsigned x, unsigned y);

static void expected_hud_text(unsigned char *pixels,
    const struct SlicksRaceFont *font, unsigned x, unsigned y, const char *text, unsigned colour)
{
    for (; *text; ++text) {
        unsigned glyph=font->lookup[(unsigned char)*text];
        unsigned width=font->widths[glyph];
        for(unsigned row=0;row<5;++row)
            for(unsigned column=0;column<width;++column) {
                unsigned pixel=font->pixels[font->offsets[glyph]+row*width+column];
                if(pixel) pixels[(y+row)*320+x+column]=pixel==1?colour:font->runtime[5+pixel];
            }
        x+=width+1;
    }
}

static int verify_hud_renderer(void)
{
    static struct SlicksRaceRuntime race;
    static unsigned char pixels[64000], expected[64000], before[64000];
    static unsigned char logical[0x40000], resource[8192];
    long size=host_archive_load("ref/SLICKS.000","alamenu.@I",resource,sizeof resource);
    if(size<0 || slicks_race_add_hud_background(&race,resource,(unsigned long)size)) return 1;
    size=host_archive_load("ref/SLICKS.000","pieni.@f",resource,sizeof resource);
    if(size<0 || slicks_race_add_font(&race,resource,(unsigned long)size)) return 1;
    memset(pixels,77,sizeof pixels); memset(expected,77,sizeof expected);
    memset(logical,77,sizeof logical);
    race.chunky=pixels;
    race.hud_colours[0]=73; race.hud_colours[1]=91;
    for(unsigned car=0;car<4;++car) race.cars[car].lap=1;
    memcpy(expected+184*320,race.hud_background,16*320);
    for(unsigned step=0;step<4;++step) {
        for(unsigned car=0;car<4;++car) {
            unsigned base=car*60;
            race.cars[car].last_lap_time_units=180;
            race.cars[car].best_lap_time_units=900;
            if(step==1) race.cars[car].lap=12;
            if(step==2) { race.cars[car].finished=1; race.cars[car].finish_position=2; }
            for(unsigned y=186;y<200;++y)
                memcpy(expected+y*320+base+90,race.hud_background+(y-184)*320+base+90,car==3?50:52);
            unsigned colour=step<2?73:91;
            if(step<2) expected_hud_text(expected,&race.font,base+(step?93:97),186,step?"12":"1",colour);
            else {
                expected_hud_text(expected,&race.font,base+95,186,"2",colour);
                expected_hud_text(expected,&race.font,base+100,186,".",colour);
            }
            expected_hud_text(expected,&race.font,base+106,191,step<2?"01.00":"05.00",colour);
        }
        memcpy(before,pixels,sizeof pixels);
        slicks_race_clear_dirty_rows(&race);
        draw_timers(&race,logical);
        for(unsigned at=0;at<64000;++at) {
            unsigned x=at%320,y=at/320;
            if(pixels[at]!=expected[at] || logical[(x&3)*65536+y*100+x/4]!=expected[at] ||
               (before[at]!=pixels[at] && !covered(&race,x,y))) {
                fprintf(stderr,"HUD renderer step=%u xy=%u,%u actual=%u expected=%u\n",step,x,y,pixels[at],expected[at]);
                return 1;
            }
        }
        if(step==3 && (race.dirty_pixel_count || race.dirty_row_count)) return 1;
    }
    puts("HUD renderer: real font, original coordinates, lap-width and finish transitions, last/best time, unchanged-frame and dirty coverage passed.");
    race.race_mode=5; race.arcade_seconds=5;
    race.arcade_colours[0]=4; race.arcade_colours[1]=40; race.arcade_colours[2]=60;
    const unsigned ticks[]={0,90,449,450,720,990,1000};
    const short rights[]={84,78,57,0,0,0,58};
    for(unsigned step=0;step<7;++step) {
        race.game_clock_ticks=ticks[step]; race.finish_deadline=step==6?1190:0;
        memcpy(before,pixels,sizeof before);
        for(unsigned y=188;y<197;++y) for(unsigned x=55;x<85;++x)
            expected[y*320+x]=4;
        if(rights[step]) {
            unsigned top=step<3?189:196,bottom=step<3?196:197;
            for(unsigned y=top;y<bottom;++y) for(unsigned x=56;x<(unsigned)rights[step];++x)
                expected[y*320+x]=40;
        }
        if(step>=3) {
            const char *text=step==4?"LAP":"LAST";
            unsigned width=0;
            for(unsigned i=0;text[i];++i) width+=race.font.widths[race.font.lookup[(unsigned char)text[i]]]+1;
            expected_hud_text(expected,&race.font,70-width/2,189,text,60);
        }
        slicks_race_clear_dirty_rows(&race);
        draw_arcade_timer(&race,logical);
        if(memcmp(pixels,expected,sizeof pixels)) {
            fprintf(stderr,"Arcade HUD pixels failed at phase %u\n",step); return 1;
        }
        for(unsigned at=0;at<64000;++at) {
            unsigned x=at%320,y=at/320;
            if(before[at]!=pixels[at] && !covered(&race,x,y)) return 1;
            if(logical[(x&3)*65536+y*100+x/4]!=pixels[at]) return 1;
        }
        slicks_race_clear_dirty_rows(&race);
        draw_arcade_timer(&race,logical);
        if(race.dirty_pixel_count || race.dirty_row_count) return 1;
    }
    puts("Arcade HUD: countdown/LAST/LAP/grace pixels, VGA/chunky agreement, dirty bounds and unchanged-frame cache passed.");
    return 0;
}

static int covered(const struct SlicksRaceRuntime *race, unsigned x, unsigned y)
{
    for (unsigned i = 0; i < race->dirty_pixel_count; ++i)
        if (race->dirty_pixels[i].x == x && race->dirty_pixels[i].y == y)
            return 1;
    for (unsigned i = 0; i < race->dirty_row_count; ++i) {
        const struct SlicksDirtyRows *r = &race->dirty_rows[i];
        if (x >= r->left && x < r->right && y >= r->top && y < r->bottom)
            return 1;
    }
    return 0;
}

int main(void)
{
    if (verify_hud_renderer()) return 1;
    static struct SlicksRaceRuntime race;
    /* Two changed positions per moving particle can fill the sparse list.
     * Changed timer glyphs share that list and must not silently disappear. */
    for (unsigned i = 0; i < SLICKS_DIRTY_PIXEL_MAX; ++i)
        mark_dirty_pixel(&race, i % 320, i / 320);
    for (unsigned i = 0; i < 320; ++i)
        mark_dirty_pixel(&race, i, 195);
    for (unsigned i = 0; i < SLICKS_DIRTY_PIXEL_MAX; ++i)
        if (!covered(&race, i % 320, i / 320)) {
            fprintf(stderr, "Lost particle update %u\n", i);
            return 1;
        }
    for (unsigned i = 0; i < 320; ++i)
        if (!covered(&race, i, 195)) {
            fprintf(stderr, "Lost overflow update x=%u y=195\n", i);
            return 1;
        }
    if (race.dirty_pixel_count > SLICKS_DIRTY_PIXEL_MAX ||
        race.dirty_row_count > SLICKS_DIRTY_ROW_MAX)
        return 1;
    slicks_race_clear_dirty_rows(&race);
    if (race.dirty_pixel_count || race.dirty_row_count)
        return 1;
    {
        static unsigned char pixels[64000], background[64000];
        static const short positions[][2]={{100,100},{0,0},{319,189},{-5,80},{100,195}};
        for(unsigned p=0;p<sizeof positions/sizeof *positions;++p) {
            memset(&race,0,sizeof race);
            for(unsigned at=0;at<sizeof pixels;++at)
                pixels[at]=background[at]=(unsigned char)(at*17+at/320);
            race.chunky=pixels; race.chunky_authoritative=1;
            race.properties[0].body_radius_x=7;
            race.properties[0].body_radius_y=1;
            for(unsigned car=0;car<4;++car) {
                race.cars[car].x=(positions[p][0]+(short)car)*100;
                race.cars[car].y=positions[p][1]*100;
                race.cars[car].special_drive_state=1000;
            }
            draw_shadows(&race,NULL);
            for(unsigned y=0;y<200;++y)
                for(unsigned x=0;x<320;++x) {
                    unsigned expected=background[y*320+x];
                    for(unsigned car=0;car<4;++car) {
                        int sx=(int)x-race.shadows[car].x;
                        int sy=(int)y-race.shadows[car].y;
                        if(y<190 && sx>=1 && sx<8 && sy>=1 && sy<8 && !((sx^sy^1)&1))
                            expected=37;
                    }
                    if(pixels[y*320+x]!=expected ||
                       (expected!=background[y*320+x] && !covered(&race,x,y))) {
                        fprintf(stderr,"shadow clip/overlap/dirty mismatch case=%u xy=%u,%u\n",p,x,y);
                        return 1;
                    }
                }
            slicks_race_clear_dirty_rows(&race);
            restore_shadows(&race,NULL);
            if(memcmp(pixels,background,sizeof pixels)) {
                fputs("overlapping shadow restoration damaged background\n",stderr); return 1;
            }
            for(unsigned car=0;car<4;++car) {
                struct SlicksCarShadow *s=&race.shadows[car];
                for(unsigned y=s->old_y;y<(unsigned)s->old_y+s->old_height;++y)
                    for(unsigned x=s->old_x;x<(unsigned)s->old_x+s->old_width;++x)
                        if(!covered(&race,x,y)) return 1;
            }
        }
    }
    puts("Shadows: four-way overlaps, screen-edge clipping, exact restore and draw/restore dirty coverage passed.");
    {
        static unsigned char pixels[64000], expected[64000], logical[0x40000];
        for(unsigned scenario=0;scenario<16;++scenario) {
            memset(&race,0,sizeof race);
            memset(pixels,77,sizeof pixels); memset(expected,77,sizeof expected);
            memset(logical,77,sizeof logical);
            race.chunky=pixels; race.fuel_option=scenario&1; race.damage_scale=scenario&2;
            race.status_colours[0]=10; race.status_colours[1]=11; race.status_colours[2]=12;
            for(unsigned car=0;car<4;++car) {
                race.cars[car].fuel_capacity=100;
                race.cars[car].fuel=car*25;
                race.cars[car].damage[0]=car*400;
                race.cars[car].service_flags=(scenario&4)?1:0;
                if(race.fuel_option || race.damage_scale)
                    for(unsigned y=187;y<190;++y)
                    for(unsigned x=106+car*60;x<126+car*60;++x) expected[y*320+x]=10;
                unsigned width=(scenario&4)?((scenario&8)?20:0):(car*10+1)/2;
                if(race.fuel_option)
                    for(unsigned x=106+car*60;x<106+car*60+width;++x) expected[188*320+x]=11;
                if(race.damage_scale && car)
                    for(unsigned x=106+car*60;x<106+car*60+(car==1?10:20);++x) expected[189*320+x]=12;
            }
            /* Exercise sparse-list saturation as well as ordinary drawing. */
            if(scenario&8)
                for(unsigned i=0;i<SLICKS_DIRTY_PIXEL_MAX;++i) mark_dirty_pixel(&race,i%320,i/320);
            if(slicks_race_draw_status(&race,logical,(scenario&8)?1:0)) return 1;
            for(unsigned at=0;at<64000;++at) {
                unsigned x=at%320,y=at/320;
                unsigned offset=(x&3)*65536+y*100+x/4;
                if(pixels[at]!=expected[at] || logical[offset]!=expected[at] ||
                   (expected[at]!=77 && !covered(&race,x,y))) {
                    fprintf(stderr,"HUD pixels/dirty mismatch scenario=%u xy=%u,%u\n",scenario,x,y); return 1;
                }
            }
        }
        memset(&race,0,sizeof race);
        memset(pixels,77,sizeof pixels); memset(expected,77,sizeof expected);
        race.chunky=pixels; race.fuel_option=1; race.status_colours[0]=10;
        for(unsigned car=0;car<4;++car) race.cars[car].fuel_capacity=100;
        race.cars[2].fuel_capacity=0;
        if(slicks_race_draw_status(&race,NULL,0)!=-1 ||
           memcmp(pixels,expected,sizeof pixels) || race.dirty_pixel_count || race.dirty_row_count) return 1;
        race.cars[2].fuel=0x10000000U; race.cars[2].fuel_capacity=0xffffffffU;
        if(slicks_race_draw_status(&race,NULL,0)!=-1 ||
           memcmp(pixels,expected,sizeof pixels)) return 1;
        struct SlicksStatusClock clock={0};
        unsigned long long frames=0;
        for(unsigned i=0;i<100000;++i) {
            unsigned elapsed=(i*7)%13;
            slicks_status_clock_advance(&clock,elapsed); frames+=elapsed;
            unsigned long long total=frames*1193182ULL;
            if(clock.ticks!=(unsigned short)(total/3276800ULL) ||
               clock.remainder!=total%3276800ULL) return 1;
        }
        puts("Status HUD: exact pixels, VGA/chunky agreement, dirty saturation and BIOS-rate clock verified");
    }
    puts("Dirty tracking preserves particle and HUD updates at sparse-list capacity.");
    return 0;
}
