#ifndef SLICKS_PROFILE_PALETTE_H
#define SLICKS_PROFILE_PALETTE_H

/* 19d04..19dad: DS:310c contains four pairs of signed RGB endpoints.
 * Divide the two products separately, truncating toward zero, then add
 * their low bytes. This is not first+(last-first)*shade/4. Only indices
 * 1..20 change; the rest of the supplied 256-colour palette is untouched. */
static inline void slicks_profile_palette(unsigned char palette[768],
    const unsigned char colours[4][6])
{
    for(unsigned car=0;car<4;++car)
        for(unsigned shade=0;shade<5;++shade)
            for(unsigned channel=0;channel<3;++channel) {
                int first=(signed char)colours[car][channel];
                int last=(signed char)colours[car][channel+3];
                palette[(1+car*5+shade)*3+channel]=(unsigned char)(
                    first*(4-(int)shade)/4+last*(int)shade/4);
            }
}
#endif
