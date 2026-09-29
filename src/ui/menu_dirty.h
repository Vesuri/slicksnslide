#ifndef SLICKS_MENU_DIRTY_H
#define SLICKS_MENU_DIRTY_H
/* Painter-reported half-open rectangles, aligned to C2P's 16-pixel blocks.
 * Overflow retains the combined bounds, never discards a pending repaint. */
struct SlicksMenuRect { unsigned short left,top,right,bottom; };
static inline void slicks_menu_dirty_add(struct SlicksMenuRect rects[16],
    unsigned short *count,int left,int top,int right,int bottom)
{
    if(left<0) left=0;
    if(top<0) top=0;
    if(right>320) right=320;
    if(bottom>200) bottom=200;
    if(left>=right || top>=bottom) return;
    left&=~15; right=(right+15)&~15;
    for(unsigned i=0;i<*count;) {
        struct SlicksMenuRect *r=&rects[i];
        if(right<r->left || left>r->right || bottom<r->top || top>r->bottom) {
            ++i; continue;
        }
        if(left>r->left) left=r->left;
        if(top>r->top) top=r->top;
        if(right<r->right) right=r->right;
        if(bottom<r->bottom) bottom=r->bottom;
        rects[i]=rects[--*count]; i=0;
    }
    if(*count==16) {
        for(unsigned i=0;i<16;++i) {
            if(left>rects[i].left) left=rects[i].left;
            if(top>rects[i].top) top=rects[i].top;
            if(right<rects[i].right) right=rects[i].right;
            if(bottom<rects[i].bottom) bottom=rects[i].bottom;
        }
        *count=0;
    }
    rects[(*count)++]=(struct SlicksMenuRect){left,top,right,bottom};
}
#endif
