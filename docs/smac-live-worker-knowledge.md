# Explicit worker knowledge in a live world

`Game::PublishSmacWorkerKnowledge` admits an explicitly supplied owned
`SmacWorkerKnowledge` into the current Map. `GetSmacWorkerAvailability` then
queries a registered Base using actual manager records. This is a controlled
native integration surface. Generated games do not opt in automatically, and
there is no script setter or default exploration/territory policy.

## Authority and binding

The [knowledge value](smac-worker-knowledge.md) owns explored bits, raw claims,
revelation, optional directed observer rows and ordered unit observations keyed
by positive port Unit IDs. Current terrain supplies sea flags. BaseManager's
ascending stable ID order supplies the derived Base list; each Base's checked
allocation supplies only the projected low21 mask. Complete selected32 and all
sixteen specialist slots remain in the [Base authority](smac-live-base-allocation.md).
Neither frame list indices nor transport slot numbers become simulation IDs.

The adapter authenticates the current frozen SMAC simulation, every available
playable observer, each Base's owner/context/allocation and every Unit's actual
conventional identity. It checks manager-to-tile membership and tile-to-manager
membership, including stray centers, duplicated stack entries and displaced
objects. All saved observation IDs must join exactly once to the live units.
Knowledge observation order is retained even when registry order differs.

The admitted map uses the current periodic-X topology. Nonwrapping knowledge,
different dimensions, legacy units, native owner0 live units, active convoy
orders, missing canonical Base authority and unavailable requested observer
rows reject. The independent value still preserves native bits and the
distinction between no claimant and native claimant0. Its broader detached
domain is not silently adopted by the narrower live adapter.

Publication requires the owning accumulating GSE space, writable scope and a
running world. It validates all supplied playable observer projections, copies
the replacement and swaps it into Map without callbacks, IDs or RNG draws.
Read-only queries rebuild the consumer frame from current terrain and entity
authority. They do not recompute exploration, territory, diplomacy or visibility.
Base allocation replacement can change the derived selections without copying
those selections into knowledge.

## Save and restoration

World21 adds an optional bounded knowledge frame after the existing resource
map policy and before the map payload. Global11, Simulation7, BaseManager7 and
Unit2 retain their respective formats. Previous world versions reject rather
than infer absent knowledge. An absent World21 field preserves ordinary worlds.

The nested frame is bounded before copying, and its geometry is compared with
the map payload before Map allocation. After tile decode, Map owns the supplied
knowledge but marks it unready. The ordinary `start` callback still precedes
pending entity registration and cannot query this unbound state. Only after
pending simulation publication and both UnitManager/BaseManager registration
passes does the adapter validate the complete live join and mark it ready.
Saving unready knowledge rejects. Restoration does not replay movement sight,
founding, territory or allocation calculations.

Unit restoration uses its existing explicit no-placement-history decode path.
Base restoration uses a private constructor tag selected only by its decoder.
These do not create a blanket exemption for live commands during initialization.

## Temporary mutation boundary

Sight and territory update producers are not integrated yet. An explicitly
admitted knowledge world therefore rejects scripted gameplay events, turn
completion/advancement, new Unit/Base placement, movement, spawn/despawn and
the native map replacement/clear/resize/edge-edit entry points before their
effects. The ordinary absence path is unchanged. The snapshot is useful for
live queries, allocation publication and exact restoration; it is not a claim
that a playable canonical campaign has been enabled.

Tile fields remain publicly writable C++ fields in this codebase. The guarded
entry points cover current commands, not arbitrary trusted native assignments.
New terrain or ownership producers must update knowledge atomically or preserve
the rejection boundary. Read-time identity checks cannot reconstruct a missed
visibility transition after a unit moves away and returns.

The next producer contracts are ordinary Scout/Rover sight and visibility
lifetime, then first/overlapping Base territory claims. Native population
allocation also needs authenticated support, prepared intake, production and
priority inputs before the complete detached allocator becomes a live caller.
The detached support calculator is available, but current live conventional
records still explicitly carry home=None; that is not a general support model.

## Live integration checks

Ten native groups use the existing asset-free backend fixture with an explicitly
supplied 8x8 terrain, canonical participants, two actual Bases and two conventional
Units. Sparse Base IDs place the evaluating Base second in registry order;
knowledge observations reverse Unit registry order. Literal mask assertions
distinguish live sea, foreign worked squares, visibility and directed recognition.
Raw native claimant0 stays distinct from absence. A later actual Base allocation
publication changes availability while the installed knowledge bytes stay fixed.

Rejected replacement and deliberately broken forward/reverse memberships preserve
the installed knowledge, complete same-instance snapshot, Base/Unit records,
frontend queue size, ID allocators and both RNG states. The authority admission
checks also reject a real legacy Unit and a Base lacking full allocation. The
twenty-one guarded native/GSE calls require the specific knowledge-fence error,
including empty-name Event, CompleteTurn, AdvanceTurn and FixTopBottomRows.
A real handler queued before publication cannot run Validate/Resolve/Apply; the
absence control runs all three phases and advances JSF, making the fence observable.

Two actual world-body restores preserve exact knowledge and individual entity
bytes, full selected32/inactive16 authority and availability with either absent or
different current simulation. A separate actual InitComplete check sees saved
knowledge present during `start` while both entity registries are still pending:
queries and new Unit/Base construction reject, then the same query succeeds after
both managers register. The fixture invokes this production checkpoint synchronously
under its owning accumulation; it does not claim network delivery, rendered startup
or the asynchronous startup scheduler. Restored world definition-table ordering is
not asserted byte-identical. Complete same-instance bytes are checked after queries
and rejected mutations.

World-prefix tests cover a valid framed value plus following body, exact-budget
nested rejection versus cap+1/large complete outer-frame rejection at the unchanged
payload cursor, non-SMAC presence before payload consumption, atomic rejected writer
and map-dimension preflight. These checks complement the independent knowledge codec
tests; they do not claim rollback of an arbitrary malformed later world body.

The final ten groups passed with memory tracking in normal Debug (1.042s) and
ASan/UBSan/LSan with leak detection (1.107s). Three copied production controls fail
the intended assertions when restore readiness is bypassed, the captured-event
fence is removed, or live sea is replaced by constant land. No source checkout was
mutated for those controls. An earlier test-only InitComplete adapter retained its
setup simulation and correctly hit the existing replacement guard; its failed log
is retained separately, and the final adapter follows the real pending-state
publication boundary. This focused receipt does not claim a new full platform run.
