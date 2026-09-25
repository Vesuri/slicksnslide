#ifndef SLICKS_POST_RACE_RECORDS_H
#define SLICKS_POST_RACE_RECORDS_H
#include "track_records.h"

struct SlicksRecordEntrant {
    signed char role,vehicle;
    unsigned char setting;
    const unsigned char *name; /* Original fixed twenty-byte name copy. */
    signed int best_lap;
    short engine,tyres;
};
struct SlicksRecordOutcome {
    signed char ranks[4];
    unsigned char show,changed;
};
/* Original 255ff..25803, including 2e000's signed upgrade qualification.
 * Record zero is untouched here: the file writer promotes record one.
 * The original adjusts earlier entrants' ranks only for strictly greater
 * positions, not equal positions. Preserve that observable tie behavior. */
static inline struct SlicksRecordOutcome slicks_post_race_records(
    struct SlicksTrackRecords *records,const struct SlicksRecordEntrant entrants[4],
    unsigned char day,unsigned char month,unsigned short year)
{
    struct SlicksRecordOutcome out={{0,0,0,0},0,0};
    for(unsigned driver=0;driver<4;++driver) {
        const struct SlicksRecordEntrant *e=&entrants[driver];
        if(!e->role || e->setting>100 || e->best_lap>=30000) continue;
        out.show=1; /* Also show existing records for an upgraded entrant. */
        if(e->engine>4 || e->tyres>4) continue;
        unsigned position=1;
        for(;position<=10;++position) {
            const unsigned char *r=records->entries[position];
            short time=(short)(r[20]|r[21]<<8);
            if(!time || (signed int)time>e->best_lap) break;
        }
        if(position>10) continue;
        out.ranks[driver]=(signed char)position;
        records->trailer=(unsigned short)(records->trailer+1U);
        for(unsigned i=0;i<driver;++i)
            if(out.ranks[i]>0 && out.ranks[i]>out.ranks[driver]) ++out.ranks[i];
        for(unsigned row=10;row>position;--row)
            for(unsigned j=0;j<29;++j) records->entries[row][j]=records->entries[row-1][j];
        unsigned char *r=records->entries[position];
        for(unsigned j=0;j<20;++j) r[j]=e->name[j];
        unsigned short time=(unsigned short)e->best_lap;
        r[20]=(unsigned char)time; r[21]=(unsigned char)(time>>8);
        r[22]=day; r[23]=month; r[24]=(unsigned char)year; r[25]=(unsigned char)(year>>8);
        r[26]=(unsigned char)(e->vehicle+1);
        short role=(short)(e->role+2);
        r[27]=(unsigned char)role; r[28]=(unsigned char)((unsigned short)role>>8);
        out.changed=1;
    }
    return out;
}
#endif
