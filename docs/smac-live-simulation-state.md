# Live SMAC simulation admission and technology continuation

SMAC Game startup now owns an explicit simulation roster and its mutable
technology state. This connects the existing [technology value](smac-runtime-technology-state.md)
to the actual `create_world`/`start` consumer boundary and world download.
The [startup design extension](smac-startup-design-state.md) now adds owned
Morgan/Spartan designs and world V10. This earlier receipt documents the
identity/technology V9 boundary; neither slice creates live conventional units
or activates research upkeep.

## Admission and identity

At the claimed owner `Start` checkpoint, an authoritative new world enumerates
its canonical occupied slots in ascending order and allocates dense simulation
IDs 1 through the participant count. Slots 0, 2 and 6 therefore receive IDs
1, 2 and 3. This is an explicit GLSMAC new-world admission policy, not a claim
about original-game faction numbering. Empty slots create no simulation state;
a single-player world admits its one participant.

Each owned record retains its simulation ID, transport slot, GSID, controller
role and selected frozen faction-source ID. The paired `SmacTechnologyState`
is initialized through the existing `NewWorld` producer exactly once. Named
TECH ownership is available before `create_world`, hence before an eventual
founding consumer. Admission consumes no RNG. The ordinary first-turn event
still changes turn 0 to 1 after the create/start event batches.

A runtime binding separately retains the accepted roster epoch, Game session
and each canonical Player's invalidatable identity token. Validation checks
these tokens and the complete occupied roster before reading current Player
metadata. Source ID/GSID/role are checks against the already accepted occupant,
not a search or rebinding fallback. Display names do not identify simulation
factions. Same-occupant canonical refresh and retained reconnect keep the
existing Player token and simulation state. Replacement, a new epoch/session,
changed source identity, or an added/missing occupant invalidates the binding.
Takeover, AI admission and late new participants require a separate policy.

## Wire and restore

The world format is V9. V1 through V8 and unknown versions are unsupported; no
stable disk-save compatibility contract is introduced. After the existing
frozen profiles, RNG and turn/completion prefix, the writer appends:

1. A canonical Bool indicating mutable SMAC simulation presence.
2. For SMAC, one framed owned simulation payload: `SMSS` integer magic,
   payload version 1, participant count, the five fields of each record, and
   one framed technology-state V1 payload.
3. The existing map and manager body, unchanged in order and content.

SMAC requires the payload; SMAX requires its absence. SMAX gameplay receives
no inferred SMAC state. The payload preserves explicit saved IDs, including
non-dense/permuted IDs, exact ownership and progress. Restore never reruns
new-world grants or the ID allocator.

Port ingestion limits are 1..7 participants, simulation IDs 1..7, controller
slots 0..6, unique IDs/slots/GSIDs, nonempty NUL-free controller/source text,
32 KiB combined text, 64 KiB total runtime payload, and the existing 8 KiB
technology payload. Integer widths, roles, complete pairing with frozen
source profiles, nested versions and trailing bytes are validated. These are
GLSMAC admission limits, not claims about the original executable's handling
of malformed input. World downloads are chunked; this payload is not added to
the unchunked global-settings packet.

`Buffer::ReadString` accepts an optional length limit, checked on the complete
frame before allocating payload storage or advancing the cursor. Existing
callers and the byte layout are unchanged. The actual Game decoder applies
the 64 KiB limit, decodes owned records, and binds them to the accepted canonical
roster before reading/allocating a replacement Map. The separate map-size check
still precedes Map allocation. Prefix rejection follows the existing safe
`Snapshot format mismatch` disconnect path without publishing runtime values,
turn/RNG continuation, setup callbacks or replacement world objects.

After successful body reconstruction the paired value remains staged. Claimed
Start validates and publishes it before the `start` hook, then validates again
after that hook and deferred unit/base registration. This precedes the existing
late turn/completion and RNG installation. Reset/Stop clear active and pending
values; failed completion clears both; canceled queued Start cannot publish.
The existing startup coordinator and native retirement exclusion own execution,
not a new independent locking or synchronous marshaling protocol.

As with the existing download path, a later malformed map/manager body or a
throwing setup hook is a terminal initialization failure, not transactional
rollback of all already reconstructed objects or script effects. This is world
continuation support, not a complete save system or multiplayer lockstep proof.

## Read-only gameplay boundary

`game.get_simulation_faction_id(player)` returns the admitted ID.
`game.get_technology_state(player)` returns new scalar/array values containing
the source ID, 89 ownership flags, score, Future Tech count, labs, optional
target and optional cached threshold. The argument is the canonical Player
wrapper already returned by `game.get_players()` (Slot wrapping uses Player).
Foreign/stale/unadmitted participants and unavailable rulesets fail explicitly.
No native pointer-backed field or mutator escapes; changing returned arrays or
scalars cannot change stored state.

