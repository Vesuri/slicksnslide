#ifndef SLICKS_DEMO_OVERLAY_H
#define SLICKS_DEMO_OVERLAY_H

struct SlicksDemoOverlayOps {
    unsigned char (*nearest)(void *,unsigned char,unsigned char,unsigned char);
    void (*colour)(void *,unsigned char,unsigned char);
    void (*text)(void *,const unsigned char *,short,short,unsigned char);
    void *context;
};

/* Original 1f84d..1f901, then 1fb3e. Return one when this replaces the
 * ordinary Arcade overlay. The owner supplies the original DS:0bff label
 * and font DS:0680, and its text callback reports glyph dirty bounds.
 * These are two explicit text passes, not the font bridge's shadow mode. */
static inline int slicks_demo_overlay(signed char flag,const unsigned char *label,
    const struct SlicksDemoOverlayOps *ops)
{
    if(flag>=0) return 0;
    void *p=ops->context;
    ops->colour(p,0,ops->nearest(p,0,0,0));
    ops->text(p,label,11,11,0);
    ops->colour(p,0,ops->nearest(p,50,50,50));
    ops->text(p,label,10,10,0);
    return 1;
}
#endif
