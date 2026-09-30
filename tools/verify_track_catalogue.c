#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/game/track_catalogue.h"

/* Independent former production algorithm; full records, not just stems. */
static void reference(char a[][12],unsigned n)
{
    for(unsigned i=0;i+1<n;++i)for(unsigned j=i+1;j<n;++j)
        if(slicks_track_stem_compare(a[j],a[i])<0) {
            char tmp[12];memcpy(tmp,a[i],12);memcpy(a[i],a[j],12);memcpy(a[j],tmp,12);
        }
}
int main(void)
{
    char a[129][12],expected[129][12];unsigned short order[129];
    unsigned state=17;
    for(unsigned trial=0;trial<2000;++trial) {
        unsigned n=trial%130;
        memset(a,0x5a,sizeof a);
        for(unsigned i=0;i<n;++i) {
            state=state*1664525U+1013904223U;
            unsigned key=trial&1?state%23:i;
            snprintf(a[i],12,"%c%07u.%s",trial&2?'a':'A',key,i&1?"ss":"SS");
        }
        for(unsigned i=n;i>1;--i) {
            state=state*1664525U+1013904223U;unsigned j=state%i;
            char tmp[12];memcpy(tmp,a[i-1],12);memcpy(a[i-1],a[j],12);memcpy(a[j],tmp,12);
        }
        memcpy(expected,a,sizeof a);reference(expected,n);
        slicks_track_catalogue_sort(a,n,trial%7?order:NULL);
        assert(!memcmp(a,expected,sizeof a));
    }
    char (*large)[12]=calloc(10000,12);unsigned short *indices=malloc(10000*sizeof *indices);
    assert(large && indices);
    for(unsigned i=0;i<10000;++i)snprintf(large[i],12,"T%07u.SS",10000-i);
    slicks_track_catalogue_sort(large,10000,indices);
    for(unsigned i=0;i<10000;++i) {
        char expected_name[12];snprintf(expected_name,12,"T%07u.SS",i+1);
        assert(!memcmp(large[i],expected_name,12));
    }
    free(indices);free(large);
    puts("Catalogue: 2000 full-record equivalence cases, scratch-null fallback, 10000 descending names passed");
}
