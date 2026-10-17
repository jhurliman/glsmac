# Owned worker knowledge

`SmacWorkerKnowledge` is an immutable owned value for the knowledge inputs of
`SmacWorkerMapFrame`. It stores dimensions and horizontal wrapping, explicit
ordinary-local mode, revelation bits, dense explored/raw-claimant tiles, optional
directed observer rows, and ordered Unit observations keyed by supplied port IDs.
Native bit/claimant zero differs from missing knowledge or an absent claimant.
At least one playable observer row must exist. Missing observer rows remain
missing; one captured row does not initialize the other factions.

This value contains no terrain sea copy, entity ownership/coordinates, Base
records, population assignments, or generated-world producer. `Create` copies
validated input; `CopyData` returns a detached copy. There are no mutation hooks.
Map/Game persistence and entity authentication are supplied by the live adapter.
See [live ordinary sight](smac-live-ordinary-sight.md) for the remembered-copy
extension and [live ZOC](smac-live-ordinary-zoc.md) for V3 directed Pact state.
The original value-only receipt below
remains historical validation.

## Projection and identity

`Project` accepts explicit current sea values in the same dense parity order,
ordered consumer Base records, and actual Unit records with IDs, owners and
coordinates. Every observation must match exactly one supplied Unit by ID.
Missing, extra, duplicate, zero, or greater-than-INT64_MAX IDs reject. Live input
order does not affect the result: the projected frame uses observation order and
the supplied Base order. Base selected masks remain the low21 consumer projection;
the existing frame rejects higher bits rather than truncating full authority.

The method bounds the resulting frame before vector construction and delegates
entity owner/coordinate and Base-center checks to `SmacWorkerMapFrame::Create`.
It does not authenticate a live Game, native Unit representation, active convoy
execution, diplomacy, territory, sight, placement or event producers. Callers must
admit their supported scenario before supplying entities. No pointers escape the
synchronous ID join. Existing single-horizontal-adjustment geometry, clipping and
aliases remain the frame/collection contract.

## Wire contract

The standalone V3 codec has a 4 MiB port budget. All unsigned multibyte values use
explicit little-endian bytes. There is no native-struct serialization.

| Offset | Field |
| --- | --- |
| 0 | Four bytes `SWKN` |
| 4 | uint32 version 3 |
| 8, 12 | uint32 width, height |
| 16, 17, 18, 19 | wrap, ordinary-local, revelation, observer-presence bytes |
| 20, 24 | uint32 tile and Unit counts |
| 28 | Eight three-byte observer rows: disclosure, exemption bits, recognition bits |
| 52 | Two bytes per dense tile: explored bits, claimant (`255` means absent) |
| after tiles | Ten bytes per observation: uint64 ID, visibility byte, convoy boolean |
| after observations | Remembered-row presence byte, then one square-presence byte per tile in each present row; known squares append uint16 features, uint16 terraforming and Base/Unit marker bits |
| after remembered rows | Directed Pact presence mask and eight bytes of directed Pact masks; absent rows encode zero and cannot belong to an absent observer |

Dimensions are the existing frame's width 2..512 even and height 1..256. Tiles
follow increasing Y and parity X, so the tile count is exactly width*height/2.
Absent observer rows must encode three zero bytes. Boolean bytes are strict;
claimants are absent/native0/playable1..7; arbitrary eight-bit knowledge masks
remain intact. Exact counts, aggregate budget and header fields are checked
before record reserves; the complete remembered and Pact suffixes are scanned first.
Truncation, trailing bytes, duplicate IDs and invalid
domains reject. The projected frame has its own larger per-record budget and
may reject a knowledge value that fits this codec.

## Original V1 evidence and validation

Tests reuse the accepted fixture in `fixtures/SmacWorkerMapFrame.h`, whose source
and original output provenance is documented in `smac-worker-map-frame.md`.
They split each of the 245 baseline/omission inputs into knowledge plus supplied
entities and terrain, then compare the complete reconstructed frame and 343 Base
results. These are the retained 35 captured states and 210 omission guests; this
change does not rerun original code or add new original producers. The test's
`1001 + 17*position` IDs are an explicit synthetic port identity policy, not a
claim about original list positions or real Unit IDs. Reversing the supplied
live-Unit list demonstrates that projection uses identity rather than position.

Additional native groups cover distinct optional observer rows, native/absent
claims and bits, copied lifetimes, narrow aliases, ID and entity mismatches,
malformed complete frames, and exact-cap versus cap+1 admission. A 2 MiB accepted
knowledge record also exercises rejection when the larger projected frame would
exceed its budget.

Final focused validation passed all 22 registered groups (six new knowledge,
six frame, five availability, five collection), with memory tracking in both
normal (1.013 s) and ASan/UBSan/LSan (1.053 s, leak detection enabled) builds.
The new production TU passed LLVM 15 analysis with zero failures (0.50 s analyzer
time; 4.282 s wrapper). Copied production controls that joined Units by list
position or collapsed native claimant zero into absence both failed the intended
assertions. Checkouts were not mutated for those controls. No full platform suite
or live binding is claimed.
