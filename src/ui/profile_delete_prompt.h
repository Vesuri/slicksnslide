#ifndef SLICKS_PROFILE_DELETE_PROMPT_H
#define SLICKS_PROFILE_DELETE_PROMPT_H
#include "palette_remap.h"
#include "list_captions.h"

/* Original 28b48..28c21. No new save-under: the player-menu caller restores
 * its existing saved background after confirmation. Return the old font
 * colour for the common 28cbd exit path, including cancellation. */
static inline unsigned char slicks_profile_delete_prompt(struct SlicksChunkyUi *ui,
    const unsigned char *name,const unsigned char *question,unsigned char percent,
    const struct SlicksListCaptionOps *ops)
{
    unsigned char table[256];
    slicks_ui_tint_table(ui->palette,table,55,55,55,percent);
    slicks_ui_remap(ui,90,80,230,110,table);
    unsigned char previous=ops->colour(ops->context,ops->nearest(ops->context,0,0,20));
    ops->text(ops->context,name,160,84,1);
    ops->text(ops->context,question,160,94,1);
    return previous;
}
#endif
