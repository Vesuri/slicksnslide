#include "resource_archive.h"

#ifndef SLICKS_ARCHIVE_HOST_TEST
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#endif

struct CachedResource {
    unsigned char name[16];
    unsigned char *data;
    unsigned long size, stored;
    unsigned char packed;
};
struct SlicksResourceCache {
    unsigned short count;
    unsigned long bytes;
    struct CachedResource *entries;
};

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

/* Shared diagnostic boundary. External linkage retains the normal stack ABI
 * for entry breakpoints on both startup and reserved-directory opens. */
__attribute__((noinline)) int slicks_resource_archive_open_impl(struct SlicksResourceArchive *archive,
    const char *path,struct SlicksArchiveDirectory *reservation)
{
    unsigned char header[5];
    unsigned long directory_size;
    archive->file = 0;
    archive->count = 0;
    archive->directory = 0;
    archive->cache = 0;
    archive->reservation = 0;
    if(reservation && (!reservation->bytes || reservation->busy)) return -1;
    archive->file = Open((CONST_STRPTR)path, MODE_OLDFILE);
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
    if(reservation) {
        if(directory_size>reservation->capacity) {
            slicks_resource_archive_close(archive); return -1;
        }
        reservation->busy=1;archive->reservation=reservation;
        archive->directory=reservation->bytes;
    } else archive->directory = (unsigned char *)AllocMem(directory_size, MEMF_ANY);
    if (!archive->directory ||
        Read(archive->file, archive->directory, (LONG)directory_size) !=
            (LONG)directory_size) {
        slicks_resource_archive_close(archive);
        return -1;
    }
    return 0;
}
int slicks_resource_archive_open(struct SlicksResourceArchive *archive,const char *path)
{ return slicks_resource_archive_open_impl(archive,path,0); }
int slicks_resource_archive_open_reserved(struct SlicksResourceArchive *archive,const char *path,
    struct SlicksArchiveDirectory *reservation)
{
    if(!reservation) return -1;
    return slicks_resource_archive_open_impl(archive,path,reservation);
}
int slicks_resource_directory_adopt(struct SlicksArchiveDirectory *reservation,struct SlicksResourceArchive *archive)
{
    if(!reservation || reservation->bytes || !archive || !archive->directory || archive->reservation) return -1;
    reservation->bytes=archive->directory;
    reservation->capacity=(unsigned long)archive->count*19UL;
    reservation->busy=1;archive->reservation=reservation;
    return 0;
}
int slicks_resource_directory_destroy(struct SlicksArchiveDirectory *reservation)
{
    if(!reservation || reservation->busy) return -1;
    if(reservation->bytes) FreeMem(reservation->bytes,reservation->capacity);
    reservation->bytes=0;reservation->capacity=0;
    return 0;
}

void slicks_resource_archive_close(struct SlicksResourceArchive *archive)
{
    if(archive->reservation) archive->reservation->busy=0;
    else if (archive->directory)
        FreeMem(archive->directory, (unsigned long)archive->count * 19UL);
    if (archive->file)
        Close(archive->file);
    archive->file = 0;
    archive->count = 0;
    archive->directory = 0;
    archive->cache = 0;
    archive->reservation = 0;
}

static int resource_range(struct SlicksResourceArchive *archive,
    const char *name, unsigned long *offset, unsigned long *length)
{
    unsigned short index;

    if (!archive->file || !archive->directory)
        return -1;
    for (index = 0; index < archive->count; ++index) {
        const unsigned char *entry = archive->directory +
            (unsigned long)index * 19UL;
        unsigned long start;
        unsigned long end;
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
        if (end < start)
            return -1;
        *offset = start;
        *length = end - start;
        return 0;
    }
    return -1;
}

long slicks_resource_archive_load(struct SlicksResourceArchive *archive,
    const char *name, void *destination, unsigned long capacity)
{
    unsigned long offset, size;
    if (archive->cache) {
        const struct SlicksResourceCache *cache = archive->cache;
        for (unsigned i=0; i<cache->count; ++i) {
            const struct CachedResource *e = cache->entries+i;
            if (!resource_name_matches(e->name, name)) continue;
            if (e->size>capacity || !destination) return -1;
            unsigned char *out = destination;
            if (!e->packed) {
                for (unsigned long j=0; j<e->size; ++j) out[j]=e->data[j];
            } else {
                unsigned long at=0;
                for (unsigned long j=0; j<e->stored; j+=2) {
                    unsigned n=e->data[j];
                    if (!n || n>e->size-at) return -1;
                    while (n--) out[at++]=e->data[j+1];
                }
                if (at!=e->size) return -1;
            }
            return (long)e->size;
        }
        return -1;
    }
    if (resource_range(archive,name,&offset,&size) || size>capacity || !destination)
        return -1;
    if (Seek(archive->file,(LONG)offset,OFFSET_BEGINNING)<0 ||
        Read(archive->file,destination,(LONG)size)!=(LONG)size) return -1;
    return (long)size;
}

