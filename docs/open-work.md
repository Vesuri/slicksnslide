# Open work

Updated 2026-09-30. Only unfinished work is listed here. An unchecked item
may require implementation, verification, or both; it is not necessarily a
known defect. Active work covers menus, presentation, resident asset loading
and fidelity. Gameplay optimization is paused.

Completed work and results belong in [menu-cache-verification.md](menu-cache-verification.md),
[fidelity-audit.md](fidelity-audit.md), [registration-support.md](registration-support.md),
[performance-profiling.md](performance-profiling.md) and
[release-verification.md](release-verification.md). Consult these to identify
uncovered cases, not to repeat completed checks. Working rules are in
[development-verification.md](development-verification.md).

## Decisions needed

- [ ] **D1 — Load Game entry:** choose whether to preserve the original's hidden
  Load entry or expose it as an explicit Amiga adaptation. The supplied executable
  hides the title row; no other usable entry has been established. This determines
  the permitted entry and scope of section 4.
- [ ] **D2 — Sparse shop participation:** choose safe active-player mapping or
  rejection of setups with inactive slots between active players. Do not reproduce
  the original's unsafe packed-column indexing.
- [ ] **D3 — Automatic language:** choose English or the language chooser for the
  negative saved-language selector. DOS KEYB detection has no agreed Amiga
  equivalent. Preserve positive saved choices and the zero-selector chooser.

## Active work

### 1. Original loading screen

- [ ] Verify that the custom display remains visibly correct while AmigaOS services
  disk I/O. Use `amiga/diag_loading_io_visual.gdb` after 50 OS-serviced refreshes.
  This needs a usable capture or manual observation; unchanged bitmap memory is
  not proof of correct visible output.
- [ ] Connect the original loading painter and filename/suffix builder to race
  and demo preparation. Preserve the track stem, font selection and resource
  lifetime across startup and subsequent loads.
- [ ] Keep the loading image visible throughout disk access. Verify failures,
  retry, cleanup and system restoration. Do not publish chunky storage while
  it is borrowed as the track-decoding arena.

### 2. Menu drawing and disk-free navigation

- [ ] Verify uncovered selection, scrolling, nested-dialog, restoration and
  transition paths for each menu owner. Check actual pixels and producer-reported
  dirty rectangles: preserve X/Y bounds, merge overlaps, and convert only covered
  aligned blocks. Fully initialize stale or completely replaced views.
- [ ] Finish uncovered Help input, page, language and error paths, including
  remaining shared text-bounds adapters and the six saved-background restore
  callers. Check exact parent restoration and successful reopen/retry.
- [ ] Verify remaining allocation lifetimes and warning layouts in saved-game,
  setup, intermission and results screens.
- [ ] After D2, verify sparse shop selection and transactions. Ignored keys and
  rejected transactions must not trigger unnecessary restores or redraws.
- [ ] Verify that every remaining RAM-only menu path avoids disk access and
  display teardown. Use [menu-resident-assets.md](menu-resident-assets.md) to
  distinguish navigation from legitimate track/exit-image/cup loads and saves.
  Track-list and saved-name caches may refresh at explicit transactions, not
  through hidden navigation-time rereads.

### 3. Title and demo fidelity

- [ ] **F08 — Animation cadence:** compare remaining registration/language
  combinations with the original and assess remaining owner-text/state drawing
  costs. Separate missed-refresh measurements from instrumented timings. Preserve
  palette/animation semantics; the DOS animation rate is CPU-dependent.
- [ ] **F03–F05/F10/F12 — Input and transitions:** finish uncovered native
  shortcut, Help, demo, mode, language, error and applicable mouse-input sequences
  against the original callers. Do not introduce unsupported menu rows or actions.
- [ ] Verify remaining font-selection and font-pointer lifetimes through dialogs,
  transitions, failures and language changes, including indirect original callers.
- [ ] Finish demo-return, persistence and error-presentation checks. Compare
  returned title pixels with the prepared startup background (original DS:4c1c).
  Verify finish rewards independently of skipped track/results branches; preserve
  shared random-number state and reward ordering.

### 4. Saved-game entry, capacity and recovery

- [ ] Resolve D1, then establish and verify the permitted Load entry without
  inventing title/intermission rows. Keep static reachability assumptions separate
  from observed runtime behavior.
- [ ] Replace `CHAMPLOAD`'s obsolete four-Down title-selection sequence with the
  permitted entry. Until then it is not a release test.
- [ ] Finish dynamic Load name/index storage and transactional playlist replacement
  after successful preparation. Validate the working-tree draft natively, including
  allocation failure, cancellation, rejected saves, preparation failure and retry.
