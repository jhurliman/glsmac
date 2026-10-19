# Named landmark height and shade profiles

The normal nonnegative detail profile path is now measured for all five native
height memberships in `0xb03`. This supplements the earlier
[field-admission contract](terrain-landmark-field-admission.md), which left these
profiles outside ordinary geometry support. It does not close landmark artwork.

Use the numeric tables in [cases](landmark-height-shade-profiles-cases.json) and
the executable [reference](landmark_geometry_reference.py). Verification covers
686 complete height/shade/projection contexts across every detail0..48, 112
sea/coast/relief/priority contexts, 16 shared-profile preparation controls and
10 corner-cache order controls. All match the original routines.

| Native bit | Landmark | Height profile bytes | Extra shade |
| --- | --- | --- | --- |
| `1` | Garland Crater | Signed | Signed profile |
| `2` | Mount Planet | **Unsigned** | Signed profile |
| `0x100` | Sunny Mesa | Signed | None |
| `0x200` | Nessus Canyon | Signed | Profile; measured stock entries are zero |
| `0x800` | Pholus Ridge | Signed | None |

These are original retained bits, not a port enum. In particular raw `0x10`
is New Sargasso, not the height profile `0x100`.

## Exact selection and arithmetic

The shared word at tile+12 contains membership flags and the signed upper detail
byte. For normal detail0..48, map `(detail,vertex)` through the supplied49x5
mapping table, then index the chosen113-byte profile. Vertex order is center,
west, north, east, south. Height priority is
`2 → 1 → 0x200 → 0x100 → 0x800`, taking the first present membership.

Let `step` be the view's effective elevation step,25 in the default zoom0 view
and8 in the separately established low-relief view. At a land center:

`height = step*(altitudeBand-3) + truncTowardZero(step*profile/100)`.

At a land corner, first compute the four contributing **deformed center heights**
and arithmetic-shift their sum right2. If any contributing tile is sea or outside
the map, that ordinary corner base is0. Then add the requesting tile's corner
profile adjustment, with the same signed truncation. A coast therefore does not
erase a nonzero landmark corner adjustment.

For a positive profile adjustment clamp the resulting height above at127; for
a negative profile adjustment clamp below at-127. The published cache byte also
has the ordinary upper127 clamp. The measured domain avoids any discrepancy
between an out-of-range uncached return and its cached signed-byte result.
Sea height returns0 before reading the word or its profile. The normal topology
and shared-corner stencil remain those in
[ordinary projection](ordinary-terrain-projection.md).

Extra shade uses a different priority: `1 → 2 → 0x200`. If the selected word's
signed detail is negative, add no extra profile shade. Otherwise add the selected
signed profile value **after** calculating ordinary vertex shade. Do not derive
ordinary shade from the deformed mesh: it still uses retained altitude-band slope
codes. Relief25/8 changes geometry but not those slope codes. These routines
produce raw signed scalars; final terrain rasterization owns its separate shade
limits. The stock Nessus extra-shade table is zero, but its branch still exists.

Projection uses the deformed outer heights: for this zoom0 viewport,
`P=(flatCenterX-48, flatCenterY-24-(sum(outerHeights)>>2))`.
The center height does not directly replace that outer-height average.

## Shared cache and invalidation

The first corner query publishes its computed height into the corresponding
corner cache slots of all four valid neighboring tiles. Querying an adjacent
tile returns that shared value; it does not apply the second tile's profile
again. This is observable with deliberately inconsistent neighboring fields.
For example, an isolated Crater detail6 at `(40,40)` produces west15 when queried
first; querying the unmarked west neighbor's east corner first produces0, which
is then returned by both tiles. The cases include both request orders.

The reference keeps this shared cache. If the port instead computes vertices
statelessly, it must establish consistent ownership for an authored shared
vertex. It must not add separate corner offsets per adjacent tile and expect
the native result for conflicting fields. Invalidate affected shared corners
when terrain bands, memberships/detail, prepared profiles, zoom or relief change.

## Profile preparation and negative detail

Map postprocessing restores the five mutable height profiles from their stock
templates. It then scans nonzero tile words. For a negative signed detail `d`,
use suppression row `-1-d`. If bit2 is set, clear the five mapped entries in
the **shared Mount Planet profile**; if bit1 is set, also clear them in the
**shared Crater profile**. Other memberships do not trigger this clearing.
Both bits can act together. This changes shared tables, can affect another tile
with normal detail, and preserves every tile word. Preparation begins by
restoring the templates, so removed suppression marks do not permanently retain
the old cleared entries.

Do not conflate this preparation with height-consumer admission. The native
height consumer performs a signed detail shift and has **no negative guard**:
a negative detail indexes memory before the normal49x5 mapping table. The
extra-shade consumer does have a negative guard. The portable reference rejects
negative height detail and nonnegative detail>=49 rather than inventing a
normal-profile interpretation of adjacent binary storage. Suppression rows0..48
are measured for preparation; this is not a claim that the subsequent negative
height query safely selects the corresponding positive row.

For integration, admit the measured nonnegative detail0..48 profiles and keep
negative/out-of-range height-bearing tiles behind the existing unsupported
geometry boundary. Dirty bookkeeping and standalone detail bytes still do not
create a profile without one of the relevant membership bits. Special landmark
materials/sprites, their raster ordering and arbitrary malformed save fields
remain separate work.

The later [ordinary named-art handoff](named-landmark-art.md) now closes the
normal quad, marker, Jungle and Dunes continuations with full indexed controls.
Its documented detail/style/zoom bounds still apply independently of geometry.
