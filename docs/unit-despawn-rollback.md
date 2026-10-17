# Unit despawn rollback

The production `despawn_unit` event now copies its rollback record before
removing the unit. The previous handler retained the deleted unit's wrapper;
Wrappable destruction changed that wrapper to `undefined`, so rollback failed
on `u.id`. Ordinary `spawn_unit` also resets movement and the moved flag and
runs new-unit gameplay, making it unsuitable for restoring prior state. Mutable
native fields are first bound to scalar locals: a direct object-literal property
read in GSE can retain a native Ptr rather than taking an independent copy.

The copied record contains every field in the authoritative unit snapshot:
numeric ID, definition ID, owner slot, tile coordinates, movement, morale,
health and moved-this-turn. The checked `um.restore_unit(record)` binding
requires every field, resolves links in the current manager/world, and rejects
an occupied or non-prior ID, unknown definition, invalid owner/coordinates,
invalid morale or nonfinite movement/health before claiming a tile or changing
registration. It preserves finite signed movement/health, including zero and
negative health. Restoration retains the next ID, invokes no new-unit hook and
consumes no RNG. Deferred startup records the unit as restored rather than new.
If restoration cancels a pending frontend despawn, it queues a refresh from
the restored unit: despawn may have coalesced an earlier movement/health update
that the frontend has not received. If the despawn was already flushed, the
usual spawn carries the restored state.

Numeric identity is restored, but C++ object and script wrapper identity are
not revived. Old wrappers remain `undefined`; a fresh lookup or decoded ID
reference resolves the restored unit. Native callback captures, arbitrary
script properties/callbacks, active movement animations and their tile locks
are not reconstructed. This is not a fix for `attack_unit`'s separate rollback
or movement-callback lifetime defects, and does not claim rollback safety for
an active animation. Allocation-failure recovery is outside this change.

The production validator still permits only caller 0. Ordinary nonhost local
requests are rejected before application. Tests first check that rejection,
then explicitly inject caller-0 fixture requests through the real optimistic
application/response path to exercise acceptance and rejection bookkeeping.
This is a handler-contract check, not a claim that ordinary nonhost despawn
currently reaches rollback over the network.

## Validation

Asset-free native tests call the production event script and actual managers.
They compare serialized unit state before and after rollback, canonical
manager/owner/tile pointers, tile membership, next ID, RNG, spawn/despawn hook
counts, stale wrappers and a freshly decoded stable ID reference. Both frontend
flush timings are exercised, including a newer movement/health refresh queued
before despawn. Additional tests cover actual accept/reject
response dispatch, 24 invalid restoration records with unchanged manager/tile/
frontend state, and deferred restoration with negative finite movement and
zero health.

With the original handler, both frontend-timing regressions failed with
`GSEInvalidDereference: Could not get .id of non-object: undefined` at the
original rollback's `u.id`. The focused fixed tests pass. Local evidence is
retained under `/home/jhurliman/src/glsmac-unit-despawn-rollback/`.

Two draft negative controls also guard details of the fix. Without explicit
scalar locals, ASAN reported a heap-use-after-free in `Ptr<float>::Clone()`
while returning rollback data after destruction; this is retained in
`asan-focused.log`. Without refreshing after a pending despawn is cancelled,
the real frontend-state regression passed after a flushed despawn but failed
before the flush because the prior movement/health update was lost
(`pending-refresh-negative.log`). Neither defective draft is committed.

Final Linux Clang 15 Debug CTest passed 8/8 in 37.55 seconds. The five focused
restoration tests also passed with `--memorydebug`, and in the separate
ASAN/UBSAN build with `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1`. The focused command is:

```sh
build/bin/GLSMAC --gse-tests --gse-tests-native-filter '^test unit (despawn|restore)' --quiet --prefix build/test-data-unit-restore
```

The sanitizer build used the existing dependency library paths and a local
`-no-pie` linker workaround for the previously observed WSL pre-main sanitizer
startup failure; no build policy is changed. These are focused sanitizer/leak
results, not a claim that the full suite's known parser leak baseline is fixed.
The focused command requires the separate native-selector change in PR #57,
which is included in the validation source. On a branch without that selector,
run the full `--gse-tests` suite; all five restoration cases are registered there.
The existing curated clang-tidy checks passed for `UnitManager.cpp`. Final logs
are `final-ctest.log`, `final-asan.log`, `final-memorydebug.log` and
`final-analysis.log` in the evidence directory above. Native Windows combined
validation remains an integration check.
