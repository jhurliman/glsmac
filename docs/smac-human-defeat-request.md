# Human-defeat ending request

Last-Base disposition now records an owned human-defeat request when restart
does not produce a replacement site. Successful restart leaves it absent. This
is separate from the inactive roster flag, local focus and a conquest result.
The [original last-Base capture receipt](behavior/last-base-capture.md) records
a human-loss request while focus remains on the captor, and the
[last-Base starvation receipt](behavior/last-base-restart.md) establishes the
existing disposition producer and complete owner-tail behavior. Its public
case projection omits per-tile exploration/site dumps from the private trace.

The native controller finishes the current owner's population summary and
empty-faction financial tail, then delivers the ending to the existing outcome
overlay. Later owners do not begin upkeep. Enqueued gameplay after the loss is
discarded, and new gameplay/turn commands are rejected. The existing menu return
remains available. A request for a different human faction displays that faction;
it does not falsely label the local faction eliminated or announce conquest.

Simulation V13 stores the optional defeated simulation ID and validates its
binding to an inactive saved participant. V11/V12 have no tracked request and
migrate with it absent, without inferring a request from roster status. The
existing local-inactive status overlay remains available for those older saves.
Actual Start restores the new request without replaying disposition, grants,
Unit removal or an upkeep pass. A saved Colony continuation cannot coexist with
a terminal request.

This supplies the ending-state dependency for last-Base capture, which remains
fenced pending faithful restart search. It does not implement the original
global endgame presentation, scores, survivor-count/conquest/council victories,
AI or network outcome scheduling.

Validation: cached Linux Debug build; 86 affected upkeep, Colony, research,
simulation and save/Start groups passed with memory debugging, plus five existing
event/session lifetime groups and the actual `smac_outcome.gls.js` overlay test.
The live test covers local and foreign human loss, the losing owner's retained
financial tail, unchanged later-owner Base, discarded prequeued movement,
read-only status, rejected native/script commands and actual Start. Codec tests
check V12 migration and invalid/active owner identities. LLVM15 correctness
analysis passed eight changed/affected production translation units in18.67s.
The change adds scalar saved state and control flow; existing last-Base ownership
sanitizer evidence is reused, without another platform/sanitizer matrix.
