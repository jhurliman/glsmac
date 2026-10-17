# Server departure and native Player retirement

The Server's `ET_CLIENT_DISCONNECT` lobby path removes the departing Player from
the roster, broadcasts its changed Slot, queues `player_leave`, and finally
destroys that Player. The final destruction now uses the existing
`Space::WithNativeMutation` collector exclusion. A disconnect during an active
game continues to retain the Player and occupied Slot for reconnection.

The notification uses an independent, shallow native Player copy. Its raw `new`
now has a matching plain `delete`, replacing the tracked `DELETE` that rejected
the untracked allocation under `--memorydebug`. This copy's destruction also
uses a short native-mutation scope: an immediate owner's notification cleanup
runs after `AccumulateImpl` returns, and thus need not still own collection
exclusion. Both original and copy destruction invalidate linked GSE Objects in
`Wrappable`; a collector can otherwise unlink the same wrapper concurrently.

Only destruction is inside these two scopes. Broadcast, readiness callbacks,
transport requests and `WTrigger` registration stay outside. Existing owner
accumulations reuse their exclusion. Queued cleanup runs while Space holds its
pending-accumulations mutex, so its new collect-only scope follows the existing
pending-before-collect lock order. The callback's Connection owner and its Space
remain alive through queue cleanup. Neither scope queues work or waits for a
transport response.

## Limits of this change

Earlier disconnect mutations remain unchanged: `RemoveCIDSlot`,
`GetPlayerAndClose` (including the reverse Player-to-Slot link), `RemovePlayer`,
`UnsetLinkedGSID`, readiness updates, and the running Player's `Disconnect` flag.
The final guard is not a transaction around those mutations and does not establish
general roster publication or script-access synchronization. Canonical Faction
ownership, shallow-copy referenced data, other join/copy paths and Player/Slot
adoption remain separate work.

Exceptions before final retirement retain the existing behavior. In particular,
an immediately throwing notification does not guarantee its cleanup executes;
this change does not make departure rollback or temporary-copy ownership
exception-transactional. The regressions do not diagnose the historical untraced
Windows test timeout.

## Actual event regressions

Seven native cases use real Network MT connect and GETEVENTS responses, actual
Server dispatch, actual broadcasts and GSE notifications. Only socket operations
are synthetic. Server gains the same protected transport-injection constructor
pattern already used by Connection and Client; its ordinary constructor still
selects the Engine's network instance.

Six cases combine native/foreign versus nested-owner dispatch with open-slot
lobby departure, closed-slot lobby departure, and running-game retention. A test
Player subtype pauses in its derived destructor, before the base Player and
Wrappable are destroyed. At this point the real Server has already registered
the queued leave notification. An explicit collector attempts actual traversal;
it must remain excluded until retirement finishes. The old-source control joins
the admitted collector before allowing base destruction, so it fails the
synchronization assertion without dereferencing freed storage or depending on
allocator reuse.

The seventh case dispatches on the owner without an outer accumulation. An actual
copy-destruction dependency notification checks that collection is excluded after
the notification's internal accumulation ends. The observation occurs after the
copy's wrapper invalidation, so its missing-guard control also avoids a deliberate
use-after-free. Both controls use bounded gate observations, not a proof of every
possible scheduler interleaving. Cleanup releases all gates and joins the workers
before retiring the runtime or test transport.

The collector promise establishes that its thread has been scheduled; it cannot
prove that thread reached the lock before the 100 ms observation expired. The
retained old-source controls are observed failures of the missing exclusion,
not scheduler-independent proofs of lock admission.

Assertions also cover pending-response behavior, exactly one notification and
copy destruction, retained wrapper invalidation, original Player destruction or
running-game retention, Slot and CID contents, one actual broadcast to the
remaining client, and repeated polling without duplicate retirement. The fixture
drains its final real disconnect response before collecting the Server.

