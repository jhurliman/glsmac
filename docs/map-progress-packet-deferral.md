# Defer connection events until map processing returns

`Map::ProcessTiles` no longer calls `State::Iterate` every 64 tiles. A full
roster update or disconnect can replace or destroy slots and players, so
applying those events from inside a map-generation pass invalidates callers'
live simulation references. Connection events remain queued for ordinary
State/Game iteration after synchronous map processing returns.

Loader text, frontend progress polling, and ping requests still use the
restricted `Game::SetLoaderText` pump described in
[loader-progress-requests.md](loader-progress-requests.md). `MT_Reset` still
sets the atomic initialization-cancellation flag immediately; the map loop
checks that flag after each module, and native reset remains deferred until
the executing map operation returns.

## Transport and responsiveness

In the normal threaded engine, Network has its own worker
(`src/engine/Engine.cpp`). `SimpleTCP::Iterate` reads sockets and performs
heartbeat handling. Its `ReadFromSocket` consumes `PT_PING` and `PT_PONG`
without escalating them to the game's connection event queue. The 12-second
ping and 18-second disconnect thresholds therefore do not require
`State::Iterate` during map generation. The current Connection/Client/Server
code has no separate application-level response or download deadline.

Chat, reconnect/roster updates, disconnect notifications, and buffered
gameplay-event sends wait for the synchronous map operation. Transport event
queues can grow during that delay. In `--singlethread` mode, map work already
blocks the Network module itself; the removed State pump did not run Network
or service its socket heartbeats. This change does not make that mode
responsive during long synchronous loads.

## Regression coverage

Native tests use the real Map module loop with 65 tile visits and a
fixture-owned Network. The normal Network MT request/response queue,
Connection event-batch handling, and Client packet decoder all run; only
socket I/O is substituted. Protected constructor overloads provide that
transport without replacing the engine's global network pointer. Ordinary
constructors retain the engine's network service.

The tests queue a full `PT_PLAYERS` replacement and an `ET_DISCONNECT` in
separate cases, prove neither applies during the map loop, then prove
ordinary State iteration applies each afterward. They also verify progress
polling and ping remain available. The existing actual-map cancellation
regression verifies reset cancellation stays signaled until the outer map
operation returns, followed by exactly one native reset.

The old nested pump fails both packet-deferral cases. A separate preparatory
repair pairs `Slot::Deserialize`'s Player allocation with State's tracked
deletion; its direct Client roster-decode/State-reset regression fails the
original allocation under `--memorydebug`. These fixtures use null factions
to avoid claiming a repair for detached faction ownership.

This is a reentrancy boundary, not general thread exclusion. It does not
serialize owner-thread world reconstruction against connection batches or
native reset, protect the initial download callback, or resolve the separate
snapshot completion handoff races.
