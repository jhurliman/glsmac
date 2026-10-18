# Recycling Tanks in the live production loop

The Base picker now offers registered Recycling Tanks when the owner has the
frozen prerequisite and neither Tanks nor Pressure Dome is present. Its command
uses source -3, matching the original queue convention for a facility; the saved
production record retains the typed Facility/source3 identity. Cost, retooling,
confirmation and stale-command handling use the existing native paths.

Completion posts the old mineral surplus, pays the actual cost and human
carryover, stages the local grant, and advances the queue. Queue eligibility
sees the newly granted Tanks, so a redundant queued Tanks entry is skipped.
An empty queue retains the completed target; subsequent upkeep checks actual
presence and does not pay for or grant it again. Unit, Stockpile and the local
facility successors described below are supported. Other facilities, projects
and orbitals remain separate.

The final resource/worker pass reads the staged facility through an explicit
registered-definition input. It does not temporarily mutate the live Base.
Minerals are not posted again, and population consumes the earlier nutrient
surplus. Facility presence, production, workers, food and retained resource cache
publish together before callbacks. Missing definitions, unsupported queue tails
and late Pop-role failures leave the old state unchanged. Construction creates
no Unit identity and consumes no RNG.

The reference contracts are RE-SMAC-UPKEEP-PRODUCTION-001,
RE-SMAC-COMPLETION-GRANT-001, RE-SMAC-FACILITY-ELIGIBLE-001 and
RE-SMAC-QUEUE-001 in the research workspace. Native fixtures use controlled
resource rows for intake4/4, then verify mineral intake4→5 without reposting,
food0→2 and mineral baseline0. This is an engine composition of those measured
boundaries, not a new original-game execution or an unmodified-map comparison.

Tests use the real GSE picker/OK handlers with DOM doubles, actual queued game
events, five ordinary turns, world save/actual Start and five more turns to
completion. Prerequisite loss and Dome suppress the offered choice. Separate
cases cover Stockpile, duplicate-Tanks/Scout and empty queue continuations,
second upkeep after completion and late-failure atomicity.

## HQ, Bank and Node extension

The same live picker/completion path now supports HQ (1), Energy Bank (7) and
Network Node (8). Other registered facilities are not automatically offered:
their missing consumers must be connected before their construction is enabled.
Current facility eligibility explicitly requires the saved no-project campaign,
so project substitutes/exclusions are absent by admitted world authority.

HQ completion stages removal of every other owned HQ and the current grant in
the same publication as production, workers, food and cache. Foreign HQs and
unrelated facilities remain unchanged. The ordinary Base update queues include
all affected Bases. Old resource caches retain their actual last calculation;
relocation does not manufacture a retrospective resource pass for an already
processed Base. Current energy queries and subsequent upkeep read the new HQ
locations. A late worker-role failure preserves the complete old world.

The current Base's final resource pass sees its new HQ center point. Bank and
Node grants reach the existing energy multipliers and research/credit callers
after the joined commit. With controlled raw energy4 and balanced rates, Bank
posts economy3/labs2 and Node economy2/labs3, each with its actual maintenance1.
Tests use the real GSE picker and native end turn, not supplied credit outputs.
Insolvent maintenance disposal remains unimplemented and is a separate producer.

Node Artifact resets are outside the admitted Combat/Former/Colony world,
which has no Artifact-link producer. An explicit package guard prevents new
packages from silently entering this completion path. Artifact linking and its
saved used/warning state still require implementation. Citizen-status facilities
and conventional defenses are likewise separate consumers, not implied by a
registered definition.

The extension passes 61 affected memory-tracked native groups and scoped
clang-tidy on all five changed production sources (8.76 seconds). New checks
include multi-HQ relocation with a foreign control, all affected frontend
refreshes, actual Start restoration preserving old cache history, and the real
picker/turn energy and research effects. These engine compositions do not
establish a rendered interactive session or SMAX equivalence.

Five focused ASan/UBSan groups also pass with leak detection, halt-on-error and
memory tracking, including HQ publication/late failure/actual Start and the
previous Tanks save/turn paths. The existing sanitizer build was reused; no
platform/sanitizer matrix was repeated for individual rule corrections.

### Prior Tanks-only validation

Validation: 59 affected native groups pass with memory tracking, retaining the
58 unchanged successes and rerunning all three Tanks groups after fixture/UI
composition corrections. Scoped clang-tidy 15 passes all seven changed
production translation units in 11.85 seconds. No new platform matrix or
rendered-session claim is made. The Base-screen target preview/forecast remains
limited, and broader facility completion/effects still need implementation.
