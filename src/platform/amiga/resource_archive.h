#ifndef SLICKS_RESOURCE_ARCHIVE_H
#define SLICKS_RESOURCE_ARCHIVE_H

#ifndef SLICKS_ARCHIVE_HOST_TEST
#include <exec/types.h>
#include <dos/dos.h>
#endif

struct SlicksResourceArchive {
    BPTR file;
    unsigned short count;
    unsigned char *directory;
};

int slicks_resource_archive_open(struct SlicksResourceArchive *archive,
                                 const char *path);
void slicks_resource_archive_close(struct SlicksResourceArchive *archive);
long slicks_resource_archive_load(struct SlicksResourceArchive *archive,
                                  const char *name, void *destination,
                                  unsigned long capacity);

#endif
