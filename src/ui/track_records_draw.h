#ifndef SLICKS_TRACK_RECORDS_DRAW_H
#define SLICKS_TRACK_RECORDS_DRAW_H
#include "../game/track_records.h"

enum SlicksRecordsDrawCommand {
    SLICKS_RECORDS_NEAREST,SLICKS_RECORDS_HIGHLIGHT,SLICKS_RECORDS_COLOUR,
    SLICKS_RECORDS_RECTANGLE,SLICKS_RECORDS_NUMBER,SLICKS_RECORDS_TEXT,
    SLICKS_RECORDS_TIME,SLICKS_RECORDS_CHARACTER,SLICKS_RECORDS_ICON
};
struct SlicksRecordsDrawOps {
    short (*emit)(void *,enum SlicksRecordsDrawCommand,const short *,const unsigned char *);
    void *context;
};
/* Original 1aabc..1ae8c, including colour changes for empty rows. Commands
 * retain original coordinates/flags. Font IDs 0/1 are kirj/pieni; icon -1
 * denotes the computer-driver marker, other IDs the original vehicle table.
 * Rectangle endpoints retain DOS semantics for the downstream painter. */
static inline void slicks_draw_track_records(const struct SlicksTrackRecords *records,
    const signed char ranks[4],short x,short y,unsigned char separator,signed char date_order,
    const struct SlicksRecordsDrawOps *ops)
{
    short a[8]={20,20,20},colour=ops->emit(ops->context,SLICKS_RECORDS_NEAREST,a,0);
    a[0]=colour; ops->emit(ops->context,SLICKS_RECORDS_HIGHLIGHT,a,0);
    for(unsigned car=0;car<4;++car) if(ranks[car]>0 && ranks[car]<=10)
        for(unsigned stripe=0;stripe<5;++stripe) {
            short top=(short)(y+9*ranks[car]+stripe+20);
            short rectangle[8]={x,top,(short)(x+225),(short)(top+2),(short)(car*5+stripe+1)};
            ops->emit(ops->context,SLICKS_RECORDS_RECTANGLE,rectangle,0);
        }
    for(unsigned row=1;row<=10;++row) {
        short rgb[8]={63,(short)(63-2*row),13};
        colour=ops->emit(ops->context,SLICKS_RECORDS_NEAREST,rgb,0);
        for(unsigned font=0;font<2;++font) {
            short args[8]={(short)font,colour};
            ops->emit(ops->context,SLICKS_RECORDS_COLOUR,args,0);
        }
        const unsigned char *r=records->entries[row];
        short time=(short)(r[20]|r[21]<<8);
        if(time<=1 || time>=30000) continue;
        short py=(short)(y+9*row+20);
        short number[8]={(short)(x+5),py,(short)row,0,0};
        ops->emit(ops->context,SLICKS_RECORDS_NUMBER,number,0);
        short text[8]={(short)(x+20+((r[27]&2)<<2)),py,0,4};
        ops->emit(ops->context,SLICKS_RECORDS_TEXT,text,r);
        if(r[27]&2) {
            short icon[8]={(short)(x+20),py,-1};
            ops->emit(ops->context,SLICKS_RECORDS_ICON,icon,0);
        }
        short timer[8]={(short)(x+137),py,time,0,0,1};
        ops->emit(ops->context,SLICKS_RECORDS_TIME,timer,0);
        if(r[22]) {
            for(unsigned i=0;i<2;++i) {
                short c[8]={(short)(x+181+11*i),(short)(py+1),separator,1,0};
                ops->emit(ops->context,SLICKS_RECORDS_CHARACTER,c,0);
            }
            short month[8]={(short)(x+177+11*date_order),(short)(py+1),(signed char)r[23],1,1};
            short day[8]={(short)(x+188-11*date_order),(short)(py+1),(signed char)r[22],1,1};
            short year[8]={(short)(x+196),(short)(py+1),(short)(r[24]|r[25]<<8),1,0};
            ops->emit(ops->context,SLICKS_RECORDS_NUMBER,month,0);
            ops->emit(ops->context,SLICKS_RECORDS_NUMBER,day,0);
            ops->emit(ops->context,SLICKS_RECORDS_NUMBER,year,0);
        }
        if(r[26]) {
            short icon[8]={(short)(x+215),py,(signed char)r[26]};
            ops->emit(ops->context,SLICKS_RECORDS_ICON,icon,0);
        }
    }
}
#endif
