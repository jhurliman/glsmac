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
presence and does not pay for or grant it again. Unit and Stockpile successors
are supported. Other facility, project and orbital completions remain separate.

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

Validation: 59 affected native groups pass with memory tracking, retaining the
58 unchanged successes and rerunning all three Tanks groups after fixture/UI
composition corrections. Scoped clang-tidy 15 passes all seven changed
production translation units in 11.85 seconds. No new platform matrix or
rendered-session claim is made. The Base-screen target preview/forecast remains
limited, and broader facility completion/effects still need implementation.
