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

Construction is detached and accepts only a completed `SmacBloomTerrainPlan`.
Its constructor and deltas are private to the original terrain producer;
consumers receive read-only changes. They cannot forge class/item combinations,
combine multiple direct fungus writes, or substitute a different turn context.
This keeps the publication boundary tied to the measured algorithm instead of
maintaining a second, incomplete imitation of its semantic constraints.

The saved-map adapter checks coordinates/parity and duplicates against its own
container, and matches source class, rockiness and landmarks to each delta's
preimage. A failure on a late peer returns no replacement and leaves the
original authority untouched. Before publishing, the future live caller must
also validate the complete live item/presence preimage, stage the render batch,
and atomically publish tiles, resource policy, history and CRT state. This API
does not replace that live transaction or add an event caller.
Generated maps still lack original class authority and reject this operation.

Validation: eleven focused native groups pass, including all 26 original
terrain fixtures replayed through the actual producer and projected into saved Planet records, byte-exact preservation
of all unrelated data, resource altitude after policy save/reload, stale late
peer rejection, and the installed Planet classification/resource regression.
Ten producer/map/publication groups pass ASan/UBSan with SmacEcology.cpp,
SmacPlanetMap.cpp and GlsmacResourceMapPolicy.cpp instrumented, remaining objects from the normal
cached build and leak checking disabled. Targeted correctness analysis passes
those three production TUs. Earlier analysis of the unchanged resource
consumer BaseEcology.cpp is retained. This is
saved authority preparation; live map/render publication and native life
creation remain unfinished.
