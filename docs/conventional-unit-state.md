# Explicit conventional Unit state

The existing backend `Unit` now owns one tagged state: its unchanged legacy
float fields, or `ConventionalUnitState`. Unit ID, manager membership, canonical
owner Slot, definition, and tile membership remain the existing authorities.
There is no parallel conventional unit registry and no default startup spawn.

This first live consumer requires an admitted SMAC simulation with **Refreshed**
retained social state. Early state is rejected before tile or next-ID effects;
unknown retained MORALE is not replaced with zero. Before-first-upkeep spawning,
complete conventional commands and upkeep are separate milestones.

## Stored state and explicit creation

The conventional record owns design ID, simulation faction ID (1–7), stored
morale, damage and spent bytes, failed-movement marker, explicit homeless state,
and an explicit render morale query mode (0 or 1). Simulation faction ID is not
the controller's lobby slot index. Stored morale accepts the observed entire
byte domain; effective morale remains a derived query. Damage must describe a
living unit in the admitted context. Spent may exceed the current allowance.

`UnitManager::SpawnConventionalUnit` and the `spawn_conventional_unit` GSE method
validate the complete context before constructing a Unit. The script method
requires `id` (zero explicitly requests allocation), `def`, live `owner` and
`tile` wrappers, `design_id`, `owner_simulation_id`, `stored_morale`, `damage`,
`spent`, `failed_move`, `home: 'none'`, and `render_morale_mode`. No supplied float
movement/health value becomes authority. Normal mutation authorization applies.

The resolver checks the frozen movement, predefined-design, technology and
complete faction profiles; the admitted bound simulation and participant; and
the actual owned design. It validates all raw faction bonuses, including those
after the original shared eight-record cap. The supported profile excludes
direct `MORALE`, so its verified absence supplies 0/OFF to the existing stats
kernel. This is not a default for an incomplete or expanded profile.

The design domain is active conventional Gun/Scout armor, reactor 1, Infantry
or Speeder component semantics, no abilities/exclusion and plan 3. Actual
creation originally required a canonical land StaticDef with the explicit
`RT_CVR_SCOUT` descriptor and **Infantry**. The
[selected Spartan presentation follow-up](conventional-selected-presentation.md)
also admits the selected owned Spartan Speeder with its distinct Rover
descriptor. An arbitrary Speeder is still rejected. Facing, view,
scale and placement still come from the explicitly validated render descriptor.
No native/Psi/special plan, home-base fixup or SMAX interpretation is introduced.

## Fresh queries and frontend copies

`Unit::QueryStats(mode)` resolves current context for every call. Effective
morale uses the supplied mode; full allowance uses the separately verified
mode-1 query. Q and basic allowance come from the frozen movement profile and
the selected chassis, and retained MORALE comes from the matching simulation
faction. The result owns effective and allowance morale, Q/basic/full/remaining
allowance and integer current/maximum HP. It uses the existing
[conventional stats kernels](smac-conventional-stats.md), not StaticDef float
movement or the legacy native-life approximation.

`get_conventional_state()` and `query_conventional_stats(mode)` return detached
scalar objects. Legacy float properties are undefined for conventional wrappers;
their setters reject. `UpdateConventionalUnit` validates a complete replacement
record before assignment and refresh, and cannot replace identity, home or the
explicit rendering policy.

Spawn/update requests carry freshly copied morale/text, HP fraction, remaining
allowance/Q, full allowance/Q and exact actionability. The frontend uses the
exact flag for conventional units, including one remaining quantum at Q=100
(0.01 in the presentation float). Its stats line uses the fresh full allowance
projection, not StaticDef movement. Morale updates retire the old sprite instance
before changing its morale key and refresh the badge/preview. These projections
are not writable gameplay authority. Existing coordinate-changing update
requests remain unsupported; this slice does not implement movement animation.

## Persistence and inactive commands

The unit-manager marker is `GLSMAC_UNITS_V2`; each unit record has version 2 and
an explicit legacy/conventional tag. The existing legacy fields are unchanged
within the tagged branch. Old V1 manager snapshots are rejected, not interpreted
as conventional data. The enclosing Simulation/world header is unchanged here.

The full record is decoded before construction. Shared identity/coordinate and
branch-specific bounds are checked before tile/ID publication. During world
download the real body reader supplies its private decoded `BoundSimulation`
to the unit manager, because UM restores before the saved simulation is
published and before BM. It cannot accidentally query a previous current world
or infer a home from a not-yet-restored base. Deferred registration preserves
existing restoration behavior: no new-unit hook or RNG draw. Reserialization
checks current conventional context and preserves stored bytes exactly.

Legacy move, attack, skip-turn and despawn-event application reject conventional
units before their float/RNG/animation paths. Their valid conventional rollback
transactions therefore do not exist. Native move/combat entry points also
reject. Actual turn processing skips legacy refill/healing/moved flags for
conventional units, preserving spent, damage and the failed-movement marker;
only its frontend projection is refreshed. Native manager retirement remains
available. No movement, combat, RNG bootstrap, promotion, death or upkeep policy
is activated by this patch.

## Validation scope

The dedicated `ConventionalUnits` native TU uses the existing asset-free backend
fixture and actual GSE manager method, world writer/body decoder, script event
handlers and turn path. Six groups cover canonical ownership, detached queries,
malformed/truncated records and byte endpoints, preflight/update nonmutation,
destination-world restore using deliberately different current context, legacy
handler exclusion and exact Q=100 actionability. Restore tests exercise actual
body decoding and deferred registration, not a second claimed-Start scheduler.
Existing scheduler/roster tests retain that responsibility.

The existing real Scout frontend/preview fixture additionally exercises exact
actionability, the full-allowance stats line and rendered morale refresh with
synthetic CVR/badge inputs. Its first added refresh attempt exposed missing
TileManager fixture setup; the retained GDB trace identifies that null fixture
manager. The corrected fixture supplies the real TileManager, with no production
null-check workaround. No proprietary assets or new original-executable captures
are introduced. This consumer reuses the previously compared pure kernels.

The source branch based on `02e8160b` plus the bounded-Q-frame prerequisite was
checked with Linux Clang 15: the normal eight CTests passed (77.40 s), and the
ASan/UBSan/LSan eight CTests passed with leak detection enabled (149.49 s).
All six new groups plus the actual frontend preview group passed separately
with memory tracking in both builds. The sanitizer executable uses the local
`-no-pie` startup workaround. Explicit correctness analysis of the nine changed
production translation units reported zero failures.

A separately compiled `-O3 -DNDEBUG` record TU passed all six backend groups;
this is a Release-TU check, not a full Release build. Seven copied production-TU
controls each exited 1 at the intended regression: restoring from current
instead of downloaded context, reusing the frontend float threshold, discarding
retained morale, dropping the spent byte during serialization, entering legacy
turn refill, projecting full instead of remaining allowance, and permitting a
Speeder with the Infantry descriptor. Intended source files were unchanged
during these controls. Logs, commands and hashes are retained locally under
`/home/jhurliman/src/glsmac-conventional-unit-state/`; no new original-executable
comparison or Windows/macOS execution is claimed for this source checkpoint.

This patch does not claim arbitrary roster synchronization,
allocation-failure transactionality, manager-wide atomic malformed restore,
rendered map reload, or default playable conventional startup.
