# Championship save/load/resume evidence

## Native integration — 2026-09-25

The native title now exposes Load Game (index 4); Read This and Quit move to
5 and 6. Intermission exposes the existing Change Cars and Save Game actions
as well as Next Track and End Match. The supplied DOS build hides the first
two rows and clamps upward navigation to Next Track; exposing those rows is
an intentional native integration change, not a claim of identical gating.
The underlying dialog, serializer and resolver retain their original-code
oracles.

Save uses the original 40-entry file picker, 8-character filename field and
.SSS byte format. Overwrites require Y confirmation. Delete requires Y and
refuses transaction recovery leftovers. The filesystem adapter validates
basenames, suppresses OS requesters temporarily, filters regular .SSS files,
and reports a catalogue overflow rather than silently hiding files.

Load stages the complete session and resolved playlist privately. It checks
the next index, track/profile availability and active drivers before loading
race assets. The live configuration and playlist are published only after
successful preparation; failure restores the previous session. No new-game
inventory/cash/points reset or random-vehicle selection occurs on resume.
Stored position scale is applied to the resumed race, with normal profile
refresh rules restored on the following race transition.

Like DOS, an .SSS checkpoint stores the *next race*, not cars in motion.
Options, bindings, colour ramps and RNG state are not in that original format.
Options/bindings and profile colours come from the normally persisted setup;
resume retains the running process's RNG rather than claiming to recover it.

### Checks

- Host championship composition: export → original-format encode/decode →
  staged resume preserves shared human profiles, AI, inactive slots, points,
  cash, inventory and scales. Invalid indices, missing names and empty driver
  selections leave both staging outputs unchanged.
- Original-code tests: 420 writer streams; 3,392 track/profile comparisons;
  2,560 vehicle fallbacks; 262,144 file-dialog action/confirmation comparisons.
- Storage faults: 1,485 save faults and 2,310 read faults/truncations, plus
  overflow, missing-file and recovery guards. Catalogue/path/delete tests
  cover 40/41 entries, non-files, case, traversal rejection and requester
  restoration.
- Muted A1200, 68020, 2 MiB Chip/no Fast: `CHAMPSAVE`, port 25144, selected
  three tracks through native Tracks, ran 40 updates, used the real F9
  advance menu, cancelled/reopened Save, entered E2E and saved. Cash at the
  intermission was 250; the file contains three names and next index 1.
  `NATIVE_CHAMPIONSHIP_MENU_SAVE_EXIT_OK`, restoration 0x1f.
- Fresh process, same isolated `.run/championship-v1/dh1`, `CHAMPLOAD`, port
  25145: native title Load opened the picker and prepared `TRACKS/66.SS`
  with `new_game=0`. All four target snapshots (points, cash, inventory,
  selected vehicles) compare byte-for-byte before save and before resumed
  race preparation. Runtime car vehicles/scales and weapon inventory match
  the restored session. The race ran 40 updates and exited cleanly:
  `NATIVE_CHAMPIONSHIP_FRESH_PROCESS_RESUME_RACE_OK`, restoration 0x1f.

The first two input attempts were invalid tests: this FS-UAE GDB stub ignores
memory writes. The successful tests use native raw-key queues and read-only
GDB assertions, as the established project fixtures do. No debugger supplies
championship state or race progress. Local saves/dumps remain ignored.
