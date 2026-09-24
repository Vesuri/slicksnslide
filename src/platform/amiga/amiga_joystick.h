#ifndef SLICKS_AMIGA_JOYSTICK_H
#define SLICKS_AMIGA_JOYSTICK_H
#include "../../game/driver_device.h"
/* Amiga HRM table 8-3: direction signals are active high after the two
 * quadrature XORs; both buttons are active low. port is JOY0/JOY1 index. */
static inline struct SlicksDeviceSample slicks_decode_amiga_joystick(
    unsigned short joy,unsigned char cia,unsigned short pot,unsigned port)
{
    struct SlicksDeviceSample s;
    s.x=(signed char)(((joy>>1)&1)-((joy>>9)&1));
    s.y=(signed char)(((joy^(joy>>1))&1)-(((joy^(joy>>1))>>8)&1));
    s.buttons=(unsigned char)((!(cia&(1U<<(6+port))))|
        ((! (pot&(1U<<(10+4*port))))<<1));
    return s;
}
#endif
