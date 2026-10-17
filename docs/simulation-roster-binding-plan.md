# Simulation roster binding required for live startup

This is the next integration contract for the owned
[technology state](smac-runtime-technology-state.md), not a claim that it already
runs in a live world. It keeps three identities separate:

| Identity | Role |
| --- | --- |
| Canonical Player identity and roster epoch | Current network occupant/lifetime |
| Faction source ID | Frozen rules/template metadata |
| Simulation faction ID1..7 | Persistent authoritative world ownership |

A transport slot is0..6. No contract currently permits deriving a simulation ID
as `slot + 1`, a source-template index, GSID or display-name match. Choosing that
mapping is world admission policy, not a decoder convenience. The initial live
slice must receive an explicit authoritative `SimulationRoster` admission and
validate a bijection between admitted world participants and their simulation
IDs before any simulation state is initialized. It must reject absent or
ambiguous assignments rather than silently create seven complete faction states.

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
stage turn/RNG/completion state. Extend that actual path with a versioned
SimulationRoster and technology-state body. Header acceptance binds the frozen
profiles. Decode must validate the complete admitted mapping and saved state
before publishing either, retaining the same session/generation and invalidatable
occupant guards as the current startup coordinator. Exact restored state must
be installed before start consumers can observe it. Never run new-world grants,
clear ownership, regenerate designs or substitute current lobby metadata during
restore. Cancellation, retirement and invalid links discard staged values.

The live implementation must reconcile this early consumer visibility with the
existing late publication checkpoint explicitly; staging into an isolated
restored-world owner is preferable to exposing a partially accepted world.
The current pure codec does not make map/body restoration transactional.

## Persistence and reconnect contract

Simulation IDs and their source associations live in the world and are
serialized. A reconnect can acquire control of an existing simulation ID only
through an explicit host-approved rebinding to the new canonical Player identity.
Changing transport occupancy never creates or resets research state. A new
roster epoch cannot authorize a stale queued binding. Losing a controller may
leave a simulation faction without a network controller; deletion, AI takeover
and late joining require separate explicit policies, not inference here.

The first live binding may reject unsupported rebinding rather than guess, but
must not make ordinary same-occupant refresh erase state. Saved world mapping
and current session controller bindings are distinct: wire data contains owned
IDs/values, never raw Player pointers or GC wrapper addresses. Runtime wrappers
used as lifetime guards must be rooted and retired by the existing coordinator.

## Concrete consumer sequence

1. Introduce the explicit SimulationRoster admission/restore boundary and wire
   the current technology value into the actual Game startup/world body. Test
   nonconsecutive/permuted IDs, one admitted participant, same-source participants,
   duplicate/missing mappings, same-occupant refresh, vacancy/replacement,
   cancellation and stale owner callbacks. A real create-world callback must
   observe the named grants before founding; a real restored callback must
   observe changed saved possession/progress instead of startup defaults.
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
The next review gate is the explicit runtime identity/admission contract and its
actual Game/Client checkpoint tests, rather than more disconnected value fields.
