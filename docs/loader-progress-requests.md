# Requests during loader progress

`Game::SetLoaderText` can be called from `Map::ProcessTiles` and
`Map::FixNormals` while those methods still need their Map. Its nested request
pump therefore accepts only `OP_GET_FRONTEND_REQUESTS` and `OP_PING`. Reset,
initialization, map access/save, backend events and debug operations remain
queued for ordinary iteration. The list is explicit: future operations are
not automatically permitted during progress.

`MT_Reset` still signals the existing atomic initialization-cancellation flag
when it queues the reset. Map processing observes that flag and can return;
the reset executes on subsequent ordinary iteration. Executing reset inside
the previous progress pump could delete the Map on its own call stack and
clear the cancellation flag before that Map method returned.

MTModule accepts a protected optional function-pointer filter. It is noexcept,
has no captured context, and must only inspect the request while the request
mutex is held. Skipped requests are not marked processing, remain cancellable,
and retain their existing payload ownership. The default dispatcher remains
unrestricted. Response transfer, response destruction, and cancellation of
already-executing work are unchanged.

Native regressions exercise the actual asset-free `Map::ProcessTiles` loop,
which invokes the production progress pump after a test terrain module queues
reset, a frontend poll and ping. A dependency observer detects Map destruction
inside the outer call. Progress and ping complete while reset remains pending
and cancellation remains signaled; ordinary Game iteration then deletes the
Map and completes reset exactly once. Further tests cover all blocked opcodes,
canceling queued owned payloads, delivered/unclaimed response cleanup, and
unrestricted ordinary MTModule iteration. The old production pump fails the
Map-lifetime assertion; the fixed tests also pass with allocation tracking
and ASan/UBSan/LSan.

The test holds GC accumulation during native reset to isolate this reentrancy
contract. This change does not establish generic collector exclusion, retire
old-session requests, serialize concurrent snapshot restoration, or fix the
separate nested network dispatch in `Map::ProcessTiles`. It does not claim a
rendered initialization or cancellation smoke test.
