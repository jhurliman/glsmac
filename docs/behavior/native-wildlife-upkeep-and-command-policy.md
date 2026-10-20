# Native upkeep and command policy for global-born wildlife

Use the [source-derived reference](native_wildlife_policy_reference.py), 27 upkeep/land controls (original source contract), and 14 native command recordings (original source contract). These translate competing decisions and live caller behavior, not routes keyed to captured positions. The 14 recordings contain 13 distinct inputs: the route case is repeated with every admitted directive candidate's score observed. Two recordings use unchanged native command frames and match the earlier [connected global→wildlife→owner receipt](global-wildlife-owner-transition.md) byte-for-byte at entry and return.

All accepted planner controls execute `5773c0(0)` from its actual upkeep frame and return at **`53bbc4`**. Command controls execute `58f6b0(0)` from its real controller frame and return at **`53cb67`**. Actual factories, deletion, linked-stack queries, sentry, pathfinding, transport, movement, RNG and action helpers execute. Model decisions are never supplied to the original.

## Upkeep: sentry, unsafe terrain and dense deletion

The live caller remains `53bb60(0)`: social refresh → Unit maintenance `53a950(0)` → Base/research processing → later computer planning `5773c0(0)`. Existing startup/transition contracts cover their retained state. The scalar Unit rule translated here is `57824a..57836d`:

1. On this planning path, clear Unit state bits `0x30000024` (`state &= 0xcfffffdb`). Earlier Unit maintenance has its separate state/flags/spent resets; do not merge their phase timing.
2. For a nonhuman Unit with order1, clear that order unless it is **land triad standing in water**. This tests triad and terrain, not whether payload names a carrier.
3. For a land-triad Unit in water outside an ordinary Base, count **role7 Units in its linked stack**, without an owner filter (`5d0c40(uid,2,7,-1,-1)`). If none exists, delete the Unit unless flags contain `0x20`. If one exists and its order is not already1, call the real sentry helper.
4. For a sea-triad Unit on land outside an ordinary Base, delete unless flags contain `0x20` (`578345→5d89f0`). Base protection uses an ordinary Base marker with affiliation0..7.

`upkeep_native_unit` translates the order/delete decisions; it is not a replacement for the whole planning scan or its state/census/goal writes. Its transport query is a linked-stack role count, not the mode0 cargo inventory or an exact assigned-carrier predicate. A positive-owner Transport Foil protects a native Worm in the tested mixed stack. A native Worm without a transport is actually deleted; the original mislabeled preparation accidentally created one and is explicitly excluded in favor of a repaired native control.

Use canonical deletion, counter debits, compaction and stack repair. A land Isle plus Worm is reduced to the surviving Worm in the Isle's dense slot. In the mixed land case, the positive Scout survives while the native Isle is deleted and the Worm is compacted; owner0 planning does not convert the Scout. The protected-Isle and ordinary-Base controls retain the Isle, proving the deletion guards rather than only the final count.

## Target Base is a weighted competition

Native command dispatcher `58f3c0` reaches `57ce60` through the actual Unit planner. It uses **`57cc80`**, not the ordinary nearest-Base helper. The reference `native_base_target` translates it.

Reset global metric `91871c` to9999. For each Base in ascending dense order:

- If the source full region is below64, require the Base's full region to match. For source region64 or above, this region filter is omitted.
- Compute the existing wrapped native integer distance `d`. Start metric at `floor(32*d / (Base[+c0] + Base[+c4] + 32))`.
- If a role12 Unit is present in the Base's stack (`5d0c40(head,2,12,-1,-1)`), halve with signed truncation. The native control uses an actual Artifact7 factory.
- If this Base is the Unit's retained home identity, halve again.
- If Unit byte38 differs from Base owner, that owner's retained PLANET field `95d43c+owner*20bc` is positive, and Base DWORD`+48` is zero, double the result.
- Replace the best on **`<=`**, so later Base records win equal metrics.

Return the chosen Base and publish its **weighted metric**, not its geometric distance, in `91871c`. The reference deliberately names original Base inputs `c0`, `c4` and `field48`; feed their retained engine equivalents rather than substituting population or raw distance. No semantic guess about those fields is needed for their exact consumption rule.

