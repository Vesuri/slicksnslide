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

#define SLICKS_CATALOGUE_NAME_BYTES 12

static inline void slicks_track_exchange_sort(char names[][SLICKS_CATALOGUE_NAME_BYTES],unsigned count)
{
    for(unsigned left=0;left+1<count;++left)
        for(unsigned right=left+1;right<count;++right)
            if(slicks_track_stem_compare(names[right],names[left])<0)
                for(unsigned byte=0;byte<SLICKS_CATALOGUE_NAME_BYTES;++byte) {
                    char tmp=names[left][byte];
                    names[left][byte]=names[right][byte];names[right][byte]=tmp;
                }
}

static inline void slicks_track_index_sift(const char names[][SLICKS_CATALOGUE_NAME_BYTES],
    unsigned short *order,unsigned root,unsigned count)
{
    unsigned short value=order[root];
    while(root<count/2) {
        unsigned child=root*2+1;
        if(child+1<count && slicks_track_stem_compare(names[order[child]],names[order[child+1]])<0)
            ++child;
        if(slicks_track_stem_compare(names[value],names[order[child]])>=0)break;
        order[root]=order[child];root=child;
    }
    order[root]=value;
}

/* Optional count-word scratch, no recursion and only one filename on stack.
 * Sort indices first, leaving filenames untouched until ties are ruled out.
 * The old exchange sort is not stable: retain its exact full-filename result
 * for equal stems (e.g. extension case variants), and on scratch failure. */
static inline void slicks_track_catalogue_sort(char names[][SLICKS_CATALOGUE_NAME_BYTES],
    unsigned count,unsigned short *order)
{
    if(count<2)return;
    if(!order) { slicks_track_exchange_sort(names,count);return; }
    for(unsigned i=0;i<count;++i)order[i]=(unsigned short)i;
    for(unsigned i=count/2;i>0;--i)slicks_track_index_sift(names,order,i-1,count);
    for(unsigned end=count-1;end>0;--end) {
        unsigned short tmp=order[0];order[0]=order[end];order[end]=tmp;
        slicks_track_index_sift(names,order,0,end);
    }
    for(unsigned i=1;i<count;++i)
        if(!slicks_track_stem_compare(names[order[i-1]],names[order[i]])) {
            slicks_track_exchange_sort(names,count);return;
        }
    for(unsigned i=0;i<count;++i) {
        if(order[i]==i)continue;
        char saved[SLICKS_CATALOGUE_NAME_BYTES];
        for(unsigned byte=0;byte<SLICKS_CATALOGUE_NAME_BYTES;++byte)saved[byte]=names[i][byte];
        unsigned dest=i;
        while(order[dest]!=i) {
            unsigned source=order[dest];
            for(unsigned byte=0;byte<SLICKS_CATALOGUE_NAME_BYTES;++byte)names[dest][byte]=names[source][byte];
            order[dest]=(unsigned short)dest;dest=source;
        }
        for(unsigned byte=0;byte<SLICKS_CATALOGUE_NAME_BYTES;++byte)names[dest][byte]=saved[byte];
        order[dest]=(unsigned short)dest;
    }
}
#endif
