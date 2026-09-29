#ifndef SLICKS_AMIGA_SAVED_FILES_H
#define SLICKS_AMIGA_SAVED_FILES_H
/* OS available. DOS 8.3 basenames in the game directory, never paths. */
int slicks_saved_file_path(char path[13],const unsigned char *name);
int slicks_amiga_saved_files(unsigned char names[40][9]);
struct SlicksAmigaSavedFilesCache {
    unsigned char names[40][9];
    int count; /* Negative enumeration status forbids using retained names. */
};
/* OS available, startup or explicit file-operation boundary only. Failure
 * preserves the previous name bytes, but publishes the error/overflow status. */
void slicks_amiga_saved_files_refresh(struct SlicksAmigaSavedFilesCache *);
int slicks_amiga_saved_file_exists(const char *path);
int slicks_amiga_saved_file_delete(const char *path);
#endif
