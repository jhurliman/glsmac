# Ordinary Planet startup scale

Fresh retained native-projection maps now start at camera scale0.125 in the
ordinary game, matching the existing native96x48 projection. Previously that
scale was selected only by the terrain-inspector flag; ordinary games used0.1
and displayed the same world20% smaller. The fixed1024x768 presentation canvas
and257-pixel console do not change.

The original main-view initializer's zoom0 behavior,96x48 tile geometry,
constructor and save-codec boundaries are recorded in
[the original visibility/zoom contract](behavior/main-map-exploration-and-startup-zoom.md).
All148 files in the frozen evidence bundle were verified against manifest SHA256
`7f8b5373a8ee751583073b1f95d9aa55a5e0cb779f36aa3da3e61ecde52a3f99`.
The original natural resume ordering remains unproven; this is not a claim that
saved original zoom must always win or always be overwritten.

Validation uses the cached incremental build, changed-Game.cpp correctness
analysis, and the existing camera/projection-provenance groups. The actual
camera projects all retained terrain-height fixtures onto96x48 diamonds with
their expected vertical offsets. A normal-menu SMAC Map of Planet run reached
first-base naming, research priorities and gameplay, accepted tile selection,
and closed normally. Inspector mode was not enabled in that run. No new
ownership/bounds changes warrant another sanitizer/platform matrix for this
constant-selection correction.

Main-map exploration masking, unknown blue outlines and foreign object
disclosure still require live renderer integration. The imported receipt
establishes their original behavior; it does not itself implement them.
