# RE-SMAC-REGIONAL-RESEARCH-PRODUCER-001

This closes the ordinary positive-human opening producer for retained research
strategy and capability inputs, with changed Base/unit distributions and
controlled foreign-cache inputs. [Numeric cases](regional-research-summary-producer-cases.json).
It covers 14 complete native owner summaries, nine capability refresh controls,
two initializer prefixes and one native save/load round trip. The independent
decision model matches all **1,792 regional strategy entries** across the 14
summary controls. The [regional lifecycle/territory follow-up](regional-lifecycle-and-territory.md)
corrects the covered-suitable label and closes actual nonzero restart behavior.
The strategy follow-up (reference receipt `regional-strategy-overrides.md`) adds live foreign-Base
route controls, staggered decisions and complete code0/2 predicates; arbitrary
map pathfinding and broad AI diplomacy remain outside those bounded controls.

## What the research comparison actually reads

The older value fixtures call two fields `attack_cache` and `defense_cache`.
Their numeric values and the existing comparison are unchanged, but the second
name is misleading. The producer establishes these meanings:

| Value input | Actual producer meaning |
| --- | --- |
| `attack_cache` | Owner's maximum available weapon capability, with a floor of 1 |
| Legacy `defense_cache` | Maximum **foreign available weapon capability** from the first qualifying relationship tier |
| Owner's maximum armor | A different cache; it is not the second operand of the research comparison |

Thus the region-code-0 comparison is **own available attack >= retained foreign
available attack**. Do not substitute the owner's armor or the strongest live
unit's defense. Learning Applied Physics raises the owner's weapon maximum to
2 without building a Laser unit; adding an ordinary Scout does not do so.

## Capability refresh algorithm

The capability subpass runs before the owner summary scans units. It also runs
inside automatic prototype generation and a workshop setup path. It is not
performed by merely valuing a technology.

1. Compute available PSI estimates using the **previous foreign caches** and
   applicable native PSI modifiers. Set the ordinary weapon maximum to 1, then
   scan the 21 weapon definitions in order. Use the original prerequisite/
   ownership eligibility; ignore definitions with signed raw power >=99.
   Available nonnegative powers contribute their value. A negative power uses
   the PSI estimate. Maximize the stored weapon capability.
2. Rebuild armor capability similarly over the 10 armor definitions, floor 1;
   also rebuild the separate PSI and available land-chassis movement caches.
   These are catalog/technology capabilities, not a walk over built units.
3. Clear the five foreign capability maxima, then scan other positive factions
   1..7, excluding self. There is no active-faction, alive-faction or human-mask
   filter here. Read their **stored** own weapon/armor/movement/PSI maxima.
4. Widen relationship eligibility in the following order, always excluding a
   forward truce bit 1: (a) Vendetta bit16, comm-link bit8, no treaty bit2;
   (b) comm-link bit8, no treaty bit2; (c) comm-link bit8; (d) any remaining
   positive faction. Finish a whole tier and take component-wise maxima.
   Stop before the next tier once the foreign weapon maximum is nonzero.

This is not a global strongest-opponent calculation. In a control, a qualifying
Vendetta peer with weapon 2 wins the earlier tier while another contacted peer
has weapon 8. Excluding the former by truce produces 8. With no contacts the
last fallback still reads the retained positive-faction caches.

Order matters. A peer's newly granted technology does not affect this consumer
until that peer's capability producer runs. PSI also prevents assuming repeated
refreshes are idempotent: with available PSI and prior foreign armor 8/attack 9,
the checked owner refresh produces own weapon 12/armor 6, then replaces foreign
maxima with zero in the empty-peer fixture. A second refresh produces 1/1.
Reuse the [general PSI estimate rules](psi-capability-estimates.md); the earlier
restart receipt established its cleared-cache entry condition only. Do not insert
extra refreshes at selection, inspector opening or read-only query time.

## Owner-summary order and reset ranges

The ordinary upkeep order is Base/research processing, later owner policy
processing, then this owner summary. Inside this summary:

1. Clear/recount the owner's aggregate Base statistics and regional Base
   counts. Clear **all 128** regional Base bytes, land-combat-unit words and
   attack-contribution words. Regional Base increments wrap as bytes; the two
   military arrays use unsigned 16-bit storage.
2. Retain only regional flag bit8, except clear it too when
   `(turn + owner + region) % 16 == 0`. Other flag bits are rebuilt. This reset
   is per owner, not a simultaneous refresh of all factions.
3. Scan current Base records in order, assigning each owned Base to its current
   map region. Run the capability refresh above. Scan units in reverse record
   order and rebuild relevant unit/strength summaries, using the **old strategy
   codes** where that scan asks for strategy. The final strategy pass comes later.
4. Rebuild strategy entries **1..127 in ascending order**. Entry 0 is untouched.
   The strategy array is not cleared with the military arrays.

For the checked ordinary land combat units, count a unit in its current region
when its chassis is land and its current square is land. The attack contribution
uses weapon capability A and effective morale M as
`trunc((M+6)*A/8)`, with the native order/role gates retained. In particular,
when attack is no greater than armor and order code2 is assigned, the attack
contribution is omitted. This is not combat odds and has no damage-percentage
multiplier here. Noncombat, sea, air and special plans have additional branches;
the changed-unit controls admit ordinary ground combat designs only.

