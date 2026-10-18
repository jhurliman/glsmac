# Sharing reviewed integration build caches

Pull-request merge-ref caches can be reused by that PR, but not by sibling PRs.
The cache inventory on 2026-10-18 showed the new caches under refs/pull/*/merge;
the only branch-scoped compiler cache was main at a8f02f56, created October 17 at 21:06 UTC,
and codex/faithful-1.0 had none. Successful newly implemented gameplay builds
therefore did not warm the next focused PR. This follows GitHub's documented
[cache scope restrictions](https://docs.github.com/en/actions/reference/workflows-and-actions/dependency-caching#restrictions-for-accessing-a-cache).

`cache-integration.yml` publishes the matching Linux Debug compiler cache after
trusted integration pushes. It performs an incremental build/cache publication,
without repeating regression tests or a platform matrix. The first publisher is
allowed to finish while the latest pending merge supersedes older queued merges;
rapid integrations cannot repeatedly cancel the initial cold build. Documentation
only pushes skip the build. Compiler hit/miss statistics are retained in both
publisher and PR logs so cross-PR warm reuse can be measured.

Portable codec payloads are seeded only when their source/build/workflow scope
requires them and no exact recipe/toolchain cache exists on the branch. Ordinary
gameplay pushes do not rebuild portable FFmpeg. The recipe, payload paths and
keys match regression CI; a cache hit needs no compiler/probe build in the
publisher. Actual packaging/cancellation verification remains in selected PR CI.

The publisher has read-only repository permissions, runs only on integration
pushes, and never imports object artifacts from untrusted PR runs. Existing PR
review, regression, lifecycle, and selected platform gates are unchanged.

The recent 30-PR audit (260Ã¢â‚¬â€œ289) had zero labeled/unlabeled issue events, so no
label-trigger runner savings were demonstrated. Existing ci:full label semantics
are preserved rather than adding a second gate/reuse mechanism for unobserved
waste.

Two successive PR292 heads with unchanged C++ source measured a Linux build of
220 seconds followed by 11 seconds from that PR's own compiler cache. This shows
the value of warm object reuse, but is not evidence of reuse across PRs. PR291's
refreshed Linux run also restored its existing exact portable-codec key, built
both probe executables in 3.44 seconds and verified them in 1.85 seconds; codec
payload restore took about 0.9 seconds. The earlier cold hosted probe build took
26 seconds. These are same-PR samples on different runners, not a controlled
cross-PR comparison. Hosted
cross-PR cold/warm measurements, hit statistics and publisher overhead must be
recorded after the first integration publication and a subsequent real PR
consumer. The publisher is useful only if the saved downstream runner time
outweighs its additional incremental build.
