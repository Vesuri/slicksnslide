#ifndef SLICKS_HELP_LINE_H
#define SLICKS_HELP_LINE_H
#include "help_text.h"
struct SlicksHelpStyle {
    short left,top,right,bottom,selected,links,total_links,country;
    unsigned char colours[5],height,centred,link_mode;
    signed char distance,spacing,link_type;
    unsigned char prefix[6],next[21],previous[21],target[21];
};
struct SlicksHelpDrawOps {
    void *context;
    void (*colour)(void *,unsigned char);
    unsigned char (*nearest)(void *,unsigned char,unsigned char,unsigned char);
    void (*rectangle)(void *,short,short,short,short,unsigned char);
    short (*measure)(void *,const unsigned char *,signed char);
    short (*text)(void *,const unsigned char *,short,short,signed char);
};
static inline void slicks_help_copy_topic(unsigned char *out,const unsigned char *in,unsigned limit)
{
    unsigned n=0;
    while(n<limit && (signed char)in[n]>=14 && in[n]!='>') { out[n]=in[n]; ++n; }
    out[n]=0;
}
static inline short slicks_help_decimal(const unsigned char *s)
{
    while(*s==' ' || (*s>=9 && *s<=13)) ++s;
    int sign=1; if(*s=='-' || *s=='+') { if(*s=='-') sign=-1; ++s; }
    unsigned value=0;
    while(*s>='0' && *s<='9') { value=(value*10+*s-'0')&65535U; ++s; }
    return (short)(unsigned short)(sign*(int)value);
}
/* Original 31a1c: only spaces/digits are consumed; no signed arguments. */
static inline unsigned slicks_help_number(const unsigned char *s,unsigned at,short *out)
{
    unsigned found=0;
    for(;;++at) {
        unsigned c=s[at];
        if(c>='0' && c<='9') { if(!found) *out=slicks_help_decimal(s+at); found=1; }
        else if(found || c!=' ') return at;
    }
}
static inline short slicks_help_draw_span(struct SlicksHelpStyle *s,const struct SlicksHelpDrawOps *ops,
    unsigned char *text,unsigned length,short x,short y,short link)
{
    unsigned char saved=text[length]; text[length]=0;
    if(s->centred) x=(short)((short)(s->left+s->right)/2-ops->measure(ops->context,text,s->spacing)/2);
    if((short)(link-2)==s->selected)
        ops->rectangle(ops->context,x,y,(short)(x+ops->measure(ops->context,text,s->spacing)),
            (short)(y+s->height),s->colours[3]);
    x=(short)(x+ops->text(ops->context,text,x,y,s->spacing));
    text[length]=saved; return x;
}
/* Original 31b53..3201f. Input is one preprocessed, NUL-terminated line.
 * Padding is local: original command dispatch reads fixed character offsets
 * even on short commands. No wrapping or alternative markup semantics. */
static inline int slicks_help_draw_line(struct SlicksHelpStyle *s,const unsigned char *source,
    short *y,const struct SlicksHelpDrawOps *ops)
{
    unsigned char line[512]={0}; unsigned length=0;
    if(!s || !source || !y || !ops) return -1;
    while(length<sizeof line-32 && source[length]) { line[length]=source[length]; ++length; }
    if(length==sizeof line-32) return -1;
    ops->colour(ops->context,s->colours[1]);
    ops->rectangle(ops->context,s->left,*y,s->right,(short)(*y+s->height+s->distance),s->colours[0]);
    short start=length?-1:-2,x=(short)(s->left+4); int link_sign=-1;
    for(unsigned at=0;at<length;++at) {
        unsigned c=line[at];
        if(c!=11) {
            if(c==13 || !c || (s->centred && c==' ') || start>=0) continue;
            start=(short)at; continue;
        }
        if(start>=0) {
            x=slicks_help_draw_span(s,ops,line+start,at-(unsigned)start,x,*y,
                (short)(link_sign*(s->links+1))); start=-2;
        }
        ++at; unsigned command=at;
        switch(line[at]) {
        case '/':
            if(line[at+1]=='c') s->centred=0;
            if(line[at+1]=='a') { ops->colour(ops->context,s->colours[1]); link_sign=-1; }
            break;
        case 'w':
            at=slicks_help_number(line,at+6,&s->left);
            at=slicks_help_number(line,at,&s->top);
            at=slicks_help_number(line,at,&s->right);
            at=slicks_help_number(line,at,&s->bottom); break;
        case 'o':
            if(line[at+7]=='l') s->link_mode=(unsigned char)slicks_help_decimal(line+command+12);
            if(line[at+7]=='c' && slicks_help_decimal(line+at+15)==s->country)
                slicks_help_copy_topic(s->prefix,line+at+19,5);
            break;
        case 'c':
            if(line[at+1]=='o') {
                short slot=0,r=0,g=0,b=0;
                at=slicks_help_number(line,at+5,&slot); at=slicks_help_number(line,at,&r);
                at=slicks_help_number(line,at,&g); at=slicks_help_number(line,at,&b);
                if(slot>=0 && slot<4) s->colours[slot]=ops->nearest(ops->context,(unsigned char)r,(unsigned char)g,(unsigned char)b);
            } else if(line[at+1]=='e') s->centred=1;
            break;
        case 'd':
            if(line[at+4]=='x') s->spacing=(signed char)slicks_help_decimal(line+command+6);
            else s->distance=(signed char)slicks_help_decimal(line+command+5);
            break;
        case 'n': slicks_help_copy_topic(s->next,line+command+5,20); break;
        case 'p': slicks_help_copy_topic(s->previous,line+command+5,20); break;
        case 'h': {
            short thickness=slicks_help_decimal(line+command+3);
            if(thickness<=0 || thickness>10) thickness=2;
            ops->rectangle(ops->context,(short)(x+2),*y,(short)(s->right-4),(short)(*y+thickness),s->colours[2]);
            if(start==-1) start=-3;
            break;
        }
        case 'a': case 'i':
            s->link_type=(signed char)(line[at+1]-105);
            if(s->selected==s->links && s->link_type)
                slicks_help_copy_topic(s->target,line+command+2,20);
            ++s->links; ops->colour(ops->context,s->colours[2]); link_sign=1; ++s->total_links; break;
        }
        /* Original 319f0 skips through '>' or any signed byte <=13. */
        while(at<length && (signed char)line[at]>13 && line[at]!='>') ++at;
    }
    if(start>=0) (void)slicks_help_draw_span(s,ops,line+start,length-(unsigned)start,x,*y,-1);
    if(start!=-1) *y=(short)(*y+(start==-3?3:s->height+s->distance));
    return 0;
}
#endif