The controls change competing inputs: retained home and an actual Artifact reduce the winning metric3→1; increasing a competing Base's `+c0` makes it win at0; positive PLANET for the former winner's owner doubles that candidate's metric and changes the winner from Base2 to Base1. Thus the rules are not inferred from a single unchanged nearest target.

## Normal neighbor scoring

After admission and non-carrier handling, `57d4e6..57d7ae` visits directions in this exact order:

`0:(1,-1), 1:(2,0), 2:(1,1), 3:(0,2), 4:(-1,1), 5:(-2,0), 6:(-1,-1), 7:(0,-2)`.

Wrap X on wrapping maps; reject bounds before reading a tile. Reject water for land triad and land for sea triad. A land-triad Unit currently in water also rejects positive-owner occupied land. For other eligible destinations:

- An ordinary positive-owner Base gives initial score9999.
- Otherwise, a positive-owner linked-stack head gives999. For predefined designs below64, a negative weapon offense rejects that Unit target. Reject air/non-air target mismatches.
- Empty/native destinations start at0.
- Add **4 per set bit** of `tile.items & 0x80208054`.
- Add8 for item bit`0x2000` when difficulty>2 and Unit state lacks`0x100`.
- Consume **one CRT draw only for each admitted candidate**, add `draw % 8`.
- Let `A = min((((facing XOR 4)-direction)&7), 8-(((facing XOR 4)-direction)&7))`. Add `(L+1)*A`, plus `L*L` if direction equals retained facing.

Here L is the turn-only return of `515d50(-1,0)`:

| Turn | L |
| --- | ---: |
| below45 | 0 |
| 45..89 | 1 |
| 90..169 | 2 |
| 170..249 | 3 |
| 250..329 | 4 |
| 330 onward | 6 |

Initialize best score0 and chosen direction-1. Replace only on **strictly greater** score, preserving the earlier candidate on ties. This direction bias favors continuing away from the reverse heading; it is not distance-to-target scoring in these early, small-target-faction worlds. The later `turn>50 && target owner Base-count>2` branch can obtain a direction from the real pathfinder; see limits below.

Every admitted intermediate score is independently compared to original `57d78e`. The tie control gives directions0 and3 score9 and selects0. The item-0x2000 control raises direction6 from6 to14, beating direction2's10; difficulty2 or Unit state`0x100` removes that bonus and selects2. Feature inputs change the winning direction. Turning one candidate into water consumes7 rather than8 draws and shifts the later candidates' jitter. Facing0/7 and turn45/90 controls change heading terms according to the formula.

Use the native result to set order`0x58` and first waypoint to the selected adjacent coordinate. Continue through the existing action/path/movement engine; that is where actual movement, fungus admission, spent budget, stack/cargo and shared RNG effects occur. Do not set final positions directly from this policy function. The healthy early controls take no later native-AI CRT checks, but the movement helper can consume the separate shared stream.

## Carrier stack interpretation and shoreline activation

After target selection, native AI increments **Unit byte37 once per native-AI invocation**, with byte wrapping. This is a retained work counter; four actions in one turn increment it four times. It is not an elapsed-turn age.

Query actual capacity `5d9730`. Enter the carrier branch only if capacity is nonzero and `5d0c40(uid,3,0,-1,-1)` finds a land-triad Unit in the linked stack. That query has **no owner, order or payload filter**. Native controls separately vary foreign ownership, order0 versus1, and payload-1 versus13; all still admit the carrier branch. Canonical boarding remains a separate helper.

If incremented counter>24, call native disband helper `5d8c40`. Otherwise scan the eight directions above:

1. A positive-owner Unit marker in water chooses the first such direction for the immediate attack path. A positive-owner occupied land tile skips this candidate.
2. Otherwise require land in the chosen target Base's full region. Compute wrapped `(abs(dx-to-Base)+abs(dy-to-Base)) & ~1`; if it is at most12, select this first shore.
3. At a selected shore, walk the **entire current linked stack**. Set every order to0 and OR each Unit observation byte39 with that land tile's explored mask. There is no owner filter. Exhaust the Isle using `5d9c80`, which assigns spent from the actual movement-allowance helper.

