# Owned worker-map consumer frame

`map::SmacWorkerMapFrame` owns the inputs needed by the detached worker-map
query. It composes the checked availability builder and radius resolver with
raw-claim recognition. It does not attach to Map, change a world version,
initialize knowledge, or bind records to live Units or Bases.

The source is based on `dbf113c80a19a7b84b41ad8a92c697400a7af037`. Its original
boundary is [WORKER-FRAME-REPLAY-001](behavior/worker-map-frame-replay.md): 35
captured states, 49 Base evaluations, 210 omission replays and five additional
JSON-roundtrip replays, with five unique semantic inputs. The original fresh
consumer guests execute their selected-base, availability and recognized-owner
queries without supplied helper returns or instruction patches. Their preceding
startup/transition producer execution retains the limitations of that receipt.

## Ownership and query boundary

`Create` validates then copies a complete `Data`; later caller changes cannot
mutate the frame. `CopyData` and `Evaluate` return detached owned values. Copying,
moving and decoding preserve record order and every supplied field. No default
frame or empty knowledge is produced for an existing world.

The frame has one explicit playable observer, geometry/wrap, an explicit
ordinary-local mode, foreign-disclosure and all-map-revelation state. It retains
two separate resolved directed predicates for each owner: foreign-unit exemption
and territory recognition. It does not assign diplomacy meanings or producers
to those predicates. Other worker modes are rejected.

Dense tile records follow Y ascending and X from row parity in steps of two.
They own sea classification, the full eight-bit exploration mask and an optional
raw claimant. None and native0 remain distinct stored values; neither yields a
positive recognized owner. An own positive claim is recognized directly. A
foreign positive claim requires the observer's recognition predicate or the
explicit disclosure override, as established by the
[recognized-territory query](behavior/recognized-territory-and-buster-integration.md).
Exploration and all-map revelation do not change that query.

Ordered Base records own owner, coordinate and an explicitly projected
`selected_radius` mask. It admits exactly bits0..20. Upper bits reject rather
than silently becoming zero: this is not the complete selected32 authority used
by collection/repair. Ordered Unit presence records own owner0..7, coordinate,
all eight visibility bits and the active-convoy predicate. Native ownership and
native knowledge bits are preserved literally; they are not attached to an
existing playable-slot Unit. List positions identify only the supplied consumer
records. No ID translation, entity creation, order inference or stack sorting
occurs. Multiple Unit records may occupy one coordinate; Base centers must be
unique.

`Evaluate(base_index)` requires that Base to belong to the frame's observer. It
derives markers/stacks from the complete supplied lists and resolves all21
coordinates through `SmacWorkedSquareCollection::Resolve`. That includes one X
adjustment, then bounds checking, and vertical clipping. Dense storage gives
coordinate aliases exactly equal knowledge. The existing availability builder
handles all-map revelation, visible foreign/native Units, visibility-independent
convoys and other-Base reservations. The resolved foreign-unit exemption enters
the builder's Pact-or-Treaty branch; this is not a new Pact producer.

The result contains stored selected21, unavailable mask, and each admitted
coordinate's exploration, raw claim and recognized owner. It does not expose the
original classification-reason bitset. It does not run resource yields, selected
mask cleanup, specialist bookkeeping, allocation or command producers.

## Codec and bounds

Version1 is an explicit little-endian port encoding, not a C++ memory dump or an
original save format. Its 56-byte header stores magic `SWMF`, version, dimensions,
wrap, observer, ordinary-local flag, disclosure, revelation and the two directed
predicate masks, followed by tile/Base/Unit counts. A tile record occupies three
bytes, a Base record 13 bytes and a Unit record 11 bytes. None claimant encodes
as 255; native0 retains
its own byte. Coordinates and selected masks use unsigned little-endian32 words
whose valid domain is checked before signed conversion.

The aggregate port budget is 2 MiB. Geometry admits positive even width 2..512 and
height 1..256, at most 65,536 parity coordinates. This storage/query range is not a
claim that such tiny or odd-height maps can be generated or used by live Map.
Base and Unit counts fit the remaining fixed-width budget; there is no Unit-per-
tile or population limit. Duplicate Base centers are incoherent marker inputs,
not an original gameplay maximum.

Decode checks the outer string length, header, dimension product, exact tile
count, each count's remaining-byte budget and exact total length before owned
record allocation. It then rejects malformed flags, owners, coordinates/parity,
duplicate Base centers and high selected bits before returning a frame. No
existing frame changes on failure. There is no outer `Buffer::ReadString` caller
in this slice; a future world integration must apply `MAX_WIRE_BYTES` before
copying the nested frame. The nested decoder cannot undo a caller's prior string
allocation. Live wire versions remain unchanged.

