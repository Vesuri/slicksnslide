#include <exec/memory.h>
#include <proto/exec.h>
#include <devices/inputevent.h>
#include <proto/keymap.h>
#include "amiga_player_menu.h"
#include "amiga_platform.h"
/* Owners that call original 36ca5 on entry/exit (pause menu, intermission,
 * Change Cars, Controllers, colour picker, list dialogs) forget a held key,
 * so it does not repeat into the next owner. */
#include "../../ui/font_resource.h"
#include "../../ui/help_text_dirty.h"
#include "../../ui/menu_bitmap.h"
#include "../../ui/menu_icon.h"
#include "../../ui/profile_delete_prompt.h"
#include "../../ui/track_records_renderer.h"
#include "../../ui/track_info.h"
#include "../../ui/track_info_renderer.h"
#include "../../ui/language_table.h"
#include "../../game/track_scene.h"

extern short slicks_menu_measure(const unsigned char *,const unsigned char *);
extern void slicks_menu_text(unsigned char *,const unsigned char *,const unsigned char *,short,short,unsigned short);
extern void slicks_records_text(unsigned char *,const unsigned char *,const unsigned char *,short,short,unsigned short,unsigned short);
extern void slicks_standings_text(unsigned char *,const unsigned char *,const unsigned char *,short,short,unsigned short,unsigned short);
extern short slicks_help_measure(const unsigned char *,const unsigned char *,short);
extern short slicks_help_text(unsigned char *,const unsigned char *,const unsigned char *,short,short,short);
extern void slicks_draw_chunky_icon(unsigned char *,const unsigned char *,unsigned short,unsigned short,unsigned short,unsigned short);

static void free_picker(struct SlicksAmigaProfilePicker *p)
{
    if(!p) return;
    if(p->owned_names) FreeMem(p->owned_names,p->owned_names_size);
    FreeMem(p,sizeof *p);
}
unsigned char g_slicks_diag_list_alloc_fault;
static int list_alloc_fault(unsigned char stage)
{
    if(g_slicks_diag_list_alloc_fault!=stage) return 0;
    g_slicks_diag_list_alloc_fault=0; return 1;
}

