# Ecology after research: cache, event admission and first bloom

RE-SMAC-ECOLOGY-HANDLER-CACHE-001, 2026-10-18. Pinned stock SMAC.
[Literal native controls](ecology-handler-research-cache-cases.json).

The current Base's damage can remain **pre-research** even when that Base's labs
actually acquire a technology. Acquisition is not an unconditional ecology
refresh. Preserve the original cache/caller boundary before connecting the live
ecological-event handler.

## Post-research resource cache

Base upkeep calculates resources, performs production/population and its second
resource calculation, processes research, then compares the remembered resource
Base ID with this upkeep Base ID. The key is **one signed Base ID only**. If equal,
it calls ecology using the stored final damage. If different (including invalid
âˆ’1), it restores this Base selection, records its ID and reruns Support â†’ intake
â†’ nutrients â†’ minerals/ecology â†’ energy/citizens before the event handler. This
conditional third calculation does not post another turn's mineral collection.

Four native upkeep prefixes stop at actual ecology entry:

| Control | Acquisition score | Ownership | Damage at ecology entry | Resource passes |
| --- | --- | --- | --- | --- |
| No award | 49 | target unowned | 0 | 2 |
| Actual award, off-view human | 49 â†’ 51 | target becomes owned | **0** | 2 |
| Actual award, viewed human | 49 â†’ 51 | target becomes owned | **0** | 2 |
| Same award; diagnostic cache invalidation after research | 49 â†’ 51 | target becomes owned | **1** | 3 |

The diagnostic row supplies only the invalid cache key; acquisition and the
conditional resource calculation execute natively. Resolved gross minerals17,
terraforming burden0, H0, PLANET0, score49, future counter0, Citizen difficulty,
rare native life and no projects isolate the rounding threshold: score49 gives0;
score51 would give1. They are declared controls, not a naturally developed
high-mineral campaign. The technology score changes through actual acquisition,
not a supplied score/result after research. Announcement/quote presentation,
object cleanup and subsequent target-picker answers are headless services.

Two subsequent native query controls after the retained-damage award prove that
an ordinary same-Base refresh executes zero mineral stages and retains0; a
forced refresh executes one and produces1. General UI invalidators, Base switches
and special acquisitions may change the key; **do not extrapolate these ordinary
human controls to every acquisition/UI path or AI award**. Do not add a universal
post-technology refresh or silently recompute damage inside the event handler.
See [general cache contract](base-recalculation-and-cache.md).

## Entry order, threshold and exact suppression inputs

Every handler invocation first adds
`trunc(4 * storedDamage / clamp(4 - currentPLANET, 1, 99))` to the owner's
retained aggregate, then consumes one **existing CRT** draw, even for damage0.
Let `r = rawCRT % 100` and
`t = trunc(storedDamage / (activePerihelion ? 2 : 1))`.
An ecological event proceeds only when **r < t**. Equality suppresses it.
Perihelion affects this comparison in addition to the earlier damage producer;
do not double stored damage again here. These controls use nonnegative damage.

After that draw/comparison, event processing is suppressed by either:

- Primary saved rules mask `0x00200000` set (native-life events disabled).
- Secondary saved state mask `0x1` set while `0x8` is clear (editor hold; setting
  both permits playtest event processing).

Active perihelion is secondary mask `0x00100000`. The disabled general random
events option is not this handler's native-life gate. The native-life abundance
category is an earlier damage input, not a replacement for the rules gate.
History, major atrocities, terrain eligibility and difficulty do not add a
pre-draw admission test on the traced ordinary human entry path.

D0/r0, D49/r50 and D50/r50 each consume exactly one CRT draw and produce no bloom.
D51/r50 proceeds. Perihelion D101/r50 fails (threshold50); D102/r50 proceeds
(threshold51). Both global suppressors still allow aggregate posting and the
first draw. No eligible tile also retains H and consumes only that draw in the
measured all-fungus fixture.

## Saved lifetimes and empty-owner pass

Primary rules, secondary phase and all eight faction status words round-trip
through actual ordinary save/load bodies. Two file controls restore 20 words
(ten per file) after poisoning. The faction's status mask `0x4000` is a separate
post-bloom one-shot hold: if present, the handler clears only that bit and returns
**after H/terrain mutation, before native spawning/warming**. It is not a pre-draw
or pre-bloom veto. Failed threshold leaves it set; even H0 â†’ 1 consumes it after
bloom. The upstream action that sets this particular bit is outside this bounded
receipt; do not synthesize it from H, facilities or terrain.

Four actual faction initialization prefixes show the entire status word clears
to0 in fresh turn-zero initialization and preserves its canary on nonzero-turn
initialization. Existing [H/atrocity/aggregate lifetimes](ecology-input-lifetimes.md)
and [perihelion world reset](perihelion-new-world-and-storage.md) remain the
separate reset authorities. Editor mode admission across full frontend loads is
not established by these ordinary file controls.

