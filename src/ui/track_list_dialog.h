#ifndef SLICKS_TRACK_LIST_DIALOG_H
#define SLICKS_TRACK_LIST_DIALOG_H

enum SlicksTrackListAction { SLICKS_TRACK_LIST_EXIT,SLICKS_TRACK_LIST_LOAD,
    SLICKS_TRACK_LIST_NAME,SLICKS_TRACK_LIST_CONFIRM_DELETE };
struct SlicksTrackListChoice { enum SlicksTrackListAction action; short index; };
/* Original Lists caller 2d7b5..2d88f. Shared list renderer returns its
 * selected row plus 4096*action, or -1 for cancellation. Preserve actual
 * comparison limits (including the 4095 overlap), not guessed bit masks. */
static inline struct SlicksTrackListChoice slicks_track_list_choice(
    short result,short lists,short selected_tracks)
{
    if(result>=0 && result<4096 && lists>0)
        return (struct SlicksTrackListChoice){SLICKS_TRACK_LIST_LOAD,result};
    if(result>=4095 && result<8000 && selected_tracks>0)
        return (struct SlicksTrackListChoice){SLICKS_TRACK_LIST_NAME,-1};
    if(result>=8192 && result<12200)
        return (struct SlicksTrackListChoice){SLICKS_TRACK_LIST_CONFIRM_DELETE,(short)(result-8192)};
    return (struct SlicksTrackListChoice){SLICKS_TRACK_LIST_EXIT,-1};
}
/* Name dialog's AL=0 accepts; confirmation requires the original Y scan. */
static inline int slicks_track_list_name_accepted(unsigned char result) { return result==0; }
static inline int slicks_track_list_delete_accepted(unsigned char scan) { return scan==0x15; }
#endif
