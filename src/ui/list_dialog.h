#ifndef SLICKS_LIST_DIALOG_H
#define SLICKS_LIST_DIALOG_H

/* Shared original list dialog 30cc4. These fields replace its persistent
 * locals; rendering, saved-background ownership and input polling are separate.
 * Callers supply valid counts/positive visible rows and screen-sized indices. */
struct SlicksListDialog {
    short selected, initial, count, visible, top, actions, pulse;
    unsigned char action, focus_actions, redraw_actions, redraw_list, done, flags;
};

/* 30cca..30ce7 followed by 30f39..30fa9 and 3113c..31155. Caption parsing
 * resolves action/count between those regions; pass -1 to retain the action
 * derived from a negative initial selection. Height is bottom minus top,
 * not an inclusive pixel count. Native callers reject unusable geometry. */
static inline int slicks_list_dialog_init(struct SlicksListDialog *s,
    short selected,short count,short height,unsigned char font_height,
    short actions,short caption_action,unsigned char flags,short *thumb)
{
    /* The result reserves bits 12..15 for actions. The platform may impose
     * tighter storage bounds, but 100 is not an original row-count limit. */
    if(!s || !thumb || count<0 || count>4095 || actions<0 || actions>127 ||
       height<10+(short)font_height+2 || height>200 || caption_action < -1 ||
       caption_action>255) return -1;
    struct SlicksListDialog next={0};
    next.action=selected<0?(unsigned char)(-1-selected):0;
    if(selected<0) selected=0;
    next.selected=next.initial=selected; next.count=count; next.actions=actions;
    next.flags=flags;
    if(caption_action>=0) next.action=(unsigned char)caption_action;
    if((signed char)next.action>=actions) next.action=0;
    short span=height-10;
    next.visible=(short)(span/(font_height+2));
    short size=0;
    if(next.visible<count) {
        next.top=(short)(selected-next.visible/2);
        if(next.top+next.visible>=count) next.top=count-next.visible;
        if(next.top<0) next.top=0;
        /* The DOS IMUL retains AX before CWD/IDIV; preserve signed wrap. */
        size=(short)((short)(span*next.visible)/count);
    }
    next.pulse=50; next.redraw_list=255; next.redraw_actions=1;
    *s=next; *thumb=size;
    return 0;
}

/* Original 315b6..316cf. Scroll clipping occurs on the following update,
 * not while processing PageUp/PageDown; keep that sequencing observable. */
static inline void slicks_list_dialog_key(struct SlicksListDialog *s,unsigned char scan)
{
    switch(scan) {
    case 1: case 0x0e:
        s->selected=(s->flags&1)?s->initial:-1; s->action=0; s->done=1; break;
    case 0x0f:
        s->redraw_actions=1;
        ++s->action;
        if((signed char)s->action>=s->actions) s->action=0;
        s->pulse=50; break;
    case 0x1c: case 0x39: case 0x44: s->done=1; break;
    case 0x4b: if(s->actions>1) s->focus_actions=1; break;
    case 0x4d: s->redraw_actions=1; s->pulse=50; s->focus_actions=0; break;
    case 0x48:
        if(s->focus_actions) { if((signed char)s->action>0) --s->action; }
        else if(s->selected>0) { s->redraw_list=(unsigned char)s->selected; --s->selected; }
        break;
    case 0x50:
        if(s->focus_actions) { if((signed char)s->action<s->actions-1) ++s->action; }
        else if(s->selected<s->count-1) { ++s->selected; s->redraw_list=(unsigned char)s->selected; }
        break;
    case 0x49: case 0x51:
        if(!s->focus_actions) {
            s->selected=(short)(s->selected+(scan==0x49?1-s->visible:s->visible-1));
            s->redraw_list=255;
        }
        break;
    case 0x47:
        if(!s->focus_actions && s->selected!=0) { s->selected=0; s->redraw_list=255; }
        break;
    case 0x4f:
        if(!s->focus_actions && s->selected<s->count) { s->selected=s->count-1; s->redraw_list=255; }
        break;
    }
}

/* Original next-update clipping/scroll adjustment, 3115d..311b5. */
static inline void slicks_list_dialog_normalize(struct SlicksListDialog *s)
{
    if(s->selected<0) s->selected=0;
    if(s->count>0 && s->selected>=s->count) s->selected=s->count-1;
    if(s->top>s->selected) { s->top=s->selected; s->redraw_list=255; }
    short top=(short)(s->selected-s->visible+1);
    if(top>s->top) { s->top=top; s->redraw_list=255; }
    if(s->count<=0 && s->actions!=0) s->focus_actions=1;
}

/* Original 31760..31773 returns the selected row plus the action in bits
 * 12..15. Cancellation has already reset action and chosen -1/original row. */
static inline short slicks_list_dialog_result(const struct SlicksListDialog *s)
{
    return (short)(s->selected+(signed char)s->action*4096);
}

struct SlicksListPulse { unsigned long tick; signed char step; };

/* Original 311b5..31223: query the palette before advancing the pulse.
 * A changed DOS BIOS tick advances once, irrespective of elapsed ticks.
 * Overshoots are retained and reverse the step, not clamped to 0..50. */
static inline unsigned char slicks_list_dialog_pulse(struct SlicksListDialog *s,
    struct SlicksListPulse *pulse,unsigned long tick,
    unsigned char (*nearest)(void *,unsigned char,unsigned char,unsigned char),void *context)
{
    unsigned char colour=nearest(context,(unsigned char)(s->pulse+30),
        (unsigned char)(s->pulse+30),(unsigned char)(s->pulse/2+30));
    tick&=0xffffffffUL;
    if(tick!=pulse->tick) {
        pulse->tick=tick;
        s->pulse=(short)(s->pulse+pulse->step);
        if(s->pulse>50 || s->pulse<0) pulse->step=(signed char)-pulse->step;
    }
    return colour;
}
#endif
