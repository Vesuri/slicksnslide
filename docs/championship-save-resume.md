# Championship save/load/resume evidence

Use **Save Game** between races, then **Load Game** on the title after restart.
The picker offers Save, Save As and Delete; Tab changes actions and arrows
choose files. Filenames are 1–8 letters/digits/hyphens/underscores, stored as
`.SSS` alongside the game. Overwrite and deletion require Y; other keys cancel
the confirmation. Escape backs out without loading or saving.

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

### Repeat saves and failure matrix

`CHAMPEDIT`, port 25146, freshly resumed race two from the saved file and
reached the following intermission through the real F9 advance menu. Its
16 checked dialog boundaries cancelled filename entry, created TEMP.SSS,
cancelled and accepted overwriting E2E.SSS, cancelled and accepted deleting
TEMP.SSS, and returned through End Match to a clean exit. The surviving E2E
file has next index 2; TEMP is absent and there are no .SSS.new/.SSS.bak
leftovers. Cash progressed from 250 to 300 through the ordinary track award.
`NATIVE_CHAMPIONSHIP_RESAVE_OVERWRITE_DELETE_CANCEL_OK`, restoration 0x1f.

A further fresh process at port 25157 loaded the overwritten E2E checkpoint,
prepared `TRACKS/8.SS` (race three) with cash 300 and `new_game=0`, ran 40
updates and exited cleanly. `championship_target_fixtures.py check-resume`
independently decodes the actual .SSS bytes and compares its points, cash,
inventory and normalized vehicles to the target's restored memory snapshots.

`CHAMPFAIL`, ports 25152–25156, loaded five deliberately unusable fixtures
through the real title and picker: truncated stream, missing track, missing
human profile, next index equal to the playlist count, and a .new recovery
file. Each run displayed the appropriate warning, returned to the title,
and exited with restoration 0x1f. Complete session and playlist snapshots
are byte-identical before/after each rejection; none reaches race preparation.
`NATIVE_CHAMPIONSHIP_REJECT_RETURN_EXIT_OK` on every case.

Earlier attempts at this matrix mounted the default debug directory instead
of the fixture directory and exercised only the empty-catalogue notice. They
are not counted as negative-save evidence. The corrected harness exports
the exact isolated directory and rejects an empty-catalogue result.

The new file-management tests intentionally delete only their generated
TEMP.SSS fixture, not user saves. Recovery fixtures remain local/ignored.

Regression suite: championship composition, native catalogue, original
serializer/resolver/dialog, transactional storage, setup lifecycle, title
Help/ABI bridge, and intermission dispatcher/drawing/renderer all pass.
The native build passes `-Werror` and the runtime strlen guard. These tests
use the supported between-race advance path; natural fuel/damage completion
and broad mode/track gameplay verification remain separate open work.
