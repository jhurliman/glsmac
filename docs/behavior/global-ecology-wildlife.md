# Global ecology and wildlife before the first owner upkeep

The208 scheduled-owner0 CRT draws in the accepted seven-first-upkeep receipt all belong to **global producer`0x537790`**, called at`0x53a81b` by global turn advance`0x53a530`, with return boundary`0x53a820`. They are forest/kelp sampling and wildlife generation. Scheduled0 is the controller context, not proof that the draws came from wildlife faction upkeep or its command body.

[Numeric cases](global-ecology-wildlife-cases.json) and [independent reference](global_ecology_reference.py) cover nine complete native producer executions, including real terrain growth and actual owner0 Unit creation. No coordinate, eligibility, growth, site-query, nearest-Base, factory or RNG result is supplied. The source phase is captured from the actual first global advance with its original controller frame; controls restore that checkpoint and change declared inputs only.

## Ordering and exact earlier-receipt attribution

The global advance has already changed turn0→1/year2101 when this producer is entered. It performs spread sampling first, then wildlife work, and returns to`0x53a820` before next global helper`0x5348f0(0)`. Wildlife faction upkeep`0x53bb60(0)` and computer body`0x58f6b0(0)` occur later, followed by first positive-owner upkeep.

Every one of the208 native `(caller,before,result,after)` CRT records exactly matches the earlier frozen v2 seven-first-upkeep receipt. Starting seed1537134187 becomes1551285371. In that earlier complete-controller run, the subsequent wildlife upkeep entry, empty wildlife command entry and owner1 upkeep entry all retain seed1551285371. Wildlife has zero Units at those boundaries, so its actual command sweeps perform no Unit work. Shared RNG remains unchanged. This establishes **208 global draws and zero intervening wildlife draws for that baseline**. A world where the global producer creates wildlife can give its later upkeep/body real work; it must not be implemented as an unconditional empty phase.

Run the global producer once at its owning global-advance phase, before owner upkeep. Running it again for each faction would repeat both ecological changes and RNG consumption.

## Forest/kelp sampling

For nonnegative turn`T` and stored actual tile count`N`, perform:

```
attempts = N // (32 + T // 4)
```

On the pinned80×80 doubled-X map, `N=3200`. Turn1 gives100 attempts; turn4 gives96. Each attempt draws X modulo width and Y modulo height from the native **CRT** stream, then adjusts X by`(Y&1)-(X&1)` to match tile parity. For normal dimensions greater than1 that consumes two draws per attempt, even when the source tile cannot grow anything. The return sites are`0x5377da` for X and`0x5377f5` for Y. Tiny/invalid geometry is outside these controls.

A sea source (elevation band<3) needs item`0x8000` for kelp; a land source needs forest item`0x200000`. Other sources do nothing. Examine neighbors in native order NE,E,SE,S,SW,W,NW,N, with horizontal wrapping unless the flat-map flag is set; reject out-of-bounds coordinates. A target must:

- Have the same land/sea class as the source, elevation band≥2, and positive moisture when band>4. A sea target is consequently shelf band2.
- Avoid active landmark predicates`(word&0x80000002)==2` and`(word&0x80000040)==0x40`.
- Have rockiness≤1 and no item bits in mask`0x208051`.

Score a remaining target as **`4*(3-moisture)-rockiness`**. Choose the first strictly highest score above0. Thus ties retain earlier neighbor order, and candidate selection adds no random draw. OR the appropriate feature into the chosen target and clear fungus bit`0x20`; the physical item store is`0x537a35`. This is a live ordered mutation: growth from an earlier attempt can affect later attempts. Do not calculate every attempt against an immutable turn-start map.

For each viewer1..7, update the destination's remembered item word only if the source is explored for that viewer or its faction status has`0x200`, **and the source's remembered items contain the spreading feature**. Apply the same feature-add/fungus-clear operation. Visibility is tested at the source, not simply at the new destination. The known-feature propagation and notification branches are source-mapped; the new seeded controls hide the changed source and do not newly exercise every observer/notification combination.

Native changed-input results:

| Input | Source | Actual new growth | Independent score |
| --- | --- | --- | --- |
| CRT seed1; add hidden forest source |(41,67)|(41,69), forest`0x200000` |8 |
| CRT seed3; add hidden kelp source |(48,76)|(49,75), kelp`0x8000` |8 |

The checker independently reproduces **every sampled coordinate and all3200 complete map records** at the pre-wildlife boundary. The unchanged baseline has no successful growth, but still performs its100 samples.

## Wildlife gates and land sampling

These checks follow spread sampling, so they do not suppress the first200 baseline draws:

- Primary rule bit`0x200000` skips wildlife work.
- A present Voice project marker (`0x95a098 != -1`) skips wildlife work.
- Global Unit count≥`0x700` skips wildlife work. This cap is source-mapped; a synthetic full-cap world was not constructed here.

