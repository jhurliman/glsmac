# Connected global → wildlife → first positive owner

Four original executions now continue from the actual global `0x537790` frame through event maintenance, wildlife upkeep, wildlife commands, and the **first positive-owner upkeep entry**. Every accepted replay ends at `0x53bb60(1)`, before any owner-1 upkeep instruction executes. No unexecuted human/computer body is skipped between these boundaries. See the [numeric boundary and state fixture](global-wildlife-owner-transition-cases.json).

This closes the connected **stock first-round transition**: the unchanged Standard Planet start has no native Units after its first global pass, so its wildlife command body is empty, but wildlife upkeep and its retained planning state still execute. The three changed-input controls demonstrate what must happen when the same connected controller reaches live native Units. They are not naturally progressed turn-8 campaigns or a full independent translation of every wildlife AI branch.

## Minimal stock implementation order

1. Global turn advance has already incremented turn and updated the calendar before `0x537790`. Execute [flora and wildlife generation](global-wildlife-closure.md) against live authorities and the two retained RNG streams.
2. At `0x53a820`, call **`0x5348f0(0)`**, returning at `0x53a827`. This is the resource-event maintenance routine: apply solar/dust/local timer maintenance (original-game source contract), then [perihelion transitions](perihelion-schedule.md), then random-event admission only if its guards permit it. The first-eligible-turn threshold is `75-10*difficulty`; Standard difficulty 3 means turn at least 45. Turns 1 and 8 do not enter new random-event selection or reseed its generator.
3. Complete the rest of global advance. The tail calls territory/region/pod-goal refill `0x538e80` when `(turn & 3) == 1` (`0x53a930..0x53a93d`). This executes at turn 1 and is skipped at turn 8. Preserve its live regional authority and goals as described in [computer planning](computer-upkeep-expansion-planning.md); it is not an RNG burn or a render-only refresh.
4. The actual local controller selects owner 0 and calls **`0x53bb60(0)`**, returning at `0x53caf2`. It runs social refresh, Unit maintenance, Base/research processing and computer planning in their original order, including `0x5773c0(0)`. No owner-0 Base is present in these worlds, so no owner-0 Base yield/research body runs. Owner-0 planning still changes retained state.
5. Dispatch **`0x58f6b0(0)`** from the real frame, returning at `0x53cb67`. Complete its native Unit commands before advancing the scheduler.
6. The common controller continuation advances scheduled owner to 1 and enters **`0x53bb60(1)`**. Keep the same turn, CRT/shared states, live map, dense Unit roster, stacks and retained faction state. Only then run the positive-owner upkeep and existing first computer-round logic. This receipt stops before that upkeep; the earlier [seven-owner authority](seven-first-upkeep-authority.md) and first-computer-command source contract (not yet imported here) supply the following positive-owner work.

In all four controls, `5348f0` performs seven writes: solar phase counter `0x95a28c` goes 0→-1, and six relationship writes from `0x5727c5` retain zero. The source visits the human faction's relations from nonhuman factions, calling `572750(computer,human,0xc00,0)` unless its existing guard bits prevent it. No new event is chosen, no timer is active, and no RNG is consumed. This does not make maintenance a no-op: other existing-event inputs use the already checked lifetime/perihelion rules.

## Complete observed boundaries

| Control | Turn input | Global CRT draws | At owner-1 entry: total CRT / shared draws | Live Units at owner-1 entry |
| --- | ---: | ---: | ---: | ---: |
| Unchanged stock first round | 1 | 208 | 208 / 0 | 13 |
| Fungus/site-suppression candidate at (29,35), seed112 | 1 | 208 | 216 / 0 | 14 |
| Sea brood, otherwise unchanged inputs | 8 | 198 | 222 / 1 | 16 |
| Sea search accepts fungus-covered land (47,67), seed1 | 8 | 198 | 218 / 1 | 14 |

The turn-8 cases supply turn index 8 at the existing captured frame; stored year remains 2101. No natural progression to turn 8 is claimed. The first-round baseline keeps the existing 208-draw sequence. Neither event maintenance nor wildlife upkeep consumes RNG in these controls. All additional CRT calls occur in native command scoring at return site `0x57d749`; the two shared calls occur in actual movement/fungus processing at `0x5aefd7`. Preserve stream separation and call order.

## Wildlife upkeep changes live roster and planning authority

The Unit maintenance prefix `0x53a950` resets the owned Unit iteration byte42 and movement-spent byte40, clears state mask `0x04002002`, clears Unit flags bit `0x1000`, and has further damage/order/periodic maintenance branches. The fresh native Units in these controls have zero damage/state/spent, so many writes retain zero. Do not infer the general maintenance body is empty from those input values.

The planning scan in `0x5773c0` can change or delete Units before command dispatch. Relevant original stock Unit branches are:

