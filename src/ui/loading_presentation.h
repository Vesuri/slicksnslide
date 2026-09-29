#ifndef SLICKS_LOADING_PRESENTATION_H
#define SLICKS_LOADING_PRESENTATION_H
#include "demo_overlay.h"

struct SlicksLoadingPresentationOps {
    struct SlicksDemoOverlayOps font;
    void (*tint)(void *,short,short,short,short,
        unsigned char,unsigned char,unsigned char,short);
};

/* Original 1b488..1b58f. The caller supplies DS:0680, the live palette,
 * filename plus DS:099c suffix, and original DS:09a0 demo caption.
 * Positive flags suppress text, not the preceding tints/font mutation. */
static inline void slicks_loading_presentation(signed char flag,
    const unsigned char *filename_caption,const unsigned char *demo_caption,
    const struct SlicksLoadingPresentationOps *ops)
{
    void *p=ops->font.context;
    ops->tint(p,118,95,218,115,20,20,20,50);
    ops->tint(p,110,90,210,110,35,35,60,75);
    ops->font.colour(p,0,ops->font.nearest(p,60,60,40));
    if(flag<=0) ops->font.text(p,flag?demo_caption:filename_caption,160,97,1);
}
#endif