The native-life category`L` controls the subsequent budgets. A preceding sea-brood branch is eligible when `T % (2*(5-L)) == 0`; it is not taken in these turn1/turn4 controls. The source also computes an early-period flag from `T < 2*(10*(3-L)-difficulty)`, used by other wildlife-strength/cluster branches. These broader branches are not independently closed by this receipt.

The observed land phase performs **`2*L+2` samples**, using CRT return sites`0x537e1f/0x537e3a` for X/Y and the same parity adjustment. Category0 still gives two attempts; it does not disable wildlife. A candidate in the independently checked empty-tile branch must be land with fungus, a valid nonzero/non63 low region ID, and no ordinary Base occupancy. Native same-region nearest-Base query`0x4f95f0` must give distance **strictly greater than1 and strictly less than6**. The resolved site/pod query must return0. Then actual factory`0x5d8520(8,0,x,y)` creates a Mind Worm, with return site`0x53811d`.

Use the existing resolved-site query, not a displayed pod bit. Preserve same-region nearest-Base selection and its later-record tie behavior. Occupied positive-owner ambushes, the no-Base branch's retained distance output, sea broods and following cluster branches need their own handlers; the bounded reference raises on unimplemented encountered branches instead of silently treating them as no spawn.

One control supplies CRT seed112 and adds fungus plus site-suppression input at the first land sample(29,35). Original queries choose nearest Base2/distance4 and original factory creates owner0 Mind Worm13. Another control—the seed1 forest scenario—naturally creates a Mind Worm at(38,36), nearest Base1/distance3. The latter spawn shares the same global pass but is attributable to the changed sampling seed, not to an assumed rule that forest growth creates worms.

## Budget controls and RNG ownership

| Changed input from the same entry | Spread attempts | Land attempts | Total CRT draws |
| --- | ---: | ---: | ---: |
| Baseline: turn1, native life1 |100|4|208|
| Turn4 |96|4|200|
| Native life0 |100|2|204|
| Native life2 |100|6|212|
| Wildlife-suppression rule bit |100|0|200|
| Voice marker present |100|0|200|

Each draw advances CRT seed`214013*s+2531011 (mod 2^32)` and returns`(s>>16)&32767`. Coordinate selection uses the returned value modulo the dimension. This producer does not use the distinct shared RNG stream in these controls. The actual new-Unit factories add no CRT draws in the tested creation paths. A fixed208-draw burn would miss the changed budgets, terrain mutations and owner0 Unit creation.

## Validation and limits

All nine controls return to`0x53a820`, without a memory fault or unanswered chooser. The checker verifies the exact old208-record sequence, formula-derived budgets, all coordinate samples, whole-map spread results, independent empty-land spawn decisions, final CRT state and every observed map/Unit write, including all newly initialized live Unit bytes. Global Unit count changes13→14 in the two spawn cases; original factories execute their bookkeeping. This is not a claim to journal every global/faction bookkeeping byte.

The new producer adds **no presentation omission** on these paths. It inherits the declared startup/headless-controller boundaries; seeded growth sources are hidden inputs, so focused notifications are not exercised. No gameplay helper result is replaced. Wildlife upkeep and commands after the changed-input spawns are not executed; the stop is still the global producer's return. The reference does not cover every branch of`0x537790`, full ecological consequences, or the pending Colony destination heuristic.

Run `tmp/reference/check_global_ecology_wildlife.py --verify-only`. Private original receipt:`tmp/reference/global-ecology-wildlife-v2-original-x86-20261019.json`; producer:`probe_global_ecology_wildlife_v2.py`. Existing frozen controls remain intact.

## Port forest/kelp implementation

`map::smac_spread::Prepare` implements the complete spread sampling slice on
owned terrain records, returning staged terrain, ordered samples/changes and
the resulting gameplay RNG pair. It preserves all unrelated tile bytes and
uses terrain changed by earlier samples. The separate wildlife portion is not
represented by this function.

With `GLSMAC_GLOBAL_ECOLOGY_REFERENCE` pointing to the private original receipt,
the C++ tests compare every sampled coordinate, all complete tile records and
CRT state at the pre-wildlife boundary in all nine controls. Asset-free tests
exercise source observer gates, first ties, fungus clearing, independent result
ownership, mandatory no-growth draws, budgets, and forest/kelp propagation from
new growth within the same pass. Those synthetic edge cases are not additional
native controls.

The function is not yet called by the live global turn. Activation must combine
it with real wildlife generation and the remaining global maintenance in their
native order, publish terrain/knowledge/render updates and any notifications,
and bind the RNG transition exactly once. No empty wildlife handler or fixed
draw burn is supplied by this slice.