- [ ] Finish uncovered load/edit, save-recovery and cached-dialog sequences, plus
  native save/load capacity checks up to the supported 10,000 tracks. Preserve
  genuine intermission Save entry, repeated editing, file safety and retained-display
  returns.
- [ ] If D1 permits a synchronous Load dialog, verify that cancellation after a
  long picker/notice visit restarts the full title idle-demo interval.

### 5. Localization

- [ ] **F15 — Automatic selection:** implement and verify the D3 policy without
  guessing a host-locale mapping for DOS KEYB.
- [ ] Audit uncovered localized-label callers and native input, error and resource
  lifetimes. Isolated all-language painter comparisons do not establish live
  caller behavior.
- [ ] Finish remaining real console-handler error and cleanup checks for the startup
  chooser. Preserve original labels, clamped navigation, acceptance, save/restart
  persistence and rejection when interactive input is unavailable.

### 6. Remaining whole-port fidelity audit

- [ ] Turn broad coverage gaps into an explicit list of unverified production
  routes and acceptance tests using [fidelity-audit.md](fidelity-audit.md).
  Include actual callers, parameters and overrides, not only isolated helpers.
  Link overlaps to sections 1–5 instead of creating duplicate work.
- [ ] Investigate each remaining route against the DOS original. Classify findings
  as defects, unverified requirements or authorized adaptations; fix confirmed
  differences and verify the real call path. Do not claim whole-game fidelity
  until that coverage is established.

### 7. Final release validation and package

- [ ] After production fixes settle, validate the stripped executable on a stock
  PAL A1200: 68020, 2 MiB Chip RAM, no Fast RAM, default 4 KiB stack. Cover startup,
  menus/Help/returns, race entry, championship save/exit, demos, affected failure
  recovery and normal system restoration.
- [ ] Run F1, CITY and WHACKO full-frame display audits on the release candidate,
  plus retention checks for any drawing-order or saved-pixel changes.
- [ ] Validate Installer, Keep/WHDLoad, Reinstall, installed Play and package
  contents. Check WHDLoad startup, idle demo, racing with and without PRELOAD,
  and normal exit at its documented memory configuration. A stock-2-MiB WHDLoad
  claim requires testing that configuration.
- [ ] Rebuild the installer archive after applicable release checks pass, following
  [release-verification.md](release-verification.md) and
  [install-original-data.md](install-original-data.md). Exclude private keys,
  original executable/data and captures.

## Paused: gameplay performance

Do not resume without the user's instruction. The target remains at most
20 ms / 312 PAL raster lines per update, including particle-heavy gameplay,
on a stock A1200 with no Fast RAM. Preserve behavior, effects, permanent marks,
audio and drawing order; averages or easy frames do not satisfy the target.

- [ ] **Particle pipeline:** reduce repeated scans and Chip-RAM traffic across
  restoration, ordering, advancement and drawing. For unmoved-particle retention,
  prove bookkeeping costs less than the work saved. Preserve underlays, permanent
  marks, slot reuse and ordering. See [point-retention-design.md](point-retention-design.md).
- [ ] **F1 track sprites:** reduce conflict processing, validation, group rebuilding
  and late restoration. Prove removed checks redundant; preserve atomic group
  retention, reverse restoration, initial saved-actor fallback and overflow rules.
- [ ] **Register-oriented 68020 simulation blocks:** profile and select larger car
  preparation/finishing, checkpoint, layer and clock blocks. Reduce stacked arguments,
  saves and repeated memory access across helper boundaries. Preserve signed/wrapping
  arithmetic, all-car-motion-before-tails order, callback invalidation,
  emission/progress/collision/effects ordering and final contact latches. Extend
  shadow checks to flag/Arcade/finish effects, lap-limit queries and reward callbacks.
- [ ] **C2P/publication (secondary):** pursue only designs that reduce total memory
  or instruction cost, counting platform publications and initial rectangles.
  Verify coverage and exact pixels.
- [ ] **Completion proof:** benchmark BASIC, F1, CITY and WHACKO, including heavy
  particles, HUD-clock phases and known transitions (F1 481/551/613; WHACKO 685/688).
  Refresh profiles after substantial changes. Keep optimizing until measured worst
  updates meet the target with fidelity intact. Consult historical rejection
  evidence before proposing experiments.

## Blocked or deferred

- [ ] **Registration order-form image:** visually verify optional external
  `webf_ord.bmp` if matching original data becomes available. It is absent from
  the supplied archive; a substitute does not satisfy this check.
- [ ] **Manual joystick test:** verify press/steer/release only when the user agrees
  to resume this deferred test.
- [ ] **General-purpose translator expansion:** exhaustive entry-point coverage,
  semantic IR/backend completion and unexercised DOS/runtime paths. Deferred
  separately from the game's active menu/fidelity work.
