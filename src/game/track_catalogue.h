#ifndef SLICKS_TRACK_CATALOGUE_H
#define SLICKS_TRACK_CATALOGUE_H

/* Original startup sorts the extension-stripped catalogue (35d28/2c301).
 * Keep the real Amiga filename for disk access, but compare its DOS stem.
 * A '.' is a terminator, not a character ordered after '!'. */
static inline int slicks_track_stem_compare(const char *a,const char *b)
{
    for(unsigned i=0;i<8;++i) {
        unsigned x=(unsigned char)a[i],y=(unsigned char)b[i];
        if(x=='.')x=0;
        if(y=='.')y=0;
        if(x!=y)return x<y?-1:1;
        if(!x)return 0;
    }
    return 0;
}
#endif
