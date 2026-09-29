#ifndef SLICKS_TITLE_START_H
#define SLICKS_TITLE_START_H
#include "../game/track_playlist.h"

/* Original GO at 2a4e7 shuffles when DS:0624 is set. F9 at 2a4c5
 * bypasses that call, although both return 99 and share preparation. */
static inline int slicks_title_start_shuffle(struct SlicksTrackPlaylist *playlist,
    unsigned action,unsigned char random_order,unsigned long *seed)
{
    return action==2 && random_order ? slicks_track_playlist_shuffle(playlist,seed):0;
}
#endif
