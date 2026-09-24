#include "resource_archive.h"

#ifndef SLICKS_ARCHIVE_HOST_TEST
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#endif

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
    unsigned long directory_size;
    archive->file = Open((CONST_STRPTR)path, MODE_OLDFILE);
    archive->count = 0;
    archive->directory = 0;
    if (!archive->file)
        return -1;
    if (Read(archive->file, header, sizeof(header)) != sizeof(header) ||
        header[0] != 'M' || header[1] != 'F' || header[2] != 0x1a) {
        slicks_resource_archive_close(archive);
        return -1;
    }
    archive->count = ((unsigned short)header[3] << 8) | header[4];
    if (!archive->count) {
        slicks_resource_archive_close(archive);
        return -1;
    }
    directory_size = (unsigned long)archive->count * 19UL;
    archive->directory = (unsigned char *)AllocMem(directory_size, MEMF_ANY);
    if (!archive->directory ||
        Read(archive->file, archive->directory, (LONG)directory_size) !=
            (LONG)directory_size) {
        slicks_resource_archive_close(archive);
        return -1;
    }
    return 0;
}

void slicks_resource_archive_close(struct SlicksResourceArchive *archive)
{
    if (archive->directory)
        FreeMem(archive->directory, (unsigned long)archive->count * 19UL);
    if (archive->file)
        Close(archive->file);
    archive->file = 0;
    archive->count = 0;
    archive->directory = 0;
}

long slicks_resource_archive_load(struct SlicksResourceArchive *archive,
                                  const char *name, void *destination,
                                  unsigned long capacity)
{
    unsigned short index;

    if (!archive->file || !archive->directory)
        return -1;
    for (index = 0; index < archive->count; ++index) {
        const unsigned char *entry = archive->directory +
            (unsigned long)index * 19UL;
        unsigned long start;
        unsigned long end;
        unsigned long size;
        if (!resource_name_matches(entry, name))
            continue;
        start = read_u24_be(entry + 16);
        if (index + 1 < archive->count) {
            end = read_u24_be(entry + 19 + 16);
        } else {
            /* HELP.TXT is the final resource, without a sentinel entry.
             * AmigaDOS Seek returns the previous position, not the new one. */
            if (Seek(archive->file, 0, OFFSET_END) < 0) return -1;
            LONG position = Seek(archive->file, 0, OFFSET_CURRENT);
            if (position < 0) return -1;
            end = (unsigned long)position;
        }
        /* Marker entries such as the first duplicate "car9" deliberately
         * have zero length.  Keep looking for a later data entry of the same
         * name instead of treating the marker as the requested resource. */
        if (end == start)
            continue;
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
