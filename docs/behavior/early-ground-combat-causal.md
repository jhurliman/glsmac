# Early ground combat: causal integration contract

2026-11-11. `RE-SMAC-EARLY-GROUND-COMBAT-001`.
[26 complete strength queries and six defender queries](early-ground-combat-causal-cases.json)
pass. This is one bounded packet for the first50-turn milestone, not a new
round/combat matrix. It reuses complete original-loader-produced numeric
components/RULES and existing supplied-world machinery. Modifier label text
alone is supplied empty; arithmetic, morale, speed, terrain and selection
helpers execute original instructions. Pinned terran.exe SHA256:
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.

Use the existing [round/RNG/promotion/casualty contract](scout-conventional-combat.md),
[full wrapper return](scout-attack-normal-return.md),
[staged FANATIC/Base modifiers](scout-defending-base.md) and
[mode1/home-morale rules](focused-scout-battles.md). This packet closes ordinary
component powers, the requested terrain/speed effects and ordinary military
stack selection. Positive component sign controls, Infantry/Speeder, reactor1,
abilities0, no Base/sensor/project, neutral homeless morale1 are the query
profile. PSI/native extensions explicitly distinguish query controls from
retained full-battle evidence and static source evidence.

## Conventional components and terrain

`5d91c0` returns weapon power×8 for the ordinary positive weapon/context path;
`5d92f0` returns armor power×8, with its minimum-one armor clamp. These feed
`515ec0` and`5160d0`, then composed strength`516390`:

```
ordinary A = 32 * weaponPower * (6 + effectiveAttackMorale)
ordinary D = 32 * armorPower  * (6 + max(1, effectiveDefenseMorale))
```

Both morale consumers use mode1. Component multiplication is before the
existing sequential percentage modifiers and later partial-movement reduction.
Do not multiply powers into health or per-hit damage. Neutral Fission
examples: Laser power2 gives448 attack; Impact power4 gives896; Synthmetal
power2 gives448 defense. Gun/Scout gives224/224. The existing round kernel's
one-damage hits and health10 are retained for this reactor1 scope.

Ordinary land terrain factor`515ab0` returns2 for flat/rolling and3 for rocky
or forest. Its rocky test is raw rockiness bits above rolling (`>=0x80`);
forest is flag`0x200000`. Forest raises the factor only when not already rocky:
**rocky+forest does not stack**. Composed defense uses that factor within its
native integer stages; in this neutral profile this is×1 or×1.5. Controls:
flat224, rolling224, rocky336, forest336, controlled rocky+forest336.
Fungus, Bases and other domains take separate source branches.

The open-ground speed bonus is attack rule`9ba4e4`, installed25. In the
ordinary conventional land/no-Base branch (`51719a`..`51725a`), require:

- Destination terrain factor2 and raw rockiness below rocky.
- Attacker **basic design allowance>Q**, not remaining movement or elite bonus.
- Defender basic allowance≤attacker basic allowance, so equal-speed Rovers qualify.

Apply `truncate(A*(100+bonus)/100)` as its own stage. Rover→Infantry on
flat/rolling gives280/224; rocky/forest gives224/336. Rover→Rover on flat
also gives280/224; Rover→faster Hover gives224/224. Infantry→Rover gets no
reciprocal speed bonus. Supplying speed rule0/50 changes Rover-flat attack
to224/336, confirming this exact parameter. Existing Base/other modifier gates
remain separate; do not apply this as a universal chassis multiplier.

Scout-armored **zero-offense** Former/Colony designs receive the installed
50% noncombat defense penalty (`51840f`..`518496`, rule`9ba500`). Both resolved
ground profiles return112 defense on neutral flat land. Source gates this on
weapon power exactly0 and armor power exactly1, with its native-attacker/Base
exception; it is not a penalty for every plan>=8 or every armored Former.
The checked positive conventional attacker meets the gate. Probe and Artifact
defense have additional special branches in`5160d0`; they are not this rule.

## PSI composition and native first50 behavior

The general ordinary ground PSI sign gate is **attacker weapon power<0 OR
defender armor power<0** (`516419`..`51648f` and component helpers). Negative
defender weapon alone is not that gate; the stock Worm also has negative armor.
In this PSI branch substitute installed domain ratios, not conventional powers:
ground offense3 (`9ba48c`), defense2 (`9ba498`). Neutral human-owned component
sign controls return672/448 whether PSI is triggered by attacker weapon or
defender armor. Conventional weapon/armor magnitudes do not multiply those
ratios. No ordinary rocky/forest/open-ground speed stage is applied on this
checked ground PSI path.

The hypothesis for the existing produced Spartan Rover/Worm battle is correct:
**`32*(6+2)*3=768`** versus **`32*(6+max(1,0))*2=448`**. See the retained
[43 complete two-command native cases](native-component-combat.md), which also
prove native defender round width224 before turn15, then448 at15/16. That
receipt checks actual rounds, promotion, reward10 and full return; no repeat
matrix is needed here.