The next consumer remains the [startup integration sequence](simulation-roster-binding-plan.md):
owned custom designs keyed by these explicit IDs, then founding production
selection/credit and later research. Transport slots remain event/controller
identities; this change does not silently replace existing unit/base ownership
fields with simulation IDs.

## Validation boundary

Native tests exercise the actual claimed Game Start path with single-player,
host, sparse occupied slots and queued owner execution; real GSE create/start
observers read named grants at turn 0, mutate returned copies, and verify the
stored state remains unchanged before the actual first-turn handler runs.
Pending and Running cancellation use real MT reset requests.

Restore tests serialize through the real writer, establish the real Client
roster -> game-state -> global-settings handshake, decode the actual runtime
and body into the existing asset-free Map fixture, and run claimed Start.
Saved IDs 7/2/5, changed ownership, signed endpoint progress, RNG, turn/completion,
and full snapshot bytes survive. Real canonical refresh packets and retained
State reconnect preserve PlayerIdentity. Before/after-start metadata changes,
same-GSID replacement with a new Player token, and reset discard the paired
continuation. These tests exercise real production checkpoints; they do not
claim successful asset-bearing Map construction or rendered network startup.

Immediate and queued final-chunk Client handlers reject malformed runtime
records before Map allocation. A separate assertion invokes the exact production
prefix decoder, so failure at a later missing map cannot hide a missing runtime
check. Valid runtime followed by missing/mistyped map frames or mismatched map
dimensions continues to test those later boundaries. Existing SMAX writer/restore
controls and unexpected-presence rejection remain covered. Client fixtures
invoke its real packet dispatcher directly; they do not claim socket or Network
MT polling coverage.

On the integrated source ancestry `8328d1ff` with this change:

- Full normal CTest: 8/8, 78.23 seconds.
- Full ASan + UBSan + LSan (`detect_leaks=1`, non-PIE executable): 8/8,
  138.29 seconds.
- All 21 new native groups pass with `--memorydebug` in both builds.
- Full correctness analysis: all 316 production translation units, LLVM 15,
  zero failures in 243.16 seconds.
- Five independently compiled copied-production controls fail their intended
  assertions (exit 1): derive ID from slot+1, replay named grants on restore,
  skip controller metadata checks before Map, omit post-start binding validation,
  and omit the framed string cap. Neither source checkout nor normal objects
  were replaced for these controls.

Retained local evidence is under
`/home/jhurliman/src/glsmac-unit-weapon-parser/`: `simulation-ctest-final.log`,
`simulation-asan-ctest-final.log`, `simulation-final-tracked.log`,
`simulation-final-asan-tracked.log`, `simulation-analysis/summary.json`, and
`simulation-controls/results.json` with per-control logs and copied TUs.
The initial full runs exposed an older turn fixture that bypassed SMAC admission;
it was updated to use canonical participants before writing its snapshot, then
both complete suites above were rerun. Those prior failed receipts remain
retained. No original assets or reference executable outputs are included in
this change, and the unchanged technology producer's earlier original-code
comparison is not claimed as a new run.


## Focused publication receipt

The publication implementation `647b2482` applies this change to dedicated base
`202eccf5`. That base preserves the histories of technology state PR131,
frozen-unit world PR129, initial-turn PR113, and refreshed canonical-roster
PR126 at `991b810a` (including the switch-scope ownership prerequisite PR66).
It also applies the existing canonical settings-envelope admission correction
from `9dc3c6d3`. These remain explicit review dependencies; this receipt does
not assert that they have merged.

The final feature delta remains 19 files. Production implementation files,
the new codec/native tests and GSE script match source `7eb1fef3`; the Game
header's existing State include and test registrations are placed to match
this ancestry. Independent merge review confirmed all parent registrations
and test groups are retained, apart from the two obsolete roster-failure
cases already replaced by PR126's actual checkpoint regressions.

On the actual publication implementation:

- Full normal CTest: 8/8, 74.71 seconds.
- All 21 new native groups pass with `--memorydebug`.
- All five copied-production controls above fail their intended assertions,
  exit 1; their objects/executables remain separate from the normal build.
- Explicit correctness analysis of Game, GameSimulation, SmacSimulationState
  and Buffer: four production translation units, LLVM 15, zero failures in
  17.02 seconds. This targeted publication run is separate from the full
  316-unit source analysis above.

The same source was independently integrated as `39172a7d` for Windows
CTest 8/8 (80.65 seconds), Linux ASan/UBSan/LSan CTest 8/8 (138.94 seconds),
and native macOS CTest 8/8 (57.99 seconds). These are combined integration
receipts, not an additional sanitizer or native-Mac run on this publication
ancestry, and they make no rendered or socket-startup claim.

Publication logs use the `simulation-publication-` prefix in the same retained
validation directory, including `ctest.log`, `tracked.log`, `controls.log`,
the `controls/results.json` directory and `analysis/summary.json`. Source
receipts and earlier failed runs remain separate and unchanged.