```sh
build/bin/GLSMAC --gse-tests --gse-tests-native-filter '^server Player retirement' \
  --memorydebug --prefix build/server-retirement-test-data
ctest --test-dir build --output-on-failure
python3 tools/analyze_cpp.py --build-dir build \
  --source src/game/backend/connection/Server.cpp --output-dir build/server-analysis
```

The filtered test and explicit analysis selector require their respective tooling
prerequisites. Ordinary `--gse-tests` includes all seven cases as well.

## Local validation

Validation uses Clang 15 Debug on Ubuntu under WSL, based on integration
`cbf106ada6eba85fb0df104a6ad99a4af1504bac`. These are local Linux results, not
native Windows/macOS or hosted CI claims. Sanitizer runs retain the existing local
non-PIE diagnostic setting; no build-policy change is included.

The initial test-first build needed one missing fixture brace correction; its
compile log is retained. With that corrected, the original production code gave
four passes and two exact final-retirement overlap failures. Its tracked owner
case rejected the mismatched event-copy delete; the subsequent original Player
leak is the existing exception path described above, not a clean negative run.

- Final normal focused run with tracking: 7/7 passed in 1.081 seconds.
- Final normal full CTest: 8/8 passed in 44.04 seconds (44.094 with runner).
- Focused ASan/UBSan/LSan with leak detection, halt-on-error and tracking: 7/7
  passed in 1.143 seconds.
- Copied final Server TU with only original Player exclusion removed: both
  native lobby cases failed the overlap assertion, exit 1; no tracked leaks.
- Copied final Server TU with only copy exclusion removed: immediate-owner
  cleanup failed the overlap assertion, exit 1; no tracked leaks.
- Copied final Server TU with the mismatched copy `DELETE` restored: the tracked
  owner case failed at that operation, exit 1, including the expected unchanged
  subsequent Player leak on this throwing path.
- Changed production TU correctness analysis: 1 TU, zero failures, 2.71 seconds
  reported analysis time (5.359 with runner).

The first fixed full suite also passed before the fixture's collector-start wait
was made explicitly bounded. Its receipt is retained separately; the final
receipts above use that refinement. No intended production source was reverted
for the copied-TU controls.

Final commands, copied source controls, fixture snapshot, logs and checksums are
retained in `/home/jhurliman/src/glsmac-server-player-retirement/`.

## Focused publication

The publication is based on terminal-cleanup PR #110 at
`248899b72f689427fb8e3c272cf37586c0942260`. The Server implementation and seven
new cases are identical to source `65f70f96d974f3363fbdbca9b851d4c5b629d23e`.
This older stack lacks the native-filter CLI. Its ordinary full suite executes
the cases; no filter option is imported for this publication.

Tracked validation exposed two older fixture allocation mismatches, whose failed
logs are retained: raw `new GSE` versus GLSMAC's tracked runtime deletion, then
raw `new Tiles` in the private asset-free Map constructor versus Map's tracked
deletion. The publication carries the established `CreateRuntime`/`NEWV` fixture
helper and the private Map allocation correction from
`69e72f069b5f4161c36eadf4468fdd291f844c19`. The normal Map constructor and the
Server retirement implementation are unchanged by these adaptations. This is
not an all-fixture tracking claim: the older `PushBaseUpdates` MapState allocation
is not used by these seven cases and is outside this publication.

After those corrections, the exact publication full CTest suite passed 6/6 in
25.79 seconds. For low-cost tracked controls, a copied test-runner TU selected
only the seven existing registrations; no fixture function or production behavior
was changed by that selection. The fixed copy passed 7/7 in 1.067 seconds.
Removing only original Player exclusion gave five passes/two overlap failures;
removing only copy exclusion gave six passes/one overlap failure. Both controls
exited 1 with no tracked leaks. Source sanitizer evidence above is separate;
no additional publication sanitizer run is claimed. Publication commands, copied
TUs and logs are retained in
`/home/jhurliman/src/glsmac-server-player-publication/`.
