# Native hill, polar and altitude stages

The detached native generator now provides the remaining shaping callees used
before and within contour morphing:

- `SeedHill` (`0x5daa40`) samples an all-land nine-cell seed footprint, then
  invokes the existing grow/paint/shape algorithm. Later walk positions may
  reach sea tiles; the seed admission rule is not reapplied to each position.
  An unsuccessful search consumes the 51st coordinate pair without inspecting
  it. Ocean category and plateau rules determine growth size.
- `SeedPolarContours` (`0x5dd890`) writes only contour bytes, preserving altitude
  classes until reconstruction. It preserves the original interleaved north,
  inner-north, south and inner-south draw order. This entry admits even-height
  native planes, as used by the measured generated worlds.
- `ReconstructAltitudes` (`0x5dbe60`) first classifies every contour, preserving
  the low five tile flags. It then shapes matching cells in class order 6 down
  to 2 and row/parity order, using the shared native shaping kernel. Checkpoints
  occur after each row in all six passes.

All use the caller's owned 44-byte records and CRT continuation. They retain
turn-zero generation flags 1/1 and do not publish a live world or reseed RNG.
The outer continent loop, hill-count calculation, morph, regions and retry
controller are still required before replacing live procedural generation.

## Original evidence and checks

Seventeen captured original entry/return boundaries from nine independently
generated worlds verify complete input/output planes, outgoing state,
**27,719 ordered byte stores**, **1,652 ordered CRT draws**, and checkpoint
counts. They cover ocean categories 0/1/2, erosion categories 0/1/2, polar
seeding, post-morph reconstruction, and the 51-pair rejected hill search.
Fixture inputs are lossless field-plane encodings of numeric original records,
used only by tests; production does not load captured stage outputs.

The source executable SHA256 is
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
The frozen `native-pretemperature-generation-evidence-20261020` manifest SHA256
is `3ccbd8480e7ae54006bc663cb1336385046cf60e33d9897da2c097352d48c3d3`;
all 32 files were verified before fixture extraction. Controls come from its
`generation-preclimate-original-x86-20261020.json.z` receipt (compressed SHA256
`f5bc66d8762162a8c6ab4db6f917b5aed0b41a726480e5cb56c23ecda3f09913`).

Validation uses the cached incremental build and five affected native groups,
production-TU correctness analysis, and a focused three-TU ASan/UBSan run.
No rendering changes or full platform matrix are part of this shaping step.
