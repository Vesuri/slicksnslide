#ifndef SLICKS_RACE_MENU_RENDERER_H
#define SLICKS_RACE_MENU_RENDERER_H
#include "race_menu.h"
#include "track_records_renderer.h"
#include "palette_remap.h"
#include "saved_rectangle.h"

/* Original 1e481..1e53f and 1e54f..1e65c. Labels are resolved by the
 * caller through the original localization table, not the lookup keys.
 * This save-under contains the TINTED menu background; the race page
 * itself is separately owned/restored by the caller on menu close. */
struct SlicksRaceMenuRenderer {
    struct SlicksSavedRectangle background;
    unsigned char old_colour;
};
static inline int slicks_race_menu_render_open(struct SlicksRaceMenuRenderer *d,
    struct SlicksRecordsRenderer *r,const struct SlicksRaceMenu *m,
    unsigned char percent,unsigned char *storage,unsigned long capacity)
{
    if(!d || !r || !m || !r->ui.pixels || !r->ui.palette || !r->fonts[0] ||
       !r->text || !storage || !m->count || m->count>6 ||
       capacity<100UL*(m->count*10U+15U)) return -1;
    d->background=(struct SlicksSavedRectangle){0};
    d->old_colour=r->fonts[0][6];
    r->fonts[0][6]=slicks_ui_nearest(&r->ui,55,55,35);
    unsigned char tint[256]; slicks_ui_tint_table(r->ui.palette,tint,15,15,45,percent);
    if(slicks_ui_remap(&r->ui,40,40,140,(short)(55+m->count*10),tint)) return -1;
    return slicks_save_rectangle(&d->background,storage,capacity,&r->ui,
        40,40,100,(short)(m->count*10+15));
}
static inline int slicks_race_menu_render_draw(struct SlicksRaceMenuRenderer *d,
    struct SlicksRecordsRenderer *r,struct SlicksRaceMenu *m,
    const unsigned char *const labels[6])
{
    if(!d || !r || !m || !labels || !m->count || m->count>6) return -1;
    if(!m->redraw) return 0;
    for(unsigned row=0;row<m->count;++row) {
        if(m->redraw>=0 && (int)row!=m->redraw-1 && (int)row!=m->redraw) continue;
        if(!labels[row] || slicks_restore_rectangle(&r->ui,&d->background,
            40,40,0,(short)(row*10+8),100,10)) return -1;
        if(row==m->row) slicks_ui_bevel(&r->ui,45,(short)(48+row*10),90,9,50,10,10);
        if(r->text(r->context,&r->ui,r->fonts[0],labels[row],90,
            (short)(50+row*10),1,0)) return -1;
    }
    m->redraw=0; return 0;
}
static inline void slicks_race_menu_render_close(struct SlicksRaceMenuRenderer *d,
    struct SlicksRecordsRenderer *r)
{
    r->fonts[0][6]=d->old_colour;
}
#endif
