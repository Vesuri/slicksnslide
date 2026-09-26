#ifndef SLICKS_SPRITE_OPACITY_H
#define SLICKS_SPRITE_OPACITY_H

/* Read-only asset metadata: (background & mask) | source implements the
 * original zero-is-transparent rule for byte, word and longword copies. */
static inline unsigned slicks_make_sprite_opacity(const unsigned char *pixels,
    unsigned count,unsigned char *mask,unsigned capacity)
{
    if(!count || count>capacity)return 0;
    for(unsigned i=0;i<count;++i)mask[i]=pixels[i]?0:255;
    return count;
}
#endif
