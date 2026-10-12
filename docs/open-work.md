# Open work

Updated 2026-10-12. This is the complete list of actionable work. Close each
item exactly as written, commit it, and tick it off.

**Adding an item requires a demonstrated defect:** a reproduced difference from
the DOS original, a crash, or a visible fault. Open-ended "verify more paths"
items are not work items. Intentional differences from the original are listed
in [fidelity.md](fidelity.md); working rules are in
[development-verification.md](development-verification.md).

## Release 0.90 (01.10.2026)

Released and tagged `0.90`. Completed release checks and package hashes are
in [release.md](release.md). The release checks were completed; subsequent confirmed defects are listed
below. Intentional differences and limitations are documented in
[fidelity.md](fidelity.md), [performance.md](performance.md) and
[phases.md](phases.md).

## Confirmed startup fidelity defects

Evidence and reproduction commands are in [fidelity.md](fidelity.md#confirmed-startup-defects--2026-10-12).

- [ ] **F20/F21:** Restore the original combined start-light/audio/engine/first
  driver-update sequence. Remove the native stationary countdown derived from
  the boundary-palette timer, preserve the actual boundary animation, and stop
  charging the extra stationary wait to lap time. Replace tests that assume
  the wait with original caller-level comparisons. Verify light/cue ordering,
  first input and physics update, clocks, fresh races and pause/resume on target.
- [ ] **F22:** Restore the startup intro and weapons-notice actors with their
  original artwork, coordinates, motion, priorities and lifetimes. Preserve
  slot allocation; compare constructor state with original code and capture
  the presentation in DOS and on the Amiga with weapons off and on.
