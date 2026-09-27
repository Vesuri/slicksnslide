/* Incremental HUD repainting versus an independently forced full repaint.
 * Original DOS command/pixel equivalence is covered by verify-dos-hud. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/game/race_runtime.c"

static struct SlicksRaceRuntime race,reference;
static unsigned char actual[64000],expected[64000],before[64000];
static unsigned seed=421;
static unsigned random_word(void) { seed=seed*1664525U+1013904223U;return seed>>8; }
static void require(int ok,const char *what,unsigned step)
{ if(!ok){fprintf(stderr,"HUD cache step %u: %s\n",step,what);exit(1);} }
static int covered(unsigned x,unsigned y)
{
    for(unsigned i=0;i<race.dirty_row_count;++i){
        const struct SlicksDirtyRows *r=&race.dirty_rows[i];
        if(x>=(unsigned)r->left && x<(unsigned)r->right && y>=(unsigned)r->top && y<(unsigned)r->bottom)return 1;
    }
    for(unsigned i=0;i<race.dirty_pixel_count;++i)
        if(race.dirty_pixels[i].x==x && race.dirty_pixels[i].y==y)return 1;
    return 0;
}
int main(void)
{
    race.chunky=actual;race.chunky_authoritative=1;race.participation_ready=1;
    race.fuel_option=1;race.damage_scale=1;race.hud_background_ready=1;
    race.status_colours[0]=31;race.status_colours[1]=51;race.status_colours[2]=61;
    race.weapon_hud_colour=71;
    for(unsigned i=0;i<64000;++i)actual[i]=(unsigned char)(i*17);
    for(unsigned i=0;i<sizeof race.hud_background;++i)race.hud_background[i]=(unsigned char)(i*13);
    for(unsigned i=0;i<8;++i){
        race.hud_weapon_icons[i].ready=1;race.hud_weapon_icons[i].width=3;race.hud_weapon_icons[i].height=2;
        for(unsigned p=0;p<6;++p)race.hud_weapon_icons[i].pixels[p]=(p+i)%3?(unsigned char)(80+i):0;
        race.weapon_capacity[i+5]=20;
    }
    for(unsigned car=0;car<4;++car){
        race.participation[car]=1;race.cars[car].fuel=75;race.cars[car].fuel_capacity=100;
        race.cars[car].damage[0]=100;race.selected_weapon[car]=car;
        for(unsigned i=5;i<13;++i)race.weapon_inventory[car][i]=10;
    }
    require(!slicks_race_draw_status(&race,0,0),"initial full paint",0);
    for(unsigned mode=0;mode<2;++mode)for(unsigned car=0;car<4;++car)for(unsigned row=0;row<3;++row){
        slicks_race_clear_dirty_rows(&race);
        unsigned x=106+car*60+7,y=187+row;
        memcpy(expected,actual,sizeof expected);actual[y*320+x]^=0xff;
        if(mode)mark_dirty_rect(&race,(int)x,(int)y,(int)x+1,(int)y+1);
        else mark_dirty_pixel(&race,(int)x,(int)y);
        require(!slicks_race_draw_status(&race,0,0),"isolated damage repaint",mode*12+car*3+row);
        require(!memcmp(actual,expected,sizeof actual),"isolated row damage not repaired",mode*12+car*3+row);
    }
    unsigned checks=0,errors=0,pending_failure=0;
    for(unsigned step=0;step<4096;++step){
        /* A failed draw cannot be presented and have its damage forgotten.
         * Production exits the race on error; this recovery stress test
         * instead retains pending damage until a successful repaint. */
        if(!pending_failure)slicks_race_clear_dirty_rows(&race);
        unsigned car=random_word()%4;
        switch(step%16){
        case 0:race.cars[car].fuel=random_word()%101;race.cars[car].fuel_capacity=100;break;
        case 1:race.cars[car].damage[0]=(short)(random_word()%1100);break;
        case 2:race.cars[car].service_flags^=1;break;
        case 3:race.status_colours[random_word()%3]=(unsigned char)(1+random_word()%254);break;
        case 4:race.participation[car]=(signed char)((int)(random_word()%3)-1);break;
        case 5:race.weapons_enabled^=1;break;
        case 6:race.selected_weapon[car]=(signed char)((int)(random_word()%9)-1);break;
        case 7:race.weapon_inventory[car][5+random_word()%8]=(short)(random_word()%21);break;
        case 8:race.fuel_option^=1;race.damage_scale^=1;break;
        case 9:
            race.cars[car].fuel=random_word()*257U;
            race.cars[car].fuel_capacity=step&32?1U:~0U; /* signed/wrapped fallback widths */
            if(step%64==9)race.cars[car].fuel=0x10000000U; /* INT_MIN / -1 fault */
            break;
        case 10:race.cars[car].fuel_capacity=0;break; /* failure before any painting */
        case 11:race.cars[car].fuel_capacity=100;race.cars[car].fuel=50;break;
        case 12:race.status_bar_cache.valid=0;break;
        default:break; /* warm unchanged calls */
        }
        /* Rectangles and individual pixels can damage any row, another
         * driver's unchanged cell, a cell gap, or the adjacent track/HUD. */
        if(step%3==0){
            int left=(int)(random_word()%320),top=184+(int)(random_word()%16);
            int right=left+1+(int)(random_word()%30),bottom=top+1+(int)(random_word()%4);
            if(right>320)right=320;if(bottom>200)bottom=200;
            for(int y=top;y<bottom;++y)for(int x=left;x<right;++x)actual[y*320+x]^=0x5a;
            mark_dirty_rect(&race,left,top,right,bottom);
        }
        if(step%3==1){
            unsigned x=106+60*(random_word()%4)+random_word()%20,y=187+random_word()%3;
            actual[y*320+x]^=0x6c;mark_dirty_pixel(&race,(int)x,(int)y);
        }
        if(step%31==0){
            /* Overflow into rectangle tracking must retain HUD coverage. */
            for(unsigned i=0;i<520;++i)mark_dirty_pixel(&race,106+(int)(i%20),187+(int)(i%3));
        }
        memcpy(before,actual,sizeof before);memcpy(expected,actual,sizeof expected);
        reference=race;reference.chunky=expected;reference.status_bar_cache.valid=0;
        __typeof__(race.status_bar_cache) old_cache=race.status_bar_cache;
        unsigned short timer=(unsigned short)(step/5);
        int want=slicks_race_draw_status(&reference,0,timer);
        int got=slicks_race_draw_status(&race,0,timer);
        require(got==want,"return value differs from full repaint",step);
        if(memcmp(actual,expected,sizeof actual)) {
            fprintf(stderr,"options fuel=%u damage=%u weapons=%u cache=%u active=%u dirtyhud=%u\n",
                race.fuel_option,race.damage_scale,race.weapons_enabled,old_cache.valid,old_cache.active,race.dirty_pixel_hud);
            for(unsigned at=0;at<64000;++at)if(actual[at]!=expected[at]) {
                fprintf(stderr,"first pixel xy=%u,%u before=%u actual=%u expected=%u\n",at%320,at/320,before[at],actual[at],expected[at]);break;
            }
            for(unsigned c=0;c<4;++c)fprintf(stderr,"car=%u active=%d fuel=%u/%u service=%u damage=%d oldends=%d,%d,%d newends=%d,%d,%d\n",
                c,race.participation[c],race.cars[c].fuel,race.cars[c].fuel_capacity,race.cars[c].service_flags,race.cars[c].damage[0],
                old_cache.ends[c][0],old_cache.ends[c][1],old_cache.ends[c][2],
                race.status_bar_cache.ends[c][0],race.status_bar_cache.ends[c][1],race.status_bar_cache.ends[c][2]);
            require(0,"pixels differ from full repaint",step);
        }
        if(got<0)++errors;
        else ++checks;
        pending_failure=got<0;
        require(race.dirty_row_count==reference.dirty_row_count &&
            race.dirty_pixel_count==reference.dirty_pixel_count &&
            race.dirty_pixel_hud==reference.dirty_pixel_hud &&
            !memcmp(race.dirty_rows,reference.dirty_rows,race.dirty_row_count*sizeof race.dirty_rows[0]) &&
            !memcmp(race.dirty_pixels,reference.dirty_pixels,race.dirty_pixel_count*sizeof race.dirty_pixels[0]),
            "dirty sequence differs from full repaint",step);
        for(unsigned at=0;at<64000;++at)if(actual[at]!=before[at])
            require(covered(at%320,at/320),"changed pixel missing dirty coverage",step);
    }
    printf("HUD cache: 24 isolated row damages, %u successful transitions, %u matched failures; full pixels and exact dirty sequences/coverage match forced-cold repaint\n",checks,errors);
    return 0;
}
