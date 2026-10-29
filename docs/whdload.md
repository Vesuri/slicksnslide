# WHDLoad launcher

Standalone remains supported on PAL A1200 / 2 MiB Chip / no Fast RAM.
WHDLoad is optional and needs **8 MiB Fast RAM** with the installed icon's
PreLoad, or 4 MiB without PreLoad; with less, use standalone.

## Memory and caches

The game asks for Chip only for its two display bitmaps, copper lists and PCM
banks ([memory.md](memory.md)); everything else is `MEMF_ANY`. The slave
therefore reserves 512 KiB Chip (`CHIPMEMSIZE`) and 2 MiB expansion memory
(`FASTMEMSIZE`), which holds the loaded executable and all other reservations.
Measured from WHDLoad's dumps of the running game (`timed` and `race` print
`MEMORY` lines from Exec's lists), Chip uses 316,480 bytes including the OS,
leaving 206,752, and expansion memory uses 1,655,192 bytes, leaving 441,928.
The track-name catalogue is the reservation that grows with the installation:
12 bytes per track, 120,000 bytes at its 10,000-track limit.

ExpMem is the 512 KiB Kickstart plus `FASTMEMSIZE`, one 2.5 MiB block. With
WHDLoad's own memory and the BaseMem backup, 3.5 MiB Fast RAM is not enough
("Can't allocate ExpMem"); 4 MiB is. At 4 MiB, PRELOAD leaves too little:
the 68020 championship and records runs stop making progress at their first
save or race load (no file log entries, no TIMEOUT dump), the 0.91 slave
(2 MiB Chip, no expansion memory) included. The same runs pass at 4 MiB
without PRELOAD and at 8 MiB with it. The cause inside WHDLoad is not
established.

Caches follow where WHDLoad uses the MMU, as Secret Weapons of Luftwaffe does.
The slave asks for `CACHE`: both caches on, ExpMem and the slave copyback,
BaseMem (Chip) noncacheable, so DMA memory is never cached. That split needs
WHDLoad's MMU, which it uses on a 68040/060 by default and on a 68030 only with
its `MMU` option. Without it WHDLoad turns every cache off (a 68020 had CACR 0),
so `_bootdos` reads CACR and, if the instruction cache is off, sets
`CACHECHIP`'s value (instruction cache, BaseMem write-through). Measured CACR:
68020 `1`, 68030 `2011`, 68030 with `MMU` `2111` (TC translating), 68040
`80008000` (TC `8000`). Only a full 68030 may use `MMU`, which software cannot
tell from a 68EC030, so the installer does not set it; the ReadMe tells
68030 owners. The harness checks this policy on every run from the report's
`CACR=`, `TC=` and `setcpu=` (the slave's last request).

WHDLoad 17+, 68020 and AGA are required. Supply Kickstart 3.1 and its matching
RTB in Devs:Kickstarts or WHDCOMMON:. The SDK supports kick40063.A600,
kick40068.A1200 and kick40068.A4000; native tests used A600 40.063 and WHDLoad
20.1.7190. ROM/RTB/WHDLoad/OS binaries are never distributed.

`whdload/SlicksSlave.s` adapts Vette's cross-assembled public-domain SDK
kick31/kickfs launcher. INITAGA is essential for the direct AGA platform.
It mounts data/, uses DOS LoadSeg, establishes PROGDIR, sets a null-terminated
DOS argument string (the game uses GetArgStr), and supplies a 16 KiB Exec
StackSwap stack. On normal return it restores arguments/PROGDIR, unloads the
game and returns through resload_Abort. Do not use the SDK's A600 STACKSIZE
ROM patch: it overwrites the wrong opcode (AmigaXDev `docs/reference/port-toolchain.md`).

Both launch methods use identical native game code and the same data/saves.
Normal menu exit saves pending changes. F10 is immediate WHDLoad quit and
cannot execute deferred game saving. Keep the installation writable.

**No special WHDLoad options are needed** beyond `MMU` on a full 68030 (above).
The installed icon sets only the template's Slave and PreLoad tooltypes; a Shell
launch is `WHDLoad SlicksNSlide.slave`.
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
`--seconds` host safety ceiling. Diagnostic races use the fixed clock; allow
`--ticks 30000`, and for the save modes `--ticks 60000 --seconds 1800`.

Build with `make -C whdload`. `race-test` and `exit-test` create separate test
slaves which only pass native diagnostic arguments to the unchanged game.
They are never packaged. `timed` uses the actual production slave and normal
startup, while `race` reaches real racing and `quit` exercises normal exit.

```sh
. amiga/env.sh
make -C whdload all race-test exit-test
python3 tools/test_whdload.py --mode timed
python3 tools/test_whdload.py --mode race
python3 tools/test_whdload.py --mode quit --ticks 5000
```

Use `--exe build/release/SlicksNSlide` to test the stripped release executable and
`--no-preload` to test live disk reads. Tests default to a 68020 with 4 MiB
Fast RAM, where the save modes need `--no-preload` or `--fast 8192` (above);
`--fast` accepts KiB. `--setup a1200-030|a1200-040` instead runs
AmigaXDev's named machine (8 MiB Fast, MMU, data caches) and checks from the
emulator log and WHDLoad's registers that the CPU, MMU translation and caches
are what the run claims; `--whdload-mmu` passes WHDLoad's `MMU` option, which
a 68030 needs for translation and caches, and `--no-mmu` removes the CPU's MMU. Without `--no-preload` the harness adds PRELOAD; it never
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
