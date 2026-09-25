#include <exec/memory.h>
#include <proto/exec.h>
#include "amiga_shop.h"
#include "../../ui/menu_icon.h"
#include "../../ui/menu_background.h"
extern void slicks_records_text(unsigned char *,const unsigned char *,const unsigned char *,short,short,unsigned short,unsigned short);

static void label(struct SlicksAmigaPlayerMenu *m,unsigned font,const unsigned char *s,
    short x,short y,unsigned char flags)
{
    /* Shop REGISTER! uses the original shadow flag, unlike the strict
     * player-menu adapter. Use the general clipped original font bridge. */
    slicks_records_text(m->renderer.ui.pixels,m->fonts[font],s,x,y,flags,0);
    if(m->renderer.ui.dirty) m->renderer.ui.dirty(m->renderer.ui.dirty_context,
        0,y,320,(short)(y+m->fonts[font][2]+1));
    __asm__ volatile("" ::: "memory");
}
static void number(struct SlicksAmigaPlayerMenu *m,unsigned font,short value,short x,short y,unsigned char flags)
{ unsigned char s[7]; slicks_records_decimal(value,s); label(m,font,s,x,y,flags); }
static signed char item(const struct SlicksShopContent *c,signed char row)
{ return slicks_shop_item(c->rules,&c->session->options,c->session->inventory[0],
    c->session->players.participation[0],c->session->players.vehicle[0],c->extra,row); }

/* Original 2c574 drawing coordinates. Full refresh substitutes for its two
 * row/column refresh masks; all pixels still come from the native painters. */
int slicks_amiga_shop_draw(struct SlicksAmigaPlayerMenu *m,const struct SlicksShopContent *c,
    const struct SlicksShopMenu *state)
{
    struct SlicksChunkyUi *ui=&m->renderer.ui;
    const struct SlicksSetupSession *s=c->session;
    if(slicks_restore_menu_background(ui,m->saved,0,0,0,0,320,200)) return -1;
    m->fonts[0][6]=slicks_ui_nearest(ui,55,55,30);
    m->fonts[1][6]=slicks_ui_nearest(ui,60,60,20);
    label(m,1,c->footer,3,193,0);
    number(m,0,c->track,284,3,2); label(m,0,(const unsigned char *)"/",286,3,0);
    number(m,0,c->total,293,3,0);
    m->fonts[0][6]=slicks_ui_nearest(ui,59,59,42);
    unsigned char selected=slicks_ui_nearest(ui,50,10,10),bar=slicks_ui_nearest(ui,70,70,10);
    for(int row=0;row<state->count;++row) {
        int it=item(c,(signed char)row); if(it<0) return -1;
        label(m,0,c->items[it],97,(short)(40+10*row),2);
        if(!c->extra && (c->rules->flags[it]&16))
            label(m,1,c->register_label,75,(short)(38+10*row),6);
    }
    unsigned column=0;
    for(unsigned d=0;d<4;++d) if(s->players.participation[d]) {
        short x=(short)(40*column);
        m->fonts[1][6]=slicks_ui_nearest(ui,40,40,75);
        label(m,1,c->names[d],(short)(115+x),(short)(38-7*(s->players.count-column)),0);
        number(m,1,s->cash[d],(short)(106+x),(short)(38-7*(s->players.count-column)),2);
        for(int row=0;row<state->count;++row) {
            int it=item(c,(signed char)row); short y=(short)(38+10*row);
            if((int)d==state->driver && row==state->row)
                slicks_ui_rectangle(ui,(short)(115+x),(short)(y+1),(short)(151+x),(short)(y+10),selected);
            short count=s->inventory[d][it];
            if(count>0) number(m,0,count,(short)(138+x),(short)(y+2),1);
            signed char height=(signed char)((short)(count*8)/c->rules->capacity[it]);
            slicks_ui_rectangle(ui,(short)(115+x),(short)(y+9-height),(short)(119+x),(short)(y+10),bar);
            short price=slicks_shop_price(c->rules,&s->options,s->inventory[d],s->players.participation[d],
                s->players.vehicle[d],it,c->extra);
            if(price>0) {
                if((c->rules->flags[it]&2) && count>0) price=(short)(price*c->rules->batch[it]);
                number(m,1,(short)(price/10),(short)(114+x),y,0);
            }
        }
        ++column;
    }
    short exit_y=(short)(42+state->count*10);
    if(state->row>=state->count) slicks_ui_bevel(ui,115,exit_y,35,10,50,10,10);
    label(m,0,c->exit_label,133,(short)(exit_y+2),1);
    return m->error || m->renderer.error?-1:0;
}

struct SlicksAmigaPlayerMenu *slicks_amiga_shop_create(struct SlicksResourceArchive *archive,
    unsigned char *chunky,const struct SlicksShopContent *c,struct SlicksShopMenu *state)
{
    static unsigned char palette[768];
    if(slicks_resource_archive_load(archive,"tuning.@p",palette,sizeof palette)!=sizeof palette) return 0;
    struct SlicksAmigaPlayerMenu *m=slicks_amiga_race_surface_create(archive,chunky,palette);
    unsigned char *resource=AllocMem(65536,MEMF_ANY);
    if(!m || !resource) goto failed;
    long size=slicks_resource_archive_load(archive,"tuning.@I",resource,65536);
    unsigned short width,height;
    if(size<0 || slicks_decode_indexed_menu_icon(resource,(unsigned long)size,m->saved,sizeof m->saved,
        &width,&height) || width!=320 || height!=200) goto failed;
    *state=(struct SlicksShopMenu){.driver=slicks_shop_driver(c->session->players.participation,-1,1)};
    while(state->count<13 && item(c,state->count)>=0) ++state->count;
    unsigned char tint[256];
    slicks_ui_tint_table(palette,tint,10,10,30,50);
    struct SlicksChunkyUi background={m->saved,palette,0,0};
    for(int row=0;row<state->count;++row) {
        int it=item(c,(signed char)row);
        if(!c->extra && (c->rules->flags[it]&16))
            slicks_ui_remap(&background,62,(short)(38+row*10),76,(short)(43+row*10),tint);
        for(unsigned column=0;column<c->session->players.count;++column)
            slicks_ui_remap(&background,(short)(115+40*column),(short)(39+10*row),
                (short)(150+40*column),(short)(47+10*row),tint);
    }
    /* Original item images at (98,40+10*row), cached against shop palette. */
    for(int row=0;row<state->count;++row) {
        int it=item(c,(signed char)row); char name[]="vir00.@16";
        name[3]=(char)('0'+it/10); name[4]=(char)('0'+it%10);
        size=slicks_resource_archive_load(archive,name,resource,65536);
        unsigned char pixels[256];
        if(size<0 || slicks_decode_menu_icon(resource,(unsigned long)size,palette,pixels,sizeof pixels,&width,&height) ||
           width>32 || height>10) goto failed;
        for(unsigned y=0;y<height;++y) for(unsigned x=0;x<width;++x)
            if(pixels[y*width+x]) m->saved[mult320[40+10*row+y]+98+x]=pixels[y*width+x];
    }
    FreeMem(resource,65536); resource=0;
    if(slicks_amiga_shop_draw(m,c,state)) goto failed;
    return m;
failed:
    if(resource) FreeMem(resource,65536);
    slicks_amiga_player_menu_destroy(m); return 0;
}
