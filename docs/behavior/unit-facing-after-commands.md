# Retained Unit facing after movement and combat

The original stores facing in Unit byte`+0x29`. A freshly constructed Scout
receives2 at`5d84fb` (`mov byte ptr [eax+9951d9],2`). This is a factory result,
not a migration default for a save with no retained facing.

[Thirty complete original wrapper controls](unit-facing-after-commands-cases.json)
cover eight ordinary directions, five refusal/partial-point conditions in both
offscreen and focused-human display contexts, and four hostile outcomes. All
execute `5466c0` through normal return. The ordinary fixtures use original-created
design2/owner1 Scouts, Q3, flat empty128×64 land, no Base/pod/ZOC/transport.
The human context uses the inherited no-input event flush and UI cleanup services;
the offscreen context excludes local-player rendering. The observed retained-state
stores, position, allowance, admission, RNG and combat outcome instructions execute
unchanged; display-renderer boundaries are stated separately below.

## Direction identity

Offsets are original staggered map coordinates, not screen pixels. Every accepted
ordinary request stores exactly its direction code, including code5 when the
prior value is already5.

| Code | Coordinate offset | Direction |
|---|---|---|
|0|`(+1,-1)`|NE|
|1|`(+2,0)`|E|
|2|`(+1,+1)`|SE|
|3|`(0,+2)`|S|
|4|`(-1,+1)`|SW|
|5|`(-2,0)`|W|
|6|`(-1,-1)`|NW|
|7|`(0,-2)`|N|

The focused-human path first writes the requested facing at`570ebe`; the
offscreen path uses`5af979`. At those stores the Unit is temporarily unplaced
at`(-2,-2)`. The common completed-command tail writes it again at`5b11df`,
after the new position is retained. Publish the completed owned transaction,
not these temporary coordinates or partial presentation steps.

## Refusals and return values

All controls start at`(8,8)` with retained facing5. The following results agree
in both display contexts. The plain edge costs3.

| Request | Wrapper return | Position | Spent after | Facing / writes |
|---|---|---|---|---|
|Exhausted, spent3|0|`(8,8)`|3|5 / none|
|Overspent, spent4|0|`(8,8)`|4|5 / none|
|Land Scout into empty water|0|`(8,8)`|0|5 / none|
|Partial movement, spent2, failed lottery seed1|1|`(8,8)`|3|5 / none|
|Partial movement, spent2, successful lottery seed3|1|`(9,9)`|5|2 / ordinary stores|

**A truthy wrapper return does not prove movement or a facing change.** The failed
lottery returns1 too. Use the actual accepted movement commit. The existing
selected-unit input gate also refuses exhausted input before emitting a command;
these exhausted full-wrapper controls are explicitly lower-level requests.

## Hostile outcomes and stable identity

The accepted focused-human hostile harness executes actual rules/rounds/casualty,
diplomacy and full wrapper continuation. It supplies frozen headless redraw,
event, notification and teardown services; its unit renderer supplies zero
animation frames. Initial attacker facing5 and defender facing6 discriminate
retention from a default. These controls do not prove attack-frame presentation.

* Neutral attacker wins, CRT seed3: attacker remains`(8,8)`, defender is deleted,
  and the common tail`5b11df` changes attacker facing5→2. This agrees with the
  AI-defender control and Elite/seed1 control. Wrapper return is0. A frontend
  update must carry the new facing even though the attacker did not move.
* Defender wins, both damage9/CRT seed308: attacker is removed; defender remains
  at`(9,9)` with facing6. Native dense compaction copies its whole record into
  slot0 (`rep movsd` at`5d8af7` includes the byte). There is no attack-direction
  store to the survivor. The numeric pool-index change is not a rotation or a
  new Unit. Preserve the surviving stable identity and remove the dead attacker.

No temporary battle-facing writes were observed in these zero-frame controls.
Do not infer initialized animation, renderer-local orientation, generalized
combat or an always-restore-prior-facing rule from that observation.

## Port boundary

Retain optional per-Unit facing separately from optional source-pool/stack links.
For older conventional records, absence remains absence; the already retained
definition view remains the explicit presentation fallback. New factory state
may retain its measured2. Actual accepted ordinary movement changes facing;
refusals preserve it. Successful hostile continuation must update the surviving
attacker even without a position change, and must not modify a replacement Unit
that inherited the dead attacker's dense pool index.

Publish the optional owned value in both Unit spawn and update packets, initialize
absence explicitly in every producer/copy path, and refresh both the main-map
body and selected/roster cards. Per-Unit queries must not mutate the shared UnitDef
or turn another Unit. Native model/cache geometry and card scales remain those
of `initialized-console-scout.md`; this contract supplies retained orientation,
not animation timing. A distinct next Unit wire extension must preserve the
existing optional stack codec and older bytes.

The public cases pin the executable, producer and private receipt hashes. Private
scripts/buffers stay in the isolated `tmp/unit-facing-contract` sandbox with copied
frozen dependencies and owned original executable/assets; the primary checkout is
read-only. The portable checker verifies all literal direction/refusal/hostile
controls; optional private-receipt checking also binds the full original records.
