#ifndef SLICKS_CHAMPIONSHIP_STANDINGS_H
#define SLICKS_CHAMPIONSHIP_STANDINGS_H
#include "player_profiles.h"
struct SlicksChampionshipStandings {
    short points[4];
    unsigned char driver[4],place[4];
};
/* 22b7c / 22bdf: called once by the real finish event, not on entry to
 * either results screen. Statistics words wrap exactly like INC word. */
static inline int slicks_finish_statistics(struct SlicksPlayerProfiles *profiles,short selected,signed char rank)
{
    if(selected<0 || selected>=profiles->count || rank<1 || rank>4) return -1;
    short *stats=profiles->statistics[selected];
    stats[0]=(short)(unsigned short)((unsigned short)stats[0]+1U);
    if(rank==1) stats[1]=(short)(unsigned short)((unsigned short)stats[1]+1U);
    return 0;
}
/* 2a66c..2a74d: retain the original all-pairs exchange order. A library
 * sort with different equal-key behavior can change tied driver order. */
static inline void slicks_championship_standings(struct SlicksChampionshipStandings *out,
    const short points[4],const signed char roles[4])
{
    for(unsigned i=0;i<4;++i) { out->driver[i]=(unsigned char)i; out->points[i]=roles[i]?points[i]:-1; }
    for(unsigned i=0;i<4;++i) for(unsigned j=0;j<4;++j) if(out->points[i]>out->points[j]) {
        short value=out->points[i]; out->points[i]=out->points[j]; out->points[j]=value;
        unsigned char driver=out->driver[i]; out->driver[i]=out->driver[j]; out->driver[j]=driver;
    }
    unsigned char place=0;
    for(unsigned row=0;row<4;++row) {
        if(!row || out->points[row]!=out->points[row-1]) place=(unsigned char)(row+1);
        out->place[row]=place;
    }
}
/* 2aa0b..2aa6b: only positive scores count as a match; every tied winner
 * receives a win. Shared profiles deliberately accumulate per entrant. */
static inline int slicks_championship_statistics(struct SlicksPlayerProfiles *profiles,
    const short selected[4],const struct SlicksChampionshipStandings *standings)
{
    for(unsigned row=0;row<4;++row) if(standings->points[row]>0) {
        unsigned driver=standings->driver[row];
        if(driver>=4 || selected[driver]<0 || selected[driver]>=profiles->count) return -1;
    }
    for(unsigned row=0;row<4;++row) if(standings->points[row]>0) {
        short *stats=profiles->statistics[selected[standings->driver[row]]];
        stats[2]=(short)(unsigned short)((unsigned short)stats[2]+1U);
        if(standings->place[row]==1) stats[3]=(short)(unsigned short)((unsigned short)stats[3]+1U);
    }
    return 0;
}
#endif
