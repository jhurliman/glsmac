# Initial world events and the first live turn

New-world startup keeps the live turn at zero until the existing `advance_turn`
event is applied. `Game::FirstTurn` queues an explicit turn ID of one and clears
its existing checksum bookkeeping; it no longer writes the live turn ID before
that event reaches the dispatcher.

Previously, `create_world` and `start` themselves observed turn zero, but events
they queued observed turn one before the first actual upkeep. In particular, the
production `spawn_base` handler could construct a base and call `base_spawn` with
the wrong initial turn. This boundary matters for founding rules that distinguish
initial world creation from later founding. This change does not implement those
rules or choose an initial production target.

## Preserved execution order

The production path remains `StartInitializedWorld`: `create_world`,
`InitComplete` (including `start`), `FirstTurn`, then `ProcessEvents`. Its first
captured event batch contains the events already queued by creation/start,
followed by `advance_turn(1)`. The real `advance_turn` script calls
`Game::AdvanceTurn(1)`, which changes the ID, publishes the frontend turn, and
runs the existing unit/base/game turn callbacks.

`ProcessEvents` still extracts one batch. An event created by another event's
handler stays queued for the next batch; it is not recursively drained or moved
ahead of the first upkeep. Such nested events can therefore observe turn one.
No additional event checkpoint or hook reordering is introduced here.

`ResetGame` already calls `Turn::Reset`, setting the ID to zero. The regression
covers restarting the same backend Game after turn 37, without changing reset
production code. Downloaded worlds use the existing restore path, publish their
saved turn once, and do not call `FirstTurn` or repeat initial upkeep.

## Regression boundary

Seven native cases use the real asset-free backend fixture, actual startup owner
dispatch, and production `spawn_base`, `advance_turn`, `add_base_pop`, and
`remove_base_pop` event handlers. The existing `_base_growth` fixture loads the
production base callbacks while substituting only asset-bearing definitions.
Tile resources are supplied explicitly by the fixture. No original files,
renderer, or network traffic are needed.

The cases cover immediate and owner-queued new-world startup; cancellation while
the owner operation is pending; cancellation requested from a real `base_spawn`
callback while that operation is running; restart from turn 37; and immediate
and queued downloaded-world startup at turn 37. They check:

- Creation, start, initial base spawn, and already queued creation/start events
  observe zero, followed by exactly one base/game upkeep at one.
- Exactly one frontend turn publication carries one for a new world, or the
  saved 37 for a restored world.
- Nested events retain their next-batch order. The actual first upkeep queues
  one population event, whose production handler creates one population.
- Pending cancellation runs no startup callbacks. Running cancellation remains
  deferred until the owner operation ends, then clears the map, queued events,
  and live turn without processing the nested events.
- Restored startup runs `start` at 37 without founding or initial upkeep.

The fixture begins at the real final startup checkpoint with an asset-free map;
it does not claim to exercise rendered map initialization, the default world
script's complete content, or a full save/load or multiplayer lockstep contract.

## Validation

Source base: `cbf106ada6eba85fb0df104a6ad99a4af1504bac`.

With the new tests and unchanged production code, the focused tracked run exited
one: three cases passed and the four new-world execution cases failed at the
initial event turn-order assertion. The fixed production code passed all seven
cases with `--memorydebug`; both runs reported no tracked allocation leaks.

The fixed branch passed all eight normal CTests (51.81 seconds). The seven
focused cases also passed with ASan, UBSan, LSan (`detect_leaks=1`) and
`--memorydebug`, with no diagnostics or tracked leaks. The local sanitizer
executable uses the previously documented non-PIE validation configuration; this
change does not modify build policy. Explicit correctness analysis of
`src/game/backend/Game.cpp` reported zero failures (16.49 seconds).

Local receipts are retained outside the checkout in
`/home/jhurliman/src/glsmac-unit-weapon-parser/initial-turn-{negative,fixed,asan}.log`,
`initial-turn-fixed-ctest.log`, and `initial-turn-analysis/summary.json`.
