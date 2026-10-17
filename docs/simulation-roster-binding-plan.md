# Simulation roster binding required for live startup

The first admission/technology step is now implemented at the actual Game
startup and restore checkpoints; see [live simulation state](smac-live-simulation-state.md)
for its tested boundary. The remaining consumer sequence below keeps three
identities separate:

| Identity | Role |
| --- | --- |
| Canonical Player identity and roster epoch | Current network occupant/lifetime |
| Faction source ID | Frozen rules/template metadata |
| Simulation faction ID1..7 | Persistent authoritative world ownership |

A transport slot is 0..6. The accepted GLSMAC new-world policy allocates dense
IDs once in ascending occupied-slot order. It does not derive IDs as `slot + 1`, a source-template
index, GSID or display-name match. Restore consumes the explicit saved mapping
and validates the accepted canonical participants; it never reruns admission.
Only occupied participants receive state, rather than seven inferred factions.

## Existing lifecycle and insertion points

The canonical roster implementation uses `Player::GetIdentity()` and a State
roster epoch. `State::ApplyInitialRoster`, `ApplyRosterRefresh`, `ApplySlotUpdate`
and `DeserializeRosterSettings` own packet publication and frozen settings
acceptance. Same occupant continuity currently means slot+GSID+role in the
captured epoch; vacancy/replacement creates a new identity. This does not itself
say which world faction a replacement controller may take over.

`Game::PrepareInitializationBindings` resolves lobby factions before the
owner-thread startup checkpoints. `StartInitializedWorld` invokes create/start
hooks and the first authoritative turn event. The reviewed initial-turn change
keeps founding work at turn0 until the actual advance event changes it to1.
Those are the real integration seams: initialize admitted simulation state
once after canonical source identities are final, before any create-world
consumer can found a base or inspect owned technology. Run it inside the existing
owner/native-retirement exclusion, not a new unsynchronized lobby callback.

For restored worlds, `SerializeWorldSnapshot`, header validation,
`DeserializeWorldSnapshotBody` and the late `InitComplete` checkpoint already
stage turn/RNG/completion state. World V9 now also carries the owned
SimulationRoster and technology state. Header acceptance binds the frozen
profiles; the runtime prefix validates the complete saved mapping against the
accepted roster before Map allocation and remains staged until claimed Start.
Exact restored state must be installed before start consumers can observe it. Never run new-world grants,
clear ownership, regenerate designs or substitute current lobby metadata during
restore. Cancellation, retirement and invalid links discard staged values.

The paired runtime value is installed before start consumers and revalidated
after start/deferred registration, before the saved turn flags and RNG are
installed. This does not make map/body restoration transactional.

## Persistence and reconnect contract

Simulation IDs and their source associations live in the world and are
serialized. Ordinary retained reconnect preserves the canonical Player token.
A different Player identity may acquire control only through a future explicit
host-approved takeover policy; this first implementation rejects replacement.
Changing transport occupancy never creates or resets research state. A new
roster epoch cannot authorize a stale queued binding. Losing a controller may
leave a simulation faction without a network controller; deletion, AI takeover
and late joining require separate explicit policies, not inference here.

The live binding rejects unsupported rebinding and preserves ordinary
same-occupant refresh without erasing state. Saved world mapping
and current session controller bindings are distinct: wire data contains owned
IDs/values, never raw Player pointers or GC wrapper addresses. Shared identity
tokens are invalidated by the canonical lifetime/session owners; no raw Player or GSE
wrapper is retained in the saved value.

## Concrete consumer sequence

1. Implemented: explicit SimulationRoster admission/restore and technology
   state at actual Game startup/world checkpoints. The native regressions cover
   nonconsecutive/permuted IDs, one admitted participant, same-source participants,
   duplicate/missing mappings, same-occupant refresh, vacancy/replacement,
   cancellation and stale owner callbacks. Actual create/start callbacks observe
   named grants before founding or exact saved possession/progress on restore.
   This is the prerequisite for the following gameplay consumers.
2. Add owned custom designs only when their producer and codec are usable
   together. START-BLOCK supplies Morgan/Spartan recipes in an explicitly empty
   owner block at64*simulationID, initial prototype marking and distinct placed
   Scout selection. It does not admit occupied blocks or a general workshop.
   Bind designs to the same SimulationRoster and persist them, not regenerate
   them during restore. Reuse the existing predefined producer/cost helper.
3. Route the real initial base creator through the admitted startup technology,
   design and accepted world-size/difficulty inputs. START-FIRST separates base
   target selection/turn-zero override and stored minerals/baseline from the
   unit actually placed. Tests must exercise the real spawn-base event at turn0
   and the first authoritative upkeep once, with state surviving world transfer.
   No Mind Worm substitution counts as a conventional Scout.
4. Later research consumes this owned progress/possession value with its own
   availability, chooser, acquisition, Future Tech and Secrets contracts.
   Numeric University extra-TECH at turn1 is separate from named startup grants.

This sequence requires no new technology catalog or duplicate cost calculator.
The next consumer is the owned custom-design producer and persistence, using
the admitted IDs. Founding can then consume those designs and the live named
technology state; no additional detached technology catalog is needed.
