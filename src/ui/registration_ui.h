#ifndef SLICKS_REGISTRATION_UI_H
#define SLICKS_REGISTRATION_UI_H

static const unsigned char slicks_registration_help_topic[]="reg";
static inline int slicks_registration_help_requested(short scan)
{ return scan==21 || scan==59; }
#include "palette_remap.h"
/* Original 1987:c72f..c758: unsigned 16-bit calendar arithmetic, not a
 * Gregorian day interval. Preserve the original 40-unit trial comparison. */
static inline int slicks_registration_trial_expired(unsigned short today,
    short installed,unsigned char registered)
{ return !registered && today>(unsigned short)(installed+40); }
static inline const char *slicks_registration_exit_image(unsigned char registered)
{ return registered?"end2.bmp":"end1.bmp"; }

typedef void (*SlicksRegistrationText)(void *,const unsigned char *,short,short,unsigned char);
static inline void slicks_registration_trial_text(const unsigned char text[6][64],
    int prompt,SlicksRegistrationText draw,void *context)
{
    static const short xs[]={55,160,160,160,160,160},ys[]={81,105,113,124,132,145};
    for(unsigned i=prompt?5:0;i<(prompt?6:5);++i)
        draw(context,text[i],xs[i],ys[i],i?1:0);
}
/* Text strings come from the original executable's generated resources,
 * never from the user's key. Original 1987:c75b..c8ad. */
static inline void slicks_registration_trial_background(struct SlicksChunkyUi *ui)
{
    unsigned char tint[256];
    slicks_ui_tint_table(ui->palette,tint,10,10,40,75);
    (void)slicks_ui_remap(ui,50,83,270,155,tint);
}
#endif
