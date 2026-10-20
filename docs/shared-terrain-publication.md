# Shared retained terrain publication

Climate, forest/kelp spread, bloom and startup marker publication now share raw
record kernels in `SmacRetainedTerrain`. Installed Planet wrappers preserve their
original header/trailer and delegate record changes to those kernels. Generated
authority uses the same kernels with its own dimensions/topology and retains
generated metadata and V3 provenance. It never acquires an installed container.

The original algorithms, event-owned masks, ordered before/after journals and
unowned-byte preservation remain shared rather than being reimplemented for a
second source type. Results own detached records. Invalid late changes throw
before a replacement authority is returned; queued readers retain their old
immutable source. Wrappers copy metadata/header/trailer separately and move the
kernel result, avoiding a redundant whole-plane copy before publication.

The common authority checks spread/climate topology against its actual source.
Installed-policy admission remains wrapped; generated flat terrain uses its own
explicit topology. Climate publication retains signed sea/climate offsets in
the generated metadata or original installed header as appropriate. Regions and
entity registries remain the owning world transaction's responsibility.

Generated retained records now accept the existing raw climate domains:
temperature 0..7 and moisture 0..3, independently of the selected 0..2 climate
settings. Rockiness remains 0..2. Restricting raw codes to normalized settings
would reject legal retained climate state already supported by installed V2.

## Verification

Existing installed publication receipts and header/trailer comparisons still
pass through the shared kernels. The original bloom fixtures also run through
generated authority at their actual smaller dimensions, comparing complete raw
records against the independently declared native changes. No installed header
is created for the generated producer or its persistence.

Additional generated controls run actual forest spread with live observer and
Former/material projection, compare the complete producer result, retain source
ownership, reject late altered journals/topology, and check ordered/stale startup
markers. A flat 16x8 climate case retains raw temperature 7/moisture 3, changes all
owned climate fields and metadata, preserves unrelated fields, round-trips V3
and rejects another topology. The full original generated startup comparison
continues to pass with owned Base/design text.

Cached incremental build and 19 affected native groups pass. Five affected
production TUs/consumers pass targeted correctness analysis. Focused seven-TU
ASan/UBSan instrumentation covers the shared kernels, both terrain owners,
policy and direct/composed tests; the same groups and owned-text startup check
pass. Other objects are normal and leak detection is disabled. The final common
topology guard is rechecked under targeted analysis and sanitizers.

This closes the policy/authority mutation layer for these operations. Live
climate/flora/bloom callers that still fetch `OriginalPlanet()` must migrate to
the common owner, and complete-world provenance/options/registry binding remains
required before switching generated games. No new rendered-game result or
complete playable-save claim is made.
