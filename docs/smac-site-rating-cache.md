# Saved site-rating history

The prepared `smac_site::RatingCache` owns ratings and Base-radius exclusion
bits separately from occupying affiliation. A nonzero rating is reused; an
excluded tile is skipped without erasing its rating. Query results0 remain
misses on subsequent passes. Refill follows parity-row order across land,
water and both polar rows, with no additional occupancy/owner/turn gate.

Local invalidation clears the ordered inner21 ratings. Base marking preserves
their ratings and sets exclusions, then clears outer28 ratings without clearing
exclusions. Removal clears49 ratings but only the inner21 exclusions, then
re-marks every surviving Base in record order before refill. Distant and sea
Bases participate. Stray exclusions outside those footprints survive. Terrain
postpass clears all ratings and preserves exclusions; region fill preserves
both. Query failure leaves the input value unchanged.

Topology V3 stores optional packed rating/exclusion bytes with bounded framing
and flag validation. V1/V2 migration preserves missing history as unavailable.
Fresh map initialization establishes empty history only after complete terrain
postpass data is available. Actual Start retains stale nonzero ratings and
exclusions through the real snapshot serializer and loader.

The [native refill fixtures](../src/gse/tests/fixtures/SmacSiteRatingCache.h)
freeze seven mixed passes, the989/52 complete founding/removal query orders,
and a91-query removal repair involving all three surviving Bases and a stray
exclusion. Query outputs supplied by these fixtures test the cache caller;
the independent31 land and40 water score cases test the actual score function.

Starting/running founding, generic Base spawn/despawn, starvation and Colony
abandonment now prepare the cache with their future Base list and publish it
with their existing authority transaction. Capture stages refill under its
private future context, restores both knowledge and topology on every exit,
then publishes the prepared result during apply. Unit retirement/replacement
commits keep their joined sight result while publishing the prepared topology
before notifications. The live constructor checks preparation before changing
IDs or attaching the candidate center.

The map adapter queries only cache misses. It uses DisplayedTerrainV1 terrain
ordinals, explicit bonus/site markers, active Jungle/Uranium flags and owned
coastal/area metadata. Existing nonzero ratings bypass decoding. Shared detail
for Mount Planet/Crater is not stored by the port Tile; an actually queried
unsupported tile is refused instead of inventing that input. Original world
generation and its complete landmark/site metadata remain separate work.
Terrain publication/restore retain known history; initial fresh admission
marks existing Bases once, without making every postpass unconditionally refill.
Ordinary Former completions preserve existing nonzero ratings. Startup and
late-turn scoring use the accepted query contract, without a turn100 refusal.

This joins the site-cache producer; it does not replace the approximate
last-Base restart search or enable last-Base capture. Those consumers are next.

Validation: cached Linux Debug build;64 affected native groups pass with
allocation tracking, including capture, founding/abandonment, actual Start and
all five ordinary Former completions. The new live case preserves poisoned
ratings through real founding/removal, rejects an unresolvable query without
changing IDs/tiles/queues/history, and reloads the saved result through actual
Start. LLVM15 analysis passes13 live production/consumer units in33.02s; prior
core codec/score analysis is reused for unchanged source. Eighteen focused
ASan/UBSan groups pass with leak detection and clean exit. PR202 is accepted
after clean current-head Codex review and the relevant CI gates.
