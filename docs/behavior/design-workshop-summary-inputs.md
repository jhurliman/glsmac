# Live human recommendation inputs: fight, air summary and PSI

Expose these retained inputs to HumanPreview; do not substitute social POLICE,
count only conventional Units, or fill unavailable summary fields with zero.
The [bounded original controls](design-workshop-summary-inputs-cases.json) and
[typed scalar reference](workshop_summary_inputs_reference.py) close the
three-counter boundary without implementing all of `5773c0`.

## Typed retained state and refresh checkpoint

| Typed field | Original runtime address + owner×`20bc` | Type |
| --- | --- | --- |
| CurrentFight | `95d584` | Signed32, current AI fight priority |
| ForeignOrdinaryAirThreats | `95d9ec` | Signed32 count |
| OwnAirPlan0 | `95f22c` | Signed32 count |
| OwnAirPlan4 | `95f230` | Signed32 count |

CurrentFight is initialized from loaded definition `+570` at
`5c8a52..5c8a5e`. The turn-zero initializer separately applies random-personality
draws (`5c8c4f..5c8cb9`) and the existing rules override (`5c8cbf..5c8cc8`).
Use the current retained result, not the immutable header on every open.
The separate policy-edit path `4f0b60` can also store −1/0/1. The summary
refresh does not replace this fight value. Full initialization/RNG rules
remain the established [faction initializer](startup_faction_prefix_reference.py).

The turn-zero `95f214..95f238` zero loop initializes both own-air counters
(`5c8d5e..5c8d6e`). That slice does not reset `95d9ec`: its poisoned value17
survives while own counters19/23 become zero. Do not promote a zero observed
in initially cleared storage into a universal threat-count default.

`5773c0(owner)` refreshes the three counters after its Base scan:

1. Call **`577140(owner)`** at `577983`, rebuilding capability summaries.
2. Clear foreign air at `57799d`, own plan4 at `5779b0`, own plan0 at `5779b8`.
3. Scan live dense Unit IDs from `unitCount−1` down to zero. Count qualifying
   instances, not distinct design definitions.

The conditional Time Warp site/Unit producer `5c6760` calls `5773c0` at
`5c76e0` after its Unit-placement loop. Standard Rules startup skips this
producer. Normal faction upkeep `53bb60` first calls the Base-processing
routine `53b7a0` at `53bb88`, then `53b0f0`, then `575db0`, then its sole
`5773c0` call at `53bbbf`, **after Base production**. The earlier version of
this paragraph incorrectly described this checkpoint as pre-Base. See the
[source-order correction](air-summary-checkpoint-order.md) for exact callers
and the first standard-start checkpoint.
By contrast, Workshop producer `597de0@597ede` calls **only `577140`**.
It does not rebuild these three counts. Retain their last summary checkpoint;
do not silently rescan them against a newer Workshop-entry capability snapshot.
The [capability reference](startup_capability_reference.py) already supplies
the independent full `577140` mapping and its technology/peer-cache inputs.

## Minimal human counter refresh

Inputs are owner ID; all live Units including Native-kind Units; their real
design/chassis definitions; directed diplomacy words; the post-`577140` peer
defense cache; and the PSI context below. No opaque full Faction buffer is
required. Design plan means byte **`+2a`**, not category byte `+2b`.

For **own Units**, increment OwnAirPlan0/4 when chassis domain is air (2) and
raw plan is respectively zero/four. There is no Native-kind, signed-power,
special-chassis, technology, visibility or exclusion gate on those increments.
Stock Locusts of Chiron (intrinsic10, chassis7, plan4, PSI offense/armor)
increment OwnAirPlan4. Mind Worms (intrinsic8, land, plan1) do not. Preserve
their original definitions when adapting the port's Native Unit representation.

For **foreign Units**, count at `577aad` only when all are true:

```text
relation = diplomacy[queryOwner][unitOwner]
(relation & 8) != 0
((relation & 3) == 0) OR ((relation & 0xc20) != 0)
chassis.domain == 2
design.rawPlan == 0
chassis.specialByte == 0
592d80(designId, queryOwner) >= queryOwner.cachedPeerDefense
```

The directed diplomacy load is `577a11`; admission tests `577a18..577a29`.
Air/plan/special tests are `577a7b..577a90`; the signed comparison is
`577aa9..577aab`. Do not replace the raw diplomatic predicate with a guessed
AtWar flag. This branch does not test visibility, distance, order, damage,
reactor or ability mask. Foreign stock Locusts have plan4 and fail the plan0
test; a PSI air design with plan0 can qualify. Own plan0/4 increments at
`57880e` / `57881b` use no such foreign-threat predicate.

The complete summary has other Unit mutation/AI routines. This typed subpass
is for the human input boundary over valid live records; it does not replace
the entire summary's role reassignment, deletion, regional or AI lifecycle.

## Negative offense is an estimate, not rejection

`592d80(designId,queryOwner)` resolves that design's signed weapon power:

```text
if power >= 0: return power
v = trunc32(cachedPeerDefense * landPsiAttackRatio / landPsiDefenseRatio)
v = trunc32(v * (100 + innatePsiPercent) / 100)   # separately rounded
if queryOwner owns Dream Twister: v += trunc32(v / 2)
return v
```

The ratio is rules `9ba48c:9ba498` (stock3:2), peer defense is
`95d9d4+owner*20bc`, bonus is definition `95a9c4+owner*59c` (`+4fc`).
Dream Twister host index is `95a068`; resolve its Base owner, with negative
host meaning absent and host zero meaning actual Base0. All context belongs
to **queryOwner**, not the foreign Unit owner. Source arithmetic uses signed
32-bit multiplication/stores and division toward zero; no minimum-one floor
is added by this helper. `592d30(weaponId,owner)` is the same attack algebra
at a component-ID boundary, checked alongside all eight design-helper cases.

For defense, component helper `592ce0(armorId,owner)` uses peer attack
`95d9d0`, reciprocal ratio and Neural Amplifier host `95a040`. Related
`592de0(armorId,owner)` returns **twice** the evaluated defense; its first
argument is an armor ID, not a design ID. Its owner<0 branch returns twice
raw signed armor. Do not confuse that doubled score with raw armor strength.

Crucially, the existing-design prefix of **`597de0` does not call `592d80`**:
it compares/divides raw signed strengths. The PSI estimate is needed here for
the foreign-threat summary, not as a new human preview gate. The
[PSI-reopen contract](design-workshop-psi-reopen.md) already admits active
PSI customs, preserves signed prefix division, and establishes that the
`597390` strategic comparison veto is AI-only. Keep the human mode1..3
availability rules; unconditional negative-offense/armor rejection is unsupported.

## Scope and frozen controls

18 Unit-scan fragments check raw diplomacy, exact equality/below threshold,
air/plan/special gates, PSI discount and own Native Locust/Mind Worm controls.
Eight complete attack-helper pairs and four complete doubled-defense helpers
match the independent formulas. Three fight-copy controls and two initialization/
ordering slices pin producers. The order slice supplies only the `577140`
boundary result; capability algebra is the separately established receipt.
Own-unit fragments supply −1 for Base-at-square lookup in the declared no-Base
context and stop after relevant counters, before unrelated summary policy. They are not
complete `5773c0` executions or natural Unit-acquisition claims.

Private immutable bundle:
`tmp/reference/independent-workshop-summary-inputs-evidence-20261021`, manifest
SHA256 `660ff6192f6816f884f95e1fa4b1df45143b7cac78b7ffc37104c21f0f28a71a`.
