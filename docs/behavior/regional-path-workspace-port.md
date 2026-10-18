# Regional path workspace in GLSMAC

The [stock SMAC reference](regional-path-workspace-lifetime.md) establishes a
transient, partial distance field shared across regional queries. This port
implements the ordinary abstract land profile and its Map-owned lifetime.
It does not establish SMAX parity or ordinary Unit pathfinding.

`RegionalLandWorkspace` compares destination, option combination, peer and the
two prototypes' **current** plans. It deliberately has no terrain, territory,
diplomacy, human-membership, source-owner or turn generation counter. A reached
source returns the retained result. A matching key with a zero source distance
restarts the queue without clearing its footprint; a changed key clears it.
The FIFO search stops expanding the requested source and processes remaining
entries under its best-distance cutoff, preserving the original partial zeros.

The Base-pair overload retains the existing record-order, regional-count and
component gates, then uses this workspace with abstract prototype 0. Its caller
must supply current plan authority; this patch supplies no invented design plan.

Each Map lazily owns one workspace. Direct summary, ordinary Base events and
world edits leave it alone. The real canonical owner tail invalidates its
destination after `RefreshCanonicalPopulationAggregate`, preserving distance
bytes. New/restored maps begin empty; generation and raw map replacement reset
the allocation. No workspace bytes enter either save format or gameplay RNG.

## Validation

- Seventeen literal original query sequences and six complete mixed-map fields,
  including all 1,024 baseline entries, current-plan edits and the matching-key
  zero-source counterexample. The fixture records original and public JSON hashes.
- Retained Base-pair behavior, explicit invalidation, count gates and rejected
  missing prototype authority or mismatched geometry.
- Actual founding/removal and summary preserve a warm route; real queued
  `complete_turn` consumes it at owner tail. Actual save/Start and raw map import
  allocate fresh state. Warming a route leaves saved bytes and RNG unchanged.
- Existing 386 fresh queries, 66 original Base-caller projections and live
  territory/Base-stage checks continue to pass.
- Correctness analysis of six production consumers; focused ASan/UBSan on
  Map, territory rules and canonical turn, with current normal objects elsewhere
  and leak detection disabled. Six affected native groups passed under that
  instrumentation. Later test-only Base-pair/bounds assertions passed normally.

The live fixture supplies citizen presentation definitions and starts without
ranking history because its synthetic rules contain negative score inputs.
Production performs the actual transition and aggregate publication.

## Still to connect

The complete regional strategy/research summary consumer remains separate work.
Its calls must use the retained overload rather than the fresh helper. The
Unit-owner wrapper invalidation, weighted routing, first-step direction ties,
sea/air paths and other AI consumers are not implemented by this patch.
