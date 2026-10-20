# Owned actor visibility snapshots

The ordinary frontend now receives an owned local snapshot of saved exploration,
observer presence, faction reveal, Unit disclosure/flags/status/source links and
separate Base discovery. Coordinates and dimensions come from the current map;
this transport is not restricted to the 80x80 terrain-inspector profile.

`PublishActorVisibilityUpdate` runs at the backend owner checkpoint after map and
Base callbacks, before queued actor updates are pushed. It captures committed
stationary disclosure as well as pending actor changes that need no terrain
update. Knowledge callbacks keep their existing dirty-event behavior; a callback
that changes a Unit is reflected in the subsequent snapshot. The producer
requires the actual local single-player participant and matching controller slot;
unavailable/network contexts deliver no usable authority. No new network query
endpoint or backend pointer is exposed.

`FR_ACTOR_VISIBILITY` owns and deep-copies the payload. The frontend retains its
own copy, rejects access after world retirement, replaces it on later delivery,
and clears it during deinitialization. The enclosing response already carries
the session world ticket. Missing migrated flags/status/links and missing Base
observations remain optional; no record order or foreign discovery is invented.
A working Former's native order remains unavailable until its original order
producer is bound, rather than mislabeling the port work-order enum as native.

Tile inspection now uses this committed knowledge when available and refreshes
its retained image on delivery. `Known` checks complete map geometry, parity,
observer presence and only the actual viewer's reveal bit. Survey alone cannot
admit a tile card. The existing backend inspection authorization remains in use.

Validation: 178 affected native groups pass, including actual publication,
FrontendRequest copy/destruction, actual frontend dispatch, post-callback flags,
retirement/clearing, missing history, generic 128x64 input, independent Base
discovery, foreign reveal isolation and malformed geometry rejection. Six
production translation units (including the preview consumer) pass correctness
analysis. Seven focused sight/native-stack groups pass with FrontendRequest.cpp,
GameWorkerKnowledge.cpp, frontend Game.cpp/GameNativeTerrain.cpp and the two
actual-consumer test TUs instrumented under ASan/UBSan. Remaining objects are
uninstrumented; leak detection is disabled.

This supplies inputs for the pending body/name/stack admission integration. It
does not yet implement those drawing gates or present main-map unknown black/grid
pixels. Native terrain zoom/map-size generalization retains its separate original
controls and acceptance boundary.
