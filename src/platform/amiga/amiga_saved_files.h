#ifndef SLICKS_AMIGA_SAVED_FILES_H
#define SLICKS_AMIGA_SAVED_FILES_H
/* OS available. DOS 8.3 basenames in the game directory, never paths. */
int slicks_saved_file_path(char path[13],const unsigned char *name);
int slicks_amiga_saved_files(unsigned char names[40][9]);
int slicks_amiga_saved_file_exists(const char *path);
int slicks_amiga_saved_file_delete(const char *path);
#endif
