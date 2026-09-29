#ifndef SLICKS_TITLE_STATUS_H
#define SLICKS_TITLE_STATUS_H

/* Original 2995f..29af6. Icons are val1, val2, pel_on, pel_ei, pel_t.
 * Zero participation consumes no icon position. Negative/positive are the
 * original signed role tests, not profile indices or controller devices. */
struct SlicksTitleStatusCommand { short kind,value,x,y,flags; };
static inline unsigned slicks_title_status_commands(
    struct SlicksTitleStatusCommand out[9],const signed char participation[4],
    short selected,short total,short inventory,short weapons,short mode)
{
    unsigned count=0,slot=0;
    for(unsigned i=0;i<4;++i) if(participation[i]) {
        out[count++]=(struct SlicksTitleStatusCommand){0,participation[i]<0?0:1,
            (short)(205+8*slot),(short)(99+(slot&1)),0};
        ++slot;
    }
    out[count++]=(struct SlicksTitleStatusCommand){1,selected,219,112,6};
    out[count++]=(struct SlicksTitleStatusCommand){1,total,222,114,4};
    if(inventory) out[count++]=(struct SlicksTitleStatusCommand){0,2,207,121,0};
    if(weapons) out[count++]=(struct SlicksTitleStatusCommand){0,3,208,130,0};
    if(mode==4) out[count++]=(struct SlicksTitleStatusCommand){0,4,202,124,0};
    return count;
}
#endif
