# Regional land reachability implementation

`smac_territory::RegionalLandReachable` implements the original abstract land
graph at a fresh-workspace boundary. It searches backward from the peer Base,
allows water endpoints but excludes intermediate water, and applies the measured
directional territory rules. Mode0 admits negative/unowned territory; nonzero
mode rejects it and evaluates third-party territory using the peer's directed
primary relations and the saved human mask. Slot0 remains a distinct owner.

`RegionalBasesReachable` uses forward Base record order, stored regional Base
count gates, endpoint region filtering and raw-owner land-component marking.
`ProjectRegionalRoutes` composes the existing owned worker knowledge with actual
sea classification, retained region labels, relations and human membership.
It preserves empty/unclaimed claims as negative territory, rather than confusing
them with native slot0. Neither query writes claims, faction state or RNG.

The [original land-path receipt](behavior/abstract-regional-land-reachability.md)
and its literal cases supply 386 query expectations and 66 actual original
Base-caller expectations. The compiled C++ fixture projects those literal inputs
and compares boolean results; the actual production routing functions pass all
452 comparisons under AddressSanitizer and UndefinedBehaviorSanitizer. Numeric
arrays in the public JSON are compacted without changing decoded values; the
fixture records both original and public hashes.

Native integration tests also compose a real two-faction world after actual
Base founding: an unclaimed land corridor passes mode0 and fails nonzero mode.
Actual save/Start retains that result, queries preserve the snapshot and RNG,
and actual Base removal eliminates the route endpoint. Existing complete
territory, worker projection, founding/removal and restore tests pass. Targeted
correctness analysis passes for the routing translation unit and its two known
production consumers; the incremental build uses the normal CMake source list.

This closes the fresh routing calculation and its owned-input composition.
It does not enable the unfinished regional-strategy/Blind Research dispatcher.
The original reusable workspace's invalidation lifetime remains under reference
investigation, so no always-fresh replacement is silently installed at that
live summary boundary. Weighted Unit movement, first-step tie breaking, sea/air
designs, project-dependent paths and complete strategic AI remain separate work.
