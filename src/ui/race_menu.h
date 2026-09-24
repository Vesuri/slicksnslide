#ifndef SLICKS_RACE_MENU_H
#define SLICKS_RACE_MENU_H

struct SlicksRaceMenu {
    unsigned char row,count;
    signed char redraw,result;
};
enum SlicksRaceMenuAction {
    SLICKS_RACE_MENU_NONE,SLICKS_RACE_MENU_HELP,
    SLICKS_RACE_MENU_CONTROLLERS,SLICKS_RACE_MENU_SPEED
};

/* Original 1e664..1e718. The blocking key helper returns a byte which is
 * sign-extended before dispatch. Result 1 resumes, -1 skips the track,
 * -2 ends the game; Help/Controllers/Speed return to this same menu. */
static inline enum SlicksRaceMenuAction slicks_race_menu_key(
    struct SlicksRaceMenu *m,unsigned char key)
{
    if(key==1) m->result=1;
    else if(key==0x43) m->result=-1;
    else if(key==0x44) m->result=-2;
    else if(key==0x48 || key==100) {
        if((signed char)m->row>0) {
            m->redraw=(signed char)m->row; --m->row;
        }
    } else if(key==0x50 || key==101) {
        if((signed char)m->row<(int)(signed char)m->count-1) {
            ++m->row; m->redraw=(signed char)m->row;
        }
    } else if(key==0x1c || key==0x1d || key==0x39 || key==108) {
        switch(m->row) {
        case 0: m->result=1; break;
        case 1: return SLICKS_RACE_MENU_HELP;
        case 2: return SLICKS_RACE_MENU_CONTROLLERS;
        case 3: m->redraw=-1; return SLICKS_RACE_MENU_SPEED;
        case 4: m->result=-1; break;
        case 5: m->result=-2; break;
        }
    }
    return SLICKS_RACE_MENU_NONE;
}
#endif
