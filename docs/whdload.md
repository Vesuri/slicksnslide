# WHDLoad launcher

Standalone remains supported on PAL A1200 / 2 MiB Chip / no Fast RAM.
WHDLoad is optional. The slave reserves 2 MiB Chip and 512 KiB Kickstart
space and **no expansion memory** (`FASTMEMSIZE = 0`): the game runs in the
same 2 MiB Chip-only memory as standalone. Tested with 2 MiB Fast RAM, with
and without PRELOAD; WHDLoad itself still needs Fast RAM for the Kickstart
image and the OS backup, so this is not a claim that WHDLoad fits a stock
no-Fast-RAM machine.

WHDLoad 17+, 68020 and AGA are required. Supply Kickstart 3.1 and its matching
RTB in Devs:Kickstarts or WHDCOMMON:. The SDK supports kick40063.A600,
kick40068.A1200 and kick40068.A4000; native tests used A600 40.063 and WHDLoad
19.2.6941. ROM/RTB/WHDLoad/OS binaries are never distributed.

`whdload/SlicksSlave.s` adapts Vette's cross-assembled public-domain SDK
kick31/kickfs launcher. INITAGA is essential for the direct AGA platform.
It mounts data/, uses DOS LoadSeg, establishes PROGDIR, sets a null-terminated
DOS argument string (the game uses GetArgStr), and supplies a 16 KiB Exec
StackSwap stack. On normal return it restores arguments/PROGDIR, unloads the
game and returns through resload_Abort. Do not use the SDK's A600 STACKSIZE
ROM patch; Vette's documented opcode-offset problem applies here too.

Both launch methods use identical native game code and the same data/saves.
Normal menu exit saves pending changes. F10 is immediate WHDLoad quit and
cannot execute deferred game saving. Keep the installation writable.

**No special WHDLoad options are needed.** The installed icon sets only the
template's Slave and PreLoad tooltypes; a Shell launch is `WHDLoad Slicks.slave`.
WHDLoad's default file and write caches apply.

## File operations

Every WHDLoad OS switch is visible as a pause, and each physical write adds
WHDLoad's write delay (3 s by default). The game therefore keeps file access
to whole files and as few operations as the original data allows:

- Saves replace each file in place with one complete write, as the DOS
  original does: no `.new`/`.bak` files, existence checks, renames, copies or
  deletes. Under WHDLoad the write is one resload_SaveFile through the
  retained 40-byte `SLKSIO01` descriptor that the slave patches. A failed
  write may leave that file incomplete; the game keeps its in-memory state
  and offers Retry.
- CFG/PLR are rewritten only when their bytes differ from those last loaded or
  saved (CRC-32 and length). Track records are rewritten only when the
  record block changes, so clearing already-clear tracks writes nothing.
- A saved track list becomes the resident catalogue directly; it is not reread.
- Reads are whole files (one DOS Read) or whole archive resources. The resident
  menu cache reads each of its 57 resources in one read, staged in the not yet
  loaded 64 KiB track-list storage; archive reopens reuse the retained
  directory instead of rereading it.

The slave's documented CBSWITCH callback counts actual OS round trips, using
no stack and returning via A0. The test harness reports per-save totals and
maximums from the slave snapshot, plus the whole-session total;
`--max-save-switches N` enforces a per-save limit. Use `--real-time` for
watchable runs without fast-forward, with real CPU speed, and allow a longer
`--seconds` host safety ceiling. Diagnostic races use the fixed clock, so with
the game in Chip RAM allow `--ticks 30000`.

Build with `make -C whdload`. `race-test` and `exit-test` create separate test
slaves which only pass native diagnostic arguments to the unchanged game.
They are never packaged. `timed` uses the actual production slave and normal
startup, while `race` reaches real racing and `quit` exercises normal exit.

```sh
. amiga/env.sh
make -C whdload all race-test exit-test
python3 tools/test_whdload.py --mode timed --rom /local/kick40063.A600 --rtb /local/kick40063.A600.RTB
python3 tools/test_whdload.py --mode race --rom /local/kick40063.A600 --rtb /local/kick40063.A600.RTB
python3 tools/test_whdload.py --mode quit --rom /local/kick40063.A600 --rtb /local/kick40063.A600.RTB --ticks 5000
```

Use `--exe build/release/Slicks` to test the stripped release executable and
`--no-preload` to test live disk reads. Tests default to 4 MiB Fast RAM;
`--fast` accepts KiB. Without `--no-preload` the harness adds PRELOAD; it never
adds write-cache options. The decoder `tools/whdload_picture.py` inspects actual
eight-plane, interleaved game output from local WHDLoad dumps, not reference
images. All dumps/pictures remain ignored local evidence.

Save regressions use `championship-test`, `championship-edit-test` and
`records-test` build targets, and the matching `--mode championship`,
`--mode championship-edit`, and `--mode records`. Every mode rejects any
FileLog operation on a `.new` or `.bak` name.
The edit test takes `--seed-save PATH/TO/E2E.SSS` from a successful first run.
It exercises ordinary menu input for cancel, create, overwrite and delete.
Use `--seed-setup PATH/TO/data` for warm CFG/PLR overwrite tests.
