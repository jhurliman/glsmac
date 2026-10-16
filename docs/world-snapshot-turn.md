# Downloaded turn metadata and presentation

The V2 world download keeps its existing final signed integer turn field.
Reading it assigns the turn ID without calling `Game::AdvanceTurn`. That
gameplay method invokes turn hooks, resets moved flags, and clears every
player's completion flag; none of those actions belong to restoring a turn
already in progress. The turn number is available to the setup `start`
callback and subsequent `get_turn`/`get_year` calls.

Turn IDs must be nonnegative, fit native `size_t`, and permit `turn + 2100` in
both `size_t` and GSE's signed 64-bit integer. Zero is accepted explicitly.
Negative, overflowing, missing, mistyped and truncated fields fail before
changing turn metadata or staging successful completion. This is a GLSMAC
ingestion policy, not a claim about the original game's malformed saves.
No wire fields or version numbers change; the V2 header's existing rejection
of older formats still applies.

Successful `InitComplete` publishes one native turn-number request and a
status after `start`, deferred unit/base registration, and installation of the
saved RNG continuation. Readiness is computed from registered local units.
The local player's existing completion flag takes precedence and selects
`waiting_for_players`; otherwise status is `active` or `turn_complete`.
The readiness cache is initialized at the same time, preventing the next
ordinary check from replacing this status merely because restoration began
with an empty unit registry. Ordinary initialization without a downloaded
turn does not emit these restore notifications.

`get_turn_status()` returns the last published status without invoking a
callback. The production minimap subscribes to future status events and then
reads this value, because its UI is created after backend initialization and
can miss the earlier status notification. Subsequent normal turn events and
status changes retain their existing presentation behavior. No gameplay turn
event is replayed to initialize the UI.

Reset, cancellation, setup failure and a new download discard pending turn
publication alongside pending RNG state. The map/managers are decoded before
the final turn field; their existing terminal, partially reconstructed failure
contract remains. Setup callbacks may also have effects before throwing.
This does not make world loading transactional or safely retryable after a
body/setup failure.

## Scope and remaining state

Existing destination Player completion flags are preserved. They are **not
serialized by this change**: the current Player/world formats still omit
them, so a fresh joining Player starts with its existing default. Persisting
completion at the same snapshot boundary needs a separate versioned turn
state block. Canonical Player faction references and lobby packet ordering
are also separate work.

This is downloaded continuation metadata and presentation, not full save/load,
an arbitrary checkpoint, pending-event/checksum persistence, or proof of
future host/client lockstep. Simulation hooks, unit movement/health/moved
state, base nutrients and population are not reset during restoration.
Ordinary `Game::AdvanceTurn` behavior is unchanged.

## Regression coverage

Native tests exercise the exact production world writer/body reader and real
`InitComplete` with canonical unit/base/pop reconstruction. They cover movable
units, exhausted units and an already-completed local Client player, metadata
visibility during `start`, publication after registration, preserved player
flags and simulation fields, and one-time publication. The real minimap
script runs against the real wrapped Game using lightweight DOM/sound/timer
factories; no renderer or original assets are loaded. A positive control calls
ordinary `AdvanceTurn` with the production unit-turn script and checks that
normal gameplay hooks, movement reset, flag clearing and UI updates still run.

Boundary tests cover zero, the largest representable mission year, invalid
signed IDs, wrong field type and truncation. Existing real reset/cancel,
native/GSE setup failure and replacement-download cases also assert pending
turn discard and absence of restored-turn publication. Ordinary initialization
is checked separately. This does not claim a rendered multiplayer join test.

```sh
./build/bin/GLSMAC --gse-tests --prefix build/test-data-world-turn
```

On integration with the separately reviewed tracked native-test support (#57),
add `--gse-tests-native-filter 'world turn restore|world RNG' --memorydebug`
to run just these cases with allocation tracking.

Local Clang 15 / Ubuntu WSL validation passed all eight CTest entries (38.48
seconds), all ten focused turn/RNG cases with allocation tracking, and explicit
correctness analysis of Game.cpp. The original production reader failed all
three new continuation cases because it invoked gameplay turn hooks. A second
negative control ran the fixed binary against copied scripts with only the
new minimap initialization removed: the active case passed, while readiness
and waiting-for-players presentation failed. Neither negative control changed
the final sources. Independent read-only review found no blocking issue.

The focused publication branch on #71 separately passed its five Linux CTests
in 23.35 seconds (full GSE 17.05 seconds). Its older ancestry does not include
all tracked-test support or ownership repairs. The separate Exception::what
repair (#72) remains required for leak-enabled testing of initialization errors.

At integration `519a0bd7`, Linux ASan/UBSan/LSan passed all eight CTests in
44.81 seconds with leak detection enabled. The first Windows run timed out at
300 seconds in the pre-existing queued invalid-download test (7/8 overall).
That case passed standalone, and a full retry passed 8/8 in 60.90 seconds.
The timeout remains an unresolved intermittent failure; the retry does not
establish its cause or repair. Logs are retained under `out/turn-windows-`
and `out/turn-windows-retry-` in the integration checkout.
