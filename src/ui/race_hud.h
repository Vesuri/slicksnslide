#ifndef SLICKS_RACE_HUD_H
#define SLICKS_RACE_HUD_H

/* ddc0 text commands, before the font rasterizer. Flags retain the original
 * text-alignment contract. Icons and fuel/damage/weapon draws are separate. */
struct SlicksHudText {
    short x, y, number;
    unsigned char numeric, flags;
    char text[9];
};

struct SlicksHudWeapon {
    short bar_left, bar_right, restore_x, icon_x;
    unsigned char icon;
};

/* 1dab9..1dc57, after the common status clear. Coordinates: bar y187..188,
 * background and transparent icon y192. Caller supplies original inventory
 * and capacity arrays (13 slots); valid game weapon selections are 0..7.
 * Return -1 for the original signed division fault, 0 for no weapon draw. */
static inline int slicks_hud_weapon(unsigned short driver, unsigned active,
    unsigned enabled, signed char selected, const short inventory[13],
    const signed char capacity[13], struct SlicksHudWeapon *out)
{
    if(!active || !enabled || selected<0) return 0;
    if(selected>7) return -2; /* Invalid asset/state index: fail explicitly. */
    unsigned icon=(unsigned)selected+5;
    int numerator=(short)((unsigned short)inventory[icon]*20U);
    int denominator=capacity[icon];
    if(!denominator || (numerator==-32768 && denominator==-1)) return -1;
    out->bar_left=(short)(106+driver*60);
    out->bar_right=(short)((unsigned short)out->bar_left+
                           (unsigned short)(numerator/denominator));
    out->restore_x=(short)(90+driver*60);
    out->icon_x=(short)(91+driver*60);
    out->icon=(unsigned char)icon;
    return 1;
}

/* Original aceb formatter: unsigned clamp, then truncating BIOS-tick scale. */
static inline void slicks_hud_time(unsigned short raw, char text[9])
{
    if(raw>17999) raw=17999;
    unsigned short value=(unsigned short)((unsigned long)raw*5/9);
    text[0]=(char)('0'+value/1000);
    text[1]=(char)('0'+value/100%10);
    text[2]='.';
    text[3]=(char)('0'+value/10%10);
    text[4]=(char)('0'+value%10);
    text[5]=0;
}

/* 2adbe: filename stem at (5,186), second record's time at (10,193).
 * The name is drawn on the current page; the time is drawn on both pages.
 * DS:696b is tested as a signed word before calling the unsigned formatter. */
static inline unsigned short slicks_hud_track_text(const char *name,
    unsigned short record_time, struct SlicksHudText out[2])
{
    struct SlicksHudText label={0};
    label.x=5; label.y=186;
    for(unsigned i=0;i<8 && name[i];++i) label.text[i]=name[i];
    out[0]=label;
    if((short)record_time<=0) return 1;
    struct SlicksHudText time={0};
    time.x=10; time.y=193;
    slicks_hud_time(record_time,time.text);
    out[1]=time;
    return 2;
}

static inline unsigned short slicks_hud_driver_text(unsigned short driver,
    unsigned short lap, unsigned char place, unsigned short last_lap,
    unsigned short best_lap, struct SlicksHudText out[3])
{
    unsigned short count=0;
    struct SlicksHudText label={0};
    label.x=(short)(driver*60+(place?99:101)); label.y=186;
    label.numeric=1; label.flags=2;
    label.number=place?place:(short)((short)lap%100);
    out[count++]=label;
    if(place) {
        struct SlicksHudText dot={0};
        dot.x=(short)(driver*60+100); dot.y=186; dot.text[0]='.';
        out[count++]=dot;
    }
    struct SlicksHudText time={0};
    unsigned short raw=place?best_lap:last_lap;
    time.x=(short)(driver*60+106); time.y=191;
    slicks_hud_time(raw,time.text);
    out[count++]=time;
    return count;
}
#endif
