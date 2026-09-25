# Weapon translation evidence

## 2026-09-25: action and AI core

`make verify-weapon-actions` executes original instructions from the local
unpacked executable with Unicorn and compares `weapon_actions.h`:

- 69,120 human request cases (20509..20595): signed participation,
  weapons enablement, all controls, selection and cycle-held state.
- 4,860 firing gates (206a5..206e8): clock boundaries, signed cooldown,
  selection and request.
- 27,648 cycling cases (20c09..20c6f): all inventory availability masks,
  selection and request, including original `1eb49` selection execution.
- 512 depletion cases (20bbf..20c09): wrapping counts and unlimited ammunition.
- 65,536 cooldown cases (21585..215a2): every signed timer word, varied ticks.
- 32,768 AI probes (1ebbb..1ed63): counters, headings, original signed ranges,
  roles, clock parity, ammunition and wrapping coordinates.

All pass. The depletion phase invalidates Unicorn's cached blocks when
changing an already translated execution boundary; without invalidation,
execution incorrectly continued into request clearing beyond the test slice.

These are isolated translated routines, not evidence of live firing, shops,
projectile rendering, impacts or gameplay-driven HUD completion. The runtime
does not yet call this new core.

Original human controls use brake to fire, and the fifth control or simultaneous
left/right to cycle once per press. AI probes every twelfth call, returns a
cycle request for empty ammunition, and throttles shots using game-clock parity
(different masks for human and computer targets). No random draw is involved.

## Projectile creation core

The same oracle compares `weapon_projectile.h` with original instructions:

- 8,192 nearest-target cases (1ea80..1eb48), including equal positions,
  inactive drivers and signed/wrapping distance calculations.
- 8,192 initialization cases (20951..20b88): all weapons/headings, positions,
  layers, lifetime and exact shared random-state consumption. Homing uses
  heading/target instead of velocity and consumes no random draws; ordinary
  shots consume two draws even at zero spread.
- 1,024 free-slot cases (20855..20890): last nonpositive handle wins, with
  slot zero reserved. This is distinct from shared actor allocation.

These helpers remain unconnected to gameplay pending the original shared
actor, projectile collision/effect and firing-path integration.
