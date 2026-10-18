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

The live last-Base starvation/Colony-abandonment caller now stages territory,
site history and the owner's regional-count removal delta before this search.
It excludes the lost Base and every retiring owner Unit from its occupancy
view. Wet neighbors use the existing21-square resolver; region-filtered nearest
Bases retain record order and the original integer metric. Regional presence
reads saved per-owner counts, including stale nonzero counts. Shared-byte Q,
actual geometry scale, retained ocean choice and Player difficulty supply the
other inputs. Missing required history fails preparation instead of selecting
an approximate site.

Search RNG and direct cache-miss writes commit with faction/Unit/territory
authority before callbacks, including failed searches and post-search
Do-or-Die/history/turn100 vetoes. Direct site queries preserve exclusions and
nonzero scores without refilling neighboring tiles. The package selector runs
only after search and later guards permit restart. Preparation changes neither
current authority nor either random stream.

Last-Base capture now supplies the captor and its prepared future Base/Unit
context to the same disposition producer. Its successful caller also owns the
separate [post-restart tracking continuation](behavior/successful-capture-restart.md),
including the additional CRT draw, terrain knowledge and fresh-stack visibility.
No original generation, AI/startup placement or golden1.0.0 acceptance is
implied here.

Wrapped64×32 search is dynamically checked. Nonwrapping margins, range≤1 draw
suppression and margin clamps follow the original static continuation; those
full candidate paths still need distinct native dynamic coverage.

Validation: cached Linux Debug build;80 affected native groups pass with
allocation tracking. The live64×32 caller matches the native starvation site's
coordinates and exact CRT result, retains those draws through both post-search
vetoes, and preserves the terminal1,025-pair consumption when all terrain is
blocked. A stray excluded miss is calculated and retained even through the
later vetoes; a zero-rated monolith still receives the replacement package.
Saved-world Start retains the replacement package and advanced RNG.
Missing old landmark history leaves IDs, wrappers, queues, streams and world
authority unchanged. Existing actual Accept/founding/reload/next-upkeep tests
pass under the joined caller. LLVM15 correctness analysis passes six affected
production/consumer units in14.62s; unchanged query/preprocessing analysis is
reused. Focused ownership sanitizers cover the prepared disposition transaction.
