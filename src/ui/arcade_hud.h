#ifndef SLICKS_ARCADE_HUD_H
#define SLICKS_ARCADE_HUD_H
#include "../game/arcade_setup.h"

struct SlicksArcadeRect { short left,top,right,bottom; unsigned char grey; };
struct SlicksArcadeHud {
    struct SlicksArcadeRect rects[2];
    unsigned char count,text; /* text: 0 none, 1 LAST, 2 LAP; centred (70,189). */
};

/* Original 1f901..1fb46. Deadline/current are DS:6862/685e, separate from
 * the game-clock read used by 198c9. Rectangles retain original endpoints.
 * Return -1 for the original division fault rather than inventing a bar. */
static inline int slicks_arcade_hud(short mode,short seconds,unsigned int ticks,
    unsigned int current,unsigned int deadline,struct SlicksArcadeHud *out)
{
    *out=(struct SlicksArcadeHud){0};
    if(mode!=5) return 0;
    int remaining=slicks_arcade_remaining_ms(mode,seconds,ticks);
    out->rects[out->count++]=(struct SlicksArcadeRect){55,188,85,197,4};
    int amount,divisor;
    if(remaining>0) { amount=remaining; divisor=seconds; }
    else {
        /* Alternates every three seconds, including LAST at exact expiry. */
        int elapsed=(int)(0U-(unsigned int)remaining);
        out->text=(unsigned char)((elapsed/3000)%2 ? 2 : 1);
        if(!deadline) return 0;
        amount=slicks_arcade_elapsed_ms(deadline-current);
        divisor=30;
    }
    int numerator=(int)((unsigned int)(amount/1000)*27U);
    if(!divisor || (numerator==(-2147483647-1) && divisor==-1)) return -1;
    short right=(short)(unsigned short)((unsigned int)(numerator/divisor)+57U);
    out->rects[out->count++]=(struct SlicksArcadeRect){56,(short)(remaining>0?189:196),right,
        (short)(remaining>0?196:197),40};
    return 0;
}
#endif
