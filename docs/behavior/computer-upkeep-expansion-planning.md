# Computer upkeep: bounded expansion production and pod goals

This handoff separates observed decisions from complete AI implementation. Controls execute original `0x5773c0(3)` from the actual first University upkeep, after social/Base/numeric TECH processing and before the main AI turn body. They restore the same native checkpoint before each change. Technology controls use native acquisition; extra Units use the native factory; map controls supply exploration/item inputs and run native territory/cache refill. No production, goal, eligibility or chooser result is supplied. The pinned executable is the one in the seven-faction startup handoff.

## Production policy

`0x4fbb90` scans computer-owned Bases and invokes `0x4fb860`; the latter calls the general candidate selector `0x50ce20(base,0,0,0)` at `0x4fba4e`, then submits its result to the ordinary production setter `0x4fb2c0`. The target store is `0x4fb2ed`. These controls execute the full candidate-scoring pass, including later facility candidates. The Pod/Former choice is **not an early return**. Do not replace this with fixed faction targets or a universal “Ecology means Former” rule.

| Changed input from the same checkpoint | Native target | Observed candidate terms |
| --- | --- | --- |
| None | Colony Pod0 | Pod cost1600; Scout cost12288 |
| Acquire Ecology6 | Former1 | Pod1600; Former64; Scout12288 |
| Ecology + create live Former1 at Base position | Colony Pod0 | Former census1 reaches regional Base count1 and is rejected |
| Ecology + create extra live Pod0 at Base position | Former1 | Former census0; cost64 still beats Pod1600 |
| Explore the whole map; native refill | Colony Pod0 | Pod1600; Scout12288 |

The costs above are actual intermediate native candidate costs observed at `0x511534`; the winning candidate retained at `0x514c57` agrees with the final selector return. They are diagnostic values for this state, not constants to implement.

The decisive Former branch is `0x510aac–0x510d30`. It rejects when local exclusion field`+0x34` is nonzero. It reads the same-region Former census at `0x510ab8`, and separately reads the queued count for the exact candidate design (`stack+0x690+4*design`). With no existing or queued matching Former, turn below25 and both local obstruction fields`+0x38/+0x5c` zero, it resets local strategy`+0x20` to zero. Under strategy below3 and no existing/queued matching Former, a computer receives signed truncating cost division by16 when regional strategy is8 or turn below40 (`0x510b33–0x510b3e`). Regional strategy7 adds a division by2; strategy2 otherwise rejects at `0x510b54`. Further regional/Base-state multipliers and discounts apply before the hard ceiling: `0x510ce8/0x510cea` rejects if the census is **greater than or equal to** the owner's retained Base count in that region (`0x95e320`). These are different quantities; the exact-design queue count is not the same-region live census.

The census itself is weighted, not simply a count of raw Unit records. The owner Unit scan starts at `0x50e307`, classifies role and chassis triad, and only adds to the regional census when the Unit tile region equals the candidate Base region (`0x50e49f`). Each matching Unit contributes1 or2 according to its reactor byte versus the selector threshold (`0x50e4cf–0x50e4e7`); the aggregate is subsequently divided by2 with signed truncation (`0x50e56e–0x50e57a`). Ordinary live Former1 changes this control's resulting census0→1. Preserve aggregate rounding; do not round each contribution separately. The complete Pod cost formula, availability helpers, facility scoring and combat production remain outside this bounded rule extraction.

## Type3 goals come from actual Supply Pod queries

The whole-map territory/cache pass `0x538e80` calls resolved-site/pod query `0x5a9570` at `0x5391d2`. Only a nonzero result reaches this producer. Use [the existing resolved-site contract](resolved-site-pod-query.md); resource-bonus and Supply Pod presence are separate authorities.

For a positive resolved territory owner, enqueue type3/priority1 when its exploration bit is set. If hidden, enqueue only for a nonhuman owner with global difficulty **greater than3**. The call is `0x53924a → 0x58fa50(owner,3,1,x,y)`. A negative/unowned territory result is normalized to owner0 before this branch. In that owner0 case, scan factions1..7 with nonzero regional Base presence, apply the same visibility/difficulty gate, skip a matching persistent goal, find a same-region owner Base, and require nearest-Base distance in the inclusive interval `[rule0x9ba4c4, 2*rule0x9ba4c4]`. Only then enqueue at `0x5392d4`. Preserve this annulus and the native territory authority; do not enqueue every known map pod for every faction.

Changed-input controls first reset owner3 goals with native `0x58fce0`, then rebuild using native `0x538e80`:

| Visibility/difficulty input | Type3 goals before planning | Scout2 after planning | Production |
| --- | --- | --- | --- |
| Original visible map, difficulty3 | 1 | order24 | Pod0 |
| All hidden, difficulty3 | 0 | order0 | Scout2 |
| All hidden, difficulty4 | 5 | order24 | Scout2 |
| All explored, difficulty3 | 5 | order24 | Pod0 |
| All explored, suppress nearest pod query with tile item`0x100000`, difficulty3 | 4 | order24, new destination | Pod0 |

Thus type3 is not a generic unknown-terrain frontier list. Higher difficulty permits hidden-pod knowledge. The fully hidden controls also affect production through other exploration-dependent inputs; their target change does not by itself establish the exact Pod rejection formula.

## Goal storage and lifetime

