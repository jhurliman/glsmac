# Saved Planet terrain authority after an ecology bloom

The original bloom changes altitude classes, rockiness, landmarks and selected
terrain item bits without changing the contour or shared corner heights. The
installed Planet resource policy previously retained its import-time values.
`WithBloomTerrain` now prepares a replacement of that saved authority from the
[original terrain producer](ecology-terrain-mutation-port.md)'s complete deltas.

The replacement retains every unrelated byte, including climate low bits,
rockiness-byte low bits, contour, region, visibility, affiliation, remembered
items, header and trailer. Only fungus and the five removable improvements
are copied from the live item projection. Base/Unit presence remains owned by
the live registry and is never copied into the source map. The existing policy
codec saves the updated records; no new wire version or missing-value default
is introduced. Automatic-resource policy remains independent and unchanged.

Construction is detached. Invalid coordinates/parity, duplicate coordinates,
out-of-range fields, affiliation changes, unrelated item changes and cross
land/sea category conversions reject. Source class, rockiness and landmarks
must match each delta's preimage. A failure on a late peer returns no replacement
and leaves the original authority untouched. Before publishing, the future live
caller must also validate the complete live item/presence preimage, stage the
render batch, and atomically publish tiles, resource policy, history and CRT
state. This API does not replace that live transaction or add an event caller.
Generated maps still lack original class authority and reject this operation.

Validation: eleven focused native groups pass, including all 26 original
terrain fixtures projected into saved Planet records, byte-exact preservation
of all unrelated data, resource altitude after policy save/reload, stale late
peer rejection, and the installed Planet classification/resource regression.
Four map/publication groups pass ASan/UBSan with SmacPlanetMap.cpp and
GlsmacResourceMapPolicy.cpp instrumented, remaining objects from the normal
cached build and leak checking disabled. Targeted correctness analysis passes
those two production TUs plus the resource consumer BaseEcology.cpp. This is
saved authority preparation; live map/render publication and native life
creation remain unfinished.
