# Occupied Base constructor and physical stacks

Seven new original `5d8520` factory calls return normally to the harness sentinel, including two followed by complete `5d89f0` deletion returns. The [checked cases](occupied-base-constructor-cases.json) bind gameplay write journals, actual sight, original startup stack links and the existing successful restart receipt. This extends [native birth sight](native-birth-sight.md) and [global wildlife closure](global-wildlife-closure.md); it does not add an ecological spawn-selection rule.

Direct constructor admission accepts design 8 Worm or design 9 Isle, owner zero, at occupied Hive Base `(34,36)`. It also accepts a Worm at occupied human Gaians Base `(17,61)`, an own Hive Scout, and a foreign University Scout at the Hive Base. No factory-level hostile-Base, mixed-owner, or land/sea-class rejection occurs on these calls. Construction causes no combat, capture, Base-owner change or casualty. Higher-level producers retain their own candidate guards.

## Ordered writes

1. Allocate the current dense Unit count, increment counters, initialize the record and determine home through the normal factory.
2. `5d81d0` finds the physical head via `5d7fe0` without filtering owner or cargo. Prepend the newborn: new next = old head, new previous = −1, old head previous = new index. Existing successor order stays intact.
3. If no old head exists, placement changes affiliation only when the tile is not a Base (`items&1==0`). Existing Base affiliation is preserved. Unit-presence and applicable site-suppression mutations are separate typed operations; do not replace the entire item word.
4. Run full sight `5cdaa0` after insertion. Existing occupants participate. Publish exploration, observation, remembered-map and relevant flag effects together with the newborn and links.

The retained Hive stack is Pod 8→Scout 1; each factory produces 13→8→1. The human Base is Pod 7→Scout 0 and becomes 13→7→0. Raw Unit offsets 48/50 are next/previous. Traverse next from previous=−1; do not sort occupants or start from selection.

For native births, center plus eight adjacent tiles gain native exploration bit zero. On the Hive square, all three occupants' stored observation bytes become four; on the Gaians square, all three become two. These exact results reflect the actual positive observer and Base-center sight branch. They do not authorize copying the old head's visibility or always ORing the newborn owner's bit into every record. The own Hive Scout adds no observation changes. The foreign University Scout also causes sight effects; its positive-faction remembered-map changes are journal-bound, not independently generalized here.

Base-center affiliation, claim and live item word remain unchanged in all seven controls; Base records remain byte-identical. Exploration changes despite those preserved fields. No RNG draws occur. Initialized native assignment references remain −1/−1; arbitrary loaded nonnegative references remain a separate tail branch.

## Casualty cleanup

Deleting newborn 13 returns the Hive stack to 8→1. Observation bytes four and native exploration remain; deletion does not rewind sight. Deleting existing Scout 1 after birth shifts later dense records left: old Pod 8 becomes 7, old Worm 13 becomes 12, giving stack 12→7. Owners and all surviving non-link Unit bytes are preserved. Base and map state remain as after birth. These full deletion returns extend the earlier bounded [dense deletion](unit-dense-identity.md) core controls.

## Fresh world and restart producers

The original retained fresh-world frame has linked orders Gaians `[7,0]`, Hive `[8,1]`, University `[9,2]`, Morgan `[3]`, Sparta `[10,4]`, Believers `[11,5]`, Peacekeepers `[12,6]`. Source `5c9f4e` constructs the selected Scout, clears home, and `5c9f65` moves it to the tail. Subsequent package Pods prepend normally. These reuse actual startup state; no startup is rerun here.

The existing admitted nonzero-turn restart constructs initial Pod C1, Former C2, second Pod C3 and further grants. `5c9ef9` moves the initial Former to the tail; later factories prepend; `5ca41d` promotes the saved second Pod after all grants. Final order is `[C3] + reverse(C4..Cn) + [C1,C2]`. The reused native four-creation receipt has IDs `[2,3,4,5]` and actual traversal `[4,5,2,3]`. This assumes an initially empty square, successful allocations and no intervening mutations. A later ordinary factory prepends without repeating initializer promotion.

The checker pins original instruction tuples for insertion, the Base-affiliation guard, sight marking, tail/head helpers and startup/restart callers. It replays gameplay writes, not host allocator scratch. No graphical acknowledgments or skipped redraws were reached by these seven calls. They return to a harness sentinel, not an outer global ecology caller. Broader ability/diplomacy/visibility combinations retain their existing bounded contracts.

## GLSMAC integration

The shared native factory now accepts canonical Base centers while preserving
separate occupancy affiliation. Positive remembered improvements retain Base
presence. Native sight marks every existing occupant after insertion. The port
control uses admitted Gaians owner1 and retained Pod1-to-Scout0 links, translating
the constructor stack discriminator rather than claiming original whole-world
identity. Both Worm and Isle are checked on occupied and empty Base centers.
Injected failure and discard publish nothing; commit preserves Base bytes and
RNG. Full world restoration precedes newborn or existing Scout deletion, checking
dense reciprocal link repair, surviving observation and retained exploration.
This admits the shared factory; global spawn selection and turn activation remain
separate callers. Fresh/restart source ordering is documented above but is not
automatically inferred for ambiguous legacy saves.
