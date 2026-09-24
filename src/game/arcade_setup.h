#ifndef SLICKS_ARCADE_SETUP_H
#define SLICKS_ARCADE_SETUP_H
/* Both host and freestanding Amiga builds use 32-bit int. No libc headers
 * are available in the target runtime. */
_Static_assert(sizeof(unsigned int)==4,"Arcade clock requires 32-bit int");

/* 19894: the original 90 Hz game counter, not BIOS/menu-clock ticks.
 * Multiplication wraps as a 32-bit long before signed division. */
static inline int slicks_arcade_elapsed_ms(unsigned int ticks)
{ return (int)(ticks*1000U)/90; }

/* 198c9: non-Arcade callers receive zero rather than a countdown. */
static inline int slicks_arcade_remaining_ms(short mode,short seconds,unsigned int ticks)
{
    if(mode!=5) return 0;
    return (int)((unsigned int)((int)seconds*1000)-
        (unsigned int)slicks_arcade_elapsed_ms(ticks));
}

/* 198fe: true outside Arcade, or when its remaining time is nonpositive. */
static inline unsigned char slicks_arcade_time_finished(short mode,short seconds,unsigned int ticks)
{ return (unsigned char)(mode!=5 || slicks_arcade_remaining_ms(mode,seconds,ticks)<=0); }

/* 1991f: the 9999 sentinel changes once on expiry. Lap inputs are original
 * signed DS:4bfe completed-lap counters, not the native one-based HUD values.
 * Original scan includes all four slots regardless of participation. */
static inline short slicks_arcade_lap_limit(short mode,short seconds,unsigned int ticks,
    short previous,const short completed[4])
{
    if(mode==5 && slicks_arcade_remaining_ms(mode,seconds,ticks)<=0 && previous>=9999) {
        short highest=0;
        for(unsigned i=0;i<4;++i) if(completed[i]>highest) highest=completed[i];
        return (short)(unsigned short)((unsigned short)highest+1U);
    }
    return previous;
}

/* 241cc: signed minimum only in Arcade; do not truncate the saved playlist. */
static inline short slicks_arcade_track_count(short mode,short limit,short selected)
{ return mode==5 && limit<selected?limit:selected; }

/* 22c4d..22cda, after a driver finishes. */
static inline unsigned int slicks_finish_deadline(short mode,unsigned int current,
    unsigned int deadline,unsigned unfinished_drivers)
{
    if(mode==5 && !deadline) deadline=current+1800U;
    unsigned int soon=current+230U;
    if(!unfinished_drivers && (!deadline || (int)soon<(int)deadline)) deadline=soon;
    return deadline;
}

/* 2037d..203f2: negative rank means unfinished (for either driver kind).
 * Suppression is independent of mode. Expiry uses a strict signed compare. */
static inline unsigned char slicks_finish_controls_suppressed(unsigned int current,
    unsigned int deadline,signed char rank)
{ return deadline && (rank>=0 || (int)(deadline-current)<270); }

static inline unsigned char slicks_finish_expired(unsigned int current,unsigned int deadline)
{ return deadline && (int)current>(int)deadline; }
#endif