This operation does not instantly move or unload passengers onto land. In the natural-shore control, Isle13 stays at(57,11), spent12; Worm14 is readied and later the normal command pass moves it to(57,13), spent3. A second control adds a real foreign owner2 Scout to the same stack: its sentry order is cleared by the same activation loop. A facing change does not alter the first eligible shoreline. The source helper order is preserved in the model.

## Carrier directive competition

If no adjacent shore/action matches, call **`57ae30(uid,99,targetBaseX,targetBaseY)`**. This helper's exact selection is in `carrier_directive`:

- If source is water and source/target full regions match, directly assign order24, status99 and the target waypoint.
- Otherwise examine 21 offsets around the target: center, the eight direction offsets above, then `(2,-2),(2,2),(-2,2),(-2,-2),(1,-3),(3,-1),(3,1),(1,3),(-1,3),(-3,1),(-3,-1),(-1,-3)`.
- Require bounds, water, and the source's full water region. If the source is inside a Base, original `521ea0(sourceBase,candidateSeaRegion)` supplies connectivity instead; the reference explicitly requires that helper callback rather than inventing a replacement.
- Score each admitted water candidate by the number of its eight neighbors that are land in the target's full region and are not ordinary Bases. Start best at0 for a land target, -1 for water. Replace on **strictly greater** count; first ties survive. Set order24/status99/waypoint only when a candidate beats the best.

The detailed route replay compares **every admitted candidate coordinate and shore count** to original `57b0d7`, across all four native-AI invocations. This is competing input policy, not a captured waypoint constant.

After the directive helper, if no order24 was assigned, return through the existing command executor; the stock held-Isle case exhausts its turn there. If order24 points to land within even wrapped Manhattan distance2, native AI explicitly exhausts the Isle; otherwise let the canonical action/path engine follow the order. The test route moves Isle13 and Worm14 together four times, (11,1)→(15,5), with remaining budgets12,9,6,3 and no CRT/shared draws. The Worm starts with order1/payload-1; original boarding changes its payload to13 during movement. Use [the existing transport contract](transport-mode0-boarding.md), including stack order and owner rules, rather than treating the unassigned passenger as already assigned cargo.

An empty Isle takes normal neighbor scoring instead of this carrier branch. When the best score is below99 and capacity is nonzero but the land-triad count is zero, source `57d9e6` calls native disband. The control scores six admitted sea neighbors, consumes six CRT draws, and deletes the empty Isle. Deletion and counter/stack effects remain canonical.

## Priority passes, retries and live budget

`58f6b0` scans live dense IDs descending in each of ten passes. Its role filters are:

| Pass | Admission before common live checks |
| --- | --- |
| 0 | canonical design helper `5d8ed0(design,1)` succeeds |
| 1 | role2, or role7 with a stack land-triad count |
| 2 | air triad and role other than4 |
| 3 | role1 |
| 4 | damage byte16 is0 |
| 5..6 | no additional role admission |
| 7 | no additional role admission |
| 8 | role7 and positive owner; owner0 skips |
| 9 | no additional role admission |

Passes0..6 additionally exclude canonical ability`0x10`, role4 and weapon ID`0x15`. These are canonical design-helper inputs to `command_pass_admits`, not assumptions that all native designs pass. After role admission, recheck live ownership/count, skip order1, and query actual remaining movement. `remaining = allowance - spent`: reject negative or zero; values above999 use the original exceptional budget path. Do not cache a birth roster or initial spent value across actions.

Call `58f3c0`. Nonzero result or changed global Unit count ends processing this Unit for the current scan. Otherwise increment retries by1 when the resulting order is24 or triad is air, else by2. If retries exceeds32, exhaust through `5d9c80`; then re-enter the same live order/budget checks. The source limit is `>32`, not a single action per Unit or per pass. The engine-facing repeat decision is `command_repeat`.

Native controls observe the Isle in pass1 and the readied Worm in pass3. The directive route has retries0,1,2,3 and remaining12,9,6,3 before four dispatches. A supplied exhausted Isle causes no native-AI call or RNG draw. Sentry Worms cause no command call while held. Final dispatcher write clears faction status bit`0x10000` at`58f90c`.