static void dirty(void *context,short left,short top,short right,short bottom)
{
    struct SlicksAmigaPlayerMenu *m=context;
    slicks_menu_dirty_add(m->dirty,&m->dirty_count,left,top,right,bottom);
    if(m->track_saved_dirty)
        slicks_menu_dirty_add(m->saved_dirty,&m->saved_dirty_count,left,top,right,bottom);
    if(m->track_info)
        slicks_menu_dirty_add(m->track_info->painted,&m->track_info->painted_count,
            left,top,right,bottom);
}
static int records_text(void *context,struct SlicksChunkyUi *ui,unsigned char *font,
    const unsigned char *string,short x,short y,unsigned char flags,unsigned char highlight)
{
    (void)context;
    short width=slicks_menu_measure(font,string);
    if(width<0 || y<0 || y+font[2]>200) return -1;
    slicks_records_text(ui->pixels,font,string,x,y,flags,highlight);
    /* Match actual glyph stores, including tabs/newlines in profile names,
     * glyph-zero advances and the records bridge's horizontal shadow. */
    slicks_font_text_dirty(ui,font,string,x,y,1,flags,width,0);
    return 0;
}
static int records_icon(void *context,struct SlicksChunkyUi *ui,short id,short x,short y)
{
    struct SlicksAmigaPlayerMenu *m=context;
    /* 2a03b loads carimage16 at DS:4e44; records index DS:4e40, so
     * saved IDs 1..10 map directly to our icon slots 1..10. */
    unsigned index=id==-1?0:(unsigned)id;
    if((id!=-1 && id<1) || index>=11 || !m->icons[index].pixels) return -1;
    const struct SlicksMenuIcon *sprite=&m->icons[index];
    if(x<0 || y<0 || x+sprite->width>320 || y+sprite->height>200) return -1;
    slicks_draw_chunky_icon(ui->pixels,sprite->pixels,x,y,sprite->width,sprite->height);
    dirty(m,x,y,(short)(x+sprite->width),(short)(y+sprite->height)); return 0;
}
static unsigned char standings_nearest(void *context,unsigned char r,unsigned char g,unsigned char b)
{ return slicks_ui_nearest(&((struct SlicksAmigaPlayerMenu *)context)->renderer.ui,r,g,b); }
static void standings_colour(void *context,unsigned char index,unsigned char value)
{ ((struct SlicksAmigaPlayerMenu *)context)->fonts[0][6+index]=value; }
static void standings_rectangle(void *context,short l,short t,short r,short b,unsigned char colour)
{ slicks_ui_rectangle(&((struct SlicksAmigaPlayerMenu *)context)->renderer.ui,l,t,r,b,colour); }
static void standings_text(void *context,const unsigned char *s,short x,short y,unsigned char flags)
{
    struct SlicksAmigaPlayerMenu *m=context;
    slicks_standings_text(m->renderer.ui.pixels,m->fonts[0],s,x,y,flags,
        slicks_ui_nearest(&m->renderer.ui,10,10,10));
    slicks_font_text_dirty(&m->renderer.ui,m->fonts[0],s,x,y,1,flags,
        slicks_menu_measure(m->fonts[0],s),1);
}
static void standings_number(void *context,short value,short x,short y,unsigned char flags)
{
    unsigned char s[7]; slicks_records_decimal(value,s);
    standings_text(context,s,x,y,flags);
}
void slicks_amiga_standings_draw(struct SlicksAmigaPlayerMenu *m,
    const struct SlicksChampionshipStandings *table,const unsigned char colours[4][6],
    const unsigned char *const names[4])
{
    const struct SlicksStandingsDrawOps ops={standings_nearest,standings_colour,
        standings_rectangle,standings_text,standings_number,m};
    slicks_draw_championship_standings(table,colours,names,&ops);
}
unsigned char g_slicks_diag_pause_fault;
static int pause_fault(unsigned char stage)
{
    if(g_slicks_diag_pause_fault!=stage) return 0;
    g_slicks_diag_pause_fault=0; return 1;
}
int slicks_amiga_race_menu_close(struct SlicksAmigaPlayerMenu *m)
{
    slicks_amiga_platform_clear_latch(0);
    if(!m || !m->race_menu || m->help || m->help_warning || m->controllers_dialog ||
       m->race_menu->speed_active) return -1;
    struct SlicksAmigaRaceMenu *d=m->race_menu;
    slicks_race_menu_render_close(&d->renderer,&d->surface);
    slicks_amiga_player_menu_restore(m);
    FreeMem(d,sizeof *d); m->race_menu=0; return 0;
}
int slicks_amiga_race_menu_draw(struct SlicksAmigaPlayerMenu *m)
{
    if(!m || !m->race_menu || m->help || m->help_warning || m->controllers_dialog ||
       m->race_menu->speed_active) return -1;
    struct SlicksAmigaRaceMenu *d=m->race_menu;
    return slicks_race_menu_render_draw(&d->renderer,&d->surface,&d->state,d->labels);
}
int slicks_amiga_race_speed_close(struct SlicksAmigaPlayerMenu *m,
    const struct SlicksConfiguration *configuration,unsigned short *timer_argument)
{
    if(!m || !m->race_menu || !m->race_menu->speed_active || !configuration || !timer_argument) return -1;
    struct SlicksAmigaRaceMenu *d=m->race_menu;
    *timer_argument=slicks_speed_dialog_close(&d->speed,&d->surface,configuration->field_05de);
    d->speed_active=0;
    if(slicks_restore_rectangle(&d->surface.ui,&d->renderer.background,40,40,0,0,100,75)) return -1;
    d->state.redraw=-1;
    return slicks_amiga_race_menu_draw(m);
}
int slicks_amiga_race_speed_open(struct SlicksAmigaPlayerMenu *m,
    struct SlicksConfiguration *configuration,unsigned char *dirty_flag)
{
    if(!m || !m->race_menu || m->race_menu->speed_active || m->help || m->help_warning ||
       m->controllers_dialog || !configuration || !dirty_flag) return -1;
    struct SlicksAmigaRaceMenu *d=m->race_menu;
    d->speed_active=1;
    if(slicks_speed_dialog_open(&d->speed,&d->surface) ||
       slicks_speed_dialog_draw(&d->speed,&d->surface,configuration->field_05de)) {
        unsigned short ignored; (void)slicks_amiga_race_speed_close(m,configuration,&ignored); return -1;
    }
    *dirty_flag=1; return 0;
}
int slicks_amiga_race_speed_key(struct SlicksAmigaPlayerMenu *m,
    struct SlicksConfiguration *configuration,unsigned char key)
{
    if(!m || !m->race_menu || !m->race_menu->speed_active || !configuration) return -1;
    struct SlicksAmigaRaceMenu *d=m->race_menu;
    configuration->field_05de=slicks_speed_dialog_key(&d->speed,configuration->field_05de,key);
    return d->speed.done?0:slicks_speed_dialog_draw(&d->speed,&d->surface,configuration->field_05de);
}
int slicks_amiga_race_menu_open(struct SlicksAmigaPlayerMenu *m,
    struct SlicksResourceArchive *archive,const char *language,
    const unsigned char keys[6][64],unsigned char row,unsigned char percent)
{
    slicks_amiga_platform_clear_latch(0);
    if(!m || !archive || !language || !keys || row>=6 || m->race_menu ||
       m->help || m->controllers_dialog || !m->renderer.ui.pixels || !m->renderer.fonts[0]) return -1;
    struct SlicksAmigaRaceMenu *d=pause_fault(1)?0:AllocMem(sizeof *d,MEMF_ANY|MEMF_CLEAR);
    if(!d) return -1;
    unsigned char resource[1000]; unsigned used=0;
    long size=slicks_resource_archive_load(archive,pause_fault(2)?"missing-language":language,resource,sizeof resource);
    if(size<0 || slicks_language_table_load(resource,(unsigned)size,d->language,sizeof d->language,&used)) {
        FreeMem(d,sizeof *d); return -1;
    }
    for(unsigned i=0;i<6;++i) d->labels[i]=slicks_language_lookup(d->language,used,keys[i],keys[i]);
    d->state=(struct SlicksRaceMenu){row,6,-1,0};
    d->surface=(struct SlicksRecordsRenderer){.ui=m->renderer.ui,.fonts={m->fonts[0],m->fonts[1]},
        .text=records_text,.context=m};
    for(unsigned long i=0;i<64000;++i) m->saved[i]=m->renderer.ui.pixels[i];
    m->saved_dirty_count=0; m->track_saved_dirty=1;
    if(slicks_race_menu_render_open(&d->renderer,&d->surface,&d->state,
        percent,d->tinted,sizeof d->tinted)) { FreeMem(d,sizeof *d); return -1; }
    m->race_menu=d;
    if(slicks_amiga_race_menu_draw(m) || pause_fault(3)) {
        (void)slicks_amiga_race_menu_close(m); return -1;
    }
    return 0;
}
/* One-shot diagnostic boundary faults; zero in normal runs. */
unsigned char g_slicks_diag_track_info_fault;
unsigned char g_slicks_diag_track_info_stage;
unsigned char g_slicks_diag_track_info_probe;
unsigned long g_slicks_diag_track_info_free,g_slicks_diag_track_info_largest;
static int track_info_fault(unsigned char stage)
{
    if(g_slicks_diag_track_info_fault!=stage) return 0;
    g_slicks_diag_track_info_fault=0; return 1;
}
static int load_car_menu_icons(struct SlicksAmigaPlayerMenu *m,struct SlicksResourceArchive *archive,
    const char *marker,unsigned char records_faults)
{
    if(!m || !archive) return -1;
    /* These eleven encoded resources are at most 338 bytes in Slix 1.51;
     * the decoded icons have their own checked destination capacity. */
    const unsigned long capacity=512;
    unsigned char *resource=records_faults && track_info_fault(4)?0:AllocMem(capacity,MEMF_ANY);
    if(!resource) return -1;
    int result=-1;
    for(unsigned i=0;i<11;++i) {
        char name[]="auto01.@16"; name[5]=(char)('0'+i-1);
        const char *resource_name=i==0?marker:i==1?"carimage16":name;
        if(records_faults && i==5 && track_info_fault(5)) resource_name="missing-track-icon";
        long size=slicks_resource_archive_load(archive,resource_name,resource,capacity);
        if(size<0 || slicks_decode_menu_icon(resource,(unsigned long)size,m->palette,m->pixels[i],sizeof m->pixels[i],
            &m->icons[i].width,&m->icons[i].height)) goto done;
        m->icons[i].pixels=m->pixels[i];
    }
    m->renderer.icons=m->icons; m->renderer.icon_count=11; result=0;
done:
    FreeMem(resource,capacity); return result;
}
int slicks_amiga_records_icons_load(struct SlicksAmigaPlayerMenu *m,struct SlicksResourceArchive *archive)
{ return load_car_menu_icons(m,archive,"top10cc.@16",1); }
int slicks_amiga_records_draw(struct SlicksAmigaPlayerMenu *m,const struct SlicksTrackRecords *records,
    const signed char ranks[4],short x,short y,unsigned char separator,signed char date_order)
{
    if(!m || m->renderer.icon_count!=11) return -1;
    struct SlicksRecordsRenderer r={.ui=m->renderer.ui,.fonts={m->fonts[0],m->fonts[1]},
        .text=records_text,.icon=records_icon,.context=m};
    return slicks_records_renderer_draw(&r,records,ranks,x,y,separator,date_order);
}
void slicks_amiga_track_info_close(struct SlicksAmigaPlayerMenu *m)
{
    if(!m || !m->track_info) return;
    struct SlicksAmigaTrackInfo *d=m->track_info;
    /* Detach before reporting restores so they cannot modify the lifetime
     * list being traversed. Failed partial opens use the same bounds.
     * The Tracks screen under the panel is its prepared background plus the
     * list draw, so restore the background here and let the caller redraw
     * the list: no 64,000-byte save-under competes with the preview arena
     * for Chip RAM on a 2 MiB machine. */
    m->track_info=0;
    for(unsigned i=0;i<d->painted_count;++i) {
        const struct SlicksMenuRect *r=&d->painted[i];
        for(unsigned y=r->top;y<r->bottom;++y)
            slicks_ui_copy_row(m->renderer.ui.pixels+mult320[y]+r->left,m->saved+mult320[y]+r->left,r->right-r->left);
        dirty(m,r->left,r->top,r->right,r->bottom);
    }
    for(unsigned i=0;i<2;++i) m->fonts[i][6]=d->font_colours[i];
    FreeMem(d,sizeof *d);
}
int slicks_amiga_track_info_open(struct SlicksAmigaPlayerMenu *m,struct SlicksResourceArchive *archive,
    const unsigned char *dat,unsigned long dat_size,const unsigned char *track,unsigned long track_size,
    const unsigned char *name,unsigned char percent,unsigned char separator,signed char date_order,
    unsigned char *arena)
{
    struct TrackSprite sprites[SLICKS_TRACK_PREVIEW_SPRITES];
    if(slicks_prepare_track_preview(dat,dat_size,arena,65536,sprites))return -1;
    return slicks_amiga_track_info_open_prepared(m,archive,track,track_size,name,
        percent,separator,date_order,sprites);
}
int slicks_amiga_track_info_open_prepared(struct SlicksAmigaPlayerMenu *m,struct SlicksResourceArchive *archive,
    const unsigned char *track,unsigned long track_size,const unsigned char *name,
    unsigned char percent,unsigned char separator,signed char date_order,const struct TrackSprite *sprites)
{
    if(!m || !archive || !name || !sprites || m->track_info || m->message || m->track_lists) return -1;
    g_slicks_diag_track_info_stage=0;
    struct SlicksTrackRecords records;
    unsigned char description[64];
    if(slicks_track_records(track,track_size,&records)!=1 ||
        slicks_track_description(track,track_size,description,sizeof description)) return -1;
    if(g_slicks_diag_track_info_fault || g_slicks_diag_track_info_probe) {
        g_slicks_diag_track_info_free=AvailMem(MEMF_ANY);
        g_slicks_diag_track_info_largest=AvailMem(MEMF_ANY|MEMF_LARGEST);
    }
    struct SlicksAmigaTrackInfo *d=track_info_fault(1)?0:AllocMem(sizeof *d,MEMF_ANY|MEMF_CLEAR);
    if(!d) return -1;
    g_slicks_diag_track_info_stage=1;
    for(unsigned i=0;i<2;++i) d->font_colours[i]=m->fonts[i][6];
    m->track_info=d;
    d->phase=1;
    if(slicks_resource_archive_load(archive,track_info_fault(3)?"missing-track-palette":"peli.@p",d->palette,sizeof d->palette)!=768) goto failed;
    g_slicks_diag_track_info_stage=2;
    if(slicks_amiga_records_icons_load(m,archive)) goto failed;
    g_slicks_diag_track_info_stage=3;
    /* The owner reserves the large workspace before loading smaller files.
     * Keep the diagnostic workspace-failure boundary transactional. */
    if(track_info_fault(2)) goto failed;
    struct SlicksChunkyUi *ui=&m->renderer.ui;
    d->phase=2;
    if(slicks_restore_menu_background(ui,m->saved,0,0,100,20,240,190)) goto failed;
    d->phase=3;
    struct SlicksRecordsRenderer renderer={.ui=*ui,.fonts={m->fonts[0],m->fonts[1]},
        .text=records_text,.icon=records_icon,.context=m};
    if(slicks_track_info_render(&renderer,&records,name,description,percent,separator,date_order)) goto failed;
    d->phase=5;
    if(slicks_draw_track_preview(ui,sprites,track,track_size,245,20)) goto failed;
    for(unsigned y=0;y<40;++y) for(unsigned x=0;x<64;++x)
        d->preview[y*64+x]=ui->pixels[mult320[y+20]+x+245];
    d->phase=6; return 0;
failed:
    slicks_amiga_track_info_close(m); return -1;
}
void slicks_amiga_track_info_tick(struct SlicksAmigaPlayerMenu *m,unsigned long *seed)
{
    if(!m || !m->track_info || !seed) return;
    struct SlicksAmigaTrackInfo *d=m->track_info; struct SlicksChunkyUi *ui=&m->renderer.ui;
    /* Original busy-loop work is independent of simulation ticks. Bound the
     * number of samples per host menu update; every sample keeps its three
     * original random draws and always reads the immutable preview copy. */
    for(unsigned i=0;i<64;++i) {
        slicks_track_preview_shimmer(ui,d->preview,d->palette,seed); ++d->updates;
    }
}
static short help_measure(void *context,const unsigned char *font,const unsigned char *string,signed char spacing)
{
    (void)context; short width=slicks_help_measure(font,string,spacing);
    __asm volatile("" : "+d"(width) :: "memory");
    return width;
}
static short help_text(void *context,struct SlicksChunkyUi *ui,unsigned char *font,
    const unsigned char *string,short x,short y,signed char spacing)
{
    (void)context;
    short advance=slicks_help_text(ui->pixels,font,string,x,y,spacing);
    slicks_help_text_dirty(ui,font,string,x,y,spacing);
    return advance;
}
int slicks_amiga_help_renderer_init(struct SlicksAmigaPlayerMenu *m,struct SlicksHelpRenderer *r)
{
    if(!m || !r || !m->renderer.fonts[0] || !m->renderer.ui.pixels || !m->renderer.ui.palette) return -1;
    *r=(struct SlicksHelpRenderer){0}; r->ui=m->renderer.ui; r->font=m->renderer.fonts[0];
    r->context=m; r->measure=help_measure; r->text=help_text;
    return 0;
}
static void text(void *context,struct SlicksChunkyUi *ui,unsigned char *font,
    const unsigned char *string,short x,short y,unsigned char flags)
{
    struct SlicksAmigaPlayerMenu *m=context;
    short width=slicks_menu_measure(font,string),left=x;
    if((flags&3)==1) left=(short)(left-width/2);
    else if((flags&3)==2) left=(short)(left-width);
    /* These menu strings are single-line and do not request shadows. */
    if(width<0 || left<0 || left+width>320 || y<0 || y+font[2]>200 || (flags&4)) { m->error=-1; return; }
    slicks_menu_text(ui->pixels,font,string,x,y,flags);
    slicks_font_text_dirty(ui,font,string,x,y,1,flags,width,0);
}
static void icon(void *context,struct SlicksChunkyUi *ui,const struct SlicksMenuIcon *sprite,short x,short y)
{
    struct SlicksAmigaPlayerMenu *m=context;
    if(x<0 || y<0 || x+sprite->width>320 || y+sprite->height>200) { m->error=-1; return; }
    slicks_draw_chunky_icon(ui->pixels,sprite->pixels,(unsigned short)x,(unsigned short)y,sprite->width,sprite->height);
    dirty(m,x,y,(short)(x+sprite->width),(short)(y+sprite->height));
}
void slicks_amiga_player_menu_clear_dirty(struct SlicksAmigaPlayerMenu *m)
{ m->dirty_count=0; }

