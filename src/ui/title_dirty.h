#ifndef SLICKS_TITLE_DIRTY_H
#define SLICKS_TITLE_DIRTY_H
/* Painter-reported, half-open bounds; no framebuffer scan or shadow copy.
 * Align horizontally for the eight-plane, 16-pixel block converter. */
struct SlicksTitleRect { unsigned short left,top,right,bottom; };
struct SlicksTitleDirty { struct SlicksTitleRect rects[4]; unsigned count; };
static inline void slicks_title_dirty_add(struct SlicksTitleDirty *d,
    unsigned left,unsigned top,unsigned right,unsigned bottom)
{
    if(right>320) right=320;
    if(bottom>200) bottom=200;
    if(left>=right || top>=bottom) return;
    struct SlicksTitleRect r={left&~15U,top,(right+15)&~15U,bottom};
    for(unsigned i=0;i<d->count;) {
        struct SlicksTitleRect *p=&d->rects[i];
        if(r.left<=p->right && p->left<=r.right && r.top<=p->bottom && p->top<=r.bottom) {
            if(p->left<r.left) r.left=p->left;
            if(p->top<r.top) r.top=p->top;
            if(p->right>r.right) r.right=p->right;
            if(p->bottom>r.bottom) r.bottom=p->bottom;
            d->rects[i]=d->rects[--d->count];i=0;
        } else ++i;
    }
    if(d->count==4) {d->count=0;r=(struct SlicksTitleRect){0,0,320,200};}
    d->rects[d->count++]=r;
}
static inline void slicks_title_dirty_unpack(const struct SlicksTitleDirty *d,
    const unsigned char *logical,unsigned char *chunky)
{
    for(unsigned i=0;i<d->count;++i) {
        const struct SlicksTitleRect *r=&d->rects[i];
        unsigned offset=r->top*100;
        unsigned char *row=chunky+r->top*320;
        for(unsigned y=r->top;y<r->bottom;++y,offset+=100,row+=320)
            for(unsigned x=r->left;x<r->right;++x)
                row[x]=logical[(x&3)*65536UL+offset+(x>>2)];
    }
}
#endif
