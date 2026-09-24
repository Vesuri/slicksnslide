#ifndef SLICKS_SAVED_FILE_DIALOG_H
#define SLICKS_SAVED_FILE_DIALOG_H
enum SlicksSavedFileAction { SLICKS_SAVED_FILE_CANCEL,SLICKS_SAVED_FILE_SELECT,
    SLICKS_SAVED_FILE_NAME,SLICKS_SAVED_FILE_DELETE };
struct SlicksSavedFileChoice { enum SlicksSavedFileAction action; short index; };
/* Original common .SSS picker caller 1d823..1d913. These comparisons are
 * decimal 4000/8000, NOT the list widget's 4096 action bit boundaries.
 * Deletion deliberately retains only the low eight bits of its row. */
static inline struct SlicksSavedFileChoice slicks_saved_file_choice(short result,unsigned char saving)
{
    if(result>8000 || (result>4000 && !saving))
        return (struct SlicksSavedFileChoice){SLICKS_SAVED_FILE_DELETE,(short)((unsigned short)result&255)};
    if(result>4000) return (struct SlicksSavedFileChoice){SLICKS_SAVED_FILE_NAME,-1};
    if(result>=0) return (struct SlicksSavedFileChoice){SLICKS_SAVED_FILE_SELECT,result};
    return (struct SlicksSavedFileChoice){SLICKS_SAVED_FILE_CANCEL,-1};
}
static inline int slicks_saved_file_delete_accepted(unsigned char scan) { return scan==0x15; }
#endif