unsigned char g_slicks_diag_change_cars_fault;
unsigned char g_slicks_diag_intermission_fault;
unsigned char g_slicks_diag_intermission_fault_reached,g_slicks_diag_change_cars_fault_reached;
struct IntermissionPreviewInput {
    const unsigned char *dat,*track;
    unsigned long dat_size,track_size;
    unsigned char *arena;
};
static int intermission_preview(void *context,struct SlicksChunkyUi *ui,short x,short y)
{
    struct IntermissionPreviewInput *p=context;
    return slicks_build_track_preview(ui,p->dat,p->dat_size,p->track,p->track_size,p->arena,65536,x,y);
}
static int intermission_copy_string(unsigned char *out,unsigned capacity,const unsigned char *in)
{
    if(!in) return -1;
    unsigned length=0; while(length<capacity && in[length]) ++length;
    if(length==capacity) return -1;
    for(unsigned i=0;i<=length;++i) out[i]=in[i];
    return 0;
}
int slicks_amiga_intermission_close(struct SlicksAmigaPlayerMenu *m)
{
    if(!m || !m->intermission || m->change_cars) return -1;
    struct SlicksAmigaIntermission *d=m->intermission;
    slicks_amiga_player_menu_restore(m);
    m->renderer.fonts[0][6]=d->old_colour;
    FreeMem(d,sizeof *d); m->intermission=0; return 0;
}
int slicks_amiga_intermission_open(struct SlicksAmigaPlayerMenu *m,const struct SlicksIntermissionContent *content,
    const unsigned char *source_palette,const unsigned char *dat,unsigned long dat_size,
    const unsigned char *track,unsigned long track_size,unsigned char *arena,unsigned long arena_size)
{
    slicks_amiga_platform_clear_latch(0);
    if(!m || !content || !source_palette || !dat || !track || !m->renderer.ui.pixels ||
       !m->renderer.fonts[0] || m->renderer.icon_count!=11 || m->intermission || m->change_cars ||
       m->race_menu || m->help || m->help_warning || m->controllers_dialog || m->picker || m->editor_active ||
       m->name_dialog || m->colour_dialog || m->message || m->track_info || m->track_lists) return -1;
    unsigned char fault=g_slicks_diag_intermission_fault; g_slicks_diag_intermission_fault=0;
    g_slicks_diag_intermission_fault_reached=0;
    if(fault==1) g_slicks_diag_intermission_fault_reached=1;
    struct SlicksAmigaIntermission *d=fault==1?0:AllocMem(sizeof *d,MEMF_ANY|MEMF_CLEAR);
    if(fault==2 && d) g_slicks_diag_intermission_fault_reached=2;
    /* Borrow caller-owned startup storage: no late 64 KiB heap request. */
    if(!d || fault==2 || !arena || arena_size<65536) {
        if(d) FreeMem(d,sizeof *d);
        return -1;
    }
    d->content=*content;
    for(unsigned i=0;i<4;++i) {
        if(content->roles[i] && intermission_copy_string(d->names[i],sizeof d->names[i],content->names[i])) goto before_paint;
        if(intermission_copy_string(d->labels[i],sizeof d->labels[i],content->labels[i])) goto before_paint;
        d->content.names[i]=d->names[i]; d->content.labels[i]=d->labels[i];
    }
    if(intermission_copy_string(d->track_name,sizeof d->track_name,content->track_name) ||
       intermission_copy_string(d->slash,sizeof d->slash,content->slash)) goto before_paint;
    d->content.track_name=d->track_name; d->content.slash=d->slash;
    d->surface=(struct SlicksRecordsRenderer){.ui=m->renderer.ui,.fonts={m->fonts[0],m->fonts[1]},
        .text=records_text,.icon=records_icon,.context=m};
    d->renderer.surface=&d->surface; d->renderer.fastest_icon=-1;
    d->old_colour=m->renderer.fonts[0][6];
    for(unsigned long i=0;i<64000;++i) m->saved[i]=m->renderer.ui.pixels[i];
    m->saved_dirty_count=0; m->track_saved_dirty=1;
    m->intermission=d;
    struct IntermissionPreviewInput input={dat,track,dat_size,track_size,arena};
    int result=slicks_intermission_renderer_open(&d->renderer,&d->state,&d->content,source_palette,
        d->buttons,sizeof d->buttons,d->cars,sizeof d->cars,intermission_preview,&input);
    if(!result && fault==3) g_slicks_diag_intermission_fault_reached=3;
    if(result || fault==3) { (void)slicks_amiga_intermission_close(m); return -1; }
    return 0;
before_paint:
    FreeMem(d,sizeof *d); return -1;
}
int slicks_amiga_intermission_key(struct SlicksAmigaPlayerMenu *m,unsigned char key)
{
    if(!m || !m->intermission || m->change_cars) return -1;
    struct SlicksAmigaIntermission *d=m->intermission;
    enum SlicksIntermissionAction action=slicks_intermission_key(&d->state,key);
    if(slicks_intermission_renderer_draw(&d->renderer,&d->state,&d->content)) return -1;
    return (int)action;
}
int slicks_amiga_intermission_refresh_cars(struct SlicksAmigaPlayerMenu *m,const signed char vehicles[4])
{
    if(!m || !m->intermission || m->change_cars || !vehicles) return -1;
    struct SlicksAmigaIntermission *d=m->intermission;
    for(unsigned i=0;i<4;++i) if(d->content.roles[i] && (vehicles[i]<0 || vehicles[i]>=10)) return -1;
    for(unsigned i=0;i<4;++i) d->content.vehicles[i]=vehicles[i];
    d->state.cars_redraw=1; d->state.redraw=1;
    return slicks_intermission_renderer_draw(&d->renderer,&d->state,&d->content);
}
int slicks_amiga_change_cars_close(struct SlicksAmigaPlayerMenu *m)
{
    slicks_amiga_platform_clear_latch(0);
    if(!m || !m->change_cars) return -1;
    struct SlicksAmigaChangeCars *d=m->change_cars;
    int result=d->renderer.active?slicks_change_cars_renderer_close(&d->renderer):0;
    FreeMem(d,sizeof *d); m->change_cars=0;
    return result;
}
int slicks_amiga_change_cars_key(struct SlicksAmigaPlayerMenu *m,unsigned char key)
{
    if(!m || !m->change_cars) return -1;
    struct SlicksAmigaChangeCars *d=m->change_cars;
    if(d->state.done) return 1;
    if(slicks_change_cars_key(&d->state,d->players->participation,d->players->vehicle,
        d->vehicle_count,key)) return -1;
    if(d->state.done) return 1;
    m->error=0;
    if(slicks_change_cars_renderer_draw(&d->renderer,&d->state,
        d->players->participation,d->players->vehicle) || m->error) return -1;
    return 0;
}
int slicks_amiga_change_cars_open(struct SlicksAmigaPlayerMenu *m,struct SlicksResourceArchive *archive,
    struct SlicksProfileSelection *players,const struct SlicksSetupProfile *profiles,short vehicle_count,
    const unsigned char *weights,unsigned long *random_state,signed char show,const unsigned char *title)
{
    slicks_amiga_platform_clear_latch(0);
    if(!m || !archive || !players || !profiles || !weights || !random_state || !title ||
       vehicle_count<1 || vehicle_count>10 || !m->renderer.ui.pixels || !m->renderer.ui.palette ||
       !m->renderer.fonts[0] || m->change_cars || m->race_menu || m->help || m->help_warning ||
       m->controllers_dialog || m->picker || m->name_dialog || m->colour_dialog ||
       m->editor_active || m->message || m->track_info || m->track_lists) return -1;
    unsigned count=0;
    for(unsigned i=0;i<4;++i) if(players->participation[i]) ++count;
    if(!count || count!=players->count) return -1;
    if(!slicks_change_cars_prepare(players,profiles,vehicle_count,weights,random_state,show)) return 0;
    unsigned char fault=g_slicks_diag_change_cars_fault; g_slicks_diag_change_cars_fault=0;
    g_slicks_diag_change_cars_fault_reached=0;
    if(fault==1) g_slicks_diag_change_cars_fault_reached=1;
    struct SlicksAmigaChangeCars *d=fault==1?0:AllocMem(sizeof *d,MEMF_ANY|MEMF_CLEAR);
    if(!d) return -1;
    /* Small bounded resource scratch: the ten menu car icons fit 192 pixels. */
    unsigned char resource[1024];
    for(unsigned i=0;i<(unsigned)vehicle_count;++i) {
        char name[]="auto01.@16"; name[5]=(char)('0'+i);
        const char *path=i?name:"carimage16";
        if(fault==2 && i==(unsigned)vehicle_count-1) {
            path="missing-car-icon";g_slicks_diag_change_cars_fault_reached=2;
        }
        long size=slicks_resource_archive_load(archive,path,resource,sizeof resource);
        if(size<0 || slicks_decode_menu_icon(resource,(unsigned long)size,m->renderer.ui.palette,
            d->pixels[i],sizeof d->pixels[i],&d->icons[i+1].width,&d->icons[i+1].height)) goto failed;
        d->icons[i+1].pixels=d->pixels[i];
    }
    d->players=players; d->vehicle_count=vehicle_count;
    d->surface=m->renderer; d->surface.icons=d->icons; d->surface.icon_count=(unsigned)vehicle_count+1;
    d->surface.text=text; d->surface.icon=icon; d->surface.context=m;
    d->renderer.surface=&d->surface; m->error=0;
    if(slicks_change_cars_renderer_open(&d->renderer,&d->state,210,71,count,title,
        d->original,sizeof d->original,d->decorated,sizeof d->decorated) || m->error) goto failed;
    if(slicks_change_cars_renderer_draw(&d->renderer,&d->state,players->participation,players->vehicle) ||
       m->error) goto failed;
    if(fault==3) {g_slicks_diag_change_cars_fault_reached=3;goto failed;}
    m->change_cars=d; return 1;
failed:
    if(d->renderer.active) (void)slicks_change_cars_renderer_close(&d->renderer);
    FreeMem(d,sizeof *d); return -1;
}
void slicks_amiga_player_menu_destroy(struct SlicksAmigaPlayerMenu *m)
{
    if(!m) return;
    if(m->change_cars) (void)slicks_amiga_change_cars_close(m);
    if(m->intermission) (void)slicks_amiga_intermission_close(m);
    if(m->help_warning) (void)slicks_amiga_help_warning_close(m);
    if(m->track_info) FreeMem(m->track_info,sizeof *m->track_info);
    if(m->track_lists) {
        FreeMem(m->track_lists,sizeof *m->track_lists);
    }
    if(m->message) FreeMem(m->message,sizeof *m->message);
    if(m->help) {
        unsigned char *saved=m->help->renderer.saved.pixels;
        if(saved && saved!=m->saved) FreeMem(saved,64000);
        FreeMem(m->help,sizeof *m->help);
    }
    if(m->controllers_dialog) FreeMem(m->controllers_dialog,sizeof *m->controllers_dialog);
    if(m->race_menu) FreeMem(m->race_menu,sizeof *m->race_menu);
    if(m->colour_dialog) FreeMem(m->colour_dialog,sizeof *m->colour_dialog);
    if(m->name_dialog) FreeMem(m->name_dialog,sizeof *m->name_dialog);
    free_picker(m->picker);
    FreeMem(m,sizeof *m);
}

