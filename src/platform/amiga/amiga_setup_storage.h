#ifndef SLICKS_AMIGA_SETUP_STORAGE_H
#define SLICKS_AMIGA_SETUP_STORAGE_H
#include "../../game/configuration.h"
#include "../../game/player_profiles.h"
#include "../../game/setup_storage.h"
#include "../../game/track_lists.h"
#include "../../game/saved_game.h"
/* Explicit TRACKSQ/TRACKSC diagnostics only; zero during normal launches. */
extern unsigned char g_slicks_diag_track_read_fault,g_slicks_diag_track_read_reached;
/* OPTIONSBX/Y: one rejected record-save scratch span, never normal input. */
extern unsigned char g_slicks_diag_record_write_alloc_fault,g_slicks_diag_record_write_alloc_reached;

struct SlicksSetupStorageReport {
    enum SlicksSetupSaveResult result;
    long io_error;
    const char *path;
};
/* AmigaOS must be available. Uses original CFG/PLR encoders; signature
 * policy and dirty-state/UI handling remain the caller's responsibility. */
struct SlicksSetupStorageReport slicks_amiga_store_setup(
    const struct SlicksConfiguration *,const struct SlicksPlayerProfiles *,unsigned char,
    unsigned char *,unsigned long);
#define SLICKS_AMIGA_SETUP_BYTES (142UL+3UL+58UL*(SLICKS_PROFILE_MAX-3))
/* Shop Scroll Lock: first free TUNING00..98.BMP; never replace a capture.
 * Caller supplies at least SLICKS_CAPTURE_SIZE bytes, separate from pixels. */
struct SlicksSetupStorageReport slicks_amiga_store_capture(
    const unsigned char *,const unsigned char *,unsigned char *,unsigned long);
/* Caller owns/validates the chosen .SSS path and keeps it alive for report.path.
 * Uses original bytes with the existing new/backup transaction. OS required.
 * Scratch is separate from game/track names and at least saved_game_size(). */
struct SlicksSetupStorageReport slicks_amiga_store_saved_game(
    const char *,const struct SlicksSavedGame *,unsigned char *,unsigned long);
/* Confirmed Clear Top 10s only. Same 8192-byte track limit as the native
 * loader. Does not alter old-format tracks. Report path borrows caller's path;
 * changed is true only after publication, including cleanup-pending status.
 * Caller provides an exclusive 8192-byte scratch span for the transaction. */
struct SlicksSetupStorageReport slicks_amiga_clear_track_records(
    const char *,unsigned char *,unsigned char *,unsigned long);
struct SlicksTrackRecords;
/* Post-race records use the same transactional writer; caller retains its
 * in-memory result on failure so Retry never re-inserts records. */
struct SlicksSetupStorageReport slicks_amiga_store_track_records(
    const char *,const struct SlicksTrackRecords *,unsigned char *,unsigned char *,unsigned long);

/* Stable platform tag, replacing the DOS BIOS-date-derived byte. This is
 * format identification, not authentication. Foreign CFGs require import. */
#define SLICKS_AMIGA_CONFIG_SIGNATURE 0xa1
enum SlicksSetupLoadResult {
    SLICKS_SETUP_LOADED=0,
    SLICKS_SETUP_LOAD_IO_ERROR,
    SLICKS_SETUP_LOAD_INVALID,
    SLICKS_SETUP_LOAD_FOREIGN,
    SLICKS_SETUP_LOAD_RECOVERY
};
struct SlicksSetupLoadReport {
    enum SlicksSetupLoadResult result;
    long io_error;
    const char *path;
    unsigned char configuration_present,profiles_present;
};
/* Selected championship file must exist. Never publishes partial state or
 * silently loads across transaction leftovers. OS must be available. Caller
 * scratch is separate from outputs and at least 6+8*track_capacity+4*53 bytes. */
struct SlicksSetupLoadReport slicks_amiga_load_saved_game(const char *,
    struct SlicksSavedGame *,unsigned char (*)[8],unsigned,unsigned char *,unsigned long);
/* Starts from caller-provided defaults. Publishes neither object on failure.
 * Missing files use the original default-reader paths; malformed/foreign
 * files and transaction leftovers must not be silently overwritten. */
struct SlicksSetupLoadReport slicks_amiga_load_setup(
    struct SlicksConfiguration *,struct SlicksPlayerProfiles *,
    unsigned short,unsigned short,unsigned short);
/* Original SLICKS.TRK codec, bounded to 64 KiB on the 2 MiB target. Caller
 * owns the catalogue buffer; load publishes neither it nor its view on
 * failure. Missing file is an empty catalogue. OS must be available. */
#define SLICKS_AMIGA_TRACK_LIST_BYTES 65536UL
/* Startup-owned full-capacity snapshot. Refresh only with OS available and no
 * borrowed modal views. Caller supplies at least TRACK_LIST_BYTES of disjoint
 * unpublished scratch for refresh/store; neither allocates its staging buffer.
 * A failed refresh preserves bytes, but records failure
 * so callers cannot silently browse stale state across recovery artifacts. */
struct SlicksAmigaTrackListCache {
    struct SlicksTrackLists view;
    struct SlicksSetupLoadReport report;
    unsigned char *storage;
};
int slicks_amiga_track_list_cache_create(struct SlicksAmigaTrackListCache *);
void slicks_amiga_track_list_cache_refresh(struct SlicksAmigaTrackListCache *,unsigned char *,unsigned long);
void slicks_amiga_track_list_cache_free(struct SlicksAmigaTrackListCache *);
struct SlicksSetupLoadReport slicks_amiga_load_track_lists(
    unsigned char *,unsigned long,struct SlicksTrackLists *,unsigned char *,unsigned long);
struct SlicksSetupStorageReport slicks_amiga_store_track_lists(
    const struct SlicksTrackLists *,int,const unsigned char *,
    const struct SlicksTrackPlaylist *,unsigned,
    const unsigned char *(*)(void *,unsigned),void *,unsigned char *,unsigned long);
#endif