## What is closed and what remains separate

The rules above independently reproduce the actual first commands of the unchanged land/sea spawned controls from the connected controller receipt, including every intermediate normal score and native carrier decision. Canonical action, movement and stack helpers retain their native effects; their entire observed write journals are checked. No empty wildlife-phase RNG burn is valid for these worlds.

Keep the following branches explicit:

- **Combat:** positive Base/Unit scoring bonuses and air compatibility are source-translated, and the separate mixed-Scout phase executes real combat. This is not a new complete native attack-resolution model. Feed chosen attacks through the existing component combat (original source contract) / movement contracts. Immediate adjacent sea attacks are identified by `carrier_step`; their combat resolution is not supplied by that function.
- **Air:** priority admission and normal candidate compatibility are translated from source. No new full Locust/air command matrix is claimed. Native design12's explicit hold branch, special abilities and damaged-air paths need their own callers/controls when required.
- **Later/damaged behavior:** opening flags`0x440` or design12 exhaust rather than choose a neighbor. No-Base retirement, work-counter retirement for isolated non-carriers, later pathfinder-derived heading (`turn>50`, target owner>2 Bases), human-target pillaging and damaged-Unit hesitation have further source branches and additional CRT calls. They are outside the healthy newborn-policy fixture. Do not silently use the healthy reference for those conditions.
- **Transport:** the tested shoreline activation and route/boarding paths are closed. Source-Base connectivity still delegates to `521ea0`; no new general Base-network or arbitrary unload policy is claimed. Foreground notifications after activation are outside these hidden/nonfocused controls.
- **Retained authority:** preserve both RNGs, Unit byte37, facing, order/waypoint/payload, spent/damage/state/flags, native faction strategy/census/goals, and conventional linked stacks through the global→native→positive-owner boundary. The model emits decisions, not a replacement for those persistent engine systems.

Run `check_native_wildlife_policy.py --verify-only` and `check_native_wildlife_sea_policy.py --verify-only`. Private producer paths, raw receipts, exclusion erratum and freeze procedure are in the audit note (original source contract). The separate Scout departure proof (original source contract) rules out the port's blanket source-hostility guard; it is not inferred from constructor or carrier success.

## C++ policy integration

`unit/SmacNativeAI` translates these decision rules, including complete weighted
Base terms, eligible-neighbor score terms and CRT state, upkeep action order,
priority admission, retry threshold, carrier activation and directive competition.
The owning controller must still execute canonical actions, maintain live dense
identity, and publish retained faction/Unit/map state. These functions do not
activate wildlife or replay a whole native command in GLSMAC. Healthy newborn
admission does not cover the later/damaged branches listed above.

`native-wildlife-decisions-cases.json` contains semantic inputs extracted from
frozen original caller snapshots and the independently checked expected outputs.
It includes all 41 recordings: 42 command decisions and 16 native Unit upkeep
decisions across ten planning cases. All weighted Base terms, every candidate
score/rejection, ordered directive shore counts, selected identities/directions,
observation bits and CRT returns are compared in the asset-free native test.
The two unchanged land/sea frames remain bound to the earlier connected original
receipts by the separately verified source checker. Movement, cargo assignment,
combat and whole command write journals are source evidence, not C++ effects
established by this decision test.

The original reference names one term `road`, but its numeric flag is `0x2000`.
The current terrain mapping identifies that flag as Monolith and Road as `0x4`.
The reverse-engineering follow-up confirms this is a monolith bonus. The C++
field remains `item_2000_bonus`; the proven numeric condition and historical
reference fixture key are unchanged.

Validation: cached incremental build and both native groups pass normally and
with the production policy and its test translation unit under ASan/UBSan;
other linked objects are normal and leak detection is disabled. Targeted
production analysis reports zero diagnostics. No full platform matrix was rerun.
The independently verified source bundle contains 32 frozen files, manifest SHA256
`b0de9b3be666c325c01f16538074135142c19c76c5301db526176bfec7fef313`.
