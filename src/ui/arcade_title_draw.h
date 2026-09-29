#ifndef SLICKS_ARCADE_TITLE_DRAW_H
#define SLICKS_ARCADE_TITLE_DRAW_H

/* Original 29afa..29f2b. Font IDs: 0=kirj, 1=pieni, 2=iso,
 * 3=the last-loaded font (DS:6bd4), deliberately kept as an alias rather
 * than assuming it means the font subsequently passed to text().
 * Text IDs: 0=players, 1..4="1P".."4P", 5=settings, 6=settings summary.
 * Caller supplies localized strings/formatting and real resource fonts. */
struct SlicksArcadeTitleDrawOps {
    unsigned char (*nearest)(void *,unsigned char,unsigned char,unsigned char);
    void (*colour)(void *,unsigned,unsigned char);
    void (*shadow)(void *,unsigned char);
    void (*restore)(void *,short,short,short,short);
    void (*rectangle)(void *,short,short,short,short,unsigned char);
    void (*text)(void *,unsigned,unsigned,short,short,unsigned char);
    void *context;
};
/* Original sprintf calls use signed 16-bit %d, literal bytes and %% only.
 * Bound caller storage instead of reproducing the DOS stack overflow. */
static inline int slicks_arcade_title_format(unsigned char *out,unsigned capacity,
    const unsigned char *format,const short *values,unsigned count)
{
    unsigned used=0,arg=0;
    while(*format) {
        unsigned char ch=*format++;
        if(ch=='%') {
            ch=*format++;
            if(ch=='d') {
                if(arg==count) return -1;
                int value=values[arg++];unsigned magnitude=value<0?(unsigned)-value:(unsigned)value;
                unsigned char digits[6];unsigned n=0;
                do {digits[n++]=(unsigned char)('0'+magnitude%10);magnitude/=10;} while(magnitude);
                if(value<0) digits[n++]='-';
                if(used+n>=capacity) return -1;
                while(n) out[used++]=digits[--n];
                continue;
            }
            if(ch!='%') return -1;
        }
        if(used+1>=capacity) return -1;
        out[used++]=ch;
    }
    if(used>=capacity) return -1;
    out[used]=0;return 0;
}
static inline void slicks_arcade_title_draw(unsigned char *counter,
    unsigned char *refresh,unsigned char selection,short players,
    const signed char colours[4][6],const struct SlicksArcadeTitleDrawOps *o)
{
    void *p=o->context;
    unsigned char dark=o->nearest(p,22,22,22),bright=o->nearest(p,54,54,54);
    *counter=(unsigned char)(*counter+4);
    unsigned char ramp=(unsigned char)(*counter/4);
    if(ramp>32) ramp=(unsigned char)(63-ramp);
    unsigned char pulse=o->nearest(p,(unsigned char)(ramp+30),
        (unsigned char)(ramp+30),(unsigned char)(ramp+20));
    o->colour(p,1,bright);o->colour(p,2,dark);
    o->shadow(p,o->nearest(p,10,10,20));
    o->restore(p,110,77,100,97);
    if(*refresh) {--*refresh;o->restore(p,210,80,40,51);}
    o->text(p,1,0,120,88,0);
    o->colour(p,2,dark);
    o->colour(p,0,o->nearest(p,70,70,15));
    for(short i=0;i<4;++i) {
        unsigned char colour=o->nearest(p,
            (unsigned char)((colours[i][0]+colours[i][3])/2),
            (unsigned char)((colours[i][1]+colours[i][4])/2),
            (unsigned char)((colours[i][2]+colours[i][5])/2));
        unsigned char label=i+1==players?(selection?bright:pulse):i<players?bright:dark;
        o->colour(p,2,label);
        o->rectangle(p,(short)(120+20*i),96,(short)(138+20*i),111,colour);
        o->text(p,2,(unsigned)(i+1),(short)(129+20*i),98,5);
    }
    o->colour(p,3,selection==1?pulse:bright);
    o->text(p,1,5,120,118,0);
    o->text(p,2,6,120,126,4);
}
#endif
