# Backend RNG continuation in world downloads

World-download version 4 carries the ongoing backend generator's state. After
the existing frozen rules profile, it writes a typed string `jsf32-v1` and four
typed integer frames containing JSF32 words `a`, `b`, `c`, `d`, in that order.
Each word must be between zero and UINT32_MAX before conversion. Algorithm
mismatch, wrong frame types, truncated frames and out-of-range words are
rejected. The existing Buffer frame checks still apply. Versions 3, 2, 1 and
unversioned downloads are rejected before map allocation; mixed versions cannot
join through this world-download format. This does not version disk map files.

The writer captures `Game::m_random` at entry to synchronous world serialization,
before writing the profile, map or manager frames. Neither capturing the state
nor serializing those records draws from the generator. The state represents
the continuation at that boundary. It is not a seed: all four zero words remain
zero, preserving the existing generator's absorbing state instead of reseeding.

The production download callback validates the entire profile, RNG and
[turn/completion prefix](world-snapshot-turn.md)
before allocating the map. It decodes the remaining map, resources, units,
bases and animations through `DeserializeWorldSnapshotBody`. Only a
successful body decode stages the state. Map construction and initialization
share the backend RNG and can consume it during reconstruction. `InitComplete`
therefore installs the staged state after the `start` setup callback and both
pending-instance queues and completion-binding validation complete, before
normal event/turn processing resumes.
An ordinary start without staged state keeps its existing draw ordering.

Reset, initialization cancellation, setup failure and configuring another
download discard any staged continuation. A malformed header reaches the
existing immediate/deferred download error handler and requests a disconnect.
A body or setup failure can leave partially reconstructed objects under the
existing terminal-failure contract; this change does not make the entire world
load transactional or recoverable. Staged RNG state is never applied on those
failure paths.

This preserves one serialized backend RNG continuation. It does not establish
future host/client lockstep: current host-only event resolution consumes draws
that peers do not reproduce. Map-generation portability, separate menu/UUID
randomness, a complete checkpoint format and simulation determinism remain
separate work. Downloaded turn metadata is now restored without gameplay
advancement, as described in [world-snapshot-turn.md](world-snapshot-turn.md).
Neither change is a general arbitrary-turn checkpoint restore.

## Regression coverage

Native tests use the production codec and actual Game writer. Real Client
final-chunk dispatch covers invalid versions, tags and every word's bounds,
types and truncation, both immediately and after worker-thread accumulation.
They verify rejection before map allocation without changes to live RNG,
settings, managers, turn or frontend requests.

The successful fixture uses the existing private asset-free Map constructor,
the exact production body decoder and real `InitComplete`. It reconstructs real
canonical unit definitions, units, bases and population records, then verifies
future integer and floating draws for both rulesets and the all-zero state.
Renderer initialization is intentionally omitted; explicit draws model that
phase, and an extra pending setup unit forces a real spawn-hook draw after the
start callback. This detects installation before deferred registration ends.
Other cases cover ordinary initialization, reset, cancellation, native/GSE setup
errors and a truncated body. No original assets, rendered map, full interactive
join or portable map-generation claim is involved.

```sh
./build/bin/GLSMAC --gse-tests --prefix build/test-data-world-rng
```

The focused selector is available after integrating the separate PR #57:
`--gse-tests-native-filter 'world.*RNG|real final download chunk'`.
These cases also run in the regular native suite. The initial RNG slice's local
Clang 15 / Ubuntu WSL integration validation passed all eight CTest entries
(36.43 seconds), all nine focused
cases with allocation tracking, and explicit correctness analysis of Game.cpp
and WorldSnapshotRandom.cpp. Three separately linked negative controls left the
intended sources and fixed binary untouched: omitting installation failed both
ruleset continuation cases; installing before deferred registration also failed
both; removing word bounds validation made the codec's malformed-state
test fail. The focused publication branch on #53 separately passed its five
Linux CTests in 22.30 seconds (full GSE 16.04 seconds).

At integration `bfe276d3`, Windows CTest passed 8/8 in 45.87 seconds. A full
ASan/UBSan/LSan run passed 7/8: the new GSE initialization-error case exposed an
existing 63-byte leak in `gse::Exception::what()`, which returns an unowned
`strdup` allocation. That separate ownership repair is required for a clean
leak-enabled integration. These ordinary passing tests do not claim leak-free
behavior of this older focused parent. Interactive join remains unvalidated.
