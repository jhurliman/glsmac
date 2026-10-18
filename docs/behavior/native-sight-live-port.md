# Live human sight with stationary native life

The production human sight producer now resolves native simulation owner0
without treating it as a conventional player record. On the captured interior
land geometry, adjacent-eight detection flags the human Unit visible to native
faction0 and the native Unit visible to the human faction. The stored native
mask is committed together with worker knowledge; unrelated stationary masks
are preserved. Positive-owner detection keeps its existing separate policy.

Source evidence: [original native detection and movement cases](native-visibility-lifetime-cases.json).
The native creation/update family covers two human profiles, five terrain
families and24 offsets. The live port test restores a supplied native Unit,
then runs actual prepared human placement for those240 combinations. Preparation
must leave live records/knowledge/IDs untouched; commit must publish both masks
without a random draw. This is a human-placement integration test against the
original detection family, not an original save importer or live ecology birth.

The receipt additionally establishes moving-human clear-before-update and
stationary-native retention. Native movement has its own exploration, distance
and territory retention branch; it is not implemented by this change. Mixed stacks, wrapped/polar detection and native AI still require their actual
production paths. PR289 remains draft pending the complete lifecycle.

Validation: cached incremental build and focused native/sight/ZOC/movie/stage
groups pass. The new placement group exercises240 actual prepare/commit paths.
The movie checks cover aspect containment and missing-file/cancellation behavior;
interactive platform evidence remains that recorded by PR286.

The subsequent [five original movement controls](native-sight-and-land-zoc-cases.json)
now execute through the real movement event: source-only control permits a
step; both-endpoint control refuses without spending or a random draw. Split
controllers reveal exactly the unseen destination native and copy its current
improvements. Positive Pact canaries do not exempt native owner0. The existing
retained Pact byte also preserves raw bit0 independently; the diagnostic bit0
case exempts native ZOC, without claiming legal native diplomacy.

Validation includes an observer departure/return sequence: only the moving
human mask clears and repopulates, while the stationary native retains its
human-visible bit. Actual world snapshot/restore preserves all five resulting
native records and knowledge, including refusal effects and raw Pact state.
This does not implement native movement, AI scheduling or frontend visibility.

Targeted correctness analysis passes GameWorkerKnowledge.cpp. The native,
worker-knowledge, ordinary sight and ZOC groups also pass ASan/UBSan with
GameWorkerKnowledge.cpp, UnitManager.cpp, NativeUnit.cpp and Unit.cpp
instrumented; remaining normal objects are reused and leak detection is off.
No additional local platform matrix was run.
