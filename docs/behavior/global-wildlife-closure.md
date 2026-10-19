# Global wildlife: complete reachable stock producer

This closes the reachable wildlife policy in original `terran.exe` routine `0x537790`, after its forest/kelp phase and through its original controller return `0x53a820`. Use the [independent producer](global_wildlife_reference.py) and [30 native controls](global-wildlife-closure-cases.json), together with the [earlier nine flora/global controls](global-ecology-wildlife.md). The executable SHA256 is `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.

The reference translates policy and emits ordered helper actions. An engine integration must execute its canonical Unit constructor, capacity, sentry and stack helpers at those points, preserving their state changes and any context-dependent RNG. The reference's stock newborn profile has stored morale 1 and inactive native AI references; it is not a replacement for the general constructor. Whole-state native journals verify the constructor effects in these controls; the policy model does not independently synthesize every constructor byte.

## Ordering and gates

Run forest/kelp spread first, then wildlife using its resulting live map and CRT state. `produce_global` expresses that composition. After flora, skip all fauna if primary rules contain `0x200000`, Voice marker `0x95a098` is not -1, or live global Unit count is at least 1792. This is an entry gate: 1791 becomes 1794 in a native control. Do not recheck 1792 before each birth. The constructor's separate hard limit is 2048; stock maximum births from this producer cannot reach it from an admitted entry.

Let T be the turn, L native-life category 0..2, and D difficulty 0..5. The early-strength flag is `T < 2*(10*(3-L)-D)`. Sampling uses CRT `s = 214013*s+2531011 mod 2^32`, result `(s>>16)&32767`; draw X then Y, reduce modulo width/height, adjust X to Y parity. Each attempt consumes two draws, even if rejected. The distinct shared RNG does not change in these controls.

## Periodic sea brood

Run the sea search when `T % (2*(5-L)) == 0`: periods 10, 8 and 6 for L=0,1,2. Consume up to 1000 coordinate pairs. Increment the trial count before eligibility: pair 999 can succeed, but pair 1000 is consumed and rejected without eligibility testing. If none succeeds, proceed to land sampling.

Eligibility is ordered:

1. Fungus bit `0x20` with elevation band at least 2 accepts immediately. It bypasses land/water, occupancy, region and area checks.
2. Otherwise require water (band below 3), reject Base/Unit marker bits `0x3` with ordinary affiliation 0..7, require low-six-bit region neither 0 nor 63, and full region area at least 50.

The fungus shortcut really accepts land and occupied tiles. Native controls establish area 49 rejection / 50 acceptance using terrain and regions reconstructed by original preparation helpers. An occupied ordinary water tile is skipped; occupied fungus in a 49-tile sea is accepted.

Compute requested passengers:

`raw = floor(k*T / (4*(4*(8-L)-D))) + L + 1`, where k=2 if secondary rules contain `0x100000`, otherwise 1. The early flag forces requested=1; otherwise requested=raw.

Execute these helpers in order:

1. Create design 9 Isle, owner 0, at the accepted coordinate (`0x5d8520`).
2. Query its actual capacity (`0x5d9730`), clamp passengers to the smaller of requested and capacity.
3. For each passenger, create design 8 Mind Worm at that same coordinate, then call sentry helper `0x5d82f0`: order 1, first waypoint X/payload -1, first waypoint Y 0. This is not an assigned carrier-ID cargo order.
4. Promote the Isle to stack head (`0x5d83b0`).
5. If either global technology ownership mask for tech 63 or 25 is nonzero, create design 10 Locust at the same coordinate after promotion. Its constructor can change the head again.

The stock newborn Isle has stored morale 1 and capacity 2 through the predefined Psi capacity helper. Reuse the helper, rather than hardcoding 2 for other birth contexts. Native turn56 controls distinguish raw request 2 versus doubled 3, both clamped to 2; turn400 requests 6 and still clamps to 2.

## Mixed-owner Scout discriminator

At T=8 with CRT seed 1, the first sea-search sample is land (47,67). Supplying fungus and creating a real owner-2 Scout 13 there leads to successful original births of Isle 14 and Worm 15. The final linked stack is **14 → 15 → 13**, with head previous=-1 and tail next=-1. The Scout remains owner 2/design 2, keeps its coordinates and damage, and the tile affiliation remains 2. Thus a publication rule rejecting positive-owner co-location would reject a reachable original result.

Preserve constructor side effects: Scout byte 39 changes 0→4, written at `0x5ce613` once for each new Unit (the second write retains 4); its previous link changes -1→15. No other Scout bytes change. The tile exploration byte changes `0x04→0x05`. This is a live mixed-owner stack, without combat resolution or Scout removal. The fixture records these differences explicitly and the checker reconstructs every observed write in the captured ranges.

## Land samples and occupied-unit ambush

After the optional sea phase, consume exactly `2*L+2` coordinate pairs. Require land (band at least 3), fungus, valid low-six-bit region, and no ordinary Base marker. Query filtered nearest Base `0x4f95f0(x,y,-1,region,-1,-1)` using the full region value. Equal distances select the later Base record. This helper resets global distance output to 9999 on entry even when no Base qualifies; it differs from the unfiltered any-Base helper that preserves prior output when absent.

For an empty tile, or a Unit marker with unknown affiliation at least 8, require `1 < distance < 6` and resolved site/pod query equal to 0. Then create a design-8 owner-0 Mind Worm. Use the real resolved-site rule, not a displayed pod bit. With no same-region Base, distance 9999 rejects this branch even if the previous global distance was 3.

Ordinary owner-0 Unit occupancy skips. Ordinary positive-owner Unit occupancy uses the ambush branch. A found Base at distance at most 2 protects it; no Base does not protect it. Search these offsets in order:

`(2,-2), (2,2), (-2,2), (-2,-2), (1,-3), (3,-1), (3,1), (1,3), (-1,3), (-3,1), (-3,-1), (-1,-3), (4,0), (-4,0), (0,4), (0,-4)`.

Apply horizontal wrap on wrapping maps, reject out-of-bounds coordinates, require destination land, reject victim faction status bit `0x200`, reject a destination explored by the victim, and reject ordinary Base/Unit occupancy. Query native adjacency helper `0x5e0410(destination,0)`: an adjacent ordinary Base/Unit of nonzero owner without outgoing owner-0 Pact bit 1 blocks and returns owner+1; otherwise it returns 0. Create one Worm at the first permitted destination. No fungus, same-region or site-query requirement applies to the destination.

Native blocker control rejects (31,33) with result 4 for owner 3, then births at (27,37). Supplying retained Pact inputs permits the first destination (31,33). This tests relationship inputs, not a treaty-signing action. Explored/omniscient controls suppress the ambush. The near-Base control protects its first occupied candidate but a later independent land sample still creates a Worm. A no-Base occupied candidate can ambush even though a no-Base empty candidate cannot spawn.

## Following cluster code is unreachable

The land loop ends at `0x538183` with count compared to `2*L+2`; `jl 0x537e09` repeats it. Fallthrough therefore has count at least the limit. Instructions `0x53818b..0x538195` reconstruct the same limit and load the same count without intervening calls or mutations. `jge 0x53843a` at `0x538197` must then take the epilogue. There is no other incoming jump to this second guard or following `0x53819d` cluster entry in the function listing.

Consequently the cluster/strength block after `0x53819d` has no reachable normal-entry behavior to implement. All admitted native controls observe the equal count/limit guard and zero cluster entries. This conclusion uses source control flow as well as observations; no forced mid-block execution is presented as a gameplay case.

## Validation boundary

Run `tmp/reference/check_global_wildlife_closure.py --verify-only`: 30 complete native returns, exact coordinate streams and CRT states, complete ordered constructor requests, all nearest-Base/adjacency results, brood request/capacity/order/head controls, and reconstruction of all captured map, live Unit, faction-0, Unit-count and nearest-distance writes. Positive faction and Base records are unchanged. The earlier nine controls independently cover flora changes; these 30 add no flora growth.

All accepted cases have zero actual presentation omissions and zero terrain-preparation view-tail omissions. The harness retains inherited headless startup boundaries. No gameplay helper result is supplied. Four historical attempts remain in raw receipts but are explicitly excluded: one absent-view preparation fault and three cached-class-only search-limit preparations overwritten by native contour reconstruction. Accepted replacements supply contour authority and check reconstructed terrain.

Scope is stock valid inputs for this pinned original and Standard Planet setup plus declared controlled inputs. This does not establish modded capacity/invalid arithmetic behavior, broad constructor AI-reference RNG contexts, wildlife upkeep/commands after births, next global helper `0x5348f0`, or the pending Colony destination heuristic. Those are separate dependencies. Frozen private receipt paths, prerequisites and hashes are recorded in the private frozen bundle manifest (SHA256 `47a8927904222c3d49e2ae6790b3b5228b30b4068f91b9ea487950148feec9cd`).
