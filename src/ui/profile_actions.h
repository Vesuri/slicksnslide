#ifndef SLICKS_PROFILE_ACTIONS_H
#define SLICKS_PROFILE_ACTIONS_H

enum SlicksProfileListAction {
    SLICKS_PROFILE_LIST_NONE, SLICKS_PROFILE_LIST_EDIT,
    SLICKS_PROFILE_LIST_CONFIRM_DELETE
};

/* Original 28b00..28b48. The caller validates the returned profile index
 * against allocated profiles before touching storage. Do not interpret an
 * action-column cancel as an edit, or apply edit protection to deletion:
 * the original only checks flag bit 2 on the edit branch. */
static inline enum SlicksProfileListAction slicks_profile_list_action(
    short result,unsigned char profile_flags,short *index)
{
    if(result>1 && result<0x1000) {
        if(profile_flags&4) return SLICKS_PROFILE_LIST_NONE;
        *index=result; return SLICKS_PROFILE_LIST_EDIT;
    }
    if(result>0x1000 && result<0x1fa4) {
        *index=result&0xfff; return SLICKS_PROFILE_LIST_CONFIRM_DELETE;
    }
    return SLICKS_PROFILE_LIST_NONE;
}

/* 28c29..28c3d: original localized yes keys Y/K/J, not Return/Space. */
static inline int slicks_profile_delete_confirmed(unsigned short scan)
{ return scan==0x15 || scan==0x25 || scan==0x24; }
#endif
