#ifndef SLICKS_AMIGA_SETUP_STORAGE_H
#define SLICKS_AMIGA_SETUP_STORAGE_H
#include "../../game/configuration.h"
#include "../../game/player_profiles.h"
#include "../../game/setup_storage.h"
#include "../../game/track_lists.h"
#include "../../game/saved_game.h"

struct SlicksSetupStorageReport {
    enum SlicksSetupSaveResult result;
    long io_error;
    const char *path;
};
/* AmigaOS must be available. Uses original CFG/PLR encoders; signature
 * policy and dirty-state/UI handling remain the caller's responsibility. */
struct SlicksSetupStorageReport slicks_amiga_store_setup(
    const struct SlicksConfiguration *,const struct SlicksPlayerProfiles *,unsigned char);
/* Caller owns/validates the chosen .SSS path and keeps it alive for report.path.
 * Uses original bytes with the existing new/backup transaction. OS required. */
struct SlicksSetupStorageReport slicks_amiga_store_saved_game(const char *,const struct SlicksSavedGame *);
/* Confirmed Clear Top 10s only. Same 8192-byte track limit as the native
 * loader. Does not alter old-format tracks. Report path borrows caller's path;
 * changed is true only after publication, including cleanup-pending status. */
struct SlicksSetupStorageReport slicks_amiga_clear_track_records(const char *,unsigned char *);

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
struct SlicksSetupLoadReport slicks_amiga_load_track_lists(
    unsigned char *,unsigned long,struct SlicksTrackLists *);
struct SlicksSetupStorageReport slicks_amiga_store_track_lists(
    const struct SlicksTrackLists *,int,const unsigned char *,
    const struct SlicksTrackPlaylist *,unsigned,
    const unsigned char *(*)(void *,unsigned),void *);
#endif
