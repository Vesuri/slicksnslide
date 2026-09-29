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
/* Original 2a1c9..2a248. Arcade is a two-row menu. Left/right on row zero
 * choose the 1..4 human-driver override, not tracks or the game mode.
 * Preserve the original asymmetric clamping even for out-of-range state. */
static inline int slicks_arcade_title_navigation(short *selection,short *players,
    unsigned char *refresh,unsigned short scan)
{
    switch(scan) {
    case 0x48:
        if(*selection>0) {--*selection;*refresh=2;}
        return 1;
    case 0x50:
        if(*selection<1) ++*selection;
        if(*selection>=2) *selection=1;
        return 1;
    case 0x4b:
        if(!*selection && *players>1) {--*players;*refresh=2;}
        return 1;
    case 0x4d:
        if(!*selection && *players<4) {++*players;*refresh=2;}
        return 1;
    default:return 0;
    }
}

/* Original 2a25b chooses navigation before input; 2a28f maps the Arcade
 * second row to the ordinary OPTIONS action without changing visual state. */
static inline int slicks_title_mode_navigation(short *selection,short *count,
    short total,short *mode,short *players,unsigned char *refresh,unsigned short scan)
{
    return *mode==5?slicks_arcade_title_navigation(selection,players,refresh,scan):
        slicks_title_navigation(selection,count,total,mode,refresh,scan);
}
static inline short slicks_title_action_selection(short mode,short selection)
{ return mode==5 && selection==1?3:selection; }
#endif
