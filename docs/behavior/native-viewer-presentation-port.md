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
