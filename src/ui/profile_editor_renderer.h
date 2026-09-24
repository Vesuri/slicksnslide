#ifndef SLICKS_PROFILE_EDITOR_RENDERER_H
#define SLICKS_PROFILE_EDITOR_RENDERER_H
#include "profile_editor_draw.h"
#include "player_menu_renderer.h"

static inline void slicks_editor_renderer_colour(void *context,unsigned char colour)
{ struct SlicksPlayerMenuRenderer *r=context; r->fonts[0][6]=colour; }
static inline void slicks_editor_renderer_number(void *context,unsigned short value,short x,short y,unsigned char flags)
{
    /* Editor percentages are unsigned bytes; the original integer painter
     * delegates this same decimal string to the normal text routine. */
    unsigned char digits[4]; unsigned at=0;
    if(value>=100) digits[at++]=(unsigned char)('0'+value/100);
    if(value>=10) digits[at++]=(unsigned char)('0'+value/10%10);
    digits[at++]=(unsigned char)('0'+value%10); digits[at]=0;
    slicks_player_renderer_text(context,0,digits,x,y,flags);
}
static inline unsigned char slicks_editor_renderer_nearest(void *context,unsigned char red,unsigned char green,unsigned char blue)
{ struct SlicksPlayerMenuRenderer *r=context; return slicks_ui_nearest(&r->ui,red,green,blue); }
static inline void slicks_editor_renderer_rectangle(void *context,short left,short top,short right,short bottom,unsigned char colour)
{ struct SlicksPlayerMenuRenderer *r=context; slicks_ui_rectangle(&r->ui,left,top,right,bottom,colour); }
static inline int slicks_profile_editor_renderer_draw(struct SlicksPlayerMenuRenderer *r,
    struct SlicksProfileEditor *e,const struct SlicksPlayerProfiles *profiles,unsigned index,
    const unsigned char *name,short vehicle_count,const struct SlicksProfileEditorLabels *labels)
{
    if(!r || !r->fonts[0] || !r->saved || !r->ui.pixels || !r->ui.palette || !r->text ||
       !r->icon || !r->icons || vehicle_count<0 || (unsigned)vehicle_count+1>r->icon_count) return -1;
    r->error=0;
    struct SlicksProfileEditorDrawOps ops={{slicks_player_renderer_restore,slicks_player_renderer_bevel,
        slicks_player_renderer_sprite,slicks_player_renderer_text,r},slicks_editor_renderer_colour,
        slicks_editor_renderer_number,slicks_editor_renderer_nearest,slicks_editor_renderer_rectangle};
    if(slicks_draw_profile_editor(e,profiles,index,name,vehicle_count,
        slicks_ui_nearest(&r->ui,60,60,40),slicks_ui_nearest(&r->ui,50,50,30),labels,&ops)) return -1;
    return r->error;
}
#endif
