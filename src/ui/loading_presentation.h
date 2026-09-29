#ifndef SLICKS_LOADING_PRESENTATION_H
#define SLICKS_LOADING_PRESENTATION_H
#include "demo_overlay.h"

/* Original 1b51c..1b53d concatenates the supplied track stem and DS:099c.
 * Reject overflow atomically at the native boundary, rather than reproducing
 * the DOS stack-buffer overwrite for an invalid filename. */
static inline int slicks_loading_caption(unsigned char *out,unsigned capacity,
    const unsigned char *track,const unsigned char *suffix)
{
    if(!out || !track || !suffix || !capacity) return -1;
    unsigned a=0,b=0;
    while(a<capacity && track[a]) ++a;
    if(a==capacity) return -1;
    while(b<capacity-a && suffix[b]) ++b;
    if(b==capacity-a) return -1;
    for(unsigned i=0;i<a;++i) out[i]=track[i];
    for(unsigned i=0;i<=b;++i) out[a+i]=suffix[i];
    return 0;
}

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
