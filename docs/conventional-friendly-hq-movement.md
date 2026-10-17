# Conventional movement at one friendly HQ

This extends the live `move_unit` route from the globally base-free checkpoint in
[conventional empty-ground movement](conventional-empty-ground-movement.md).
A healthy, explicitly created design-2 Infantry Scout can leave or enter one
friendly HQ under the measured context below. It does not activate default
starting units, founding, home assignments or selected Spartan Rover movement.

## Admitted context

The saved `NoProjectsOrdinaryTerrain` capability and existing local single-player,
canonical participant, living Unit, destination occupancy, locking and animation
checks remain required. The capability is authoritative port campaign admission;
facility absence is not used to infer that no projects exist.

When the world contains a Base, this extension requires all of the following:

- Exactly one registered Base and one registered Unit.
- The Base is canonical, belongs to this Game and the Unit's accepted owner and
  faction, has population one, and contains exactly the canonical
  `HEADQUARTERS` facility definition. This is an explicit port logical-ID mapping,
  not an English-name match or an inferred original source-slot classification.
- The HQ is at one endpoint and its tile points back to that Base. The other
  endpoint has no Base. Remote or additional bases remain unsupported.
- The Unit has design ID 2, Infantry chassis, stored morale 1 and zero damage.
  Existing conventional admission establishes Gun/Scout/reactor1/no abilities.
  Frozen Q, basic allowance and full mode-1 allowance are all 3. Retained MORALE
  and PLANET are both zero. These are current receipt limits, not universal
  movement or faction rules.
- The HQ square is flat with no separate improvement/feature bits. The adjacent
  flat land square is ordinary, road, tube or fungus, with no combinations.
  Forest, river, rocky terrain and improved/fungus combinations in an HQ world
  remain outside this extension even where a pure edge-cost calculation exists.

The endpoint relationship also bounds the nearest-base context for HQ-to-fungus
movement: this HQ is adjacent to the destination. No distant-base branch is
inferred. All current conventional Units have `Home::None`; the original receipt's
created-home cases are intentionally excluded.

After this complete admission, the canonical HQ endpoint contributes the existing
pure edge kernel's `Base` bit. No alternative cost or lottery calculation is
introduced. A base supplies a road/tube endpoint but does not bypass actionability:
spent exhaustion still rejects before pricing, even when the raw tube cost is
zero. Ordinary HQ edges cost Q; HQ/road costs one; HQ/tube costs zero; HQ-to-fungus
still executes the existing strict partial-point lottery.

Preparation, destination/update allocation and the synchronous Unit/tile/paired-
RNG authority commit are unchanged. The movement operation does not mutate Base,
facilities, population or tile feature bits. Presentation remains postcommit,
without animation-owned authority. The continuation format remains Simulation V5,
World V15 and Unit/manager V2; no new schema is needed for existing Base records.

## Reference projections

The independent human-owned RE-SMAC-FRIENDLY-BASE-MOVE-001 contract executes 140
complete original requests for a prepared healthy Infantry/Gun/Scout/R1 design2,
Q3, one same-owner HQ/population1, neutral retained PLANET, no projects or other
units. It is not a complete new-game/founding or local display run.

The native fixture contains 70 literal `actual` result projections selected by
`home_mode == homeless`: 42 HQ-departure, 19 HQ-arrival and 9 empty-world controls.
The other 70 created-home cases are excluded. The test supplies the same spent
byte, original feature choice and shared seed; it compares resulting spent,
failure marker, shared word and source/destination membership through the actual
port event. It also checks unchanged CRT, JSF, IDs, Base serialization and terrain.
Original absolute coordinates, lookup implementation, callbacks, visibility,
wrapper return value and local display output are not compared. The port fixture
uses adjacent doubled coordinates on its existing 8x8 synthetic Map.

Pinned evidence, retained outside Git:

- Executable SHA256:
  `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
- `alpha.txt` SHA256:
  `38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4`.
- `docs/behavior/friendly-base-movement-cases.json` SHA256:
  `0cec28996e4efad56b373f3a0a61c702295d307a27c24155d5298ad0a7a33b20`.
- `tmp/reference/friendly-base-movement-original-x86-results-20261017.json`
  SHA256: `afd61ed5b9dc2aea531d5309180534780a4f93d148dffb8c287bb6eb768e398c`.
- The report's independent checker SHA256:
  `c1cce0b43c46322a5f549d0df291ec20ed9011201d4d3b2a27f1da1fc6d1c35e`.

No original implementation, asset or report is copied into the repository. The
small owned fixture records public inputs and captured scalar outcomes only.

## Validation

Three added native groups cover the 70 projections, 22 unsupported/corrupt native
contexts and actual whole-world writer/body restoration. The restore test checks
canonical HQ facility references, exact bytes, saved failed attempt and RNG, then
runs genuine upkeep and another actual move event in uninterrupted/restored worlds.
Upkeep preserves the failure marker; the subsequent successful step clears it
according to the existing kernel. Both complete final world serializations match.
This is the existing synthetic-map checkpoint route, not a socket or rendered test.

The invalid-native fixture drains valid spawn presentation before deliberately
corrupting metadata; it tests admission and restores those temporary links before
teardown. It does not claim arbitrary malformed Base presentation is safe.

Source checks on parent `4fa1514d` plus this change:

- Full normal CTest: 8/8, 81.43 seconds.
- Full leak-enabled ASan/UBSan/LSan CTest: 8/8, 164.38 seconds.
- Seven focused movement groups pass with allocation tracking in both builds.
- The sole changed production translation unit passes LLVM15 analysis with zero
  failures in 2.65 seconds. No shared production header changes are introduced.
- The old production source rejects the first HQ case. Two copied-TU controls
  fail the intended assertions: omit the Base bit (original case056), and skip
  canonical HQ metadata validation (missing population, scenario5). These controls
  leave intended source and application binaries unchanged.

Logs and a final hash manifest are retained under
`/home/jhurliman/src/glsmac-unit-weapon-parser/friendly-hq-*` outside Git.

A subsequent [selected Rover extension](conventional-selected-rover-hq-movement.md)
admits its separately measured startup-selected Spartan context. The Infantry
receipt and restrictions documented here remain unchanged.
