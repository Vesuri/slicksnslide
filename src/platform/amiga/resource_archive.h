#ifndef SLICKS_RESOURCE_ARCHIVE_H
#define SLICKS_RESOURCE_ARCHIVE_H

#ifndef SLICKS_ARCHIVE_HOST_TEST
#include <exec/types.h>
#include <dos/dos.h>
#endif

struct SlicksResourceCache;
struct SlicksResourceArchive {
    BPTR file;
    unsigned short count;
    unsigned char *directory;
    const struct SlicksResourceCache *cache;
};

int slicks_resource_archive_open(struct SlicksResourceArchive *archive,
                                 const char *path);
void slicks_resource_archive_close(struct SlicksResourceArchive *archive);
long slicks_resource_archive_load(struct SlicksResourceArchive *archive,
                                  const char *name, void *destination,
                                  unsigned long capacity);

/* Explicit memory-only provider: a miss never opens a disk archive. The cache
 * owns copies of names/data and must outlive every borrowed archive handle. */
struct SlicksResourceCache *slicks_resource_cache_create(
    struct SlicksResourceArchive *disk, const char *const *names,
    unsigned short count);
void slicks_resource_cache_destroy(struct SlicksResourceCache *cache);
int slicks_resource_archive_cached(struct SlicksResourceArchive *archive,
    const struct SlicksResourceCache *cache);
unsigned long slicks_resource_cache_bytes(const struct SlicksResourceCache *cache);

#endif
