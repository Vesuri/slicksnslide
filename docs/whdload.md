# WHDLoad launcher

Standalone remains supported on PAL A1200 / 2 MiB Chip / no Fast RAM.
WHDLoad is optional and has separate overhead: the slave reserves 2 MiB Chip,
1 MiB expansion memory and 512 KiB Kickstart space. Tested with 4 MiB Fast RAM
without PRELOAD, or 8 MiB Fast with PRELOAD; this is not a claim that WHDLoad
fits a stock no-Fast-RAM machine.

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

**Use WRITECACHE without PRELOAD at 4 MiB Fast.** This is the installed default.
For a Shell launch use `WHDLoad Slicks.slave WRITECACHE`. PRELOAD is supported
at 8 MiB Fast. Creating cached files stalls in the tested 4 MiB/PRELOAD
combination, but passes at 4 MiB without PRELOAD and 8 MiB with PRELOAD.
NOWRITECACHE avoids the hang but exposes every transaction write as a slow
OS switch; it is a diagnostic fallback, not the installed default.

The slave prefixes the game arguments with `WHDLOAD ` and patches the retained
40-byte `SLKSIO01` interface. All WHDLoad file writes now call resload_SaveFile
with a complete image: no empty-file creation followed by partial writes.
Copy/delete replacements for Rename read the complete source into an 80,218-byte
WHDLoad-only buffer reserved at startup and freed on shutdown. Standalone keeps
native Rename and allocates no extra buffer. Both paths retain the same staging,
backup and recovery guards. Incomplete copies never cause deletion of their
source; unremovable partial destinations preserve recovery files.

The slave's documented CBSWITCH callback counts actual OS round trips, using
no stack and returning via A0. The test harness reports per-transaction totals
and maximums from the slave snapshot; `--max-save-switches 1` enforces the
one-switch limit. This does not count the final exit-time cache flush. Whole-file
writes alone do not guarantee a single switch: the measured 4 MiB/no-PRELOAD
case still takes up to four per transaction; 8 MiB/PRELOAD meets the limit.
Use `--real-time` in the test harness for watchable runs without fast-forward,
with real CPU speed. Allow a longer `--seconds` host safety ceiling for these runs.

On 2026-10-01 the user watched the real-time, muted records test with 8 MiB
Fast RAM and PRELOAD and confirmed that disk accesses were now reasonable,
including the observed exit. Fixture `tmp/whdload-test-qu_10c8u` returned
normally: two save transactions, nine whole-file writes, one measured OS
round trip in total (maximum one per transaction). Record changes and setup
files passed the harness checks. This confirms the visible save behavior in
that configuration, not the unresolved 4 MiB case or the full production-icon
release checklist.

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
`--fast` accepts KiB. The decoder `tools/whdload_picture.py` inspects actual
eight-plane, interleaved game output from local WHDLoad dumps, not reference
images. All dumps/pictures remain ignored local evidence.

Save regressions use `championship-test`, `championship-edit-test` and
`records-test` build targets, and the matching `--mode championship`,
`--mode championship-edit`, and `--mode records`. Pass `--no-preload` for
the default 4 MiB configuration, or `--fast 8192` to test PRELOAD.
The edit test takes `--seed-save PATH/TO/E2E.SSS` from a successful first run.
It exercises ordinary menu input for cancel, create, overwrite and delete.
Use `--seed-setup PATH/TO/data` for warm CFG/PLR overwrite tests.
