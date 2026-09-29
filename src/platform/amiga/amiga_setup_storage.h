#ifndef SLICKS_AMIGA_SETUP_STORAGE_H
#define SLICKS_AMIGA_SETUP_STORAGE_H
#include "../../game/configuration.h"
#include "../../game/player_profiles.h"
#include "../../game/setup_storage.h"
#include "../../game/track_lists.h"
#include "../../game/saved_game.h"
/* Explicit TRACKSQ/TRACKSC diagnostics only; zero during normal launches. */
extern unsigned char g_slicks_diag_track_read_fault,g_slicks_diag_track_read_reached;

struct SlicksSetupStorageReport {
    enum SlicksSetupSaveResult result;
    long io_error;
    const char *path;
};
/* AmigaOS must be available. Uses original CFG/PLR encoders; signature
 * policy and dirty-state/UI handling remain the caller's responsibility. */
struct SlicksSetupStorageReport slicks_amiga_store_setup(
    const struct SlicksConfiguration *,const struct SlicksPlayerProfiles *,unsigned char);
/* Shop Scroll Lock: first free TUNING00..98.BMP; never replace a capture. */
struct SlicksSetupStorageReport slicks_amiga_store_capture(
    const unsigned char *,const unsigned char *);
/* Caller owns/validates the chosen .SSS path and keeps it alive for report.path.
 * Uses original bytes with the existing new/backup transaction. OS required. */
struct SlicksSetupStorageReport slicks_amiga_store_saved_game(const char *,const struct SlicksSavedGame *);
/* Confirmed Clear Top 10s only. Same 8192-byte track limit as the native
 * loader. Does not alter old-format tracks. Report path borrows caller's path;
 * changed is true only after publication, including cleanup-pending status. */
struct SlicksSetupStorageReport slicks_amiga_clear_track_records(const char *,unsigned char *);
struct SlicksTrackRecords;
/* Post-race records use the same transactional writer; caller retains its
 * in-memory result on failure so Retry never re-inserts records. */
struct SlicksSetupStorageReport slicks_amiga_store_track_records(
    const char *,const struct SlicksTrackRecords *,unsigned char *);

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
 * silently loads across transaction leftovers. OS must be available. */
struct SlicksSetupLoadReport slicks_amiga_load_saved_game(const char *,
    struct SlicksSavedGame *,unsigned char (*)[8],unsigned);
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
/* Startup-owned exact-size snapshot. Refresh only with OS available and no
 * borrowed modal views. A failed refresh preserves bytes, but records failure
 * so callers cannot silently browse stale state across recovery artifacts. */
struct SlicksAmigaTrackListCache {
    struct SlicksTrackLists view;
    struct SlicksSetupLoadReport report;
};
void slicks_amiga_track_list_cache_refresh(struct SlicksAmigaTrackListCache *);
void slicks_amiga_track_list_cache_free(struct SlicksAmigaTrackListCache *);
struct SlicksSetupLoadReport slicks_amiga_load_track_lists(
    unsigned char *,unsigned long,struct SlicksTrackLists *);
struct SlicksSetupStorageReport slicks_amiga_store_track_lists(
    const struct SlicksTrackLists *,int,const unsigned char *,
    const struct SlicksTrackPlaylist *,unsigned,
    const unsigned char *(*)(void *,unsigned),void *);
#endif