unsigned char g_slicks_diag_controllers_fault;
int slicks_amiga_controllers_open_at(struct SlicksAmigaPlayerMenu *m,struct SlicksResourceArchive *archive,short x,short y)
{
    slicks_amiga_platform_clear_latch(0);
    if(!m || !archive || m->controllers_dialog || m->picker || m->name_dialog || m->colour_dialog) return -1;
    unsigned char fault=g_slicks_diag_controllers_fault; g_slicks_diag_controllers_fault=0;
    struct SlicksAmigaControllersDialog *d=fault==1?0:AllocMem(sizeof *d,MEMF_ANY|MEMF_CLEAR);
    if(!d) return -1;
    unsigned char resource[512];
    static const char *const names[]={"ohj_key.@I","ohj_joy.@I","ohj_lptc.@I",
        "keys_m1.@16","keys_m2.@16","keys_m3.@16","keys_m4.@16","keys_m5.@16"};
    for(unsigned i=0;i<8;++i) {
        long size=slicks_resource_archive_load(archive,fault==2 && i==7?"missing-controller-icon":names[i],resource,sizeof resource);
        if(size<0) { FreeMem(d,sizeof *d); return -1; }
        d->icons[i].pixels=d->pixels[i];
        int error=i<3?slicks_decode_indexed_menu_icon(resource,(unsigned long)size,d->pixels[i],sizeof d->pixels[i],&d->icons[i].width,&d->icons[i].height):
            slicks_decode_menu_icon(resource,(unsigned long)size,m->renderer.ui.palette,d->pixels[i],sizeof d->pixels[i],&d->icons[i].width,&d->icons[i].height);
        if(error) { FreeMem(d,sizeof *d); return -1; }
    }
    d->renderer.surface=&m->renderer; d->renderer.icons=d->icons; m->error=0;
    int result=slicks_controllers_renderer_open(&d->renderer,&d->state,x,y,
        d->original,sizeof d->original,d->tinted,sizeof d->tinted);
    if(result || m->error) {
        if(d->renderer.active) slicks_controllers_renderer_close(&d->renderer);
        FreeMem(d,sizeof *d); return -1;
    }
    m->controllers_dialog=d; return 0;
}
int slicks_amiga_controllers_open(struct SlicksAmigaPlayerMenu *m,struct SlicksResourceArchive *archive)
{ return slicks_amiga_controllers_open_at(m,archive,100,80); }
int slicks_amiga_controllers_draw(struct SlicksAmigaPlayerMenu *m,
    const struct SlicksConfiguration *configuration,const struct SlicksControllersLabels *labels)
{
    if(!m || !m->controllers_dialog) return -1;
    m->error=0;
    struct SlicksAmigaControllersDialog *d=m->controllers_dialog;
    int result=slicks_controllers_renderer_draw(&d->renderer,&d->state,configuration,labels);
    return result?result:m->error;
}
int slicks_amiga_controllers_capture_prompt(struct SlicksAmigaPlayerMenu *m,const unsigned char *prompt)
{
    if(!m || !m->controllers_dialog) return -1;
    m->error=0;
    struct SlicksAmigaControllersDialog *d=m->controllers_dialog;
    int result=slicks_controllers_renderer_capture(&d->renderer,&d->state,prompt);
    return result?result:m->error;
}
int slicks_amiga_controllers_close(struct SlicksAmigaPlayerMenu *m)
{
    slicks_amiga_platform_clear_latch(0);
    if(!m || !m->controllers_dialog) return -1;
    struct SlicksAmigaControllersDialog *d=m->controllers_dialog;
    int result=slicks_controllers_renderer_close(&d->renderer);
    FreeMem(d,sizeof *d); m->controllers_dialog=0;
    return result;
}
static unsigned char menu_keymap[8][128],menu_keymap_ready;
int slicks_amiga_menu_keymap_init(void)
{
    /* Snapshot the installed keyboard layout while AmigaOS is available.
     * No library calls occur from the hardware-owned input loop. */
    if(menu_keymap_ready) return 0;
    struct Library *KeymapBase=OpenLibrary((CONST_STRPTR)"keymap.library",37);
    if(!KeymapBase) return -1;
    for(unsigned mode=0;mode<8;++mode) for(unsigned raw=0;raw<128;++raw) {
        struct InputEvent event={0}; unsigned char bytes[8];
        event.ie_Class=IECLASS_RAWKEY; event.ie_Code=(UWORD)raw;
        event.ie_Qualifier=(mode&1?IEQUALIFIER_LSHIFT:0)|(mode&2?IEQUALIFIER_CAPSLOCK:0)|(mode&4?IEQUALIFIER_LALT:0);
        WORD count=MapRawKey(&event,bytes,sizeof bytes,0);
        menu_keymap[mode][raw]=count==1?(unsigned char)bytes[0]:0;
    }
    CloseLibrary(KeymapBase); menu_keymap_ready=1; return 0;
}
static int prepare_keymap(struct SlicksAmigaPlayerMenu *m)
{
    if(!menu_keymap_ready) return -1;
    for(unsigned mode=0;mode<8;++mode) for(unsigned raw=0;raw<128;++raw)
        m->key_characters[mode][raw]=menu_keymap[mode][raw];
    return 0;
}
unsigned char slicks_amiga_menu_character(struct SlicksAmigaPlayerMenu *m,unsigned char code)
{
    unsigned raw=code&127;
    if(raw>=0x60 && raw<=0x65) {
        unsigned char bit=(unsigned char)(1U<<(raw-0x60));
        if(code&128) m->key_modifiers&=(unsigned char)~bit;
        else m->key_modifiers|=bit;
        return 0;
    }
    if(code&128) return 0;
    if(raw==0x44 || raw==0x43) return 13;
    if(raw==0x45) return 27;
    if(raw==0x41) return 8;
    unsigned mode=(m->key_modifiers&3?1:0)|(m->key_modifiers&4?2:0)|(m->key_modifiers&48?4:0);
    return m->key_characters[mode][raw];
}
unsigned char g_slicks_diag_surface_create_fault;
static int surface_create_fault(unsigned char stage)
{
    if(g_slicks_diag_surface_create_fault!=stage) return 0;
    g_slicks_diag_surface_create_fault=0;return 1;
}
struct SlicksAmigaPlayerMenu *slicks_amiga_help_surface_create(
    struct SlicksResourceArchive *archive,unsigned char *chunky,const unsigned char *palette)
{
    if(!archive || !chunky || !palette) return 0;
    struct SlicksAmigaPlayerMenu *m=surface_create_fault(1)?0:AllocMem(sizeof *m,MEMF_ANY|MEMF_CLEAR);
    unsigned char *resource=surface_create_fault(2)?0:AllocMem(8192,MEMF_ANY);
    if(!m || !resource) goto failed;
    long size=slicks_resource_archive_load(archive,surface_create_fault(3)?"missing-surface-font":"kirj.@f",resource,8192);
    if(surface_create_fault(4)) size=0;
    if(size<0 || slicks_decode_font_resource(resource,(unsigned long)size,m->fonts[0],sizeof m->fonts[0])<0) goto failed;
    for(unsigned i=0;i<768;++i) m->palette[i]=palette[i];
    m->renderer.fonts[0]=m->fonts[0];
    m->renderer.ui=(struct SlicksChunkyUi){chunky,m->palette,dirty,m};
    FreeMem(resource,8192);
    return m;
failed:
    if(resource) FreeMem(resource,8192);
    slicks_amiga_player_menu_destroy(m);
    return 0;
}
unsigned char g_slicks_diag_help_fail_allocation;
/* Explicit nested-Help fixture: fail after the viewer allocation succeeds. */
unsigned char g_slicks_diag_help_fail_backing,g_slicks_diag_help_backing_fault_reached;
struct SlicksAmigaPlayerMenu *slicks_amiga_race_surface_create(
    struct SlicksResourceArchive *archive,unsigned char *chunky,const unsigned char *palette)
{
    struct SlicksAmigaPlayerMenu *m=slicks_amiga_help_surface_create(archive,chunky,palette);
    if(!m) return 0;
    unsigned char *resource=surface_create_fault(5)?0:AllocMem(8192,MEMF_ANY);
    long size=resource?slicks_resource_archive_load(archive,surface_create_fault(6)?"missing-surface-font":"pieni.@f",resource,8192):-1;
    if(surface_create_fault(7)) size=0;
    int failed=size<0 || slicks_decode_font_resource(resource,(unsigned long)size,m->fonts[1],sizeof m->fonts[1])<0;
    if(resource) FreeMem(resource,8192);
    if(failed) { slicks_amiga_player_menu_destroy(m); return 0; }
    m->renderer.fonts[1]=m->fonts[1]; m->renderer.text=text; m->renderer.icon=icon; m->renderer.context=m;
    return m;
}
struct SlicksAmigaPlayerMenu *slicks_amiga_intermission_surface_create(
    struct SlicksResourceArchive *archive,unsigned char *chunky,const unsigned char *palette)
{
    struct SlicksAmigaPlayerMenu *m=slicks_amiga_race_surface_create(archive,chunky,palette);
    if(!m) return 0;
    /* Original /PartII sequential resource 27, loaded at 19fec -> DS:4c30.
     * Dedicated owner: records and Players retain their own slot-zero icon. */
    if(load_car_menu_icons(m,archive,"clock.@16",0)) {
        slicks_amiga_player_menu_destroy(m); return 0;
    }
    return m;
}
static int help_open_backing(struct SlicksAmigaPlayerMenu *m,struct SlicksResourceArchive *archive,
    const unsigned char *topic,unsigned char borrow)
{
    if(!m || m->help || !archive || !topic || prepare_keymap(m)) return -1;
    struct SlicksHelpViewer *v=g_slicks_diag_help_fail_allocation?0:AllocMem(sizeof *v,MEMF_ANY|MEMF_CLEAR);
    g_slicks_diag_help_fail_allocation=0;
    if(!v) return -1;
    unsigned char *saved;
    if(!borrow && g_slicks_diag_help_fail_backing) {
        g_slicks_diag_help_fail_backing=0;
        ++g_slicks_diag_help_backing_fault_reached;
        saved=0;
    } else saved=borrow?m->saved:AllocMem(64000,MEMF_ANY);
    if(!saved) { FreeMem(v,sizeof *v); return -1; }
    long size=slicks_resource_archive_load(archive,"HELP.TXT",v->source,sizeof v->source);
    if(size<0 || slicks_amiga_help_renderer_init(m,&v->renderer)) goto failed;
    v->source_size=(unsigned)size;
    if(slicks_help_viewer_open(v,topic,0,saved,64000)) {
        if(v->renderer.active) slicks_help_renderer_close(&v->renderer);
        goto failed;
    }
    m->help=v; return 0;
failed:
    if(!borrow) FreeMem(saved,64000);
    FreeMem(v,sizeof *v); return -1;
}
int slicks_amiga_help_open(struct SlicksAmigaPlayerMenu *m,struct SlicksResourceArchive *archive,const unsigned char *topic)
{ return help_open_backing(m,archive,topic,0); }
/* Only a fresh title Help surface: its parent save-under has no owner.
 * Nested Help (Players, Tracks, shop, pause, etc.) must retain separate storage. */