Faction PSI percentage is applied **inside** the component/morale helpers:

```
component = 8 * domainPsiRatio
component = truncate(component * (100 + factionPsiPercent) / 100)
strength  = 4 * component * (6 + roleMorale)
```

Attack helper`51603e`..`5160ac` and defense helper`51629b`..`5162fe` read their
own faction scalar at`95a9c4 + faction*59c`. Projects have their own later
component-stage branches and are absent here. Keep the truncation before the
morale multiplication. A diagnostic17% on each side produces attack784,
defense504 before PLANET, rather than end-stage rounded percentages.

For PSI **attack**, composed strength then reads the attacker's **retained
PLANET** at`95d43c + faction*20bc` (`516b72`..`516c2e`) and applies
`truncate(A*(100 + retainedPlanet*rule9ba514)/100)`, installed rule10.
The diagnostic17% PSI/+1 retained PLANET therefore returns**862/504**,
despite current PLANET−3. Supplying current PLANET+3 alone gives672/448.
There is no corresponding retained-PLANET defense multiplier in this source
path. Do not fold faction PSI percent and PLANET into one percentage division.

Wild owner0 takes native-level query`515d50`, bypassing ordinary stored/home
MORALE. Existing [birth](native-birth-producer.md) and
[record/level controls](ecology-native-unit-records.md) establish global-turn
levels: `<45:0`, `45..89:1`, `90..169:2`, `170..249:3`, `250..329:4`, `>=330:6`.
Thus first50 crosses the0→1 boundary at turn45; it is not Unit age or stored
morale. Native defense still floors role morale at1. Source additionally adds:
one for nonzero opposing-faction major-atrocity word; one for Unit flag`0x100`;
one for design10; and `(Unit.byte8>>3)&3`, then clamps0..6. The existing record
contracts validate selected terms; do not invent their producers. Native
owner0's level helper does not query a home Crèche/Base. Human-owned units
retain their existing mode1/home logic.

Static core source`51bd4b`..`51bd85` applies early native protection before
turn15: **owner0 attacker width is divided by3; owner0 defender width by2**.
Only the defender-half case has the retained43-case full-battle validation.
Native offensive full-wrapper composition remains outside that receipt; the
attacker-third statement is source evidence for the shared transaction.

Native kill reward is not derived from stored morale. Static casualty helper
`519db0` at`51a101`..`51a13b` requires killed owner0, a valid positive-owner
killer and a reward accumulator, queries killed Unit`5d8f50(unit,1,0)`, then
adds **`10*(neutralEffectiveLevel+1)`** before deletion. Opponent context is0:
do not accidentally use an atrocity-boosted opposing-context battle level.
The full battle later adds the accumulated reward to the winner's treasury at
`51f3cc`..`51f3ee`. Retained full fights validate10 at neutral native level0;
the general formula and turn45 reward20 are static source extensions.

## Mixed same-owner ordinary military stack selection

Battle core calls complete`5187e0(seedDefender,attacker,mode)` at`51ad73`.
It follows previous-link`Unit+50` to the head, then next-link`Unit+48` through
candidates. It calls complete`516390` for **each candidate**: selection depends
on this attacker, not just the candidate's armor or morale.

For the checked conventional military plan3 candidates, no special priority
flags and reactor1, its exact scoring stages are:

```
Dhealth = truncate(candidateFinalDefense * remainingHealth / maxHealth)
ratio   = truncate(Dhealth * referenceAttack / candidateFinalAttack)
score0  = truncate(ratio / 8) - candidateRawWeaponPower
score   = ((score0 * 16) << 11) + candidateUnitId
```

`referenceAttack` is the initial`515ec0(attacker,seedDefender,0,0,0)` query;
it precedes candidate-specific composed modifiers. Use integer truncation at
each stage. This is selection arithmetic, not extra damage scaling inside
actual battle rounds. Native source has other plan/domain/priority branches;
do not apply this simplified profile formula to every civilian, aircraft or
flagged candidate. In particular the large priority adjustment at`518d8f`
must not be erased when that source path is admitted later.

The six controls establish: healthy Synthmetal beats Scout regardless of
linked-list order; otherwise equal candidates select **higher Unit ID**,
again independent of traversal order; Synthmetal at damage7 loses to healthy
Scout; equal-defense Laser candidate loses to Gun because of the weapon-power
subtraction. Scores and chosen IDs are in the JSON, independently recalculated.
This closes ordinary military defender choice, not mixed-owner admission,
collateral/stack-wipe policy or complete stacked-battle casualty effects.

The original round entry observations remain`51b008` (health/strength),
`51d3a2` /`51d3bb` (attacker/defender CRT draws),`51d3be` (strict comparison)
and`51d949` (round loop exit). Reuse their established two-modulo-draw kernel,
tie-damages-attacker rule, promotion and source-square retention contracts.
Private probes, component input receipt and bounded source are frozen at
`tmp/reference/independent-early-ground-combat-evidence-20261111/`.
