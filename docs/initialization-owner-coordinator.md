# Initialization owner checkpoints

An accepted `MT_Init` now returns its terminal admission response before running
script hooks. Its ticket continues to identify map polling, cancellation and any
later initialization error. Native preparation and owner work advance through
explicit checkpoints; `GS_RUNNING` alone does not release the backend while the
owner is still finishing startup.

| Checkpoint | Executor and work |
| --- | --- |
| Admission | Backend: validate the exact configured session, perform any prior-world reset/rebind, return the accepted ticket and local slot. |
| Initialize/configure | Owner: `initialize`, create all five managers, then `configure`. |
| Bindings | Backend: revalidate the current session, connection and local roster; assign factions and install connection handlers. A server enters `GS_INITIALIZING` here. |
| Configure events | Owner: dispatch one real event batch captured before publication. Existing event validation, caller identity, authority and broadcast rules apply. Late or generated events stay queued. |
| Map acquisition | Backend: generate/load the host map or request the client download. Existing loader progress permits only its narrow progress operations. |
| Download ingress | Owner: consume captured packet/message/download payloads in arrival order after the complete native connection batch. Validate the exact admission, session and connection before decoding. |
| Map initialization | Backend: initialize map presentation and prepare the owning frontend response. |
| Start | Owner: complete `create_world`, `start`, deferred manager registration, saved turn/player/RNG restoration or ordinary FirstTurn, event dispatch and status publication before releasing the backend. |
| Failure | Owner: retain the original error on the exact ticket, notify and clean up through `InitFailed`, including failures before a map exists. |

`Game::Iterate` finishes its entire MT response batch and `State::Iterate` packet
batch before publishing an owner operation. It returns after publication even
when owner execution happens immediately on the calling thread. This prevents a
single-thread execution from falling through into another native stage.

## Claim, cancellation and ownership

Each owner operation belongs to an existing owned admission record and moves
through Pending, Running and a terminal phase. The admission mutex serializes
Pending-to-Running claims with reset intent. No script, Space queue registration,
transport wait or native teardown runs while that mutex is held.

While Pending or Running, ordinary backend iteration and the loader pump dispatch
no native requests, including Ping and frontend queue polling. A Pending reset
makes the operation unclaimable, so the backend can complete reset without asking
the owner to drain its queue. A Running reset records intent and stays queued;
the exact session remains active through the complete owner operation. Its RAII
exit retires the canceled binding and releases native execution. A stale cleanup
only clears its own operation, never a replacement's Pending operation.

The engine's main-thread loop unwinds its current owner work before requesting
worker shutdown. Backend Stop cancels a remaining Pending operation and checks
that no Running owner remains. The separate single-thread Engine stop-order and
polled public-reset prerequisites preserve runtime lifetime and avoid owner waits.
This change does not add a wait inside an MT request.

Published operations root their State, captured connection and payload owners.
A separate root handoff covers publication through Space queue registration,
including cancellation in between. Configure events retain the existing real
event-batch handoff. Packet bytes and native messages are owned values, not views
into a temporary network response. A failed download stops later captured ingress
and reaches a separate Failure checkpoint; it does not fall through into Start.

Startup ingress covers Client game events/responses, Server game events, game
messages and final download completion. Nested work on the exact Running owner
operation executes inline. Lobby packets without a startup admission keep their
existing session-bound path: this coordinator does not grant client definitions
host authority or discard lobby game-setting/chat events.

## Validation boundary

The asset-free native tests exercise actual MT request extraction and response
retirement, State/Client GETEVENTS batches, real event validation/application,
actual owner queueing, collection with captured roots, and the full Start helper.
They cover inline and queued owner execution, cancellation in Pending and Running,
an old finalizer preceding replacement startup, client caller identity, a captured
configure batch with late/generated events, and a roster packet following a final
download chunk. Start probes attempt actual native iteration during hooks,
FirstTurn/event application and status publication, then verify the saved turn/RNG
continuation or normal FirstTurn and delayed reset completion.

Snapshot codec fixtures that directly construct a world still test the real
pre-map validation helper explicitly. Those fixtures are not claims that an
unadmitted world can enter production initialization. Map generation, asset-bearing
map presentation construction and rendered multiplayer startup need separate
installed-asset integration checks.

The coordinator does not introduce native collector exclusion, transactional
world-body restoration, general lobby/console callback synchronization, or a
rendered map-reload guarantee. In particular, native Map/Tile and retained old-map
destruction still require the separately audited collector-retirement change.

Local validation uses Clang/LLVM 15 on WSL. Sanitizer checks enable ASan, UBSan and
leak detection; the isolated sanitizer link uses the existing local `-no-pie`
workaround for the independently diagnosed pre-main ASan loader failure. No
repository toolchain workaround or suppression is introduced.

The 15 focused coordinator cases pass with allocation tracking and with
ASan/UBSan/LSan. The complete normal CTest suite passes 8/8 (49.45 seconds),
and the complete sanitizer CTest suite passes 8/8 (44.61 seconds).
The full correctness policy analyzed 297 production translation units with
LLVM 15 at four jobs in 116.10 seconds, with zero failures; other builds were
running concurrently, so this is a local measurement rather than a CI promise.

Four copied-production negative controls each fail their real regression:
removing native parking, draining configure events without the captured batch,
executing startup ingress inline, and letting an old finalizer clear the current
operation. The first stale-finalizer control exposed a test-order weakness:
Space does not promise iteration order across owner keys. The final test invokes
the real old cleanup while the replacement is still Pending, then also drains
the actual queued cleanup. No production source was mutated for these controls.

## Focused publication ancestry and checks

The publication combines the reviewed startup prerequisite chain at `ccd8561a`,
snapshot restoration at `6805e3a3`, and map packet deferral at `186ab971`, then
applies coordinator source `1676caf6`. These are joint dependencies: the saved
turn/RNG continuation and the complete native packet-batch boundary are exercised
by this change. The publication retains both prerequisite implementations rather
than replacing files from the larger integration branch.

On this narrower parent, Clang 15.0.7 full CTest passes 6/6 in 24.35 seconds,
including the complete GSE suite (17.04 seconds). The opt-in real Engine shutdown
harness passes both default and single-thread modes with allocation tracking.
The earlier 8-test sanitizer and analysis measurements above refer to the
original integration source, not this reduced publication checkout.

The first publication run exposed a test-only stack Event whose early destruction
relied on a later collector repair absent from this parent. The final fixture uses
a collector-owned Event, matching production ownership, and the captured failing
GDB trace is retained with the local publication evidence. No collector change or
additional unrelated prerequisite was imported to make that test pass.