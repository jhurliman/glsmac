# World admission and MT request ownership

Every initialization reserves a never-reused, nonzero native ticket. The MT
request and response carry this scalar; an owned `Game::WorldAdmission` retains
the intended State and its exact immutable `State::GameSession` identity. Game's
collector traversal visits those States while an admission, accepted world,
failed-world result, or controller barrier retains them. The token alone is not
a collector root.

`MT_Init` returns a pending handle containing both request ID and reserved
ticket. Before teardown, execution checks the original State binding and, for
reinitialization, the explicitly supplied previous world. After its internal
reset/rebind, the accepted record retains the new binding. Reading the terminal
init response releases pending bookkeeping, while the accepted record remains
available for subsequent requests. The frontend promotes a ticket only from
its matching successful init response.

Map polling, saving, frontend queue reads and animation acknowledgments carry
that accepted ticket and validate it at execution, including requests already
extracted into the same MT batch as a reset. Retired requests return `R_ABORTED`
through the ordinary response queue. A valid ticket does not imply readiness:
save and animation operations also require a running map/animation manager.
Initialization errors have owned response strings, and later map errors belong
only to the failed ticket. Reset retires that ticket's retained error too.

`MT_Cancel` revokes an initialization before waiting for the actual MT request.
It also consumes an already completed response, which generic `MTModule`
cancellation leaves for its caller. Neither waiting nor generic request/response
destruction takes the admission mutex. Pending and accepted ticket scalars
survive cancellation long enough for a targeted reset of that frontend's world.

## Reset ordering

`MT_Reset(active, pending)` targets only those two identities. An old reset
completes harmlessly when its world has already retired; it cannot reset a newer
binding. `MT_RetireWorld` performs the same intent cancellation without enqueueing
reset, allowing a caller to retire its owner work before draining MT IDs.

The application controller has a two-step barrier:

1. `MT_BeginResetController` reserves and roots a cutoff record and cancels older
   admissions. It does not enqueue native teardown.
2. The caller drains old frontend request IDs outside script accumulation.
3. `MT_SubmitResetController(barrier)` enqueues that exact saved record. It never
   samples the replacement State or changes the cutoff. Repeated submission
   returns the same request ID until its terminal response is consumed.

`MT_AbandonResetController` releases an unsubmitted barrier, or cancels and
consumes its submitted request. It is idempotent after retirement and does not
revive canceled world intent. A consumed/abandoned barrier cannot be submitted
again. `MT_ResetController()` remains a convenience for immediate begin/submit;
the later public polling-controller change will use the split API.

Reserve/root publication/enqueue is ordered by the admission mutex, including
concurrent init/reset producers. That mutex is released before script execution,
Space queueing, native teardown, network work or MT waits. MT extraction filters
and payload destruction remain admission-lock-free.

## Frontend and reload

The frontend drains every owned MT ID, including save and pre-initialization
requests, before reload or retirement. Aborted replies are released without
reading a success union or submitting more work for the retired ticket. Old
animation acknowledgments are discarded during reload/map polling.

`MT_LoadMap(active_ticket, path)` obtains the intended State from the accepted
record and retains the path with the new admission. This avoids the old reload
path's dereference of a frontend State pointer cleared after successful startup.
It tests admission and rebind behavior without claiming the dormant rendered
LoadMap flow is complete: native reset still clears lifecycle handlers, whose
reinstallation is a separate concern. The loader currently ignores its cancel
callback, so these tests do not claim to exercise a rendered cancel button.

## Validation and boundaries

Native tests use actual MT extraction, responses and cancellation, real State
bindings and asset-free client initialization. Coverage includes queued,
in-flight and completed cancellation; same-address State rebinding; collector
root retention; stale requests behind reset; valid reload; concurrent cutoff
reservation; two-step/repeated/abandoned controller submission; ticket-specific
errors; and readiness checks. The in-flight case pauses actual connection
handler installation, revokes the admission, then releases and joins before
draining owner callbacks. It does not depend on sleeps or a race outcome.

An additional frontend fixture uses the real UI with null graphics and actual
`FR_QUIT` delivery through MT responses. It verifies that retiring ordinary
requests preserves the newly enqueued exit reset, avoids the old-world UI tail,
and delivers exactly one terminal completion. Only application navigation is
replaced with a counter; no rendered game or proprietary assets are required.

Implementation integration
[`a5af8fe88515846535f0244066654f68ddda270c`](https://github.com/jhurliman/glsmac/commit/a5af8fe88515846535f0244066654f68ddda270c)
passed all 15 focused Linux cases with allocation tracking,
then all eight CTests in 55.73 seconds. The separate ASan/UBSan build passed the
same focused cases and all eight CTests in 46.32 seconds with leak detection
enabled. Its linker uses the previously documented local `-no-pie` workaround
for sanitizer startup; this does not change the repository toolchain. LLVM
15.0.7 correctness analysis of the four changed production translation units
reported no failures in 11.86 seconds. These are local measurements, not hosted
or cross-platform runtime guarantees.

The focused publication at
[`8cb32997dbc0b05451a26f50da73244f2888dd0b`](https://github.com/jhurliman/glsmac/commit/8cb32997dbc0b05451a26f50da73244f2888dd0b)
passes Linux CTest 5/5 in 22.28 seconds. The broader integration at
[`8392f67aed7a331cef6c978a7e99abd6c89b580d`](https://github.com/jhurliman/glsmac/commit/8392f67aed7a331cef6c978a7e99abd6c89b580d)
passes Windows CTest 8/8 in 50.48 seconds and a fresh full-scope LLVM 15.0.7
scan of all 298 Linux production translation units in 62.67 seconds with zero
failures. The Windows-only network implementation is outside that analyzer
scope. The focused publication excludes the separately held world-completion
and map-packet-deferral changes; its test-context adaptation preserves the
request-admission cases without importing those unrelated tests.
The older publication base also lacks the integration's tracked fixture
allocation repairs; the tracked/sanitizer measurements above apply to the
named integration, not to the focused publication tree.

Hosted Windows CTest on `0592d21144a2e6289379021a5d05edf8ce159bea`
timed out at the immediate initialization-error case. That fixture throws a
native exception from the initialize callback; the publication's older
`Wrappable::Trigger` still released a mutex owned by a lock guard and only
reacquired it for GSE exceptions. The unchanged reviewed PR #54 fix and its
regressions are now included at
`685f13768571d1528e57efbd1791948db8b41d75`, which passes focused-branch
Linux CTest 5/5 in 22.28 seconds. Hosted Windows validation of that repair is
separate from this local result. Later failure-cleanup/reset PRs already carried
the same prerequisite; moving it earlier makes this admission PR independent of
those descendants. No timeout was extended and no error-path test was removed.

Five isolated source-copy negative controls each failed their targeted test:
omitting completed-response retirement, skipping execution-time ticket checks,
using only State pointer identity, ignoring the controller cutoff, and canceling
the newly queued frontend exit reset. Production sources stayed unchanged during
these controls.

This slice does not serialize already executing owner callbacks against native
reset, add collector exclusion around native teardown, or repair the public
singlethread reset busy-wait. Deferred initialization exceptions and startup
phase ordering require the separate owner-transition coordinator. It also does
not claim atomic world restoration, complete rendered reload, or general lobby
thread safety.