/* Two streaming passes avoid a 64K temporary allocation. RLE is private,
 * lossless storage of the supplied bytes, never a substitute rendered asset. */
static long pack_resource(BPTR file, unsigned long offset, unsigned long size,
    unsigned char *out, unsigned long capacity)
{
    unsigned char buffer[256], value=0;
    unsigned run=0;
    unsigned long used=0;
    if (Seek(file,(LONG)offset,OFFSET_BEGINNING)<0) return -1;
    while (size) {
        unsigned n=size>sizeof buffer?sizeof buffer:(unsigned)size;
        if (Read(file,buffer,n)!=(LONG)n) return -1;
        for (unsigned i=0; i<n; ++i) {
            if (run && (buffer[i]!=value || run==255)) {
                if (out) {
                    if (used+2>capacity) return -1;
                    out[used]=(unsigned char)run; out[used+1]=value;
                }
                used+=2; run=0;
            }
            value=buffer[i]; ++run;
        }
        size-=n;
    }
    if (run) {
        if (out) {
            if (used+2>capacity) return -1;
            out[used]=(unsigned char)run; out[used+1]=value;
        }
        used+=2;
    }
    return (long)used;
}

void slicks_resource_cache_destroy(struct SlicksResourceCache *cache)
{
    if (!cache) return;
    if (cache->entries) {
        for (unsigned i=0; i<cache->count; ++i) {
            struct CachedResource *e=cache->entries+i;
            if (e->data) FreeMem(e->data,e->stored);
        }
        FreeMem(cache->entries,(unsigned long)cache->count*sizeof *cache->entries);
    }
    FreeMem(cache,sizeof *cache);
}

struct SlicksResourceCache *slicks_resource_cache_create(
    struct SlicksResourceArchive *disk, const char *const *names, unsigned short count)
{
    if (!disk || !disk->file || disk->cache || !names || !count) return 0;
    struct SlicksResourceCache *cache=AllocMem(sizeof *cache,MEMF_ANY);
    if (!cache) return 0;
    cache->count=count;
    cache->bytes=sizeof *cache+(unsigned long)count*sizeof *cache->entries;
    cache->entries=AllocMem((unsigned long)count*sizeof *cache->entries,MEMF_ANY);
    if (!cache->entries) { slicks_resource_cache_destroy(cache); return 0; }
    for (unsigned i=0; i<count; ++i) cache->entries[i].data=0;
    for (unsigned i=0; i<count; ++i) {
        struct CachedResource *e=cache->entries+i;
        unsigned long offset;
        if (resource_range(disk,names[i],&offset,&e->size)) goto failed;
        unsigned j=0;
        while (j<16 && names[i][j]) { e->name[j]=(unsigned char)names[i][j]; ++j; }
        if (j==16 && names[i][j]) goto failed;
        while (j<16) e->name[j++]=0;
        long packed=pack_resource(disk->file,offset,e->size,0,0);
        if (packed<=0) goto failed;
        e->packed=(unsigned long)packed<e->size;
        e->stored=e->packed?(unsigned long)packed:e->size;
        e->data=AllocMem(e->stored,MEMF_ANY);
        if (!e->data) goto failed;
        if (e->packed) {
            if (pack_resource(disk->file,offset,e->size,e->data,e->stored)!=(long)e->stored)
                goto failed;
        } else if (Seek(disk->file,(LONG)offset,OFFSET_BEGINNING)<0 ||
                   Read(disk->file,e->data,(LONG)e->size)!=(LONG)e->size) goto failed;
        cache->bytes+=e->stored;
    }
    return cache;
failed:
    slicks_resource_cache_destroy(cache); return 0;
}

int slicks_resource_archive_cached(struct SlicksResourceArchive *archive,
    const struct SlicksResourceCache *cache)
{
    archive->file=0; archive->directory=0; archive->count=0; archive->cache=cache;archive->reservation=0;
    return cache?0:-1;
}

unsigned long slicks_resource_cache_bytes(const struct SlicksResourceCache *cache)
{
    return cache?cache->bytes:0;
}
