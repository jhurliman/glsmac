# Retained generated terrain policy

`GlsmacResourceMapPolicy::GeneratedTerrainV3` owns generated terrain through the
same immutable `SmacTerrainAuthority` used by resource queries and snapshots.
It has no installed Planet pointer or manufactured TERRANMAP header. Automatic
resources come from the retained generation primary-rule bit 0x2000 rather than
an independent default. Generated equality includes metadata and all raw bytes;
comparing its raw vectors does not allocate whole-plane temporary strings.

The V3/kind3 policy payload retains explicit width, height, wrapping, resource
pattern, signed sea/climate offsets, all seven resolved generation categories,
primary rule flags and the complete 44-byte records. Metadata is mandatory;
missing or unresolved categories are not silently filled. Generated dimensions
are even, width 16..512 and height 8..256. Records preserve contours, opaque
fields, Base/Unit presence, exploration and remembered improvements. Known
climate/rockiness/region fields retain their supported native domains. The later
[shared publication layer](shared-terrain-publication.md) distinguishes raw
temperature 0..7/moisture 0..3 from normalized selected climate settings 0..2.

The decoder bounds dimensions and integer fields before reading/allocating the
raw plane, requires its exact byte count, rejects contradictory automatic-resource
flags and origin/version tags, and rejects trailing data. The complete policy
frame budget is now 4MiB, sufficient for the maximum 2,883,584-byte raw plane plus
metadata/framing. Nested reads apply that budget before copying their payload.
Existing displayed V1 and installed V1/retained-climate V2 formats retain their
exact serialized bytes and existing interpretation. Old readers reject V3.

`GeneratedRetained` reconstructs an immutable raw owner from validated metadata
and records. `GeneratedPrestartup` retains its producer-stage check and now also
checks explicit topology before using the same validation. Coordinates still
require source bounds and row parity. Installed provenance remains a separate
factory; generated policies cannot be created from an installed source object.

## Verification

The complete original generated startup fixture now serializes/restores its
post-setup terrain policy, comparing every raw byte and generated provenance.
The full original buffer/RNG comparison still passes with purchased Base/design
labels, using `GLSMAC_STARTUP_ASSETS` as documented in
[complete generated startup](native-generated-startup.md).

Synthetic codec controls additionally cover opaque gameplay fields, resource
queries despite contradictory displayed Tile fields, metadata/equality, queued
ownership after retirement, every truncated prefix of the small fixture,
malformed dimensions/categories/options/tags, nested-frame continuation and the
maximum supported geometry. Installed climate/flora/bloom and existing policy
draw-order, snapshot and cancellation controls continue to pass.

Cached incremental build and 15 affected native groups pass. Four production
TUs/consumers pass targeted correctness analysis. Focused seven-TU ASan/UBSan
instrumentation includes the raw authority, policy, Buffer and direct/retained
tests; the same groups and the owned-text startup comparison pass. Other objects
are normal and leak detection is disabled. The final allocation-free generated
equality is rechecked under analysis and the focused sanitizer scope.

## Remaining world connection

This adds terrain-policy persistence, not a new playable save format by itself.
Regions, gameplay RNG, faction/Base/Unit state and settings retain their own
authorities. The complete-world boundary must bind their geometry, selected
options and provenance before publication. Current campaign/world guards and
climate/flora/bloom callers that still fetch the installed source need that migration.
The policy and common-owner mutation kernels now support both source types as
documented in the shared publication layer.
Ordinary generated games are not switched by this change, and no new rendered
gameplay result is claimed.