- A nonhuman Unit with sentry order 1 is reset to order 0 when it is not a land-triad Unit standing in water (`0x578269..0x578282`). Thus a newly born Worm on land is readied; a Worm alongside an Isle in water stays on sentry.
- For a land-triad Unit in water outside an ordinary Base, the planner checks for suitable stack transport and retains/assigns sentry when available; absence can take the native deletion path. Use the actual stack query and protection flags.
- For a sea-triad Unit on land outside an ordinary Base, absent Unit flags bit `0x20`, `0x578345` calls deletion helper `0x5d89f0`. This executes for the Isle created by the fungus shortcut at (47,67).

That Isle is Unit 13, with Worm 14 linked behind it at the global return. Wildlife planning deletes Isle 13, moves the last live Unit into the dense slot, and the surviving Worm becomes **Unit 13**, singleton, order 0. Count 15→14. The subsequent command uses that surviving identity; stale Unit IDs or a copied birth roster would be wrong. Use canonical deletion/compaction and stack repair. This control does not contain the positive-owner Scout from the separate mixed-owner discriminator (original-game source contract); do not extrapolate its deletion outcome to that unexecuted later scenario.

Faction-0 planning is observable even in the no-Unit baseline: 42 aligned DWORD groups differ between its upkeep entry/return. The fixture records exact deltas, including strategy/census/goal regions. Keep the owner-0 faction record and its region tables through the transition. The constructor AI-reference coordinates `0x95d598/0x95d59c` specifically stay **-1/-1** across all boundaries in all four controls; do not replace the whole record with zeros or assume other planning fields remain uninitialized.

## Command traversal and resulting native Units

`0x58f6b0` makes ten priority passes. Within a pass it scans dense Unit IDs in descending order, checks the live global count and owner, applies role/pass filters, skips sentry order 1, checks remaining movement, and calls `0x58f3c0`. It repeats action processing while the return/count/movement conditions allow it, with the original bounded retry/exhaustion path. For owner 0, actual dispatch reaches native AI **`0x57ce60`**; positive-owner planning is a different path. The body finally clears faction status bit `0x10000` at `0x58f90c`.

Native AI is not one random direction per Unit. Its candidate scoring, path choice, movement primitive and end-turn helper determine both the number of draws and repeated commands:

| Control / Unit at command return | Position | Spent | Order | Other retained effects |
| --- | --- | ---: | ---: | --- |
| Turn1 Worm13 | (30,36), from (29,35) | 3 | 0 | work37=1, state=`0x10004` |
| Sea8 Isle13 | (58,66), unchanged | 12 | 0 | work37=1; still head of Worm14 |
| Sea8 Worm14 passenger | (58,66), unchanged | 0 | 1 | remains linked behind Isle13 |
| Sea8 independent Worm15 | (26,34), from (23,33) | 3 | 0 | work37=3, state=`0x18004` |
| Land-brood surviving Worm13 | (47,71), from (47,67) | 3 | 0 | work37=3, state=`0x18004` |

The independent sea8 Worm receives movement calls `(15,2,3)`, `(15,1,3)`, `(15,1,3)`; the land-brood survivor receives `(13,3,3)`, `(13,3,3)`, `(13,4,3)` at `0x5abe40`. These are native `(Unit,direction,mode)` requests, not coordinate constants to implement. Their shared draw advances seed1→1015568748 and returns respectively 1 or 2 in the two actual movement contexts. The sea Isle is passed to end-turn helper `0x5d9c80`; it does not travel or unload in this world. These literal results provide integration controls, not a general wander/attack/transport policy substitute.

## Validation, persistent state and remaining scope

`check_global_wildlife_transition.py --verify-only` checks all four connected finite returns, the original caller boundaries, both RNG state recurrences, every captured non-stack memory write against its before bytes and final touched-byte authority, and live map/Unit/faction/Base snapshots at every boundary. It checks the exact dense deletion and sentry stack outcomes. This includes native gameplay side effects as well as scratch/library writes; broad journal coverage does not independently prove a semantic translation of every AI branch.

The minimal unchanged first-round implementation can now wire the whole sequence without guessing an empty missing controller slot. For generated wildlife, retain canonical Unit construction/deletion, stack links, movement state, native planning counters, both RNG streams, faction/region census and active/persistent goals. The fixture carries all captured boundary state changes; the private receipt retains full bytes and traces for source extraction.

Full native AI target/scoring/attack/unload translation and general ownership/deletion edge cases remain open. The new controls establish their actual callers, complete observed first commands and RNG effects, so a port lacking those policies must keep them explicit rather than claim the spawned-world path complete. Later round advancement, owner-1 upkeep/body, Colony destinations, combat and map presentation are not newly executed here. Stock baseline completion is not a claim that arbitrary live-wildlife campaigns are behaviorally complete.

Presentation boundary: v2 skips only the absent map-view flag/read and optional redraw at `0x5b2db9→0x5b2dd1`, already admitted by the first positive computer-command probe. Counts are 0/1/3/3 for the table's controls. Inherited scheduler tick services `0x615c40/0x615cd0` are supplied. The generic growth/birth presentation hooks execute zero times. V1's three spawned controls faulted on the absent-view read; they remain excluded historical attempts. No native planner, movement, deletion, RNG, terrain or command return is replaced.
