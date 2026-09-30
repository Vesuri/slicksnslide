#ifndef SLICKS_KEY_REPEAT_H
#define SLICKS_KEY_REPEAT_H

/* Original 36ce0, the latched-scan repeat reader. latch is DS:1714, the
 * last raw keyboard byte (>=0x80: break or none, signed word compare);
 * ticks is the BIOS count at 0040:006c. A fresh hold returns the scan at
 * once; later calls return it when ticks > last+arg (DX:AX signed-high
 * compare), so a held key repeats every arg+1 ticks. */
struct SlicksKeyRepeat { unsigned long last; unsigned char armed; };

static inline unsigned char slicks_key_repeat_poll(struct SlicksKeyRepeat *r,
    unsigned short latch,unsigned long ticks,unsigned char arg)
{
    if((short)latch>=0x80) { r->last=ticks; r->armed=0; return 0; }
    if(!r->armed) { r->armed=1; return (unsigned char)latch; }
    unsigned long due=(r->last+arg)&0xffffffffUL;
    short due_high=(short)(unsigned short)(due>>16),now_high=(short)(unsigned short)(ticks>>16);
    if(due_high<now_high || (due_high==now_high && (due&65535UL)<(ticks&65535UL))) {
        r->last=ticks; return (unsigned char)latch;
    }
    return 0;
}
#endif
