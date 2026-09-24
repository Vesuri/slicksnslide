#ifndef SLICKS_LIST_CAPTIONS_H
#define SLICKS_LIST_CAPTIONS_H

struct SlicksListCaptionOps {
    short (*measure)(void *,const unsigned char *);
    unsigned char (*colour)(void *,unsigned char); /* returns prior font colour */
    unsigned char (*nearest)(void *,unsigned char,unsigned char,unsigned char);
    void (*text)(void *,const unsigned char *,short,short,unsigned char);
    void *context;
};

/* Original 30b5b..30cc3: comma-separated, right-aligned action captions.
 * A leading '-' is skipped. Negative signed selection measures only; drawing
 * returns count*256 (width stays zero). DOS temporarily terminates each label
 * in-place; a bounded local copy keeps original resource strings immutable.
 * Caller ensures at most 255 bytes before the terminator. Failure is atomic. */
static inline int slicks_list_captions(const unsigned char *captions,
    short x,short y,unsigned char font_height,unsigned char selected,
    unsigned char highlight,const struct SlicksListCaptionOps *ops)
{
    if(!captions || !ops) return -1;
    unsigned size=0; while(size<256 && captions[size]) ++size;
    if(size==256) return -1;
    unsigned start=0,count=0; short width=0;
    y=(short)(y+3);
    for(unsigned end=0;end<=size;++end) if(!captions[end] || captions[end]==',') {
        if(captions[start]=='-') ++start;
        unsigned char label[256]; unsigned n=0;
        while(start<end) label[n++]=captions[start++];
        label[n]=0;
        if((signed char)selected<0) {
            short measured=ops->measure(ops->context,label);
            if(measured>width) width=measured;
        } else {
            unsigned char colour=(short)(signed char)selected==(short)count?highlight:
                ops->nearest(ops->context,40,40,40);
            unsigned char previous=ops->colour(ops->context,colour);
            ops->text(ops->context,label,(short)(x-1),y,2);
            ops->colour(ops->context,previous);
        }
        start=end+1; ++count; y=(short)(y+font_height+1);
    }
    return (unsigned short)(count*256U+(unsigned short)width);
}

struct SlicksListCaptionLayout {
    const unsigned char *labels;
    short count,width,height;
    unsigned char action;
};

/* Caller 30d28..30dc1: optional signed control byte selects the action.
 * Preserve the executable's unusual '-' rule: it assigns total count, not
 * the marked label index. The following dialog-init clamp then resets it.
 * Empty captions bypass measurement entirely, unlike the helper above. */
static inline int slicks_list_caption_layout(struct SlicksListCaptionLayout *out,
    const unsigned char *captions,unsigned char font_height,unsigned char action,
    const struct SlicksListCaptionOps *ops)
{
    if(!out || !captions || !ops) return -1;
    unsigned size=0; while(size<256 && captions[size]) ++size;
    if(size==256) return -1;
    struct SlicksListCaptionLayout next={captions,0,0,0,action};
    if(size) {
        if((signed char)*captions<10) next.action=*captions++;
        int packed=slicks_list_captions(captions,0,0,font_height,255,40,ops);
        if(packed<0) return -1;
        next.labels=captions; next.count=(short)((unsigned)packed>>8);
        next.width=(short)(packed&255); next.height=(short)((font_height+1)*next.count);
        unsigned start=1;
        for(unsigned i=0;captions[i];++i) {
            if(start && captions[i]=='-') next.action=(unsigned char)next.count;
            start=captions[i]==',';
        }
    }
    *out=next;
    return 0;
}
#endif
