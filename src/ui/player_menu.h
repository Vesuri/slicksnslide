#ifndef SLICKS_PLAYER_MENU_H
#define SLICKS_PLAYER_MENU_H
#include "../game/profile_setup.h"

enum SlicksPlayerMenuAction {
    SLICKS_PLAYER_MENU_NONE, SLICKS_PLAYER_MENU_PICK,
    SLICKS_PLAYER_MENU_ADD, SLICKS_PLAYER_MENU_EDIT,
    SLICKS_PLAYER_MENU_DELETE, SLICKS_PLAYER_MENU_HELP
};
struct SlicksPlayerMenu {
    unsigned char row,redraw,done,dirty;
};

/* 28999..28dab, with modal dialogs returned to the caller as actions.
 * Input is the original decoded scan byte, not an Amiga raw key. Rows 0..3
 * select drivers, 4..6 add/edit/delete, and 7 exits. Modal completion must
 * perform the original redraw/dirty lifecycle separately. */
static inline enum SlicksPlayerMenuAction slicks_player_menu_key(
    struct SlicksPlayerMenu *menu,short selected[4],
    struct SlicksSetupProfile *profiles,short count,short vehicle_count,
    unsigned char scan)
{
    if(scan==1 || scan==0x43 || scan==0x44) menu->done=1;
    else if(scan==0x48) {
        if(menu->row>0) { --menu->row; menu->redraw=1; }
    } else if(scan==0x50) {
        if(menu->row<7) { ++menu->row; menu->redraw=1; }
    } else if(scan==0x4b || scan==0x4d) {
        if(menu->row<4) {
            selected[menu->row]=slicks_menu_step_profile(selected,profiles,count,
                selected[menu->row],scan==0x4b?-1:1);
            menu->redraw=menu->dirty=1;
        }
    } else if(scan==0x1e || scan==0x2e) {
        /* DOS indexes beyond the four driver slots on action rows. Reject
         * that out-of-bounds access at the native boundary. */
        if(menu->row<4) {
            unsigned char *vehicle=&profiles[selected[menu->row]].vehicle;
            *vehicle=(signed char)*vehicle>vehicle_count?0:(unsigned char)(*vehicle+1);
            menu->redraw=1;
        }
    } else if(scan==0x3b) return SLICKS_PLAYER_MENU_HELP;
    else if(scan==0x1c || scan==0x1d || scan==0x39) {
        if(menu->row<4) { menu->dirty=1; return SLICKS_PLAYER_MENU_PICK; }
        if(menu->row==4 && count<100) { menu->dirty=1; return SLICKS_PLAYER_MENU_ADD; }
        if(menu->row==5 || menu->row==6) {
            menu->dirty=1;
            return menu->row==5?SLICKS_PLAYER_MENU_EDIT:SLICKS_PLAYER_MENU_DELETE;
        }
        if(menu->row==7) menu->done=1;
        menu->redraw=1;
    }
    return SLICKS_PLAYER_MENU_NONE;
}
#endif
