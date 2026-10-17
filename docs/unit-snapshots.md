# Unit snapshot ownership and startup

UnitManager snapshots use the typed string marker `GLSMAC_UNITS_V1`. Old
unversioned records and unknown markers are rejected before publishing state.
This changes the development world-download format: mixed-version peers cannot
exchange these unit snapshots. It does not establish a stable disk-save format
or add protocol negotiation.

The manager owns morale sets and unit definitions. Each unit record stores a
definition ID and resolves it in the destination manager; each definition resolves
its morale-set ID there too. Units borrow these objects. Removing a definition
while a registered or pending unit uses it, or a morale set while a definition
uses it, fails without unregistering it. Unused entries can still be removed in
reverse order. Teardown deletes units before definitions before morale sets and
does not dereference the map, which can already have been destroyed.

Snapshots retain unit ID, definition ID, owner slot, tile coordinates, movement,
morale, health, moved-this-turn, and the next ID. Decoding checks the complete
individual unit record before claiming a tile or advancing IDs. It rejects
missing definitions, duplicate/invalid IDs, invalid slot indices, out-of-map or
wrong-parity coordinates, morale outside 0–6, and nonfinite movement/health.
All finite signed movement and health values remain representable, including
negative and zero values: current setters do not clamp them, and combat can
leave health below zero until its later death callback. Definition movement
must also be finite, and enum tags are checked before narrowing. Sprite numeric
fields retain their existing decoder; this is not full rendering-metadata
validation.

Restoring a unit registers it and queues the usual frontend spawn message, but
does not invoke `unit_spawn` or consume gameplay randomness. This applies both
to running games and deferred startup. New units retain the existing master-only
spawn hook. Deferred startup transfers ownership into the registered map before
invoking a hook, so a throwing hook cannot leave the same unit owned twice;
untouched entries stay pending. Registration and any callback effects already
completed are not rolled back.

Serialization requires no pending units and rejects before writing if startup
registration is unfinished. Deserialization requires an empty manager. Header
and first count rejection leave it untouched; later malformed records may leave
earlier definitions or units registered, queued, or pending, and may advance the
ID allocator. Such a world load is terminal and must be torn down, not retried
as if the manager were empty. Whole-manager transactional restore, arbitrary
allocation-failure recovery, movement animation state, and the existing
despawn-event rollback defect are separate work.

Native tests use real source/destination Game, State, UnitManager, map, slots,
GSE event dispatch, and RNG objects, with no assets or renderer. They exercise
immediate/deferred restore as master/client, frontend messages, ordinary spawn
hooks, malformed records, signed float boundaries, registry ownership, map-first
teardown, and cleanup/resumption after a deferred hook throws. They do not claim
a playable conventional unit, full rollback fidelity, or interactive rendering.
