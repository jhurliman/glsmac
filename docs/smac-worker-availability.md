# Detached worker-square availability

`SmacWorkerAvailability::Build` returns the ordinary unavailable-square mask
for the evaluating Base's 21 ordered radius positions. It reuses
`SmacWorkedSquareCollection::Resolve` for doubled-X coordinates, at most one
horizontal wrap adjustment followed by bounds checking, and vertical clipping.
It does not mutate the selected workers,
allocate population, collect yields or publish world state.

The caller supplies owned values for exploration, all-map revelation, valid
Base markers, sea classification, recognized territorial owners, ordered unit
presence, unit visibility, active convoys, and directed Pact/Treaty relations.
Recognized ownership is already the result of the territory query, **not a raw
claim**. The builder does not infer recognition from exploration or diplomacy.
Playable simulation identities are 1..7; unit owner 0 explicitly denotes native
units. No identity is derived from a transport slot or faction name.

Clipped coordinates are unavailable. A valid square is unavailable when it is
unexplored without the revelation override, contains a Base marker, has a
recognized foreign owner, contains any active convoy, or contains a visible
foreign unit without the evaluator's directed Pact or Treaty toward that owner.
Visibility and diplomacy never exempt a convoy. The revelation override only
bypasses exploration. A friendly ordinary unit does not itself block the square.

Other Bases reserve their center regardless of its selected bit, and selected
radius positions 1..20. Same-owner reservations apply to land and sea; foreign
reservations apply to sea only. The complete `uint32_t` selection is retained
as input, with higher bits ignored by this particular stage. There is no
population or Base-count limit borrowed from allocation. The evaluating Base
ID may appear in the supplied list only with matching owner and center and is
skipped. Other IDs must be unique.

All request shape, identity and coordinate checks run before computing a
result. Optional squares must match clipping exactly. Coordinate aliases must
contain equal owned values, including the ordered unit sequence. No borrowed
pointer or callable escapes. Alias consistency and the broad integer resolver
domain are explicit port admission checks; the original unavailable-builder
comparison uses the reference's 128-by-64 maps, not a narrow-map gameplay claim.
The shared resolver's single-adjustment rule comes from the original collection
instructions. Width-two tests distinguish it from repeated modulo wrapping.

## Original evidence and comparison

The contract is RE-SMAC-WORKER-AVAILABLE-001 in the human-owned
`docs/behavior/worker-square-unavailability.md`. The original complete builder
at `0x4fc140` and selected-mask cleanup at `0x4fd6c9..0x4fd6e1` execute with
actual unit/stack lookup and recognized-territory helpers in ordinary mode.
Those receipts supply resolved state; they do not prove its live producers.

The committed projector recreates only the retained input schedule: 36 named
cases, 500 deterministic spatial cases and 200 unit-stack cases. It never
imports or executes the research checker, its expected-value model, or original
machine code. It copies only `actual_mask` and the actual retained selection
from the private report. Native tests compare all 736 masks, then feed each
mask into the real detached collection to compare the 736 cleanup results.
That cleanup composition supplies population 127 and a zero-yield callback;
it is not an additional original resource or population calculation.
Classification/reason bitfields and full live availability are not claimed.

Pinned SHA256 values:

