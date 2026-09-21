#include "resource_archive.h"

#include <dos/dos.h>
#include <proto/dos.h>

static int resource_name_matches(const unsigned char *field, const char *name)
{
    unsigned short index;
    for (index = 0; index < 16; ++index) {
        unsigned char expected = (unsigned char)name[index];
        if (field[index] != expected)
            return 0;
        if (!expected)
            return 1;
    }
    return name[16] == 0;
}

static unsigned long read_u24_be(const unsigned char *bytes)
{
    return ((unsigned long)bytes[0] << 16) |
           ((unsigned long)bytes[1] << 8) | bytes[2];
}

int slicks_resource_archive_open(struct SlicksResourceArchive *archive,
                                 const char *path)
{
    unsigned char header[5];
    archive->file = Open((CONST_STRPTR)path, MODE_OLDFILE);
    archive->count = 0;
    if (!archive->file)
        return -1;
    if (Read(archive->file, header, sizeof(header)) != sizeof(header) ||
        header[0] != 'M' || header[1] != 'F' || header[2] != 0x1a) {
        slicks_resource_archive_close(archive);
        return -1;
    }
    archive->count = ((unsigned short)header[3] << 8) | header[4];
    return archive->count ? 0 : -1;
}

void slicks_resource_archive_close(struct SlicksResourceArchive *archive)
{
    if (archive->file)
        Close(archive->file);
    archive->file = 0;
    archive->count = 0;
}

long slicks_resource_archive_load(struct SlicksResourceArchive *archive,
                                  const char *name, void *destination,
                                  unsigned long capacity)
{
    unsigned char entry[19];
    unsigned char next_entry[19];
    unsigned short index;

    if (!archive->file || Seek(archive->file, 5, OFFSET_BEGINNING) < 0)
        return -1;
    for (index = 0; index < archive->count; ++index) {
        unsigned long start;
        unsigned long end;
        unsigned long size;
        if (Read(archive->file, entry, sizeof(entry)) != sizeof(entry))
            return -1;
        if (!resource_name_matches(entry, name))
            continue;
        if (index + 1 >= archive->count ||
            Read(archive->file, next_entry, sizeof(next_entry)) !=
                sizeof(next_entry))
            return -1;
        start = read_u24_be(entry + 16);
        end = read_u24_be(next_entry + 16);
        if (end < start || end - start > capacity)
            return -1;
        size = end - start;
        if (Seek(archive->file, (LONG)start, OFFSET_BEGINNING) < 0 ||
            Read(archive->file, destination, (LONG)size) != (LONG)size)
            return -1;
        return (long)size;
    }
    return -1;
}
