# Open work

Updated 2026-10-29. This is the complete list of actionable work. Close each
item exactly as written, commit it, and tick it off.

**Adding an item requires a demonstrated defect:** a reproduced difference from
the DOS original, a crash, or a visible fault. Open-ended "verify more paths"
items are not work items. Intentional differences from the original are listed
in [fidelity.md](fidelity.md); working rules are in
[development-verification.md](development-verification.md).

## Version 0.91 (13.10.2026)

Release notes and historical release checks are in [release.md](release.md).
Intentional differences and limitations are documented in
[fidelity.md](fidelity.md), [performance.md](performance.md) and
[phases.md](phases.md).

## Current work

- **WHDLoad hangs at 4 MiB Fast with PRELOAD.** On a 68020 with 4 MiB Fast,
  `tools/test_whdload.py --mode championship` and `--mode records` stop at
  their first save or race load: no further file log entries and no TIMEOUT
  dump. The 0.91 slave hangs the same way; both pass at 4 MiB without PRELOAD
  and at 8 MiB with it. The ReadMe asks 4 MiB users to remove PRELOAD
  ([whdload.md](whdload.md#memory-and-caches)). Find what WHDLoad is waiting
  for and make the installed icon work at 4 MiB, or confirm it as a WHDLoad
  limitation.

F20, F21 and F22 are fixed and verified; the original-code
comparisons and target capture results are recorded in
[fidelity.md](fidelity.md#confirmed-startup-defects--2026-10-12).