Each owner has75 active records at `0x95eba0 + owner*0x20bc` and25 persistent records at `0x95f050 + owner*0x20bc`. A16-byte record is signed typeH, signed priorityH, X DWORD, Y DWORD, auxiliary DWORD. `0x58fa50` inserts into the persistent list. A new insertion immediately calls active inserter `0x58f910` with auxiliary−1; a duplicate `(type,x,y)` only raises the persistent priority to `max(old,new)` and returns. It does **not** immediately refresh the active duplicate. Active insertion similarly raises duplicate priority without overwriting its auxiliary data.

Persistent replacement: an unused record (`type<0`) scores1000; an occupied record is eligible only if its priority is strictly lower than the incoming priority, and scores`20-oldPriority`. Choose the first strictly best positive score; if none exists, do nothing. Active replacement has the same unused score and eligibility, but occupied priority≤0 scores`100+oldPriority`; occupied positive priority scores`20-oldPriority`. Active insertion additionally rejects out-of-bounds coordinates. The persistent helper has only a parity/debug diagnostic, not the active helper's bounds guard. Preserve first-tie behavior and signed16-bit stored priorities.

At the **end** of owner planning, `0x57ae1a` calls `0x58fc60`: active priority<0 invalidates type; otherwise negate priority. Then reinsert every nonnegative-type persistent goal into active storage. Priority0 survives as0, though dispatch skips nonpositive priority. Persistent entries survive this renewal. Retained active goals already exist before the first planning call, because startup territory refill inserted them immediately; do not regenerate a fresh arbitrary destination list at Scout dispatch. Removal/expiry after pod consumption remains a separate dependency.

## Assignment writer and dispatch boundary

The planner examines active goals in index order. It skips priority≤0 and negative type (`0x57a5e2`); rejects invalid coordinates and different regions; verifies chassis/terrain compatibility. It tracks per-goal assignments and computes priority remaining after assignments (`0x57a87a`). Type3 is single-assignment in this pass (`0x57a94a`). For type3 goals, planner roles other than3 incur a doubled distance term (`0x57a8ea`); the tested Scout has planner role3 and does not receive this multiplier. Native ranking uses signed integer distance-cost/remaining-priority and replaces the best only on a strictly smaller result (`0x57ace5–0x57acf5`), retaining the earlier goal on ties. Other role admission, diplomacy, special abilities and Base tasks have additional branches; this is not a complete generic dispatcher contract.

For the admitted role3 Scout, let `dx` be wrapped absolute X difference and `dy` absolute Y difference. Native distance cost is `256 * (max(dx,dy) - ((abs(dx-dy)//2 + 1)//2))`. The five explored-map goals receive costs512,1024,1536,2048,2048, all with remaining priority1. Suppressing the nearest actual pod at(25,35), before native goal rebuild, removes that source and changes the selected waypoint to(29,35), cost1024. The checker independently evaluates this formula and strict-minimum selection against every recorded native rank in all five goal controls. No expected goal or order was injected into the native runs.

After a goal is selected, `0x57ad34` writes order24; `0x57ad40` writes status ASCII`1` (`0x31`); `0x57ad47/0x57ad4d` load X/Y from the selected active record; `0x57ad53/0x57ad5a` store first waypoint X/Y. Type3 causes `0x57ad8c` to OR Unit state with`0x40000000`. It does not move the Unit or spend movement here. The waypoint-control byte remains its native value; do not invent a waypoint count1 from the nonempty coordinates. Baseline Scout2 stays at(24,38) with spent0 and receives the selected known-pod destination(25,35).

Native receipts: `tmp/reference/computer-expansion-planning-v3-original-x86-20261019.json` and `tmp/reference/computer-pod-goal-controls-v2-original-x86-20261019.json`; producers have corresponding `probe_*.py` names. [Checked numeric cases](computer-upkeep-expansion-planning-cases.json) exclude original map/asset bytes. Checker `tmp/reference/check_computer_upkeep_expansion_planning.py --verify-only` verifies decisions, full Unit write journals, the Former census gate, distance ranks and changed destination. These extend the accepted startup/first-upkeep checkpoint. The earlier expansion v1 block trace was incomplete because translated blocks were already cached; v2/v3 invalidate the translated cache before each invocation. No full computer turn, movement execution or general AI completeness is claimed.

## Port storage implementation boundary

`SmacComputerGoals` implements insertion and renewal on supplied owned records.
The ten native planning controls compare the retained valid active records after
renewal; the type3 persistent inputs follow the audited territory producer.
Additional structural tests exercise replacement scores, duplicate auxiliary
retention, bounds differences, signed expiry and incoming-DWORD comparisons
before signed-word storage. Those boundary inputs are source-derived tests, not
additional native changed-input controls.

Direct helper inspection also establishes that persistent insertion writes only
type, priority, X and Y (`0x58fb27–0x58fb43`); it leaves that record's auxiliary
DWORD untouched. It forwards the sign-extended stored type/priority to active
insertion with auxiliary−1 (`0x58fb58–0x58fb69`). Active duplicate priority updates
likewise leave auxiliary untouched (`0x58f9c6–0x58f9e8`).

The class does not establish world initialization/reset or post-consumption
lifetime. It is not yet stored in the live simulation or called by the computer
controller. Production scoring, territory goal admission, assignment, actual
pathfinding and movement remain separate integration work; storage alone is not
a playable opponent.