int slicks_amiga_title_help_open(struct SlicksAmigaPlayerMenu *m,struct SlicksResourceArchive *archive,const unsigned char *topic)
{ return help_open_backing(m,archive,topic,1); }
int slicks_amiga_help_close(struct SlicksAmigaPlayerMenu *m)
{
    if(!m || !m->help) return -1;
    int result=slicks_help_renderer_close(&m->help->renderer);
    unsigned char *saved=m->help->renderer.saved.pixels;
    if(saved && saved!=m->saved) FreeMem(saved,64000);
    FreeMem(m->help,sizeof *m->help); m->help=0; return result;
}
static short picker_measure(void *context,const unsigned char *font,const unsigned char *s)
{
    (void)context; short width=slicks_menu_measure(font,s);
    /* Keep a normal call: elf2hunk cannot relocate GCC's cross-section
     * PC32 sibling branch to the assembly font bridge. */
    __asm volatile("" : "+d"(width) :: "memory");
    return width;
}
/* Only one modal can own this reserve. Never allocate while reporting a
 * failed child allocation, and never treat the warning as a delete prompt.
 * The legacy help_warning ownership field is shared by these warnings. */
static struct SlicksAmigaMessageDialog help_warning;
static struct SlicksAmigaPlayerMenu *help_warning_owner;
static void emergency_text(void *context,struct SlicksChunkyUi *ui,unsigned char *font,
    const unsigned char *message,short x,short y,unsigned char flags)
{
    (void)context;
    slicks_menu_text(ui->pixels,font,message,x,y,flags);
    /* Keep a call: elf2hunk cannot relocate a PC32 sibling branch from C
     * into the separately assembled font bridge. */
    __asm__ volatile("" ::: "memory");
}
int slicks_amiga_emergency_warning_open(unsigned char *chunky,const unsigned char *palette,
    unsigned char *font,const unsigned char *message)
{
    if(help_warning_owner || help_warning.renderer.active || !chunky || !palette || !font) return -1;
    struct SlicksMessageDialog *d=&help_warning.renderer;
    d->painter.ui=(struct SlicksChunkyUi){chunky,palette,0,0};
    d->painter.font=font; d->painter.measure=picker_measure;
    d->painter.text=emergency_text; d->painter.context=0;
    int result=slicks_message_dialog_open(d,message,160,100,66,help_warning.saved,sizeof help_warning.saved);
    if(result && d->active) (void)slicks_message_dialog_close(d);
    return result;
}
int slicks_amiga_emergency_warning_close(void)
{
    if(help_warning_owner || !help_warning.renderer.active) return -1;
    return slicks_message_dialog_close(&help_warning.renderer);
}
int slicks_amiga_emergency_warning_bounds(struct SlicksMenuRect *bounds)
{
    const struct SlicksMessageDialog *d=&help_warning.renderer;
    if(!bounds || help_warning_owner || !d->active) return -1;
    *bounds=(struct SlicksMenuRect){d->left,d->top,
        (short)(d->left+d->original.width),(short)(d->top+d->original.height)};
    return 0;
}
int slicks_amiga_warning_open(struct SlicksAmigaPlayerMenu *m,const unsigned char *message)
{
    if(!m || !message || help_warning_owner || help_warning.renderer.active || m->help || m->help_warning) return -1;
    struct SlicksMessageDialog *d=&help_warning.renderer;
    d->painter.ui=m->renderer.ui; d->painter.font=m->fonts[0];
    d->painter.measure=picker_measure; d->painter.text=text; d->painter.context=m;
    m->error=0;
    if(slicks_message_dialog_open(d,message,
        160,100,66,help_warning.saved,sizeof help_warning.saved) || m->error) {
        if(d->active) slicks_message_dialog_close(d);
        return -1;
    }
    m->help_warning=1; help_warning_owner=m; return 0;
}
int slicks_amiga_help_warning_open(struct SlicksAmigaPlayerMenu *m)
{ return slicks_amiga_warning_open(m,(const unsigned char *)"HELP UNAVAILABLE - PRESS A KEY"); }
int slicks_amiga_help_warning_close(struct SlicksAmigaPlayerMenu *m)
{
    if(!m || help_warning_owner!=m || !m->help_warning) return -1;
    if(slicks_message_dialog_close(&help_warning.renderer)) return -1;
    m->help_warning=0; help_warning_owner=0; return 0;
}
int slicks_amiga_message_open_font(struct SlicksAmigaPlayerMenu *m,const unsigned char *message,
    unsigned char percent,unsigned font)
{
    if(!m || m->message || !message || font>=3) return -1;
    struct SlicksAmigaMessageDialog *d=AllocMem(sizeof *d,MEMF_ANY|MEMF_CLEAR);
    if(!d) return -1;
    d->renderer.painter.ui=m->renderer.ui; d->renderer.painter.font=m->fonts[font];
    d->renderer.painter.measure=picker_measure; d->renderer.painter.text=text; d->renderer.painter.context=m;
    m->error=0;
    if(slicks_message_dialog_open(&d->renderer,message,160,100,percent,d->saved,sizeof d->saved) || m->error) {
        if(d->renderer.active) slicks_message_dialog_close(&d->renderer);
        FreeMem(d,sizeof *d); return -1;
    }
    m->message=d; return 0;
}
int slicks_amiga_message_open(struct SlicksAmigaPlayerMenu *m,const unsigned char *message,unsigned char percent)
{ return slicks_amiga_message_open_font(m,message,percent,0); }
int slicks_amiga_message_close(struct SlicksAmigaPlayerMenu *m)
{
    if(!m || !m->message) return -1;
    int result=slicks_message_dialog_close(&m->message->renderer);
    FreeMem(m->message,sizeof *m->message); m->message=0; return result;
}
/* Diagnostic: 1 fails allocation, 2 fails after the dialog has painted. */
unsigned char g_slicks_diag_profile_dialog_fault;
static int name_dialog_open_field(struct SlicksAmigaPlayerMenu *m,unsigned char *name,
    const unsigned char *caption,short x,short y,unsigned char percent,unsigned limit,unsigned flags)
{
    if(!m || !name || m->name_dialog) return -1;
    unsigned char fault=g_slicks_diag_profile_dialog_fault;
    g_slicks_diag_profile_dialog_fault=0;
    struct SlicksAmigaNameDialog *d=fault==1?0:AllocMem(sizeof *d,MEMF_ANY|MEMF_CLEAR);
    if(!d) return -1;
    d->renderer.painter.ui=m->renderer.ui; d->renderer.painter.font=m->fonts[0];
    d->renderer.painter.measure=picker_measure; d->renderer.painter.text=text; d->renderer.painter.context=m;
    m->error=0;
    if(slicks_name_dialog_open_field(&d->renderer,name,caption,x,y,percent,limit,flags,
        d->original,sizeof d->original,d->field,sizeof d->field,d->cursor,sizeof d->cursor) || m->error || fault==2) {
        if(d->renderer.active) slicks_name_dialog_close(&d->renderer);
        FreeMem(d,sizeof *d); return -1;
    }
    m->name_dialog=d; return 0;
}
int slicks_amiga_name_dialog_open_at(struct SlicksAmigaPlayerMenu *m,unsigned char name[21],
    const unsigned char *caption,short x,short y,unsigned char percent)
{ return name_dialog_open_field(m,name,caption,x,y,percent,20,0x203); }
int slicks_amiga_saved_filename_open(struct SlicksAmigaPlayerMenu *m,unsigned char name[9],
    const unsigned char *caption,unsigned char percent)
{
    if(!m || m->picker || prepare_keymap(m)) return -1;
    return name_dialog_open_field(m,name,caption,100,65,percent,8,0x1b);
}
int slicks_amiga_name_dialog_open(struct SlicksAmigaPlayerMenu *m,const unsigned char *caption,unsigned char percent)
{
    if(!m || !m->editor_active) return -1;
    return slicks_amiga_name_dialog_open_at(m,m->editor_name,caption,170,85,percent);
}
int slicks_amiga_name_dialog_close(struct SlicksAmigaPlayerMenu *m)
{
    if(!m || !m->name_dialog) return -1;
    int result=slicks_name_dialog_close(&m->name_dialog->renderer);
    FreeMem(m->name_dialog,sizeof *m->name_dialog); m->name_dialog=0;
    if(m->editor_active) { m->editor_pending=0; m->editor.redraw=255; }
    return result;
}
int slicks_amiga_name_dialog_tick(struct SlicksAmigaPlayerMenu *m,unsigned long vblank)
{
    if(!m || !m->name_dialog) return -1;
    struct SlicksAmigaNameDialog *d=m->name_dialog;
    if(slicks_name_dialog_cursor(&d->renderer,!d->hidden)) return -1;
    /* Original cursor toggles once when the clock/200ms bucket changes. */
    unsigned long bucket=vblank/10;
    if(bucket!=d->blink_bucket) { d->blink_bucket=bucket; d->hidden^=1; }
    return 0;
}
int slicks_amiga_colour_dialog_open(struct SlicksAmigaPlayerMenu *m,struct SlicksPlayerProfiles *profiles,
    const unsigned char *caption)
{
    slicks_amiga_platform_clear_latch(0);
    if(!m || !profiles || !m->editor_active || m->colour_dialog || m->name_dialog ||
       m->editor.row<3 || m->editor.row>4 || m->editor_index<0 || m->editor_index>=SLICKS_PROFILE_MAX) return -1;
    unsigned char fault=g_slicks_diag_profile_dialog_fault;
    g_slicks_diag_profile_dialog_fault=0;
    struct SlicksAmigaColourDialog *d=fault==1?0:AllocMem(sizeof *d,MEMF_ANY|MEMF_CLEAR);
    if(!d) return -1;
    d->renderer.painter.ui=m->renderer.ui; d->renderer.painter.font=m->fonts[0];
    d->renderer.painter.text=text; d->renderer.painter.context=m;
    m->error=0;
    if(slicks_colour_dialog_open(&d->renderer,profiles->setup[m->editor_index].colours+3*(m->editor.row-3),
        caption,160,(short)(100+15*m->editor.row),d->saved,sizeof d->saved) || m->error || fault==2) {
        if(d->renderer.active) {
            d->renderer.state.result=-1;
            (void)slicks_colour_dialog_close(&d->renderer);
        }
        FreeMem(d,sizeof *d); return -1;
    }
    m->colour_dialog=d; return 0;
}
int slicks_amiga_colour_dialog_draw(struct SlicksAmigaPlayerMenu *m,unsigned long tick)
{
    if(!m || !m->colour_dialog) return -1;
    return slicks_colour_dialog_draw(&m->colour_dialog->renderer,tick);
}
int slicks_amiga_colour_dialog_close(struct SlicksAmigaPlayerMenu *m)
{
    slicks_amiga_platform_clear_latch(0);
    if(!m || !m->colour_dialog) return -1;
    int result=slicks_colour_dialog_close(&m->colour_dialog->renderer);
    FreeMem(m->colour_dialog,sizeof *m->colour_dialog); m->colour_dialog=0;
    m->editor_pending=0; m->editor.redraw=255;
    return result;
}
int slicks_amiga_profile_picker_open(struct SlicksAmigaPlayerMenu *m,unsigned row,short selected,
    const struct SlicksPlayerProfiles *profiles,const unsigned char *caption,unsigned char percent)
{
    slicks_amiga_platform_clear_latch(0);
    if(!m || m->picker || !profiles || (row>=4 && row!=5 && row!=6) ||
       (row<4 && (selected<0 || selected>=SLICKS_PROFILE_MAX))) return -1;
    struct SlicksAmigaProfilePicker *p=AllocMem(sizeof *p,MEMF_ANY|MEMF_CLEAR);
    if(!p) return -1;
    struct SlicksListRenderer *r=&p->renderer;
    r->ui=m->renderer.ui; r->font=m->fonts[0]; r->names=&profiles->names[0][0]; r->stride=21;
    r->left=160; r->top=(short)(30+16*row); r->right=310; r->bottom=r->top+100;
    if(row>=5) {
        r->left=120; r->top=90; r->right=270; r->bottom=190;
        selected=(short)(4-(short)row);
    }
    r->measure=picker_measure; r->text=text; r->context=m;
    m->error=0;
    if(slicks_list_renderer_open(r,selected,profiles->count,caption,percent,row<4?1:0,
        p->original,sizeof p->original,p->tinted,sizeof p->tinted,p->caption,sizeof p->caption) || m->error) {
        FreeMem(p,sizeof *p); return -1;
    }
    m->picker=p; return 0;
}
short slicks_amiga_profile_picker_close(struct SlicksAmigaPlayerMenu *m)
{
    slicks_amiga_platform_clear_latch(0);
    if(!m || !m->picker) return -1;
    short result=slicks_list_renderer_close(&m->picker->renderer);
    if(m->picker->scrollbar_saved.pixels)
        (void)slicks_list_restore_scrollbar(&m->picker->renderer,&m->picker->scrollbar_saved);
    free_picker(m->picker); m->picker=0;
    return result;
}
int slicks_amiga_saved_files_picker(struct SlicksAmigaPlayerMenu *m,const unsigned char names[][9],
    unsigned count,unsigned char saving,const unsigned char *captions,unsigned char percent)
{
    slicks_amiga_platform_clear_latch(0);
    /* Original file enumeration caps at 40, with nine-byte name records. */
    if(!m || m->picker || m->name_dialog || count>40 || (count && !names) || !captions || prepare_keymap(m)) return -1;
    for(unsigned i=0;i<count;++i) {
        unsigned length=0; while(length<9 && names[i][length]) ++length;
        if(length==9) return -1;
    }
    struct SlicksAmigaProfilePicker *p=AllocMem(sizeof *p,MEMF_ANY|MEMF_CLEAR);
    if(!p) return -1;
    p->owned_names_size=9UL*(count?count:1);
    p->owned_names=AllocMem(p->owned_names_size,MEMF_ANY|MEMF_CLEAR);
    if(!p->owned_names) { free_picker(p); return -1; }
    for(unsigned i=0;i<count;++i) for(unsigned j=0;j<9;++j) p->owned_names[9*i+j]=names[i][j];
    struct SlicksListRenderer *r=&p->renderer;
    r->ui=m->renderer.ui; r->font=m->fonts[0]; r->names=p->owned_names; r->stride=9;
    r->left=180; r->top=50; r->right=260; r->bottom=150;
    r->measure=picker_measure; r->text=text; r->context=m; m->error=0;
    if(slicks_list_save_scrollbar(r,&p->scrollbar_saved,p->scrollbar_background,sizeof p->scrollbar_background)) {
        free_picker(p); return -1;
    }
    if(slicks_list_renderer_open(r,saving?-2:0,(short)count,captions,percent,0,
        p->original,sizeof p->original,p->tinted,sizeof p->tinted,p->caption,sizeof p->caption) || m->error) {
        if(r->active) slicks_list_renderer_close(r);
        (void)slicks_list_restore_scrollbar(r,&p->scrollbar_saved);
        free_picker(p); return -1;
    }
    m->picker=p; return 0;
}
int slicks_amiga_track_lists_picker(struct SlicksAmigaPlayerMenu *m,
    const unsigned char *captions,unsigned char percent)
{
    slicks_amiga_platform_clear_latch(0);
    if(!m || !m->track_lists || m->picker || !captions ||
       m->track_lists->catalogue.count>(SLICKS_AMIGA_TRACK_LIST_BYTES-8)/23) return -1;
    struct SlicksAmigaTrackLists *lists=m->track_lists;
    struct SlicksAmigaProfilePicker *p=list_alloc_fault(1)?0:AllocMem(sizeof *p,MEMF_ANY|MEMF_CLEAR);
    if(!p) return -2;
    /* The catalogue stays resident and immutable throughout this modal.
     * Index its validated titles instead of duplicating up to 59,829 bytes. */
    p->owned_names_size=2UL*(lists->catalogue.count?lists->catalogue.count:1);
    p->owned_names=list_alloc_fault(2)?0:AllocMem(p->owned_names_size,MEMF_ANY|MEMF_CLEAR);
    if(!p->owned_names) { free_picker(p); return -2; }
    /* The loader validated every record. Walk once rather than repeatedly
     * searching from the start for each title in a large catalogue. */
    const unsigned char *entry=lists->catalogue.bytes+8;
    for(unsigned i=0;i<lists->catalogue.count;++i) {
        ((unsigned short *)p->owned_names)[i]=(unsigned short)(entry+2-lists->catalogue.bytes);
        entry+=23UL+8UL*slicks_track_list_word(entry);
    }
    struct SlicksListRenderer *r=&p->renderer;
    r->ui=m->renderer.ui; r->font=m->fonts[0]; r->names=lists->catalogue.bytes; r->stride=21;
    r->name_offsets=(const unsigned short *)p->owned_names;
    /* Original 2d774..2d7af, called with (150,20). */
    r->left=150; r->top=20; r->right=280; r->bottom=90;
    r->measure=picker_measure; r->text=text; r->context=m; m->error=0;
    if(slicks_list_save_scrollbar(r,&p->scrollbar_saved,p->scrollbar_background,sizeof p->scrollbar_background)) {
        free_picker(p); return -1;
    }
    if(slicks_list_renderer_open(r,0,(short)lists->catalogue.count,captions,percent,0,
        p->original,sizeof p->original,p->tinted,sizeof p->tinted,p->caption,sizeof p->caption) || m->error) {
        if(r->active) slicks_list_renderer_close(r);
        (void)slicks_list_restore_scrollbar(r,&p->scrollbar_saved);
        free_picker(p); return -1;
    }
    m->picker=p; return 0;
}
void slicks_amiga_track_lists_close(struct SlicksAmigaPlayerMenu *m)
{
    if(!m || !m->track_lists) return;
    if(m->message) (void)slicks_amiga_message_close(m);
    if(m->name_dialog) (void)slicks_amiga_name_dialog_close(m);
    if(m->picker) (void)slicks_amiga_profile_picker_close(m);
    FreeMem(m->track_lists,sizeof *m->track_lists); m->track_lists=0;
}
struct SlicksSetupLoadReport slicks_amiga_track_lists_open(struct SlicksAmigaPlayerMenu *m,
    const struct SlicksAmigaTrackListCache *cache,const unsigned char *captions,unsigned char percent)
{
    struct SlicksSetupLoadReport report={SLICKS_SETUP_LOAD_INVALID,0,"SLICKS.TRK",0,0};
    if(!m || m->track_lists || m->picker || !captions || prepare_keymap(m)) return report;
    if(!cache) return report;
    report=cache->report;
    if(report.result!=SLICKS_SETUP_LOADED) return report;
    struct SlicksAmigaTrackLists *lists=AllocMem(sizeof *lists,MEMF_ANY|MEMF_CLEAR);
    if(!lists) { report.result=SLICKS_SETUP_LOAD_IO_ERROR; report.io_error=ERROR_NO_FREE_STORE; return report; }
    m->track_lists=lists;
    lists->catalogue=cache->view; /* Immutable borrow until modal close. */
    int picker_result=slicks_amiga_track_lists_picker(m,captions,percent);
    if(picker_result) {
        report.result=picker_result==-2?SLICKS_SETUP_LOAD_IO_ERROR:SLICKS_SETUP_LOAD_INVALID;
        report.io_error=picker_result==-2?ERROR_NO_FREE_STORE:0;
        report.path="SLICKS.TRK"; goto failed;
    }
    return report;
failed:
    slicks_amiga_track_lists_close(m); return report;
}
struct SlicksTrackListChoice slicks_amiga_track_lists_choice(struct SlicksAmigaPlayerMenu *m,short selected_tracks)
{
    if(!m || !m->track_lists || !m->picker)
        return (struct SlicksTrackListChoice){SLICKS_TRACK_LIST_EXIT,-1};
    short result=slicks_amiga_profile_picker_close(m);
    return slicks_track_list_choice(result,(short)m->track_lists->catalogue.count,selected_tracks);
}
int slicks_amiga_profile_picker_draw(struct SlicksAmigaPlayerMenu *m,unsigned long tick)
{
    if(!m || !m->picker) return -1;
    m->error=0; slicks_list_renderer_draw(&m->picker->renderer,tick); return m->error;
}
int slicks_amiga_profile_delete_prompt(struct SlicksAmigaPlayerMenu *m,short index,
    const struct SlicksPlayerProfiles *profiles,const unsigned char *question,unsigned char percent)
{
    if(!m || !profiles || index<=0 || index>=profiles->count || m->picker || m->delete_pending) return -1;
    struct SlicksListRenderer r={0};
    r.ui=m->renderer.ui; r.font=m->fonts[0]; r.text=text; r.context=m;
    struct SlicksListCaptionOps ops={slicks_list_measure,slicks_list_colour,slicks_list_nearest,slicks_list_text,&r};
    r.measure=picker_measure; m->error=0;
    m->delete_old_colour=slicks_profile_delete_prompt(&r.ui,profiles->names[index],question,percent,&ops);
    m->delete_index=index; m->delete_pending=1;
    return m->error;
}
void slicks_amiga_player_menu_restore(struct SlicksAmigaPlayerMenu *m)
{
    /* Same prepared background as original 28ce4. Track all writes since
     * that snapshot, not just the last publication or selected row. */
    if(m->delete_pending) m->fonts[0][6]=m->delete_old_colour;
    m->delete_pending=0;
    if(m->track_saved_dirty) {
        m->track_saved_dirty=0;
        for(unsigned i=0;i<m->saved_dirty_count;++i) {
            const struct SlicksMenuRect *r=&m->saved_dirty[i];
            for(unsigned y=r->top;y<r->bottom;++y)
                for(unsigned x=r->left;x<r->right;++x)
                    m->renderer.ui.pixels[mult320[y]+x]=m->saved[mult320[y]+x];
            dirty(m,r->left,r->top,r->right,r->bottom);
        }
        m->saved_dirty_count=0; m->track_saved_dirty=1;
    } else {
        for(unsigned i=0;i<64000;++i) m->renderer.ui.pixels[i]=m->saved[i];
        dirty(m,0,0,320,200);
    }
}
int slicks_amiga_profile_editor_open(struct SlicksAmigaPlayerMenu *m,struct SlicksPlayerProfiles *profiles,
    short index,unsigned char is_new,unsigned long *random_state)
{
    if(!m || !profiles || m->picker || m->delete_pending || m->editor_active ||
       index<0 || index>=SLICKS_PROFILE_MAX || (is_new?index!=profiles->count:index>=profiles->count)) return -1;
    if(is_new) {
        if(slicks_begin_new_profile(profiles,(unsigned)index,random_state)) return -1;
        m->editor_name[0]=0;
    } else {
        unsigned n=0; while(n<21 && profiles->names[index][n]) ++n;
        if(n==21) return -1;
        for(unsigned i=0;i<=n;++i) m->editor_name[i]=profiles->names[index][i];
    }
    m->editor=(struct SlicksProfileEditor){0,1,0}; m->editor_index=index;
    m->editor_active=1; m->editor_new=is_new; m->editor_pending=0;
    m->editor_old_colour=m->fonts[0][6]; m->fonts[0][6]=1;
    return 0;
}
int slicks_amiga_profile_editor_draw(struct SlicksAmigaPlayerMenu *m,struct SlicksPlayerProfiles *profiles,
    const struct SlicksProfileEditorLabels *labels,unsigned char field_01a6)
{
    if(!m || !m->editor_active) return -1;
    m->error=0;
    slicks_profile_editor_limits(profiles,(unsigned)m->editor_index,field_01a6);
    if(slicks_profile_editor_renderer_draw(&m->renderer,&m->editor,profiles,(unsigned)m->editor_index,
        m->editor_name,10,labels)) return -1;
    return m->error;
}
int slicks_amiga_profile_editor_close(struct SlicksAmigaPlayerMenu *m,struct SlicksPlayerProfiles *profiles)
{
    if(!m || !m->editor_active || !m->editor.result) return -1;
    int result=slicks_finish_profile_edit(profiles,(unsigned)m->editor_index,m->editor_new,
        m->editor.result,m->editor_name,sizeof m->editor_name);
    if(result<0) return -1;
    if(!result && m->editor_new) ++profiles->count;
    m->fonts[0][6]=m->editor_old_colour;
    m->editor_active=0; m->editor_pending=0;
    slicks_amiga_player_menu_restore(m);
    return result;
}
struct SlicksAmigaPlayerMenu *slicks_amiga_player_menu_create(
    struct SlicksResourceArchive *archive,unsigned char *chunky,const unsigned char *title,
    const unsigned char *footer,unsigned char footer_percent,const struct SlicksPlayerMenuLabels *labels)
{
    if(!archive || !chunky || !title || !footer || !labels) return 0;
    struct SlicksAmigaPlayerMenu *m=AllocMem(sizeof *m,MEMF_ANY|MEMF_CLEAR);
    unsigned char *resource=AllocMem(32768,MEMF_ANY);
    if(!m || !resource || prepare_keymap(m)) goto failed;
    unsigned width,height; unsigned long consumed;
    long size=slicks_resource_archive_load(archive,"players.bmp",resource,32768);
    if(size<0 || slicks_decode_menu_bitmap(resource,(unsigned long)size,chunky,m->palette,&width,&height,&consumed) || width!=320 || height!=200) goto failed;
    const char *fonts[]={"kirj.@f","pieni.@f","iso.@f"};
    for(unsigned i=0;i<3;++i) {
        size=slicks_resource_archive_load(archive,fonts[i],resource,32768);
        if(size<0 || slicks_decode_font_resource(resource,(unsigned long)size,m->fonts[i],sizeof m->fonts[i])<0) goto failed;
        m->renderer.fonts[i]=m->fonts[i];
    }
    for(unsigned i=0;i<11;++i) {
        char name[]="auto01.@16"; name[5]=(char)('0'+i-1);
        size=slicks_resource_archive_load(archive,i==0?"computer.@16":i==1?"carimage16":name,resource,32768);
        if(size<0 || slicks_decode_menu_icon(resource,(unsigned long)size,m->palette,m->pixels[i],sizeof m->pixels[i],&m->icons[i].width,&m->icons[i].height)) goto failed;
        m->icons[i].pixels=m->pixels[i];
    }
    FreeMem(resource,32768); resource=0;
    m->labels=*labels;
    m->renderer.ui=(struct SlicksChunkyUi){chunky,m->palette,dirty,m};
    m->renderer.saved=m->saved; m->renderer.icons=m->icons; m->renderer.icon_count=11;
    m->renderer.text=text; m->renderer.icon=icon; m->renderer.context=m;
    if(slicks_player_renderer_prepare(&m->renderer,title,footer,footer_percent) || m->error) goto failed;
    m->saved_dirty_count=0; m->track_saved_dirty=1;
    m->dirty_count=1; m->dirty[0]=(struct SlicksMenuRect){0,0,320,200};
    return m;
failed:
    if(resource) FreeMem(resource,32768);
    slicks_amiga_player_menu_destroy(m);
    return 0;
}
static short options_measure_bridge(const unsigned char *font,const unsigned char *string)
{
    short width=slicks_menu_measure(font,string);
    __asm volatile("" : "+d"(width) :: "memory");
    return width;
}
struct SlicksAmigaPlayerMenu *slicks_amiga_options_menu_create(
    struct SlicksResourceArchive *archive,unsigned char *chunky,const unsigned char *palette,
    const unsigned char *title,struct SlicksOptionsRenderer *renderer)
{
    if(!archive || !chunky || !palette || !title || !renderer) return 0;
    struct SlicksAmigaPlayerMenu *m=AllocMem(sizeof *m,MEMF_ANY|MEMF_CLEAR);
    unsigned char *resource=AllocMem(8192,MEMF_ANY);
    if(!m || !resource) goto failed;
    for(unsigned i=0;i<768;++i) m->palette[i]=palette[i];
    const char *names[]={"kirj.@f","pieni.@f"};
    for(unsigned i=0;i<2;++i) {
        long size=slicks_resource_archive_load(archive,names[i],resource,8192);
        if(size<0 || slicks_decode_font_resource(resource,(unsigned long)size,
            m->fonts[i],sizeof m->fonts[i])<0) goto failed;
        m->renderer.fonts[i]=m->fonts[i];
    }
    m->renderer.ui=(struct SlicksChunkyUi){chunky,m->palette,dirty,m};
    m->renderer.saved=m->saved; m->renderer.text=text; m->renderer.icon=icon; m->renderer.context=m;
    if(slicks_options_renderer_init(renderer,&m->renderer,options_measure_bridge) ||
       slicks_options_renderer_prepare(renderer,title) || m->error) goto failed;
    FreeMem(resource,8192);
    m->dirty_count=1; m->dirty[0]=(struct SlicksMenuRect){0,0,320,200};
    return m;
failed:
    if(resource) FreeMem(resource,8192);
    slicks_amiga_player_menu_destroy(m);
    renderer->surface=0;
    return 0;
}
struct SlicksAmigaPlayerMenu *slicks_amiga_track_menu_create(
    struct SlicksResourceArchive *archive,unsigned char *chunky,const unsigned char *title,
    const unsigned char *footer,short total,unsigned char percent,struct SlicksTrackRenderer *renderer,
    const unsigned char *(*name)(void *,unsigned),void *name_context)
{
    if(!renderer) return 0;
    renderer->surface=0;
    if(!archive || !chunky || !title || !footer || total<0 || !name) return 0;
    struct SlicksAmigaPlayerMenu *m=AllocMem(sizeof *m,MEMF_ANY|MEMF_CLEAR);
    unsigned char *resource=AllocMem(64003,MEMF_ANY);
    if(!m || !resource) goto failed;
    long size=slicks_resource_archive_load(archive,"trckmenu.@p",m->palette,sizeof m->palette);
    if(size!=768) goto failed;
    unsigned short width,height;
    size=slicks_resource_archive_load(archive,"trckmenu.@I",resource,64003);
    if(size<0 || slicks_decode_indexed_menu_icon(resource,(unsigned long)size,
        chunky,64000,&width,&height) || width!=320 || height!=200) goto failed;
    const char *fonts[]={"kirj.@f","pieni.@f","iso.@f"};
    for(unsigned i=0;i<3;++i) {
        size=slicks_resource_archive_load(archive,fonts[i],resource,64003);
        if(size<0 || slicks_decode_font_resource(resource,(unsigned long)size,
            m->fonts[i],sizeof m->fonts[i])<0) goto failed;
        m->renderer.fonts[i]=m->fonts[i];
    }
    m->renderer.ui=(struct SlicksChunkyUi){chunky,m->palette,dirty,m};
    m->renderer.saved=m->saved; m->renderer.text=text; m->renderer.context=m;
    if(slicks_track_renderer_init(renderer,&m->renderer,percent,name,name_context) ||
       slicks_track_renderer_prepare(renderer,title,footer,total,percent) || m->error) goto failed;
    FreeMem(resource,64003);
    m->dirty_count=1; m->dirty[0]=(struct SlicksMenuRect){0,0,320,200};
    return m;
failed:
    if(resource) FreeMem(resource,64003);
    slicks_amiga_player_menu_destroy(m); renderer->surface=0;
    return 0;
}
int slicks_amiga_player_menu_draw(struct SlicksAmigaPlayerMenu *m,unsigned row,
    const short selected[4],const signed char participation[4],const struct SlicksPlayerProfiles *profiles)
{
    if(!m) return -1;
    m->error=0;
    if(slicks_player_renderer_draw(&m->renderer,row,selected,participation,profiles,10,&m->labels)) return -1;
    return m->error;
}
