# Explicit CVR composition policy

The existing Infantry Scout and Spartan Rover loaders select an immutable,
private composition profile independently of optional software palette data.
Geometry, view/light construction, the Infantry attachment offset, fixed Rover
shade replacement indices and linked VLIGHTS decoding follow that profile.
Palette colors remain owned values used for final RGB conversion.

Both existing public loaders select the same behavior they used before this
split. Scout/Rover descriptors, wire values, cache keys, placement, keyed alpha
and texture lifetimes are unchanged. This adds no public arbitrary-composition
loader and no initialized Infantry faction profile. The latter still requires
accepted source material and palette evidence; diagnostic test colors do not
establish those inputs.

## Validation

Source parent: `c2d14520832666bcd3971f4e22f42ca0824e5c6b`.

A synthetic native regression loads the same Infantry body/reactor with and
without owned diagnostic RGB data, destroys the caller's material values, then
checks three facing/zoom/ratio/anchor combinations. Indexed pixels, depth,
draw/record/write counts and anchors remain equal; RGB reflects the copied
colors, and transparency still keys index 9. The scene shade tables stay intact.
A copied production translation unit restoring the old palette-presence test
for view/attachment geometry fails this regression at its intended assertion.
The intended checkout and application binary were not mutated for that control.

- Full normal CTest: 8/8, 86.32 seconds.
- The same 23 focused CVR native cases pass with allocation tracking in normal
  and leak-enabled ASan/UBSan/LSan builds. These include both actual registered
  Scout/Rover preview ownership fixtures. No new interactive display claim is
  made.
- Full header-triggered LLVM15 analysis: 337 production translation units,
  zero failures, 254.50 seconds; the Windows-only network implementation is
  outside that Linux scan.
- Fresh optimized, UBSan-instrumented production probes pass the existing
  unchanged, hash-pinned installed-asset comparisons: Scout 19 drawing views
  and all 20 metadata views; Rover 15 material projections, 19 drawing views,
  all 20 metadata views and 68 detail-selection queries. The existing lighting,
  complete indexed/depth/RGBA and independent keyed-alpha comparisons pass.
  This reruns the port against retained original receipts, not the original
  executable. See [Scout evidence](scout-idle-renderer.md) and
  [Rover evidence](cvr-spartan-rover-renderer.md) for their admission limits.

Private logs, comparison results, the copied-TU control and a source/log hash
manifest are retained under `/home/jhurliman/src/glsmac-cvr-policy/`. Proprietary
asset and pixel data are not committed.
