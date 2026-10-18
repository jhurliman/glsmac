# Startup and late-turn site scores

The shared site query accepts nonnegative signed32 turns. Turn0 uses the same
formula as turn1; the earlier land/sea receipts'1..100 fences described their
measured inputs, rather than original executable admission limits.

For turn>150, count land fungus in the ordered inner21 footprint, including
the center and Base-radius-excluded, arid and rocky land. Water and outer28
fungus do not count. Subtract floor(count/3) from the ordinary score before
outer-Base/rock penalties, existing center rejection and the final1..15 clamp.
The adjustment is independent of how far the turn exceeds150. A rejected
center remains rejected; negative adjusted scores at accepted centers clamp
to1. Subtracting from an already clamped rating would be wrong.

[Forty complete original queries](../src/gse/tests/fixtures/SmacLateSiteQuality.h)
freeze startup,100/150/151 boundaries,300/500 controls, fungus-count thresholds,
clipped/wrapped visits, excluded land, deep water fungus, both clamps and
penalty ordering. Tests compare final raw scores, ratings and the other
recorded consumed terms. No original generation or global scheduling claim is
made by these supplied semantic contexts.

This rule belongs to the uncached query. Crossing turn150 does not invalidate
existing nonzero ratings. The normal cache caller still skips exclusions and
queries only zero ratings. There is no new gameplay refusal at turn100.

Validation: cached Linux Debug build; nine affected native query/cache groups
pass with allocation tracking, including111 complete original scoring cases.
LLVM15 correctness analysis passes the scorer, prepared cache and topology/map
consumers (four units,2.75s). Shared geometry is separate from scorer declarations
so future query API changes do not invalidate saved-world consumers.
