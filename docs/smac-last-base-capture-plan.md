# Last-Base capture integration

The ordinary capture, assimilation tail and screen-entry slices are PRs192–194.
Last-Base capture remains an explicit refusal in `BaseCapture.cpp`. The next
implementation must use the original disposition caller rather than interpreting
zero Bases as a conquest victory.

The [bounded original investigation](behavior/last-base-capture.md) reaches the captured-Base screen
after losing the final population2 Base. Its remote homed Former or Colony Pod
is removed before disposition; the Pod does not exempt that lost-home scan.
Disposition removes remaining faction units, reinitializes nonzero-turn faction
state, attempts restart, and can request an ending on failure. A successful
notification acknowledgment does not establish a global victory result. The
public last-base-capture receipt is frozen and retained with this plan;
private executable research and original assets stay outside the PR.

## Required dependencies and live callers

Current prerequisites: PR195 owns human-defeat requests and their saved/controller
path; PR196 owns retained regional Base counts and their incremental writers;
PR197 preserves the original ocean-choice index. PR198 adds saved region areas,
fertility and coastal postpass values. PR199 implements the31-case original land
site-quality query. PR200 adds the40-case sea query and PR201 the40-case
startup/late-turn extension; all are accepted after current-head review and CI.
None replaces restart search
by itself.

The [joined site-cache producer](smac-site-rating-cache.md) now owns ratings and
exclusions, retains them through save/Start, and stages founding/removal/capture
refill with their future Base list. Ordinary Road/Farm/Solar/Mine/Forest preserve
nonzero history; water/polar/startup/late queries use the original formula.
Old saves retain unavailable history, since a fresh recount cannot recover a
stale score or stray exclusion. PR202 is accepted with18 focused sanitizer
groups and current-head review/CI. The [original candidate loop](smac-restart-search.md)
now matches both full native traces; its live future-world producer is next.
original world generation and complete landmark-detail ownership remain open.

1. Complete the restart search contract used by `PrepareLastSmacBaseDisposition`.
   Its current farthest-site policy is explicitly a port approximation. Capture
   introduces a captor identity and a reached same-land-region/nearest-captor-Base
   rejection. Original candidate generation and retry consumption use the CRT
   stream. Establish success and all-attempts-failed outputs before substituting
   this policy into capture. Keep reference coordinates as expected outputs,
   never hardcoded sites. Reuse the existing restart package selector.
2. Reuse the owned ending request and actual controller accepted in PR195.
   The checked two-human local capture sets an ending request on
   defender loss even though focus remains the attacker; that does not prove
   the attacker has won. Existing last-Base starvation/disposition must share
   this producer, save ownership and controller instead of silently dropping
   the request. Establish the separate conquest/survivor-count caller before
   claiming a conquest result.
3. Join the capture and faction transactions. Stage post-loot defender economy,
   nonzero initialization, retained technologies/research with invalidated cost
   cache, hostile-history reset, roster activity, Unit retirement/replacement,
   territory, worker calculation, capture RNG and ending request together. Native
   queries need a future Unit registry/occupancy view so workers and sight cannot
   observe units that disposition will remove. Do not invoke callbacks or retire
   wrappers until the combined authority is coherent. Preparation and failures
   must preserve current entities, queues, IDs, RNG and retained script objects.

## Acceptance for the next functional PR

Use real `move_unit` and saved-world Start. Cover a failed restart, a successful
restart with its actual package and next founding/upkeep, and an explicit
Do or Die/late-turn refusal under the established rule. Check old homed and
unhomed Units, a homed Pod discriminator, retained Base/Pop identities, exact
loot and initializer credits, stale faction population aggregate, diplomacy
history, RNG, roster/controller identity and pending ending state. Separate
screen entry from unverified dynamic screen return and final victory UI.

Use the cached incremental build, affected capture/restart/upkeep test groups
and targeted production analysis. Unit retirement/replacement and the future
query view warrant focused ownership sanitizers. Full platform and network
acceptance belong to the playable conquest milestone and release gates; this
slice alone cannot close M2 or golden1.0.0.
