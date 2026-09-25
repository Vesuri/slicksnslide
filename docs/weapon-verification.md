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

## Projectile motion and expiry

Additional original-instruction comparisons pass:

- All 1,792 homing heading/target combinations (21753..2184b).
- 16,384 ordinary/homing movement cases (2184b..21965, excluding the
  sprite-pointer update), including signed velocities and wrapping positions.
- 2,048 expiry cases (21965..21b47), stopping at actor retirement/detonation
  boundaries: every type, signed lifetime/tick boundaries and owner exclusion.

The original homing heading advances one of 112 steps per update, not per
elapsed tick. Movement divides the direction-table entry by seven before
multiplying by ticks. Candidate positions remain separate from the old
positions used by collision probing. Mines become able to hit their owner
at expiry; homing projectiles do so below 800 remaining ticks. Type 7 expiry
requests radial damage and an explosion, rather than ordinary retirement.
Those actor/effect side effects are not claimed by this boundary test.

## Original shop transactions

`make verify-weapon-shop` compares 32,768 prices, 65,536 buy/sell
transactions and 4,096 complete four-driver computer-shopping calls against
original instructions. Inventory, cash and shared random state all match.
The tests cover hidden/full items, carrying capacity, option gates, upgrades,
first weapon purchases, ammunition batches, partial affordability and sales.

The executable's computer-shopping minimum-price accumulator starts at -1
and only changes if greater than a positive price. It consequently permits
one random purchase attempt per computer, not a spend-all-cash loop. Failed
attempts still advance the random stream. These verified helpers are not yet
connected to the native shop surface or race entry.

## Native shop integration

The native race preparation now enters the shop under the original 24bee
buyable-item gate, after new-game inventory initialization and before vehicle
configuration. It uses `tuning.@I`, `tuning.@p`, the thirteen `virNN.@16`
images, `kirj.@f` and `pieni.@f`, not captured pixels. Buy, ammunition batch,
sell, driver/row selection and exit act on the persistent setup inventory.
Computer shopping consumes the same setup random stream. Failed preparation
restores the pre-entry session transaction.

Additional original-code comparisons pass for 1,215 driver navigation cases
and 2,720 visible-item row mappings. In the muted 2 MiB/no-Fast A1200
`NATURALW` diagnostic, ordinary shop keys buy a machine gun, buy ammunition,
sell one unit and exit. Observed cash/count transitions are
`1000/0 -> 900/1 -> 890/6 -> 891/5`. This fixture configures a human profile
and starting money before original new-game initialization; it never writes
weapon inventory directly. `diag_shop.gdb` checks the resulting inventory
and selection on race entry. The native shop capture was visually inspected;
a full original-screen pixel comparison is not yet claimed.
The same native input test now opens the original tuning help and returns
before exiting the shop; purchases and race selection remain intact. Missing
help uses the existing recoverable warning. The DOS screen-capture shortcut
has an explicit unavailable warning rather than silently pretending to save.

The natural four-computer, weapons-disabled race still finishes at update
556 with original HUD font, records/standings/statistics/save/restore checks
passing after integration. This does not establish live firing or impact
completion: the isolated weapon action/projectile helpers still need runtime
integration and the gameplay-driven HUD gates remain open.
