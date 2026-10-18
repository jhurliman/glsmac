# Native viewer presentation context

The original map badge queries wild native level with the viewing simulation
faction as the opposing context; ordinary description text queries neutral
context0. The frozen source and installed-asset mapping is recorded in
[native-display-identity-cases.json](native-display-identity-cases.json).

`Unit::GetPresentation(native_viewer)` now exposes that distinction without
changing the authoritative native record. Its default remains neutral0.
`GetMoraleString()` deliberately keeps that neutral default, so a viewer with
retained major atrocities can see a higher map badge level while the neutral
description keeps its original label. The native level query validates the
viewer and uses retained major-atrocity state. Ordinary conventional and legacy
presentation paths retain their existing semantics.

The live sea/air Unit decode tests cover all six returned original records,
comparing viewer and neutral levels, unchanged movement/health, neutral labels,
invalid viewer rejection, exact record roundtrip and unchanged gameplay RNG.
The cached incremental build,18 affected native groups and targeted Unit.cpp
correctness analysis pass. This arithmetic/query change does not add ownership,
pointer or serialization behavior; the existing native birth sanitizer result
is not represented as a fresh run of these changed sources.

The renderer still needs explicit owner0 publication, native badge templates
from UNITS.PCX and the correct viewer argument at its publication boundary.
This change does not certify native rendering or screenshot parity.

## Live bottom-bar consumers

The native wrapper now exposes `query_native_stats(viewer)` as an owned query
result with integer movement allowance, quantum, health and the queried native
level/name. The viewer is range-checked before narrowing. Both production
bottom-bar description and same-tile list query native context0 instead of
reading the deliberately absent legacy float fields or Def movement999.

The actual birth-to-UI test publishes one native Unit of each supported
design8/9/10 through the transaction, then invokes the real production
`unit_preview` and `unit_select` callbacks twice. It checks movement1/4/8,
neutral level labels, same-tile row refresh and stable row identity. A copied
query mutation cannot change the next query; an oversized viewer is rejected.
World snapshot, both RNG streams, next Unit ID and frontend queue remain
unchanged. Existing Infantry/Rover and legacy native preview tests also pass.
Combat strength formatting remains the pre-existing placeholder; this pass
connects checked movement/level description fields, not complete combat UI.

Validation of the bottom-bar addition:35 focused ecology/native/preview groups
pass, together with targeted Unit.cpp correctness analysis.

## Native frontend publication

Spawn requests now carry an explicit native identity. Slot0 in that request is
ignored for natives; the renderer creates no human Slot association and uses
a separate owner0 badge bank. Backend spawn/refresh queries the actual viewing
simulation participant for the map level while keeping neutral description
text. Actual born8/9/10 publication and copied-request tests verify this split.

The owner0 badge bank uses the original variant0 independently of remaining
movement. All seven renderer levels retain their own decoded texture through
the instanced actor lifetime and release it when the bank is retired. Existing
Scout/initialized Scout/Rover preview teardown tests exercise that lifetime
with doubled badge assets; the separate purchased-asset probe verifies exact
original badge pixels. Native CVR body art, stack marker composition and the
normal per-Base ecology caller remain separate work. In particular the current
default scripts still lack canonical Isle/Locust presentations; synthetic test
definitions are not advertised as live-game native body fidelity.

Targeted analysis covers8 production translation units. Focused ASan/UBSan
instruments10 ownership/publication/loader consumers and passes19 native/UI/
render groups; other objects reuse the current normal build and leak detection
is disabled. The cached normal build and5 renderer/palette groups pass.
