# Player roster ownership and canonical factions: implementation plan

Status: source audit and proposed work, not implemented behavior. Audited at
[708511942604826959cd476200f69662133b5c3d](https://github.com/jhurliman/glsmac/commit/708511942604826959cd476200f69662133b5c3d).
That source is an ancestor of immutable retention branch
`codex/validation-20261016-player-roster-audit`, pinned at
[fa939625bc87de69d193c8e732e883807e8d3c93](https://github.com/jhurliman/glsmac/commit/fa939625bc87de69d193c8e732e883807e8d3c93).
The separate world V3 completion
restoration is documented in [the V3 snapshot contract](https://github.com/jhurliman/glsmac/blob/a16fa30b4f8d1a812d306cf7a719de318cbcba3a/docs/world-snapshot-turn.md).
This plan neither changes that wire format nor claims complete save/load or
host/client lockstep.

## Observed lifecycle

The locations below refer to the audited revision; function names identify
the paths if later changes move the lines.

| Path | Current behavior and consequence |
| --- | --- |
| `src/game/backend/Player.cpp:21,32,58,198,212` | Native Players borrow a Faction pointer. Event copies shallow-copy it. Full Player serialization embeds a Faction; decoding allocates a separate Faction and does not own or free it. Repeated decoding also loses the previous pointer. Adding a deleting Player destructor would incorrectly delete native manager-owned factions. |
| `src/game/backend/State.cpp:93,99,148` | State owns the Players in `m_players`; Add/Remove manage the set and Reset deletes it. Slots are a separate container with reverse Player-to-Slot links. |
| `src/game/backend/slot/Slot.cpp:79,98,159` | Normal SetPlayer/GetPlayerAndClose maintain reverse links. Deserialize bypasses that lifecycle: a new Player is not adopted by State, and an open/closed record leaves the old Player pointer behind. A later occupied record can reuse it. |
| `src/game/backend/connection/Client.cpp:46` | Initial PT_PLAYERS decodes directly into live Slots, adopts its Players, and queues player_join copies. Full roster updates clear Slots but retain the old Players in State, with obsolete Slot links. Individual PT_SLOT_UPDATE does not run the full-roster adoption loop. |
| `src/game/backend/connection/Server.cpp:145,234` | Authentication assigns a lobby slot or finds a running-game reconnect through linked GSID. The server then sends PT_PLAYERS, PT_GAME_STATE, PT_GLOBAL_SETTINGS, in that order. |
| `src/game/backend/connection/Client.cpp:86,185,346` | Settings handling reads slot zero and checks host readiness before decoding. Game-state announcement already waits for global settings. Sending settings before players would break the readiness dependency. |
| `src/game/backend/State.cpp:212`; `src/game/backend/faction/FactionManager.cpp:48,249` | State decodes settings and then replaces the faction registry. Manager decoding clears old factions first and can leave earlier complete records on failure. Canonical raw Faction pointers would become invalid on an ordinary later lobby settings update. |
| `src/game/backend/connection/Client.cpp:124`; `src/game/backend/connection/Server.cpp:254` | Individual slot updates mutate a live slot before their readiness checks. A canonical decoder must validate the incoming faction ID and applicable update rules before publication or broadcast. |
| `src/game/backend/connection/Connection.cpp:370,377` | Lobby events and OnOpen use State::WithGSE and can run later. Client and Server allocate temporary Player copies for join/leave callbacks. Original Player or faction lifetimes cannot be assumed to cover those copies. |

The lobby script makes notification order observable. Its Players section
enumerates `game.get_players()` and initializes `players_count`, then increments
that count for later `player_join` events
(`GLSMAC_data/default/ui/parts/mainmenu/steps/multiplayer_lobby/players.gls.js:141,155`).
The section is built by the connection-open callback
(`.../multiplayer_lobby.gls.js:103`). Deferring initial join notifications until
after that callback would count the initial roster twice even though row
creation itself checks for an existing row.

Player's GSE reference codec is a separate format: the WRAPIMPL serialization
at the end of Player.cpp stores a slot number and resolves its current Player.
Do not silently replace that codec while changing full Player records.

## Prerequisite: exclusion during native publication and retirement

Neither implementation PR below may introduce unsafe native destruction on
the backend thread. Game::Iterate calls State::Iterate and network dispatch
before its later accumulation (`Game.cpp:176,183,334`). Those handlers can
retire Player, Slot and Faction Wrappables while the collector or queued
script callbacks can still inspect them.

GSE construction selects the Space owner thread (`src/gse/GSE.cpp:22`).
State::WithGSE uses Space::Accumulate; a foreign-thread call queues work, and
`need_now` asserts that the caller already owns the Space
(`src/gc/Space.cpp:95`). Neither is a synchronous native retirement boundary.
Do not execute scripts on the backend thread or block the owner waiting for a
backend reset in order to work around this distinction.

The separately proposed State::WithGame session gate addresses stale queued
gameplay work and roots State/owner while that work is pending. It does not
provide native teardown exclusion and leaves generic lobby WTrigger unchanged.
Roster work must wait for a reviewed native exclusion mechanism, or establish
a proven owner-thread publication boundary with equivalent lifetime guarantees.
The test suite must exercise the real foreign-thread path; an owner-thread-only
fixture is insufficient. This document does not prescribe a competing GC API.

## PR 1: State-owned roster and slot lifecycle

Keep this prerequisite focused on native ownership, with the existing wire
format. Provide one adoption/retirement boundary for full roster replacement
and individual slot updates. Each live occupant belongs to State exactly once;
retirement clears both directions of the Slot link before releasing ownership.
Use matching tracked allocation/deletion for State-owned Players. Retired
wrappers must invalidate normally. Do not leave detached Players referring to
reallocated or destroyed Slots.

This PR does not claim to repair the existing detached Faction allocations;
that is the next PR. Avoid introducing temporary legacy faction-ownership
machinery solely to remove it in the next change.

### Occupant identity is an unresolved protocol contract

The existing Player record has no stable player identity distinct from its
name, role, and containing slot. Slot records also carry linked GSID, which the
server uses for reconnect lookup, but the current update decoder does not
establish it as an immutable, validated occupant-generation identity. Do not
infer replacement from a changed name, infer continuity from an unchanged
name, or use pointer/address equality as a wire identity.

Before implementation, choose and enforce the bounded update contract. One
candidate is to define occupied-to-occupied PT_SLOT_UPDATE as metadata for
the current occupant, require an explicit vacancy transition for replacement,
and treat a permitted full PT_PLAYERS snapshot as a new roster publication.
That contract must be enforced on both server input and client application,
including allowed session phases and reconnect behavior. It is not currently
an established guarantee. If real flows require distinguishing occupants
without such a transition, an explicit host-issued identity/generation field
and versioned protocol change are needed instead.

Until that choice is made, this plan does not promise to preserve a Player
object across an ambiguous update. The V3 world completion reader already
rejects actual Player/Slot replacement while a downloaded world is pending;
roster changes must not weaken its lifetime checks.

Queued join/leave copies also need a deliberate Slot contract. They cannot
retain a dereferenceable raw Slot after roster retirement. Audit actual
`is_ready`/mutation uses and choose either owned event snapshot fields or an
invalidatable binding; do not solve faction ownership while leaving a new
Slot use-after-free in the callback. Generic session callback retirement is
separate, not implicitly supplied by this PR.

## PR 2: canonical faction IDs and initial publication barrier

Player should own an optional exact faction ID and a lifetime-checked binding
to its owning State. GetFaction resolves through that State's current
FactionManager on each call. No Player owns or caches a Faction allocation.
Explicit absence retains the current random/unselected meaning; a selected
ID that cannot resolve is an error rather than a random-faction fallback.
Mutating native/script selection paths must validate against the same owner.

Event Player copies own their copied ID and establish their own resolver
lifetime binding. Existing Wrappable dependency notifications are a candidate
for invalidating the borrowed context, under the prerequisite exclusion.
The current blanket copy assignment is insufficient to register such a
dependency. Destruction of the original Player must not invalidate an otherwise
valid event copy; destruction of its resolver must prevent later dereference.

Proposed full Player wire format: typed `GLSMAC_PLAYER` marker, integer version
1, name, validated role, faction-presence Boolean, optional exact faction ID,
and difficulty string. Parse into owned temporary record values, validate
the complete record and trailing bytes, and only then apply. Do not add
completion flags here: the world V3 completion block is already authoritative
for downloaded continuation. Old embedded-Faction records are unsupported;
mixed peers cannot exchange these records successfully. This local format
guard is not top-level protocol negotiation or an original-game save contract.

The publication sequence should be:

1. Decode initial PT_PLAYERS into owned pending records, with bounded counts,
   valid slot states/flags, a valid local occupied slot, and an occupied host
   slot. Do not construct/publish unresolved live Players or issue roster
   callbacks. Keep the currently implemented server packet order.
2. Record PT_GAME_STATE without announcing startup. The settings readiness
   check uses the pending host record; it must retain the existing lobby vs.
   initializing/running policy.
3. Stage global settings and a faction registry. Preserve the existing locked
   ruleset/numeric-profile checks. Resolve every selected pending faction ID
   before publishing anything. Unknown IDs are protocol errors.
4. Under the proven publication/exclusion boundary, commit the registry and
   roster, bind the local Player, and mark settings ready. Enqueue initial
   roster notifications before OnOpen, then announce game state. Test both
   immediate-owner and queued-owner delivery; do not assume enqueueing runs
   a callback synchronously.

Reject duplicate initial rosters, missing host/local occupants, unexpected
slot updates before initial settings, and invalid record versions instead
of inventing defaults or an unbounded pre-initialization packet queue. This
policy must be checked against the actual server send sequence in tests.
Clear pending records on disconnect, cancellation, error and a fresh session;
do not confuse handler replacement during startup with a fresh connection.

For repeated lobby settings, parse a temporary registry and validate selected
IDs against it before replacing live contents. Preserve the existing manager
object and its wrapper identity; swap only its owned faction contents. Stage
settings using the current identity lock rather than a fresh unlocked default.
A missing selected ID or malformed registry must leave the previous settings,
registry and Players usable. This adds a transaction at the settings/registry
publication boundary; it does not retroactively make every world-body reader
transactional. Allocation-failure recovery needs its own explicit contract.

Ordinary slot updates must reject unknown IDs before live mutation and server
broadcast. Repeated running-game settings remain rejected. Other native
borrowers, including Base faction pointers, are outside this Player repair;
do not claim arbitrary runtime FactionManager removal/replacement is safe.
An event copy whose selected ID is absent after an otherwise allowed registry
change must report an invalid reference, not dereference freed memory.

## Evidence required before the PRs are ready

These are planned tests, not results of this audit. Reuse the actual packet
dispatcher exposed by RulesetClient in `src/gse/tests/Backend.cpp:1128`; add a
similarly narrow Server fixture where necessary. Do not substitute a mock
decoder for the production publication path.

- Ownership: open/occupied/update/open/occupied and full roster refresh, State
  adoption exactly once, reverse links, agreed occupant-identity semantics,
  invalidation of retired wrappers, and cleanup after a failed decode.
- Actual PT_PLAYERS -> PT_GAME_STATE -> PT_GLOBAL_SETTINGS for lobby and running
  rejoin, including a ready host where the current phase permits it. No early
  callbacks or startup; every published selected Faction pointer equals the
  destination manager's lookup result.
- Immediate and foreign-thread queued callbacks, actual script get_faction,
  and the production lobby section with a lightweight UI factory to verify
  initial player counts and notification order without claiming a rendered
  multiplayer smoke test.
- Selected/unselected records; bad markers/versions/roles, invalid flags,
  unknown IDs, impossible counts, truncation and trailing bytes. Rejection
  must precede the mutation/publication guaranteed by the relevant API.
- Same-ID faction registry replacement with changed metadata; live Players
  and pending event copies resolve the replacement canonical objects. A
  replacement omitting an active selection leaves the previous state intact.
- Event copy after original Player destruction, copied resolver registration,
  resolver destruction, delayed cleanup, and the separate slot-based GSE
  Player reference round trip. No copied raw pointer lifetime assumptions.
- Pending join cancellation/reset, repeated settings, and roster replacement
  during V3 world initialization. Saved completion/RNG publication must retain
  its current rejection behavior when Player lifetimes change.
- Deterministic collector overlap with native retirement under the eventual
  exclusion boundary, tracked-allocation checks, and ASan/UBSan/LSan. Old-code
  negative controls should demonstrate the detached/canonical pointer defect
  and the missing roster ownership rather than relying on allocator reuse.

No production files or research evidence were changed for this plan. No tests
were executed to claim the proposed behavior; the evidence here is source
inspection. Canonical restoration remains a persistent-simulation priority after
the native lifetime prerequisite is established.
