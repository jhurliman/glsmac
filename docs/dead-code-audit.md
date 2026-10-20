# Legacy and dead-code audit

Initial focused audit against integration d3b1e102 (2026-10-19). This is a
reachability investigation, not a claim that every old subsystem is obsolete.
An unconnected, source-backed gameplay rule is implementation work awaiting a
caller; deleting it would slow the reimplementation.

## Removed in the first pass

| Candidate | Evidence | Effect |
| --- | --- | --- |
| `Parser::match_char`, `read_until_char_any`, `read_until_sequence` | Exact identifier searches across tracked C++, GSE scripts, tools and build files find only their declarations and definitions. Protected, nonvirtual methods; the only concrete parser is `JS`, whose scanners use the remaining helpers. | Removes three unused scanner implementations and declarations without changing grammar or parser state. |
| `graphics/opengl/actor/CachingActor.h` | No includes, subclasses, construction sites, build references or other identifier occurrences anywhere in the tracked repository. | Removes an orphan abstract actor header. The live `Cache` and `Actor` hierarchy remains. |
| `util/Struct.cpp` | Contains only its header include and an empty namespace. `Struct::FlipMap` is a header-defined template, still called by `gse::Wrappable`. | Removes an empty compilation unit and its source registration; retains the useful template. |
| Second `types/Packet.cpp` source entry | The same file is registered twice in one source list. CMake already deduplicates it. | Removes redundant build metadata; no claimed compile-time saving from this entry. |

The first pass removes 92 production/build lines. It does not justify a broad
engine rewrite or a claim that dead code is the main implementation bottleneck.

## Investigated but retained

- `Game::get_settings` is marked deprecated but multiplayer host, join, lobby
  and event scripts still call it. Its label is not deletion evidence.
- Legacy float Unit state and its movement/attack script path still have live
  diagnostic/game-event consumers and old snapshot decoding. Remove these only
  after all supported callers have migrated and an explicit compatibility
  decision is made.
- Native wildlife/AI, flora transactions, faction cursors and computer goals
  contain intentionally staged implementations with focused source-backed
  tests. Their incomplete live wiring is a gameplay integration task.
- Native terrain's legacy instance tracking actively hides/replaces old sprite
  instances while the exact terrain renderer is enabled. Removing it would
  duplicate art or break fallback rendering.
- `OP_ADD_EVENT` rejects execution, but queued request destruction still owns
  its serialized payload and cancellation tests exercise that ownership. Do not
  delete only its destructor case or silently change request enum identities.
- Graphics caching, `Scroller`, and `Struct::FlipMap` all have live callers.
  An old name or an empty companion source file does not make a subsystem dead.

## Next candidates

1. The three `UnitManager::AttackUnitValidate/Resolve/Apply` C++ forwarding
   wrappers have no C++ callers and are not exposed in its GSE wrapper table.
   They emit old `unit_attack_*` trigger names with no script listeners. Check
   these separately from the still-registered `attack_unit` event and canonical
   Scout combat. They are good candidates for a small follow-up removal.
2. `Turn::FinalizeAndChecksum` and `Game::GlobalFinalizeTurn` are uncalled TODO
   throwers; `Game::IsTurnChecksumValid` and associated checksum bookkeeping also
   have no external readers. Reconcile with the current multiplayer turn path
   before removing this abandoned checksum protocol as one unit.
3. Several private/public convenience methods have declaration/definition-only
   references (for example old frontend selection helpers). Confirm virtual,
   macro, script-wrapper and platform consumers before deleting each group.

## Validation of this pass

- Exact identifier/include searches, plus review of the concrete `JS` parser.
- Fresh Debug CMake configure; generated link lists `Packet.cpp` exactly once
  and no longer registers `Struct.cpp`.
- Recompiled `Parser.cpp` and its `JS.cpp` consumer; linked a private executable
  using a read-only snapshot of the existing cached objects, excluding the
  removed empty `Struct.cpp` object. Shared caches were not modified.
- All 28 existing native groups in `Parser.cpp`, `Runner.cpp`, `GSE.cpp` and
  `Value.cpp` passed in that executable.
- Targeted repository C++ correctness analysis of both parser translation units
  passed with no diagnostics. No new mirrored deletion tests or platform matrix.
