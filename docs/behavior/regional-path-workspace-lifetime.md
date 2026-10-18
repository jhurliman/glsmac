# Retained regional path workspace

RE-SMAC-REGIONAL-PATH-WORKSPACE-001, 2026-10-18. Pinned stock SMAC.
[Literal sequences and distance fields](regional-path-workspace-lifetime-cases.json).
This extends the [fresh-workspace graph contract](abstract-regional-land-reachability.md).

**The owner-summary call does not guarantee a fresh workspace. Always-fresh
queries change measured behavior.** Complete native summaries can reuse a
previously successful route after the intervening terrain is disconnected.
The full Base-to-Base caller returns true from the retained workspace while
the same query with an invalidated workspace returns false. Directed-relation,
human-mask and territorial-owner changes have equivalent controlled examples.

## Cache guard and distance field

For this ordinary abstract land profile, the native query can reuse its
retained field when all of these match:

- Requested destination coordinates.
- Exact path-option combination and peer owner B.
- The current plan of the requested prototype and the current plan of the
  cached prototype ID.
- The field already has a nonzero distance at the requested source coordinate.

Source coordinates and owner A are not key fields. Terrain, territorial owner,
directed relation words, human mask and current turn have no generation key.
The plan comparison reads both current definitions; it is not a saved plan
snapshot. Mutating plan 8→9 on prototype 0 still matches when both IDs are 0.
A component change on that same prototype also leaves this guard satisfied.

A reached source can therefore reuse the field even when A changes. A changed
destination, peer B or option combination clears and recomputes the field.
With a matching key but a zero distance at the new source, the query instead
restarts the queue at the destination **without clearing retained distances**.
Already-populated neighbors prevent fresh exploration in the measured control:
source (0,0) returns unreachable, preserves the partial field, and becomes
reachable when the same request is explicitly invalidated first. These are
different paths; an unreached source is not an automatic fresh-workspace miss.

The field is more than a cached boolean. On a fresh abstract query:

1. Clear the field, seed the destination with distance 1, and queue it.
2. Pop in FIFO order, using the established eight-neighbor order and the
   [abstract terrain/territory admission](abstract-regional-land-reachability.md).
3. Skip expansion when the popped distance exceeds the best source distance
   found so far.
4. On popping the requested source, record its distance and skip expanding
   that node itself. Continue processing the remaining queue under step 3.
5. Store each newly admitted tile at current distance +1 and enqueue it.

This leaves a partial distance footprint, including some cells beyond the
source's distance. An independent FIFO model matches all 1,024 baseline entries
and six additional mixed-map fields. Preserve those zeros: completing the
entire connected component would change later reuse decisions. The normal
distinct-Base abstract result also uses already-populated neighboring distances;
the retained hit does not revalidate the intervening terrain or diplomacy.
First-step direction tie-breaking and other weighted options remain outside
this boolean consumer's contract.

## Actual lifecycle boundaries

| Path | Measured workspace effect |
| --- | --- |
| Owner-summary entry and Base-pair helper | No automatic invalidation; a warm matching route is reused |
| Ordinary owner upkeep | Its explicit invalidation occurs **after** that owner's summary, setting both cached destination coordinates invalid; it retains the distance bytes until the next recomputation |
| Direct manual Scout movement | Preserves key and distance bytes in the admitted ordinary move |
| Colony factory and native Base founding | Preserve key and distance bytes |
| Native nonlast Base removal | Preserves key and distance bytes |
| Actual survivor capture, through outer movement return | Preserves key and distance bytes; Base-window virtual show is supplied presentation, while native recalculation and return execute |
| Completed Farm generic terrain stage | Preserves key and distance bytes; earlier full command legality/later special completions are separate |
| Directed relation primitive | Preserves key and distance bytes |
| Unit path-request wrapper changes its last requesting Unit owner | Invalidates both cached destination coordinates before proceeding; the same owner leaves them alone at that gate |
| Ordinary file load | Allocates/reinitializes the workspace and invalidates its destination; it does not restore a saved warm route |

Founding/removal/capture can change which actual Base pair the caller considers,
and those changed coordinates/owners can subsequently miss the guard. That is
different from clearing the workspace merely because a Base event occurred.
Likewise, a different ordinary movement path query can replace this shared
field through its own key/options; direct manual movement need not query it.

Source inspection additionally identifies workspace allocation/reinitialization
at whole-map/session setup, map replacement and ordinary-load paths. This is
not a claim that every terrain edit performs such a rebuild. Do not introduce
a blanket terrain/diplomacy/Base generation counter as original parity.

## Evidence and implementation boundary

The receipt includes 18 retained-workspace sequences, a real owner-upkeep
prefix through the post-summary invalidation, three Unit-owner guard prefixes,
six mixed-map distance-footprint controls and seven live-operation controls.
The capture control supplies only the previously unconstructed window's show
method in addition to inherited runtime/presentation services. The ordinary
load observation reuses this follow-up's actual file round trip.

The full owner-summary counterexample uses real owned Base records and native
Base, Unit and route calculation; the topology change is a declared field
control, not an organically executed sea-raising action. The operation canaries
test retirement separately from route legality. Nonlast removal and survivor
capture do not certify every last-Base restart, AI, sea/air path, project or
network branch. They do close the named current human land-world boundaries.
