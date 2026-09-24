#ifndef SLICKS_CONTROLLERS_DIALOG_H
#define SLICKS_CONTROLLERS_DIALOG_H
#include "../game/configuration.h"

struct SlicksControllersDialog {
    unsigned char row,column,done,capturing;
    signed char redraw;
};

/* 2da76..2da8c: the last row contains Defaults and Exit, not six fields. */
static inline void slicks_controllers_limits(struct SlicksControllersDialog *d)
{ if(d->row==4 && d->column>1) d->column=1; }

/* Key validation at 1e1e1. Index zero and a missing entry both mean reject;
 * compare the signed original table byte with the entire returned key word. */
static inline unsigned slicks_controller_key_index(const unsigned char keys[69],short scan)
{
    for(unsigned i=0;i<69;++i) if((signed char)keys[i]==scan) return i;
    return 0;
}
static inline int slicks_controllers_capture(struct SlicksControllersDialog *d,
    struct SlicksConfiguration *c,const unsigned char supported[69],short scan)
{
    if(!d->capturing || d->row>=4 || d->column<1 || d->column>5) return -1;
    if(slicks_controller_key_index(supported,scan))
        c->keys[5*d->row+d->column-1]=(unsigned char)scan;
    d->capturing=0;
    return 0;
}

/* Original dispatch 2dd91..2df4b; capture returns to the caller instead of
 * blocking inside a hardware keyboard read. Edits are immediate: Escape
 * exits, it does not roll back keys or selected devices. */
static inline int slicks_controllers_key(struct SlicksControllersDialog *d,
    struct SlicksConfiguration *c,const unsigned char defaults[20],unsigned char scan)
{
    if(d->row>4 || d->column>5 || d->capturing) return -1;
    d->redraw=111;
    if(scan==1 || scan==0x43 || scan==0x44) d->done=1;
    else if(scan==0x48 && d->row) { --d->row; d->redraw=-1; }
    else if(scan==0x50 && d->row<4) { ++d->row; d->redraw=-1; }
    else if(scan==0x4b && d->column) { --d->column; d->redraw=(signed char)d->row; }
    else if(scan==0x4d && d->column<5) { ++d->column; d->redraw=(signed char)d->row; }
    else if(scan==0x1c || scan==0x1d || scan==0x39) {
        d->redraw=(signed char)d->row;
        if(d->row==4) {
            if(d->column) d->done=1;
            else { for(unsigned i=0;i<20;++i) c->keys[i]=defaults[i]; d->redraw=-1; }
        } else if(!d->column) {
            unsigned char value=(unsigned char)(c->player_input[d->row]+(scan==0x39?-1:1));
            if((signed char)value<0) value=2;
            if((signed char)value>2) value=0;
            c->player_input[d->row]=value;
        } else d->capturing=1;
    }
    return 0;
}
#endif
