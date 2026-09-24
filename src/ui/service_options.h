#ifndef SLICKS_SERVICE_OPTIONS_H
#define SLICKS_SERVICE_OPTIONS_H

/* Original custom-option records 00da/00e2 and input path 294d1..29659.
 * The surrounding menu must pass only these supported DOS scan codes. */
static inline short slicks_service_option_key(short value,
                                               unsigned char damage,
                                               unsigned short scan)
{
    short delta = 0;
    if (scan == 0x4b || scan == 0x4d)
        delta = (short)((damage ? 20 : 5) * (scan == 0x4b ? -1 : 1));
    else if (scan != 0x47 && scan != 0x49 && scan != 0x4f &&
             scan != 0x51 && scan != 0x1c)
        return value;
    value = (short)(unsigned short)((unsigned short)value + delta);
    if (value > 300)
        value = 300;
    if (value < 0)
        value = 0;
    if (!delta)
        value = value == 0 ? 300 : 0;
    if (scan == 0x47 || scan == 0x49)
        value = 0;
    if (scan == 0x4f || scan == 0x51)
        value = 300;
    return value;
}

/* Native navigation: 0 ignored, 1 redraw, 2 return to the title menu. */
static inline unsigned short slicks_service_menu_key(unsigned short *selection,
    short *fuel, short *damage, unsigned short scan)
{
    if (scan == 0x01 || (scan == 0x1c && *selection == 2))
        return 2;
    if (scan == 0x48) {
        *selection = *selection ? *selection - 1 : 2;
        return 1;
    }
    if (scan == 0x50) {
        *selection = *selection < 2 ? *selection + 1 : 0;
        return 1;
    }
    if (*selection < 2 && (scan == 0x4b || scan == 0x4d || scan == 0x1c ||
        scan == 0x47 || scan == 0x49 || scan == 0x4f || scan == 0x51)) {
        short *value = *selection ? damage : fuel;
        *value = slicks_service_option_key(*value, *selection != 0, scan);
        return 1;
    }
    return 0;
}

#endif
