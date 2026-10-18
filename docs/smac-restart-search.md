# Original nonzero human restart search

`map::smac_restart::Search` implements the original candidate/RNG/budget loop
over a prepared future-world query. It uses CRT horizontal then vertical draws,
the shrinking margins and parity correction, original gate order and integer
thresholds. Preprocessing runs before incrementing the attempt counter. Failure
therefore consumes the terminal1,025th pair and retains its distance scratch.
The shared RNG is untouched. Do-or-Die, restart-history and turn100 refusal
belong after this search in the disposition caller, not before its draws.

The rating callback runs at its original stage even if a later landmark,
territory or captor gate rejects the candidate. A zero calculated score is not
an admission veto on this nonzero-turn path. The function returns prepared RNG
and scratch; it does not mutate input RNG or publish world state. Callback
cache writes must likewise remain in the caller's prepared world until commit.

[The frozen native traces](../src/gse/tests/fixtures/SmacRestartSearch.h)
compare every coordinate, CRT state and rejection across1,793 candidates and
3,586 draws: turn3 success at pair768 and turn6 capture failure at pair1,025.
Tests use supplied literal terrain/region/presence/Base inputs, the already
verified worker footprint and integer nearest-Base metric. They do not feed
candidate outcomes or final coordinates into the search. The second fixture
preserves stale regional counts3/2 despite two live captor Bases and no defender
Base; neither counts nor fertility are freshly derived by the search.

[Two further original cache-miss selections](../src/gse/tests/fixtures/SmacRestartCacheMiss.h)
compose the real land/water scorer with this loop. The reached native terrain
postpass supplies F1=960. Both select the first generated pair with two CRT
draws; one calculates/stores1 and the monolith discriminator calculates/stores0.
The tests cover these original results without hardcoding a selected location
in production or requiring a positive rating.

This commit owns the candidate loop, not its live future-world producer. Scale,
landmark proximity, wet neighbors, region-filtered nearest Base and retained
metadata must be supplied by their owning producers. The two long traces have
no positive shared-landmark detail; the separate S/Q receipt now closes their
broader helpers. Current live disposition still uses its explicitly approximate
farthest-site policy until the joined caller replaces it. Last-Base capture
remains refused until that transaction is complete. No original generation,
AI/startup placement or golden1.0.0 acceptance is implied here.

Wrapped64×32 search is dynamically checked. Nonwrapping margins, range≤1 draw
suppression and margin clamps follow the original static continuation; those
full candidate paths still need distinct native dynamic coverage.

Validation: cached Linux Debug build; two focused native groups with allocation
tracking; LLVM15 correctness analysis of the new production unit passes in1.16s.
Unchanged scorer/cache checks are reused. This bounded arithmetic loop introduces
no new ownership, save codec or platform dependency requiring another matrix.
