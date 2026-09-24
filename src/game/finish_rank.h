#ifndef SLICKS_FINISH_RANK_H
#define SLICKS_FINISH_RANK_H

/* 22b89..22bd6. Unfinished ranks start negative; negate this driver's
 * byte and scan the other three slots four times to skip occupied ranks.
 * Inactive slots participate in the comparisons too. */
static inline signed char slicks_assign_finish_rank(signed char ranks[4],unsigned driver)
{
    unsigned char candidate=(unsigned char)(-(int)ranks[driver]);
    for(unsigned pass=0;pass<4;++pass)
        for(unsigned i=0;i<4;++i)
            if(i!=driver && (unsigned char)ranks[i]==candidate) ++candidate;
    ranks[driver]=(signed char)candidate;
    return ranks[driver];
}

/* 22bfd..22c4d, only on the original rank==1 branch. Count other drivers
 * with a strictly greater signed lap WORD, then subtract from every rank
 * BYTE. Equal laps do not move a rank; inactive slots are not filtered. */
static inline void slicks_adjust_finish_laps(signed char ranks[4],const short laps[4])
{
    for(unsigned i=0;i<4;++i) {
        unsigned ahead=0;
        for(unsigned j=0;j<4;++j) ahead+=j!=i && laps[j]>laps[i];
        ranks[i]=(signed char)(unsigned char)((unsigned char)ranks[i]-ahead);
    }
}
#endif