The actual owner Base-pass prefix clears the active owner's aggregate **even
when no owned Base runs**: both zero global Bases and two foreign-only Bases
change owner aggregate777 â†’ 0, preserve peer888, and enter no Base upkeep. This
clear precedes traversal; it is not conditional on finding the first owned Base.
These controls stop after the Base loop, before later owner continuation or
elimination. Existing [two-Base aggregate ordering](base-ecology-research-aggregate.md)
is reused: research precedes each contribution; previews do not post it.

## Ordered bloom selection

The literal fixture contains all48 ordered offsets (excluding the center).
Coordinates wrap horizontally once unless flat; invalid coordinates are skipped.
Existing fungus on altitude class â‰¥2 is excluded; deep-water fungus is not
excluded by that test. A real Base item with affiliation nibble0..7 is excluded.

For the first24 offsets start weight0. For outer24 candidates, consider them only
while best score is0, and only if the nearest global Base is this Base; start
weight1. Nearest ties choose the later Base record. The distance is
`max(dx,dy) - floor((floor((dx+dy)/2) - min(dx,dy) + 1)/2)` with wrapped absX and
absY. Add2 for each of the four diagonal neighbors containing fungus at altitude
class â‰¥2. Zero weight skips without a draw. If weight>1, add the number of set
improvement bits in `0x00288050`, plus1 for the **first eight candidate offsets**. This bonus is applied only in
that weight>1 branch; it is not added to an isolated outer candidate of weight1.
For sea class<3 use `floor((weight+1)/2)`, then
multiply by10. Draw `rawCRT % weight` when weight>1. Replace the chosen candidate
only for a **strictly greater** score, preserving earlier score ties. Best0 means
no selected candidate even if eligible candidates drew0.

A selected candidate increments H once **before** the per-tile protection checks.
The installed Planet fixture has Base(64,16), selected offset17/tile(63,19),
winning score39. Its first roll raw250 is followed by eleven candidate draws:
`18237,2761,26456,23942,4890,14066,5852,2359,24204,29007,30964`.
The fixture records every candidate's weight, result and prior best. No shared
RNG entry occurs in any of the eighteen handler controls.

## Mutation and first native spawn

On the measured land bloom, the selected cell is rechecked for existing eligible
fungus/Base exclusion, then landmark protection:
`(landmarkWord & 0x80000002) == 2` suppresses the tile write. H has already
incremented, so a protected winner gives H0 â†’ 1 without replacing it with a
runner-up. Setting the sign bit makes the same low-bit landmark writable.

The cell receives fungus bit `0x20`, then clears exactly `0x00288050`: Mine,
Solar Collector, Farm, Soil Enricher and Forest bits. Road, tube, condenser and
borehole are outside that removal mask. An explicit **post-selection** improvement
control supplies those bits to the already selected cell: `0x0168805c â†’
0x0140002c`. It tests mutation independently of their effect on candidate weight.
For deep class<2 the source also calls the terrain producer targeting class2;
complete deep-water changes are not part of the executed land controls.

Source-only sea extension: when selected class<3 and new Hâ‰¥4, compute
`n = 10 - trunc(H/8) - ownerDifficulty`. For nâ‰¤1 choose nine cells without a
draw; otherwise another CRT draw with remainder0 modulo n chooses nine, any
other remainder chooses one. Only non-land neighbors are eligible after the
center. This branch requires a separate sea mutation receipt before claiming
complete sea bloom parity.

After human bloom processing, the one-shot status hold is consumed first.
Otherwise H<7 returns. Hâ‰¥7 with **any Voice project host marker other thanâˆ’1**
returns before spawning/warming. This post-bloom suppression does not require
the earlier damage-doubling rule. The measured H6 â†’ 7, no-project land control
creates five real design8 native Mind Worm units owned by neutral faction0,
at (63,19), (65,19), (64,20), (63,21), (62,20), each tagged with ecology owner1.
The first local sweep examines at most min(H,9) cells. Current population3 does
not cap that first sweep; it participates only in the later fallback condition.
All five actual factory calls and their resulting records are preserved in the
fixture. Neither shared RNG nor extra CRT draws occur after candidate selection
in this human land fixture. Counter warming0 â†’ 1 occurs; climate/sea offset does
not change. H<7, Voice suppression and one-shot hold leave warming0.

The source contains alternate sea/region/design and fallback limits, AI rollback,
and later high-H climate stages. They are mapped privately but are not certified
by these first land-spawn controls. Major-atrocity canary3 and stored final
damage remain unchanged in every handler control. This receipt does not implement
or certify full climate evolution, ecological AI, general spawn policy or natural
campaign arrival at the supplied damage/history values.

## Port sequence

Keep the current resource cache semantics â†’ research continuation â†’ conditional
third calculation â†’ aggregate contribution â†’ one CRT threshold draw â†’ saved gates
â†’ deterministic ordered candidate draws â†’ H increment â†’ protected terrain mutation
â†’ one-shot/H/Voice gates â†’ admitted native creation/warming. Preserve these as
separate stages. Never post a second aggregate or consume ecology RNG in an
Inspector preview. Existing frozen arithmetic, aggregate and RNG receipts remain
unchanged. No live GUI, production edits or broad standing-goal work was performed.
