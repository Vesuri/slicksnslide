#ifndef SLICKS_COLOUR_PICKER_H
#define SLICKS_COLOUR_PICKER_H

struct SlicksColourPicker {
    unsigned char rgb[3],channel;
    signed char result;
};

struct SlicksColourPickerPulse { unsigned long tick; unsigned char phase; };
struct SlicksColourPickerDrawOps {
    unsigned char (*nearest)(void *,unsigned char,unsigned char,unsigned char);
    void (*rectangle)(void *,short,short,short,short,unsigned char);
    void *context;
};

/* Dynamic drawing loop 2f43f..2f5b6. Palette/font/dialog background setup is
 * separate. Tick is a platform-provided BIOS-tick snapshot; pulse phase moves
 * by two on a changed tick, regardless of how many ticks elapsed. */
static inline void slicks_colour_picker_draw(const struct SlicksColourPicker *s,
    struct SlicksColourPickerPulse *pulse,unsigned long tick,short x,short y,
    const unsigned char bars[3],unsigned char unselected,
    const struct SlicksColourPickerDrawOps *ops)
{
    for(unsigned i=0;i<3;++i) {
        short split=(short)(unsigned short)(x+10+(signed char)s->rgb[i]);
        short top=(short)(unsigned short)(y+8+7*i),bottom=(short)(unsigned short)(y+14+7*i);
        ops->rectangle(ops->context,(short)(unsigned short)(x+10),top,split,bottom,bars[i]);
        unsigned char colour=unselected;
        if(i==s->channel) {
            unsigned char level=(unsigned char)((signed char)pulse->phase>30?90-pulse->phase:pulse->phase+30);
            colour=ops->nearest(ops->context,level,level,level);
            if(pulse->tick!=(tick&0xffffffffUL)) {
                pulse->phase=(unsigned char)(pulse->phase+2);
                pulse->tick=tick&0xffffffffUL;
            }
            if((signed char)pulse->phase>60) pulse->phase=0;
        }
        ops->rectangle(ops->context,split,top,(short)(unsigned short)(x+73),bottom,colour);
    }
    unsigned char colour=ops->nearest(ops->context,s->rgb[0],s->rgb[1],s->rgb[2]);
    ops->rectangle(ops->context,(short)(unsigned short)(x+3),(short)(unsigned short)(y+8),
        (short)(unsigned short)(x+7),(short)(unsigned short)(y+28),colour);
}

/* Original 2f381..2f3a0: edit a local RGB copy; cancellation leaves the
 * caller's endpoint untouched, unlike direct profile-property controls. */
static inline void slicks_colour_picker_begin(struct SlicksColourPicker *s,
    const unsigned char rgb[3])
{
    for(unsigned i=0;i<3;++i) s->rgb[i]=rgb[i];
    s->channel=0; s->result=0;
}

/* 2f5be..2f676. Arithmetic wraps the byte BEFORE its signed clamp. Keep
 * that behavior for out-of-range saved colours as well as normal 0..63. */
static inline void slicks_colour_picker_key(struct SlicksColourPicker *s,
    unsigned char scan)
{
    if(scan==0x48 && s->channel>0) --s->channel;
    if(scan==0x50 && s->channel<2) ++s->channel;
    if(scan==0x4b) {
        unsigned char v=(unsigned char)(s->rgb[s->channel]-3);
        s->rgb[s->channel]=(signed char)v<0?0:v;
    }
    if(scan==0x4d) {
        unsigned char v=(unsigned char)(s->rgb[s->channel]+3);
        s->rgb[s->channel]=(signed char)v>63?63:v;
    }
    if(scan==1) s->result=-1;
    if(scan==0x39 || scan==0x1c) s->result=1;
}

/* 2f6ad..2f6d6, after display cleanup: 1 cancelled, 0 accepted. */
static inline unsigned slicks_colour_picker_finish(const struct SlicksColourPicker *s,
    unsigned char rgb[3])
{
    if(s->result<0) return 1;
    for(unsigned i=0;i<3;++i) rgb[i]=s->rgb[i];
    return 0;
}
#endif
