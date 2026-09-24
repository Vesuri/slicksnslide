#ifndef SLICKS_PROFILE_EDITOR_H
#define SLICKS_PROFILE_EDITOR_H
#include "../game/player_profiles.h"

struct SlicksProfileEditor {
    unsigned char row,redraw;
    signed char result;
};
enum SlicksProfileEditorAction {
    SLICKS_PROFILE_EDIT_NONE,SLICKS_PROFILE_EDIT_NAME,
    SLICKS_PROFILE_EDIT_COLOUR_FIRST,SLICKS_PROFILE_EDIT_COLOUR_SECOND
};

/* Original 27b92..27bea, at the start of each editor iteration. Unknown
 * global DS:01a6 is supplied explicitly, not interpreted as a new option. */
static inline void slicks_profile_editor_limits(struct SlicksPlayerProfiles *p,
    unsigned index,unsigned char field_01a6)
{
    unsigned maximum=(p->setup[index].flags&1) || field_01a6?150:100;
    if(p->setting[index]<50) p->setting[index]=50;
    if(p->setting[index]>maximum) p->setting[index]=(unsigned char)maximum;
}

/* 27f5b..28120. Caller supplies valid index and row 0..5. The original
 * clamp runs on the following iteration, not inside the key event. */
static inline enum SlicksProfileEditorAction slicks_profile_editor_key(
    struct SlicksProfileEditor *e,struct SlicksPlayerProfiles *p,
    unsigned index,short vehicle_count,unsigned char scan)
{
    unsigned char *vehicle=&p->setup[index].vehicle;
    short last=(short)(unsigned short)((unsigned short)vehicle_count+1);
    if(scan==1 || scan==0x43 || scan==0x44) e->result=-1;
    else if(scan==0x3c) e->result=1;
    else if(scan==0x48) {
        if(e->row>0) { --e->row; e->redraw=1; }
    } else if(scan==0x50) {
        if(e->row<5) { ++e->row; e->redraw=1; }
    } else if(scan==0x4b || scan==0x4d) {
        if(e->row==1) {
            p->setting[index]=(unsigned char)(p->setting[index]+(scan==0x4b?-1:1));
            e->redraw=1;
        } else if(e->row==2) {
            if(scan==0x4b) { if((signed char)*vehicle>0) --*vehicle; }
            else if((signed char)*vehicle<last) ++*vehicle;
            e->redraw=2;
        }
    } else if(scan==0x1c || scan==0x1d || scan==0x39) {
        e->redraw=255;
        switch(e->row) {
        case 0: return SLICKS_PROFILE_EDIT_NAME;
        case 1: p->setup[index].flags^=1; break;
        case 2: *vehicle=(signed char)*vehicle>last?0:(unsigned char)(*vehicle+1); break;
        case 3: return SLICKS_PROFILE_EDIT_COLOUR_FIRST;
        case 4: return SLICKS_PROFILE_EDIT_COLOUR_SECOND;
        case 5: e->result=1; break;
        }
    }
    return SLICKS_PROFILE_EDIT_NONE;
}
#endif
