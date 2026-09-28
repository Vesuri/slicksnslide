#ifndef SLICKS_TITLE_NAVIGATION_H
#define SLICKS_TITLE_NAVIGATION_H

/* Original 2a0cb..2a1b6. Entry 4 exists internally but is skipped by arrows.
 * Track-count changes preserve the unused playlist tail, just like DS:0090.
 * The refresh byte is the original DS:1146 status-area invalidation. */
static inline int slicks_title_navigation(short *selection,short *count,
    short total,short *mode,unsigned char *refresh,unsigned short scan)
{
    switch(scan) {
    case 0x48:
        *selection=(short)(unsigned short)((unsigned short)*selection-1U);
        if(*selection<0) *selection=0;
        if(*selection==4) --*selection;
        return 1;
    case 0x50:
        *selection=(short)(unsigned short)((unsigned short)*selection+1U);
        if(*selection>=7) *selection=6;
        if(*selection==4) ++*selection;
        return 1;
    case 0x4b:
        if(*selection==2 && *count>1) --*count;
        if(*selection==3 && *mode>0) --*mode;
        *refresh=2;
        return 1;
    case 0x4d:
        if(*selection==2 && *count<total) ++*count;
        if(*selection==3 && *mode<5) ++*mode;
        *refresh=2;
        return 1;
    default: return 0;
    }
}
#endif
