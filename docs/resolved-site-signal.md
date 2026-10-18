# Live resolved site signals

Installed Planet site-rating queries now use the original three-class
[resolved site/pod query](behavior/resolved-site-pod-query.md), rather than
`FEATURE_UNITY_POD`. The saved raw item word, original pattern, decoded altitude,
coordinates and active automatic-land-resources option feed the query. The
installed stock profile binds its executable coefficients11/61; these are not
invented map-header fields or gameplay RNG draws.

`GlsmacResourceMapPolicy::SiteSignal` preserves classes0/1/2. The live
`Map::PrepareSiteRatings` caller consumes nonzero presence, matching the
site-quality rule. DisplayedTerrainV1 retains its explicit legacy marker policy;
it is not relabeled original map semantics. Rendering pod artwork and executing
pod rewards remain separate work.

The change adds no new save fields. InstalledPlanetV1 already retains all query
inputs, and policy restore preserves the result. Site caches keep their existing
lifetime: nonzero stale ratings survive, while actual misses/invalidation/refill
use the corrected input. An inspector query must not clear historical ratings
just to obtain a newer value.

Validation:36 literal original query/coefficient controls; all109824 ordered
original synthetic outputs hash-match production under ASan/UBSan; five focused
production correctness units pass. Actual native import of the installed3200-tile
Planet asset gives classes3031/109/60 under Standard and3200/0/0 with automatic
land resources enabled, matching the complete original loader/preparation
receipt. Every tile also retains its class after policy serialization; changing
only its display marker does not affect the installed query. Site-cache,
site-quality, last-Base restart and all42 faction/difficulty claimed-Start
cases pass through the cached Linux build. This is not rendered pod parity,
full map-preparation parity or completed regional/Blind Research integration.
