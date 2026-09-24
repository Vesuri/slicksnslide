#ifndef SLICKS_OPTIONS_MENU_H
#define SLICKS_OPTIONS_MENU_H
#include "../game/configuration.h"

struct SlicksOptionSpec {
    short maximum;
    signed char minimum,step;
    unsigned char modes;
};
struct SlicksOptionsMenu {
    unsigned char row,dirty,done;
    signed char redraw;
};
enum SlicksOptionsAction {
    SLICKS_OPTIONS_NONE,SLICKS_OPTIONS_HELP,
    SLICKS_OPTIONS_CONTROLLERS,SLICKS_OPTIONS_CLEAR_RECORDS
};

/* Decode the original initialized option records; no captured menu state. */
static inline int slicks_option_specs(struct SlicksOptionSpec out[15],
    const unsigned char *data,unsigned long size)
{
    if(!data || size<0x10a) return -1;
    for(unsigned i=0;i<15;++i) {
        const unsigned char *p=data+0x92+8*i;
        out[i].minimum=(signed char)p[3];
        out[i].maximum=slicks_config_default_word(p,4);
        out[i].step=(signed char)p[6]; out[i].modes=p[7];
    }
    return 0;
}

/* Exact signed-byte delta, 16-bit add, clamp and extrema order at
 * 294d1..29659 for the fifteen real option records. */
static inline short slicks_option_value(short value,const struct SlicksOptionSpec *spec,
    unsigned char scan)
{
    signed char delta=scan==0x4d?spec->step:scan==0x4b?
        (signed char)(unsigned char)(-(int)spec->step):0;
    value=(short)(unsigned short)((unsigned short)value+(short)delta);
    if(value>spec->maximum) value=spec->maximum;
    if(value<spec->minimum) value=spec->minimum;
    if(!delta) value=value==spec->minimum?spec->maximum:spec->minimum;
    if(scan==0x47 || scan==0x49) value=spec->minimum;
    if(scan==0x4f || scan==0x51) value=spec->maximum;
    return value;
}
static inline int slicks_option_enabled(const struct SlicksOptionSpec specs[15],
    unsigned row,unsigned mode)
{
    return row>=15 || (mode<16 && (specs[row].modes&(1U<<mode)));
}

/* Original dispatch/navigation at 293eb..29659. Redraw markers retain DOS
 * semantics: 123 idle, -1 all, 111 values, otherwise the affected row.
 * Rows 15..17 are actions, not option records. Never reproduce the DOS
 * out-of-bounds option-table writes reachable with arrows on those rows. */
static inline enum SlicksOptionsAction slicks_options_menu_key(
    struct SlicksOptionsMenu *m,struct SlicksConfiguration *c,
    const struct SlicksOptionSpec specs[15],unsigned char scan)
{
    m->redraw=123;
    if(m->row>17 || c->options[0]<0 || c->options[0]>5) return SLICKS_OPTIONS_NONE;
    if(scan==1 || scan==0x44) m->done=1;
    else if(scan==0x3b) return SLICKS_OPTIONS_HELP;
    else if(scan==0x48 && m->row) {
        do { --m->row; } while(m->row && m->row<15 &&
            !slicks_option_enabled(specs,m->row,(unsigned)c->options[0]));
        m->redraw=(signed char)m->row;
    } else if(scan==0x50 && m->row<17) {
        m->redraw=(signed char)m->row;
        do { ++m->row; } while(m->row<15 &&
            !slicks_option_enabled(specs,m->row,(unsigned)c->options[0]));
    } else if(scan==0x1c || scan==0x1d || scan==0x39 || scan==0x47 ||
              scan==0x49 || scan==0x4b || scan==0x4d || scan==0x4f || scan==0x51) {
        m->dirty=1; m->redraw=m->row?111:-1;
        if(m->row<15) c->options[m->row]=slicks_option_value(c->options[m->row],&specs[m->row],scan);
        else if(scan!=0x4b && scan!=0x4d) {
            if(m->row==15) return SLICKS_OPTIONS_CONTROLLERS;
            if(m->row==16) return SLICKS_OPTIONS_CLEAR_RECORDS;
            m->done=1;
        }
    }
    return SLICKS_OPTIONS_NONE;
}
#endif
