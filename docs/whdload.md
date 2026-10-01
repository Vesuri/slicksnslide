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

The slave prefixes the game arguments with `WHDLOAD `, selecting a bounded
512-byte copy/close/delete replacement for Rename. Standalone keeps native
Rename. Both paths retain the same staging, backup and recovery guards; no
runtime allocation is added. Incomplete copies never cause deletion of their
source, and failure to remove a partial destination stops the transaction
with recovery files retained.

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
