#ifndef SLICKS_INTERMISSION_MENU_H
#define SLICKS_INTERMISSION_MENU_H

enum SlicksIntermissionAction {
    SLICKS_INTERMISSION_NONE,
    SLICKS_INTERMISSION_CHANGE_CARS,
    SLICKS_INTERMISSION_SAVE_GAME
};
struct SlicksIntermissionMenu {
    signed char selected, redraw, cars_redraw, exit_code;
};

/* Original 245bb..245c8 overwrites 1-ChangeCars with 2. The supplied
 * executable therefore starts navigation at NEXT TRACK even when Change
 * Cars is enabled. F2 still enters the first action directly. */
static inline void slicks_intermission_init(struct SlicksIntermissionMenu *m)
{
    m->selected=2; m->redraw=-1; m->cars_redraw=-1; m->exit_code=0;
}

/* Original 2470e..2479f; key is the DOS scan returned by the input helper.
 * Modal actions are returned to the platform owner, not simulated here. */
static inline enum SlicksIntermissionAction slicks_intermission_key(
    struct SlicksIntermissionMenu *m, unsigned char key)
{
    switch(key) {
    case 1: case 67: m->exit_code=1; break;
    case 68: m->exit_code=2; break;
    case 60: m->selected=0; m->redraw=1; /* F2 accepts immediately. */
        /* fall through */
    case 28: case 29: case 57:
        if(m->selected==0) {
            m->cars_redraw=1;
            return SLICKS_INTERMISSION_CHANGE_CARS;
        }
        if(m->selected==1) return SLICKS_INTERMISSION_SAVE_GAME;
        if(m->selected==2) m->exit_code=1;
        if(m->selected==3) m->exit_code=2;
        break;
    case 72:
        if(m->selected>2) { --m->selected; m->redraw=1; }
        break;
    case 80:
        if(m->selected<3) { ++m->selected; m->redraw=1; }
        break;
    }
    return SLICKS_INTERMISSION_NONE;
}
#endif
