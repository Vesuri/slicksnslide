#ifndef SLICKS_CONFIGURATION_H
#define SLICKS_CONFIGURATION_H

struct SlicksConfiguration {
    /* field_05e1: original persisted language selector (positive table ID;
     * zero invokes the startup chooser, negative invokes default detection). */
    unsigned char field_05e1, keys[20], player_input[4], field_062e, bindings[60];
    short date_code, field_05de, field_0626, options[15], selected_profile[4];
    short field_172c, field_1728, field_719c, field_172e, field_172a, field_719e;
};

static inline short slicks_config_default_word(const unsigned char *data,unsigned at)
{
    return (short)((unsigned)data[at]|((unsigned)data[at+1]<<8));
}

/* Initial configuration immediately before the PLR/CFG readers in main.
 * Input is the initialized DS data from the independently unpacked executable,
 * not a captured running-game state. Startup clears DS:2fa4..7821; main then
 * sets 172c/172e to 40 and copies the 20 default keys at 06ac to 5358.
 * No dependency on whatever bytes happened to occupy the executable's BSS.
 * CFG loading may subsequently replace 40 with its unconditional default 30.
 */
static inline int slicks_configuration_defaults(struct SlicksConfiguration *out,
    const unsigned char *data,unsigned long size)
{
    if(!data || size<0x2fa4UL) return -1;
    out->field_05e1=data[0x5e1]; out->field_062e=data[0x62e];
    out->field_05de=slicks_config_default_word(data,0x5de);
    out->field_0626=slicks_config_default_word(data,0x626);
    for(unsigned i=0;i<20;++i) out->keys[i]=data[0x6ac+i];
    for(unsigned i=0;i<60;++i) out->bindings[i]=data[0x5e6+i];
    for(unsigned i=0;i<15;++i)
        out->options[i]=slicks_config_default_word(data,0x92+8*i);
    for(unsigned i=0;i<4;++i) {
        out->selected_profile[i]=slicks_config_default_word(data,0x44c+2*i);
        out->player_input[i]=data[0x5e2+i];
    }
    out->field_1728=slicks_config_default_word(data,0x1728);
    out->field_172a=slicks_config_default_word(data,0x172a);
    out->field_172c=out->field_172e=40;
    out->date_code=out->field_719c=out->field_719e=0;
    return 0;
}
static inline short slicks_config_word(const unsigned char *data,unsigned *at)
{
    short value=(short)(((unsigned)data[*at]<<8)|data[*at+1]);
    *at+=2; return value;
}

/* 2b486..2b709, excluding the final platform volume application (392ef).
 * Caller supplies the expected DOS machine-signature byte explicitly. An
 * absent/bad-header/bad-signature file retains prior fields except the
 * original unconditional defaults/postprocessing below. A recognized but
 * truncated file is rejected atomically rather than emulating unchecked EOF.
 * Unknown field names retain original addresses until their meaning is proved. */
static inline int slicks_load_configuration(struct SlicksConfiguration *out,
    const unsigned char *data,unsigned long size,unsigned char signature,
    unsigned short date_first,unsigned short date_second,unsigned short date_third)
{
    int recognized=data && size>=2 && data[0]==15 && data[1]==signature;
    if(data && size==1 && data[0]==15) return -1;
    if(recognized && size<142) return -1;
    out->field_172c=30;
    out->date_code=(short)(unsigned short)(date_first+31UL*date_second+372UL*date_third);
    if(recognized) {
        unsigned at=2;
        out->field_05e1=data[at++];
        out->date_code=slicks_config_word(data,&at);
        out->field_05de=slicks_config_word(data,&at);
        out->field_0626=slicks_config_word(data,&at);
        for(unsigned i=0;i<20;++i) out->keys[i]=data[at++];
        for(unsigned i=0;i<15;++i) out->options[i]=slicks_config_word(data,&at);
        for(unsigned i=0;i<4;++i) {
            out->selected_profile[i]=slicks_config_word(data,&at);
            out->player_input[i]=data[at++];
        }
        out->field_062e=data[at++]; out->field_172c=data[at++];
        out->field_1728=slicks_config_word(data,&at);
        out->field_719c=slicks_config_word(data,&at);
        ++at; /* Loaded into 172e, then unconditionally overwritten below. */
        out->field_172a=slicks_config_word(data,&at);
        out->field_719e=slicks_config_word(data,&at);
        for(unsigned i=0;i<60;++i) out->bindings[i]=data[at++];
    }
    out->field_172e=out->field_172c;
    return recognized;
}

static inline void slicks_config_put_word(unsigned char *data,unsigned *at,short value)
{
    data[(*at)++]=(unsigned char)((unsigned short)value>>8);
    data[(*at)++]=(unsigned char)value;
}

/* CFG portion of 2b097's save routine, through 2b2ca. The caller owns file
 * I/O and supplies the explicit platform signature; no implicit writes. */
static inline int slicks_save_configuration(const struct SlicksConfiguration *c,
    unsigned char *data,unsigned long capacity,unsigned char signature)
{
    if(!c || !data || capacity<142) return -1;
    unsigned at=0;
    data[at++]=15; data[at++]=signature; data[at++]=c->field_05e1;
    slicks_config_put_word(data,&at,c->date_code);
    slicks_config_put_word(data,&at,c->field_05de);
    slicks_config_put_word(data,&at,c->field_0626);
    for(unsigned i=0;i<20;++i) data[at++]=c->keys[i];
    for(unsigned i=0;i<15;++i) slicks_config_put_word(data,&at,c->options[i]);
    for(unsigned i=0;i<4;++i) {
        slicks_config_put_word(data,&at,c->selected_profile[i]);
        data[at++]=c->player_input[i];
    }
    data[at++]=c->field_062e; data[at++]=(unsigned char)c->field_172c;
    slicks_config_put_word(data,&at,c->field_1728);
    slicks_config_put_word(data,&at,c->field_719c);
    data[at++]=(unsigned char)c->field_172e;
    slicks_config_put_word(data,&at,c->field_172a);
    slicks_config_put_word(data,&at,c->field_719e);
    for(unsigned i=0;i<60;++i) data[at++]=c->bindings[i];
    return (int)at;
}

/* 392ef consumes only the low byte, then clamps it to 100. */
static inline unsigned char slicks_configuration_volume(const struct SlicksConfiguration *config)
{
    unsigned char value=(unsigned char)config->options[1];
    return value>100?100:value;
}
#endif