One morale-0 Scout contributes **zero**, not one, after integer truncation.
Two actual factory-created Scouts give regional unit count 2 and attack sum 0.
A controlled Laser prototype passed through the actual unit factory changes
that pair to unit count 2 and attack sum 1. Two controlled owned Base records
recount to regional Base count 2. None of these counters is synthesized inside
the research selector.

## Strategy 0 → 7, and the 7/8 decision

The existing startup receipt observes the Gaian first Base/research pass with
strategy **0**, followed by the owner summary writing **7**. The new controlled
zero-strategy run reproduces that transition. An empty owner instead produces
8 in the ordinary land region. This is an update boundary, not a fixed faction
constant or an instruction to initialize every occupied region to 7.

For an ordinary region define C = owner's regional Base count, N = owner's
total cached Base count, G = retained owned suitable-tile count, F = retained
owned Base-radius-covered suitable count, Z = retained neutral suitable count, R = retained
maximum site rating, X/B = stored Explore/Build priorities, and H = number of
positive factions with retained Base presence in the region, **including self**.
The source uses the following threshold:

`Q = 3 + leadingHumanPressure + (R<4) - setupStateAdjustment
       + (N>4) + H + (worldTiles>3200) + (turn>75)`

`setupStateAdjustment` is 2 when the retained game-state bit256 is set, else 0.
The checked ordinary opening has it set. `leadingHumanPressure` is the existing
leading-human dominance predicate (reference receipt `leading-human-dominance-score-path.md`)
queried for the retained score leader in mode0; it is zero in these controls.

If C is positive, choose the pressure result when **any** of these holds:

- R<2, retained regional flag bit8, C>=20, or G−F<4;
- `2*(X-B+Q)*C > Z+G` **and**
  `trunc((G-F)/ceil(C/2)) + R < 2*Q`.

The initial strategy is **7 for pressure, otherwise 8**. With C=0 the initial
result is 8. Preserve the strict comparisons and signed integer arithmetic.
Rating2/G4/F0 gives 8 in the checked context; changing F to1, R to1, or retaining
flag8 separately gives7. The actual opening has retained rating1, which is
sufficient for its occupied-region result7.

Entries63,64,127 with no owned Base are immediately assigned8. After the normal
calculation, entries64..127 that did not take that shortcut are forced to7.
Consequently an ordinary empty sea-region entry65 can become7, while an empty
ordinary land-region entry2 becomes8. Human research's regional value loop uses
1..62; do not replace this producer with that narrower consumer's bounds.

## Foreign regional inputs and codes 0/2

The strategy pass also reads other factions' retained regional unit counts,
attack contributions, Base counts and the owner's directed relations. These
are not refreshed for every peer by the current owner's pass. The first turn
can query same-region Base-to-Base reachability in two modes. A retained peer
Base count with no actual peer Base can make both route queries false; an
absent peer Base count takes a different shortcut. This discrepancy is a real
reason to preserve cache lifetime rather than rebuild peer presence on demand.

Reachable war counts and capability comparisons can override the initial7/8:
the code0 branch runs first, then code2 can override it. A hostile take0 also
clears the Pact mask before the code2 test, as detailed in the follow-up.
An owned region with
zero reachable-war count bypasses these overrides. In the checked foreign-cache
controls, a single Scout against peer regional count2/attack2 produces2;
Scout+Laser against peer count1/attack1 produces0. Controlled Pact-only retained
inputs instead retain7/8 in an owned region without reachable wars.

These controls execute the real native reachability/policy code; no predicate
result is supplied. They do **not** establish a complete foreign-Base routing
implementation, later turn-staggered route decisions or AI planning. The closed
single-human opening has no such foreign presence. Keep wider contexts outside
that admitted slice until their real route/policy producers exist.

## Territory inputs and persistence

The separate territory/site-cache pass owns the regional tile summaries and
maximum site rating. It clears/recounts them across faction slots and regions;
the owner-summary pass above preserves them. Suitable tiles have nonzero
moisture, less than rocky terrain and no fungus; the covered subset additionally
has the Base-radius exclusion flag, not a Farm. The legacy numeric fixture key
`owned_farmed_suitable` means covered suitable; its numbers are unchanged.
Counts are attributed to the pass's resolved territorial owner,
including neutral slot0. Maximum rating comes from retained or computed native
site ratings. Known-tile counts have their separate visibility/override gate.
These are neither total region area nor a fresh calculation made by valuation.

The tested **fresh turn-zero** faction-initialization prefixes zero own/foreign capability caches
and the regional Base/unit/attack arrays but **preserve poisoned strategy
bytes**. Cold executable memory supplies zero strategy in the observed startup;
these prefixes do not prove an unconditional strategy clear on every restart.
The actual nonzero restart caller skips that fresh regional reset, preserving
the regional Base/unit/attack/flag/strategy arrays through search/package return.
Later owner-summary recount is a separate boundary, as the linked follow-up proves.
The owner summary also preserves strategy entry0. Ordinary native save/load
restores both capability fields and strategy canaries, including stale values,
without recomputing them.

Use the previous retained snapshot during Base/research processing; publish the
new snapshot when the actual owner-summary stage finishes. The earlier native
positive-research composition proves that moving this refresh before the Base
pass changes research values. Opening a picker or inspector must not refresh
these caches to manufacture current values.
