# Snapshot owner checkpoints and native completion exclusion

This follow-up combines the admitted initialization coordinator with canonical
roster ownership. It addresses the three held PR77 paths without a new GC API or
a whole-`State::Iterate` lock. It requires those prerequisites; it is not a safe
standalone backport to the earlier PR77 base.

## Held review findings

The findings were reported against PR77 source `6805e3a3`; this follow-up is tested
on the later admitted-owner and canonical-roster ancestry. It does not claim the
old branch became safe merely because newer sequential tests passed.

| Finding | Current correction and evidence |
| --- | --- |
| [4295011127: roster mutation during flag commit](https://github.com/jhurliman/glsmac/pull/77#discussion_r4295011127) | Shared native exclusion covers validation plus all flag writes. The actual collector barrier and full Start/terminal Client response cases exercise native and claimed-owner boundaries. |
| [4295011133: unclaimed failure races reset](https://github.com/jhurliman/glsmac/pull/77#discussion_r4295011133) | The legacy independent callback is removed. Actual admitted Failure cases exercise Pending cancellation and reset attempted inside the Running error hook, including eventual successful reset. |
| [4295011139: requests consumed before parking](https://github.com/jhurliman/glsmac/pull/77#discussion_r4295011139) | A known Failure is published before MT extraction; no new batch runs while claimed. Save/animation/Ping responses remain pending, and cancellation still permits actual reset progress. |

## Production boundaries

The production map checkpoint is reached through `AdvanceInitialization`;
`InitComplete` runs inside the full claimed `StartInitializedWorld` operation.
The old unadmitted `CheckPendingSnapshotTurn` callback path is removed. Native
validation now either succeeds or throws. The coordinator retains the original
failure on its exact admission and executes `InitFailed` inside its claimed
Failure operation. There is no generation check followed by an unclaimed
`InitFailed` call that can overlap an actual reset.

A failure recorded by a completed native or ingress checkpoint is published at
entry to the next `Game::Iterate`, before extracting another MT request. Pending
and Running operations retain the coordinator's existing native dispatch gate.
Canceling Pending invalidates that claim and allows native reset to complete
without draining owner work. Canceling Running records intent, leaves the session
live until owner exit, and then allows the queued reset to complete. No admission
mutex spans Space registration, script callbacks, transport waits or teardown.

Snapshot binding validation and the complete saved-flag/turn commit use the
existing `Space::WithNativeMutation` exclusion. This is the same collection mutex
used by canonical roster publication and retirement. The lock order is collection
exclusion then snapshot mutex, matching root traversal; owner accumulation reuses
its already-held exclusion. No callback, queue registration or network operation
occurs inside these small native scopes. The snapshot mutex alone is no longer
used as protection against Player lifetime changes.

## Actual-path regressions

Five added native cases cover these boundaries:

- Three cases admit real `MT_Init`, execute initialize/configure and native
  bindings, then make the actual map checkpoint reject a broken reverse Player
  link. Save, animation-finished and Ping requests remain unconsumed before the
  owner Failure claim. They verify ordinary Failure's exact terminal error,
  Pending cancellation with completed reset before owner drain, and a reset
  attempted during the real error hook that completes only after owner exit.
- A native completion worker attempts the actual `ApplySnapshotTurn` while an
  actual collector is held inside root traversal. It cannot finish until the
  collector is released; both saved flags and the turn then appear together.
- A real Client handshake and admission reach the full claimed Start operation.
  During its `start` hook, a synthetic socket transport completes a real MT
  disconnect response. Actual backend iteration leaves it unpolled while Start
  is Running. A collector started during that hook sees the fully installed saved
  flags after owner exit. The next native iteration consumes the real terminal
  response, retires the Player and invalidates its retained wrapper.

These tests use the existing private asset-free Map constructor and explicitly
select the map-validation/Start checkpoint after real earlier phases. They do not
claim terrain generation, rendered map initialization or a complete networked
join. They do not invent unsupported running-roster occupancy replacement. Older
snapshot decoder fixtures remain synchronous layer tests under owner accumulation,
with explicit synchronous failure dispatch; the removed legacy generation tests
are not relabeled as coordinator tests.

Barrier cleanup releases held gates and joins workers before roots, observers,
transport or runtime are retired. The held-collector check uses a bounded 100 ms
noncompletion observation after the worker announces entry; the copied negative
control demonstrates actual overlap, not a universal scheduler-fairness claim.
The Start collector's entry announcement similarly proves worker scheduling, not
that every platform must reach its mutex before owner return. Source lock ordering
and the actual retirement/commit outcomes are part of the evidence.

## Validation

Local evidence is retained under
`/home/jhurliman/src/glsmac-snapshot-owner-checkpoints`. Intended production remains
unchanged while copied translation units are compiled and linked for controls.

Three copied-production controls each fail the corresponding real assertion with
exit 1 and no tracked leak: omitting the pre-MT Failure publication; omitting native
collection exclusion around saved-flag commit; and retiring the session during a
Running Failure callback instead of waiting for owner exit. The last test detects
that invalid claim/session state before deliberately attempting unsafe concurrent
native teardown. These are bounded missing-guard controls, not reproduced historical
Windows hangs or sanitizer-induced use-after-free demonstrations.

Final source validation on Clang/LLVM 15 under Ubuntu WSL:

- All 35 completion/RNG/coordinator groups pass with allocation tracking in the
  normal build (1.06 seconds) and ASan/UBSan/LSan build (1.15 seconds).
- Full normal CTest: 8/8, 48.90 seconds. Full sanitizer CTest: 8/8, 64.02 seconds,
  with leak detection and halt-on-error enabled.
- Existing correctness analysis: Game.cpp, one production TU, zero failures;
  14.90 seconds of analysis (17.95 seconds including runner setup).

The first full normal run reported 684 passed / 1 failed because the adjusted
synchronous decoder fixture caught an unrelated native RNG setup exception that
its existing test expects to propagate. The fixture now requests failure dispatch
explicitly only for identity-rejection cases; the existing RNG assertion and
production code were preserved. That failed log is retained separately. The final
results above include the corrected fixture, rather than counting that failure as
a production reproduction.

The sanitizer configuration uses the existing local no-PIE linker workaround
only; no repository suppression or toolchain policy changes. Native Windows,
macOS, hosted CI and the eventual combined publication are separate validation.

This does not establish arbitrary lobby/console callback synchronization, all
native Player methods, transactional world rollback, network event catch-up or
full save/load. No protocol format, gameplay completion rule or initial-roster
acceptance rule changes here. The original PR77 and later dependency branches
remain held until the combined publication receives its own checks and review.