## Literal comparison and native tests

`tools/reference/project_smac_worker_map_frame.py` reads the public semantic
inputs and private literal original report. It imports no research module or
expected-output model. It verifies ordered faction/stage identity, the captured
input's semantic digest, original replay equality, six omission names, and exact
output equality before deduplicating the five repeated frames. The generated
fixture retains all 35 ordered state references and all captured unavailable,
selected, raw-claim, recognized-owner and exploration outputs. Classification
reasons are explicitly excluded from the comparison.

Pinned SHA256 values:

- Public fixture: `a6b11009ef265f2c44b0448ffff2a7c4d7c095a41103af262cd6a403740d1457`.
- Literal report: `44a9e50623dd06d6474b4f78527da83c6a7960902c43aed45a2af60d8afcd086`.
- Original executable: `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
- Original checker: `d2f0cde498944e24c485117c5d576f5b923b68502b0e6e57d6f12b274dc17b74`.
- Original replay support: `ea4e1eae9fdf67507f9e49ae3efed667b6b81e9c4c668283d6f6f8e38a07051a`.

The native comparison constructs 35 baseline plus 210 omission frames and checks
343 Base evaluations before/after the new binary codec. The original five JSON
roundtrips are evidence for the semantic projection; the native binary
roundtrips test this different port encoding. Original omission controls clear
exploration, claims, visibility, convoy state, selections or directed predicates
independently. Idle convoy and false relationship examples remain constant-input
controls; they do not prove those fields generally irrelevant. Additional native
tests distinguish the broader checked recognition/exemption inputs, copied
ownership, native bits, record order, unqueried tiles, narrow aliases, clipped
coordinates, selected32 rejection, malformed complete framing and the exact
2-MiB boundary. These synthetic extensions are not new original executions.

Validation on this isolated source:

- Normal full CTest: 8/8, 104.423 seconds. Leak-enabled ASan/UBSan/LSan full
  CTest: 8/8, 223.246 seconds. Both preceded one test-only extension checking
  nonzero header predicates and Base metadata; production was unchanged.
- Final rebuilt affected native groups: 16/16 with allocation tracking, normal
  1.016 seconds and sanitizer 1.078 seconds, including all six new frame groups.
- Explicit complete production-TU LLVM15 correctness analysis: zero findings,
  1.87 seconds (3.975-second driver). The new header is used only by this TU and
  its native test; no wider header-impact claim is made.
- Five copied-TU controls each fail their intended native assertion: raw foreign
  claims treated as recognized; dropped native Unit presence; lost visibility
  bit0; accepted full selected32 truncation; and accepted trailing bytes. The
  checkout was never mutated for these controls.
- Re-running the projector reproduced the committed fixture byte-for-byte.
  The original executable was not rerun by this implementation validation.

GC independently reviewed production/schema; port independently reviewed the
projector and claims. Neither review is an independent test execution. Private
reports, logs, build products and controls remain outside Git. The retained
manifest is `/home/jhurliman/src/glsmac-unit-weapon-parser/worker-frame-manifest.json`.

## Remaining live integration

The earlier provisional design used resolved per-observer ownership planes and
live Unit-ID observations. This receipt instead supports the raw-claim and
explicit record projection above. A future Map-owned optional value can reuse
its knowledge fields, but must not introduce a parallel movable Unit/Base or
allocation authority. Existing positive Unit IDs/playable Slots do not provide a
native0 identity. A live adapter must reject unsupported native binding rather
than substituting a Mind Worm owned by a playable Slot.

The current Map and world decoder still have no such worker frame. Before live
queries, integration must validate saved geometry, explicit simulation owners,
and actual registered/pending entity bindings, and gate query publication until
those bindings exist. Restored Unit placement must not replay exploration or
visibility. The current `start` callback precedes pending manager registration;
checking only the registered Unit map would miss restored records at that point.

Map placement-history bits are not exploration. Known first reveal, movement,
Colony factory and second-founding transitions have separate measured producer
requirements; they cannot be replaced by an all-owned/all-explored overlay.
Their updates must stage with the actual command's authority mutation or reject
unsupported commands while the capability is active.

Full selected32, all16 specialist slots including inactive padding, count and
stable Pop bindings belong to a separate Base allocation authority. The frame's
selected21 is a supplied consumer projection only. A live allocation preview
must combine a checked frame query, that complete Base authority and actual
resource yields; mutable allocation and its full calculation pipeline remain
separate. This slice does not activate first-base play, movement sight, second
founding, networking, default campaigns or original save loading.
