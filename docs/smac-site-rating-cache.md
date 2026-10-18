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

This commit establishes saved ownership, fresh-world initialization and the
prepared mutation operations. Actual Base founding/removal/capture and global
territory callers still need to publish their joined prepared cache. Do not
claim live cache parity or original restart search until those callers are
connected. The newly frozen late-turn adjustment must also be integrated before
enabling refill throughout an unrestricted campaign.

Validation: cached Linux Debug build;41 affected native groups pass with
allocation tracking (40 unchanged successes reused after correcting the
restore fixture, then the one corrected actual Start case passes). Eleven
production units pass LLVM15 analysis in20.24s, with three changed producer
units rechecked in2.34s; query/cache consumers also pass the sea follow-up check.
Focused sanitizer validation remains due with the joined live producer.
