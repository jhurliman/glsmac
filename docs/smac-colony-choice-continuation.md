# Explicit Colony choice and upkeep continuation

The default turn controller and UI now use the native question, Delay, Base
Control Screen, abandonment, last-base restart/disposition and saved question
state. An eliminated local faction gets a port outcome window with a return to
the main menu; this is not a recreation of the original ending presentation.

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
without its own mineral posting. A foreign compacted successor remains an
explicit preflight rejection; a rejected answer
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

GitHub Codex review found that a rejected Accept left the UI submission guard
set. Submission now blocks only after successful event queuing; the real GSE
script passes both rejected-Accept-to-Delay and rejected-Accept-to-Screen paths
without replacing the request or resetting the game.

## Last-base integration

The working branch now stores faction activity and restart prohibition
independently, plus Do or Die, in simulation V11. Restoring these fields does
not reinitialize treasury, research or controller identity. Inactive factions
cannot acquire live canonical Bases/Units or issue production/worker commands;
upkeep skips them. A suspended traversal can retain an already visited faction
that was eliminated during that upkeep. The installed SMAC manual (PDF page
199) describes restart as normal behavior and Do or Die as its disabling option.
Custom rule selection and the special Mind Worm/interlude prohibition producer
remain separate consumers; prohibition must never become a restart-used latch.

Starvation and accepted abandonment stage removal of every old owned Unit,
replacement factories, record order, tile membership, sight/claims, placement
history and Base retirement before publication. Accept includes its actual
manufactured Pod, which is then retired with the old roster. IDs advance only
on commit; all callbacks observe the final roster and faction state. A late
factory failure leaves the old world and available question unchanged.

Loss preserves technologies, target, labs and research fraction, invalidates
the research threshold, adds the frozen starting-energy scalar and faction
ENERGY bonus to treasury, and clears pending debit. Successful restart resets
psych/labs to 0/5. Refusal sets activity false; the empty faction's later upkeep
tail performs the second initialization independently (100→110→120 in the
reference inputs). A successful restart leaves the independent prohibition
flag clear and does not act as a once-only restart latch.

The port placement policy scans ordinary founding-capable land, maximizes
distance from surviving Bases and the lost center, and breaks ties in row-major
order. It consumes no RNG. **This is not the original placement search.** There
is no hardcoded reference coordinate. No site produces ordinary elimination.
Custom designs lacking an admitted renderer still reject before publication.

A zero-Base faction can found again. The first-Base count predicate grants HQ;
the non-network turn package grants population 1..5, Tanks at 75, Commons at
100 and Bank at 150. Resource caches use the actual new population and current
SUPPORT. Source contracts: RE-SMAC-LAST-BASE-RESTART-001,
RE-SMAC-RESTART-PACKAGE-001, RE-SMAC-FOUND-FAC-001,
RE-SMAC-FOUND-POP-001 and RE-SMAC-FOUND-TERR-001 in the research workspace.

Validation: 72 affected Linux native groups pass with memory tracking. New
cases include starvation/Accept × turn3/100/101/Do or Die/prohibition/no site,
actual Start restoration, and ten first-Base population/facility boundaries.
The integrated Accept → restart → founding → world save/Start → turn4 path
matches food0→2, Scout completion/mineral baseline1, credits110→111 and labs0→1
with fraction70 retained. World-body tests explicitly admit the saved Player
difficulty, which belongs to the campaign roster envelope. The actual GSE
Colony callback and eliminated-outcome scripts pass. Scoped clang-tidy 15 passes
for the changed production sources. A rendered UI session remains unverified.
