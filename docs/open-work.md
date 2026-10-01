# Open work

Updated 2026-10-01. This is the complete list of actionable work. Close each
item exactly as written, commit it, and tick it off.

**Adding an item requires a demonstrated defect:** a reproduced difference from
the DOS original, a crash, or a visible fault. Open-ended "verify more paths"
items are not work items. Intentional differences from the original are listed
in [fidelity.md](fidelity.md); working rules are in
[development-verification.md](development-verification.md).

## Release 0.90 (30.09.2026)

- [x] Fixes B1–B14 (real-time race clock, adaptive publication, fidelity
  fixes, one complete write per saved file, zero-expansion WHDLoad slave).
- [x] Packaging in the sibling WHDLoad layout
  ([install-original-data.md](install-original-data.md), [whdload.md](whdload.md)).
- [x] D-1 clean-tree `make release-check`; D-2 F1/CITY/WHACKO display audits
  and the RETCHECK retention check; D-3 manual stock-A1200 session
  (installer, standalone, WHDLoad icon with and without PRELOAD).
- [ ] D-4: build and audit `Slicks-0.90.lha`, record its hashes in
  [release.md](release.md), then tag `v0.90` when the user asks.

## Not in scope

- Worst-case 20 ms updates. Game speed is correct at any frame rate; see
  [frame-pacing.md](frame-pacing.md) and [performance.md](performance.md).
- The rate of once-per-drawn-update work (particle ageing, actor animation,
  homing turn step). On DOS it is CPU-dependent and the Amiga's up to 50
  updates/s falls inside the PC range.
- The registration order-form image: the original `webf_ord.bmp` is absent.
- The manual joystick test, deferred by the user.
- General translator expansion; see [phases.md](phases.md).
