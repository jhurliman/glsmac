# Explicit Colony choice and upkeep continuation

This checkpoint implements the native question, Delay, Base Control Screen,
non-last-base abandonment and saved question state. The default turn controller
still uses its strict upkeep entry; activation waits for the last-owned-base
restart/disposition path. The new UI module is likewise prepared but not added
to the default game modules yet.

`Base::BeginSmacUpkeep` repairs workers and calculates the actual first resource
pass. At a size-one Colony question it commits posted minerals and allocation,
retaining old food and the independent mineral baseline. The owned pause holds
that first calculation and morale mode. No Unit is prepared before an answer.

The request binds a monotonic ID to owner, Base and turn. Delay skips mineral
posting and resumes baseline, population, final resources, research and credit.
Duplicate answers are harmless. Commands extracted before the pause are retired;
reset clears the continuation without reusing its request ID.

Base Control Screen runs the same deferred tail, posts current-base research and
income, then pauses before the next Base. Its owned-base worker and production
commands remain available. Closing resumes traversal exactly once; it does not
post that Base's income again. Commands carry the Screen request identity so
stale edits cannot apply after close or to a later screen.

Accept uses a distinct staged abandonment provider. It creates the actual Pod,
clears the newborn's home, rehomes older units, and retires the Base and claims.
An eligible compacted successor consumes its retained population/resource tail
without its own mineral posting. Last-owned-base disposition and a foreign
compacted successor remain explicit preflight rejections; a rejected answer
leaves the question available for Delay or Screen.
An unsupported population-removal tail in the successor likewise rejects before
manufacturing or deleting anything.

`SmacBaseUpkeepCache` retains committed N/M values and ordered support markings.
Actual founding, upkeep and worker recalculation publish this history with the
corresponding Base state. Read-only queries and supplied allocations do not
refresh it. Missing history is never replaced by zeros or a fresh query.

World snapshot V31 and Base manager V11 preserve these caches. Only the stable
Colony question is saveable during upkeep. Its frame preserves traversal and
the first calculation, without callbacks or request IDs. Loading restores both
managers, home links, knowledge, turn and RNG before validating the question and
minting a new request. It does not reset movement or replay resource posting.
Other incomplete stages, including research and Base Control, remain unsaveable.

Reference contracts in the primary research workspace:
`docs/behavior/colony-choice-continuation.md`,
`docs/behavior/colony-abandonment.md`, and
`docs/behavior/colony-screen-upkeep-boundary.md`.
The last contract covers native control permission and modal caller ordering;
it does not claim a captured original graphical session or network/SMAX coverage.

Focused native groups cover the Colony prefix/answers, Screen edits and close,
retained cache codecs, and saved-question restoration. The actual GSE module
callback test also covers Save, explicit answers and stale/duplicate Screen
close actions.

Validation on 2026-10-17 used the cached Linux Debug build with memory tracking:
48 affected native groups passed across the initial run and focused corrections,
including campaign disk restore, actual founding/startup, production/governor,
cache codecs, Colony choices, abandonment and world snapshot framing. The real
GSE callback script passed. Scoped clang-tidy 15 passed on all 15 changed
production translation units (13 in 33.22 seconds, the two codecs in 2.32 seconds).
The final test changes at `5902aee1` corrected setup order, the appended empty
continuation frame and the fixture's confirmed production-loss quote; no
production behavior changed during those reruns. Windows/macOS suites and a
rendered dialog session were not run for this inactive gameplay scaffold.
