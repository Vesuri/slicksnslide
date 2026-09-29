#ifndef SLICKS_TITLE_DEMO_H
#define SLICKS_TITLE_DEMO_H
#include "../game/configuration.h"
#include "../game/track_playlist.h"

/* Native ownership of original DS:6b52/6bcc/6b50 backup. Only the option
 * values in the copied 0090..0109 records are mutable in the native port.
 * Do not snapshot the RNG, track-list tail, input bindings or player data. */
struct SlicksTitleDemo {
    short options[15],profiles[4],first_track;
    unsigned short count;
    unsigned char active;
};

/* Original 23f65..23ff8. A negative flag denotes the title's idle demo;
 * positive values belong to a separate mode. F11/F12 take a pixel-view
 * diagnostic branch, not the demo exit. Keyboard release/idle scans remain
 * >=128, as returned by the original keyboard latch. */
static inline int slicks_title_demo_exit_key(signed char flag,short scan)
{ return flag<0 && scan<128 && scan!=0x57 && scan!=0x58; }

/* Original 2a3db..2a4c9. The selected profiles here are the pre-selection
 * inputs: the subsequent original Arcade override is a separate stage. */
static inline int slicks_title_demo_begin(struct SlicksTitleDemo *demo,
    struct SlicksConfiguration *config,struct SlicksTrackPlaylist *playlist,
    unsigned total,unsigned long *seed,short *selection)
{
    if(demo->active || !slicks_track_playlist_valid(playlist) ||
        !playlist->capacity || !total || total>32767 || !seed) return -1;
    for(unsigned i=0;i<15;++i) demo->options[i]=config->options[i];
    for(unsigned i=0;i<4;++i) {
        demo->profiles[i]=config->selected_profile[i];
        config->selected_profile[i]=1;
    }
    demo->count=playlist->count;demo->first_track=playlist->tracks[0];
    *selection=0;demo->active=1;
    config->options[0]=5;config->options[7]=1;
    config->options[8]=config->options[9]=config->options[10]=0;
    config->options[3]=3;config->options[13]=20;
    playlist->count=1;
    playlist->tracks[0]=(short)slicks_track_random_index(total,seed);
    return 0;
}

/* Original title re-entry 2a2f4..2a343. The demo consumed random numbers;
 * restoration must not rewind them. No profile statistics are copied here. */
static inline void slicks_title_demo_restore(struct SlicksTitleDemo *demo,
    struct SlicksConfiguration *config,struct SlicksTrackPlaylist *playlist,
    unsigned char *refresh)
{
    if(!demo->active) return;
    for(unsigned i=0;i<15;++i) config->options[i]=demo->options[i];
    for(unsigned i=0;i<4;++i) config->selected_profile[i]=demo->profiles[i];
    playlist->count=demo->count;playlist->tracks[0]=demo->first_track;
    demo->active=0;*refresh=2;
}
#endif
