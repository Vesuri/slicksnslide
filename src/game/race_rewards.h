#ifndef SLICKS_RACE_REWARDS_H
#define SLICKS_RACE_REWARDS_H

/* Original 22cf1..22d1b. Caller supplies the original rank/table byte and
 * DS:4c16-rank multiplier; both are signed bytes, additions wrap words. */
static inline void slicks_finish_reward(short *cash,short *points,
    unsigned char table_points,unsigned char multiplier,short money_per_position)
{
    *points=(short)(unsigned short)((unsigned short)*points+(signed char)table_points);
    *cash=(short)(unsigned short)((unsigned short)*cash+
        (signed char)multiplier*(int)money_per_position);
}

/* Original 2557e..255ff: the minimum accumulator is a SIGNED WORD even
 * though lap times are signed dwords. All four slots receive the base
 * payment, and every exact tie with that accumulator receives the bonus.
 * Preserve the 29999 sentinel and word truncation, not a generic min(). */
static inline void slicks_track_rewards(short cash[4],short points[4],
    const signed int best_laps[4],short per_track,short per_position,
    unsigned char fastest_points)
{
    short best=29999;
    for(unsigned i=0;i<4;++i)
        if((signed int)best>best_laps[i]) best=(short)(unsigned short)best_laps[i];
    for(unsigned i=0;i<4;++i) {
        cash[i]=(short)(unsigned short)((unsigned short)cash[i]+per_track);
        if((signed int)best==best_laps[i]) {
            points[i]=(short)(unsigned short)((unsigned short)points[i]+(signed char)fastest_points);
            cash[i]=(short)(unsigned short)((unsigned short)cash[i]+per_position);
        }
    }
}
#endif
