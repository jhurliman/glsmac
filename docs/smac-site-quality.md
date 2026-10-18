# Original land-site quality

`map::smac_site::LandQuality` implements the decoded-input land query needed
by the restart-rating cache. The query visits the measured inner21 squares in
order, then counts Base markers in the outer28. It keeps original integer
division stages, center penalties, stored-area threshold50, landmark-detail
suppression, Base-radius exclusions and the coastal flag of the **last valid
inner square**, rather than the center. The normal result clamps to1..15;
fungus, monolith or rocky land centers return0.

The [31 frozen original queries](../src/gse/tests/fixtures/SmacSiteQuality.h)
retain resolved semantic inputs, original results and arithmetic terms. Tests
compare every recorded term used by the score, not just the final clamp. The
fixture records the source receipt's SHA256. Resource/site automatic-pattern
examples supply actual resolver outputs, rather than reimplementing those
resolvers in the scoring test. Water neighbors, clipped/wrapped visits, stored
area49/50, the last-square metadata discriminator and outer cardinal/noncardinal
Base penalties are included.

This is a pure query: no cache, world mutation or RNG draw. The decoded inputs
use original terrain ordinals, not GLSMAC Tile enum values. Live resource/site
resolution and saved cache ownership must remain explicit at the caller. This
query does not replace last-Base restart search or add a rating gate to nonzero
restart. The [water branch](smac-sea-site-quality.md) and
[startup/late-turn extension](smac-site-quality-turns.md) now share this query;
ordinary restart admission remains a separate caller.

Next integration: own saved ratings and Base-radius exclusions, wire world
postpass/founding/removal invalidation and territory query-on-miss, then consume
the cache in the original CRT search. Road/Farm/Solar/Mine/Forest completion
must preserve existing nonzero ratings, as recorded by the original completion
traces; current terrain alone cannot reconstruct a stale rating on load.

Validation: cached Linux Debug build; both focused native groups pass with
allocation tracking, covering31 complete original queries and unsupported-input
rejection. LLVM15 correctness analysis passes the one new production unit in
1.14s. This arithmetic query introduces no pointer ownership, save codec or
platform changes requiring a new sanitizer/platform matrix.
