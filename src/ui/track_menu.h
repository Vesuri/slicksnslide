#ifndef SLICKS_TRACK_MENU_H
#define SLICKS_TRACK_MENU_H

/* 26dd5: 22 visible track names at eight-pixel pitch; action rows 1..6
 * occupy the right-hand column. The selection/top pair persists in DOS
 * DS:10b2/10b4 while column, previous cursor and exit are dialog-local. */
struct SlicksTrackMenu {
    short cursor,top,previous;
    short random_count;
    unsigned char column,done,random_order;
};
enum SlicksTrackMenuAction {
    SLICKS_TRACK_MENU_NONE,SLICKS_TRACK_MENU_HELP,SLICKS_TRACK_MENU_TOGGLE,
    SLICKS_TRACK_MENU_LISTS,SLICKS_TRACK_MENU_SHUFFLE,SLICKS_TRACK_MENU_ALL,
    SLICKS_TRACK_MENU_CLEAR,SLICKS_TRACK_MENU_RANDOM,SLICKS_TRACK_MENU_RECORDS
};

static inline short slicks_track_word_add(short value,int delta)
{ return (short)(unsigned short)((unsigned short)value+delta); }

/* Original navigation and dispatch, 274e5..2794a. Playlist mutation and
 * modal actions are explicit caller boundaries, not silently ignored keys.
 * The caller applies each action before drawing the next menu iteration.
 * Original draw clamps random_count to the number of discovered tracks. */
static inline enum SlicksTrackMenuAction slicks_track_menu_key(
    struct SlicksTrackMenu *m,short track_count,unsigned char scan)
{
    enum SlicksTrackMenuAction action=SLICKS_TRACK_MENU_NONE;
    if(scan==1 || scan==0x44) m->done=1;
    else if(scan==0x3b) action=SLICKS_TRACK_MENU_HELP;
    else if(scan==0x4b) {
        m->previous=-1;
        if(m->column==5) {
            if(m->random_count>1) --m->random_count;
        } else m->column=0;
    } else if(scan==0x4d) {
        m->previous=-1;
        if(m->column==5) m->random_count=slicks_track_word_add(m->random_count,1);
        else if(!m->column) m->column=1;
    } else if(scan==0x1c || scan==0x39) {
        m->previous=-1;
        switch(m->column) {
        case 0: action=SLICKS_TRACK_MENU_TOGGLE; break;
        case 1: action=SLICKS_TRACK_MENU_LISTS; break;
        case 2:
            m->random_order=(unsigned char)!m->random_order;
            if(m->random_order) action=SLICKS_TRACK_MENU_SHUFFLE;
            break;
        case 3: action=SLICKS_TRACK_MENU_ALL; break;
        case 4: action=SLICKS_TRACK_MENU_CLEAR; break;
        case 5: action=SLICKS_TRACK_MENU_RANDOM; break;
        default: m->done=1; break;
        }
    } else if(scan==0x12 || scan==0x14 || scan==0x3c) {
        if(!m->column) { action=SLICKS_TRACK_MENU_RECORDS; m->previous=-1; }
    } else if(scan==0x47) {
        if(!m->column) m->cursor=0;
    } else if(scan==0x4f) {
        if(!m->column) m->cursor=track_count;
    } else if(scan==0x48) {
        if(!m->column) m->cursor=slicks_track_word_add(m->cursor,-1);
        else { --m->column; m->previous=-1; }
    } else if(scan==0x50) {
        if(!m->column) m->cursor=slicks_track_word_add(m->cursor,1);
        else if(m->column<6) { ++m->column; m->previous=-1; }
    } else if(scan==0x49 || scan==0x51) {
        if(!m->column) m->cursor=slicks_track_word_add(m->cursor,scan==0x49?-21:21);
    }
    /* Preserve signed ordering, including the original empty-list cursor -1. */
    if(m->cursor<0) m->cursor=0;
    short last=slicks_track_word_add(track_count,-1);
    if(m->cursor>last) m->cursor=last;
    if(m->top>m->cursor) m->top=m->cursor;
    short bottom=slicks_track_word_add(m->cursor,-21);
    if(m->top<bottom) m->top=bottom;
    if(m->top<0) m->top=0;
    return action;
}
#endif