- Public 36-case fixture: `51b646e776efef392aa1485c372a5c3c2a81df351d581f3c9904730b93e3460f`.
- Private original 736-result report: `877b9f9b93004adc047c813a71460281de161c74b596248636a70d98ec9e80c2`.
- Original executable: `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
- Retained checker from which the input schedule was inspected: `f9b0eae8d0a3481db02feaabee8c7bf01a9ec86949e252b9188b8ddd646d9f6c`.

The report binds its public fixture and executable, but does not contain its
700 generated input records or a checker hash. The projector's explicit input
schedule and the inspected checker fingerprint make that reconstruction
reviewable; no generated expected-mask model substitutes for captured results.
The private report and executable remain outside Git.

## Persistent-state audit for the next live consumer

The current backend has no authoritative exploration, remembered improvements,
raw territorial claims, directed recognition state, all-map revelation, unit
visibility by faction, or convoy orders. `SmacMovementHistory` is placement
history and retained nearest distance; it explicitly is not exploration.
Frontend unit hiding concerns stack presentation and cannot supply visibility.
`Tile` serializes geometry and terrain fields, and `Unit` serializes its current
tagged gameplay record; neither supplies these missing inputs.

The legacy `get_base_workable_tiles` callback walks neighbor pointers and
deduplicates them. Pole neighbors can alias the same tile. This is not the
original indexed radius traversal. `BaseManager::ValidateAssignment` checks
coordinate bounds/parity, non-center, worker type and unique coordinate
reservation; it does not itself check the radius or original availability.
Existing `Pop` assignments remain the allocation authority. A future live
adapter must produce a detached unavailable mask for collection cleanup,
preserving selected and visited masks as separate values.

RE-SMAC-FIRST-WORKER-MAP-001 now establishes a narrower real producer chain:
the startup reveal loop (`0x5c9d31..0x5c9dc0`) explores the 21 positions and
copies current improvements before founding (`0x4fa490`). Actual territory
rebuild (`0x538e80`) then claims those positions in the measured single-base
connected-land scenario. The ordinary visibility update (`0x5cdaa0`) happens
later and does not replace the earlier reveal. A produced visible native unit
can subsequently block an owned square without territory recognition.

That fixture SHA256 is
`6e59ceb03faab5061ca86b3e8c9e324902d1b4665128c5fc3c8b9e2ee8de648e`;
the bound 7-composition/17-reveal-case receipt is
`9c32378bc74aee9c4f7ffd5fed5cc9310aa09a4de7cfdb630f446a378c54fc0b`.
This slice reads that evidence but does not implement or rerun its producer.

The proposed later ownership boundary is optional Map-owned knowledge keyed
by explicit simulation identity: exploration, remembered port improvements and
raw claims with explicit recognition provenance. Its separately bounded world
frame should be validated against saved dimensions/roster before Map allocation.
Dense map data must not be squeezed into Simulation's existing 64-KiB budget.
Restoration must preserve exact state and never replay reveal, founding or sight.
The initial producer must run under claimed startup ownership before the first
canonical Base calculation, on an explicitly admitted controlled scenario.

The later RE-SMAC-WORKER-MAP-TRANSITIONS-001 receipt follows seven continuous
controlled sequences: starting-unit movement grows exploration from 21 to 22
squares, a nearby Colony factory placement grows it to 25, and second founding
leaves it at 25. The second Base still has six unexplored radius positions
despite all 21 being owned. A visible native then blocks a square in both
overlapping radii. Its fixture SHA256 is
`241a23e559be9564cb2d6183b5ff690ac0562af59e7d20e7f90e19694905e1bd`.
This is additional producer evidence, not a producer implemented or rerun here.

No blanket explored/owned overlay is justified. Those controlled movement and
second-base transitions can guide the next producer, while general terrain
sight, visibility lifetime, playable-faction recognition and orders still need
their own produced inputs. Until the admitted producers exist, a live adapter
must reject unsupported contexts rather than supply neutral relationships or
invisible foreign units. Even friendly Unit admission needs an explicit
supported no-convoy/order contract. These remain future integration decisions;
this feature adds no persistent state, wire version or default activation.

## Validation

The five new native groups cover all 736 original masks and cleanup results,
directed relations and visibility-independent convoys, mandatory centers and
full-width selections, coordinate clipping/aliases, and 14 malformed request
shapes. The projector's `--check` reproduces the committed fixture exactly.

Full normal CTest passed 8/8 in 93.94 seconds; leak-enabled ASan/UBSan/LSan CTest
passed 8/8 in 202.87 seconds. These full runs preceded the separate shared
resolver correction from modulo to one horizontal adjustment. After inheriting
that correction and strengthening the width-two assertion, all 16 availability,
collection and allocator groups passed with allocation tracking in both builds
(1.01 and 1.05 seconds). Final explicit LLVM 15 analysis of the three production
translation units reported zero failures (4.79 seconds command wall time).

Five copied-translation-unit controls independently fail on visibility-gated
convoys, foreign land reservations, omitted mandatory centers, the wrong
directed relationship and conflicting aliases. The final narrow-map assertion
also rejects a copied old-modulo resolver; the alias control was repeated after
the shared correction. Controls replace only private compiled objects and never
mutate the production checkout. They are regression sensitivity checks, not new
original executions or proof of persistent/live worker admission.
