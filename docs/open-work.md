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
in [release.md](release.md). No implementation or release-validation items
remain open. Intentional differences and limitations are documented in
[fidelity.md](fidelity.md), [performance.md](performance.md) and
[phases.md](phases.md).

## Audio follow-up

- [ ] Restore the DOS pre-race sample-1 cue sequence, including its final
  pitch change; the Amiga adapter currently never receives these requests.
  Verify against original code and on target, then commit separately from
  the full-amplitude sample adaptation.
