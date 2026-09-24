#ifndef SLICKS_TRACK_INFO_RENDERER_H
#define SLICKS_TRACK_INFO_RENDERER_H
#include "track_records_renderer.h"
#include "palette_remap.h"

/* Original 2682c..269e5: records, heading/description and preview surround.
 * The caller has restored the menu background and supplies loaded records.
 * Original 269e8 then draws the track preview at (245,20). */
static inline int slicks_track_info_render(struct SlicksRecordsRenderer *r,
    const struct SlicksTrackRecords *records,const unsigned char *name,
    const unsigned char *description,unsigned char percent,
    unsigned char separator,signed char date_order)
{
    unsigned char tint[256]; const signed char ranks[4]={0,0,0,0};
    if(!r || !name || !description) return -1;
    slicks_ui_tint_table(r->ui.palette,tint,10,10,40,50);
    if(slicks_ui_remap(&r->ui,75,67,307,160,tint) ||
       slicks_records_renderer_draw(r,records,ranks,80,40,separator,date_order)) return -1;
    slicks_ui_tint_table(r->ui.palette,tint,20,20,50,percent);
    if(slicks_ui_remap(&r->ui,90,20,270,52,tint)) return -1;
    r->fonts[0][6]=slicks_ui_nearest(&r->ui,60,50,60);
    if(r->text(r->context,&r->ui,r->fonts[0],name,170,29,1,0)) return -1;
    if(description[0]) {
        r->fonts[1][6]=slicks_ui_nearest(&r->ui,50,60,50);
        if(r->text(r->context,&r->ui,r->fonts[1],description,170,39,1,0)) return -1;
    }
    return slicks_ui_remap(&r->ui,240,15,315,65,tint);
}
#endif
