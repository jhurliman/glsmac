# RE-SMAC-CLEAN-M-001: clean-mineral history and intermediate ecology

Status: original-x86 checked for the SMAC fingerprint in this directory's
README. Thirty-seven named cases and 720 additional combinations match the
specified original fragments and facility helpers. AI rollback checks supply
explicit RNG return values. Bloom selection/placement, complete construction
and final eco-damage do not execute in these checks. Full-game confirmation
remains pending.

This adds history transitions and the next stage after
[terraforming ecology input](terraforming-ecology-input.md). The intermediate
burden below is **not** displayed eco-damage or a bloom probability.

## Ecological history transitions

Use a faction-wide stored history value `H`. The later
[input-lifetime receipt](ecology-input-lifetimes.md) establishes fresh clearing,
nonzero initializer preservation and ordinary save/load retention. Complete
capture lifecycle remains open; the following transitions are
established when their specified original event stage is reached.

For Tree Farm, Hybrid Forest, Centauri Preserve or Temple of Planet completion:

- If H is zero, leave it zero.
- If H is nonzero, add one.
- Other facility completions do not increment it through this examined branch.

Thus building those facilities before H becomes nonzero does not retroactively
earn a history increment when it later becomes nonzero. This increment is an
event, not a recalculation from how many facilities currently exist. Capture,
demolition and rebuilding legality still require separate transition evidence.

When the ecological fungus-growth path selects a qualifying target, it adds
one to H before processing the growth. At the later history-retention stage:

- Human factions keep that increment without an RNG call at this stage.
- For AI, obtain a random integer. If it is odd, keep the increment.
- If it is even and global difficulty is zero, keep the increment without a
  second draw.
- If it is even and global difficulty is positive, obtain a second random
  integer. Keep the increment if its remainder modulo `(difficulty + 1)` is
  zero; otherwise undo this event's increment.

Difficulty here is the global zero-based level. These rules define operations
on supplied nonnegative RNG outputs; they do not claim exact probabilities or
specify the RNG algorithm. The AI rollback occurs after the earlier growth
stage; do not infer from the history rollback that fungus placement was undone.
Which events select a target, how many tiles change and native-unit spawning
are outside this contract.

## Clean allowance and terraforming

For normal nonnegative history:

1. Begin with clean allowance `C = 16 + H`.
2. Take the scaled terraforming input T from the preceding specification:
   sum its per-square points, then divide by eight rounded downward.
3. If T is positive, consume the smaller of T and C from both. Otherwise leave
   both unchanged, including a negative T.

The result is residual terraforming R and unused clean allowance C. The
allowance is applied to terraforming first, so it cannot also shield the same
amount of mineral production. Negative forest input is retained at this stage.
For example, raw points 80 become T=10. With H=0, this leaves R=0 and C=6.
Raw points 160 become T=20, leaving R=4 and C=0.

## Mineral term and facility divisor

Start an ecological divisor D at one. Add:

- One if a Nanoreplicator is present **or** the faction owns Singularity
  Inductor. Both together add only one.
- One for Centauri Preserve at the base.
- One for Temple of Planet at the base.
- One if the faction owns Pholus Mutagen.

These effects are additive; the maximum here is D=5. Project ownership applies
faction-wide. These are current presence/ownership checks, separate from the
history increments earned by past construction. Singularity Inductor's
substitution here is for the Nanoreplicator divisor condition, irrespective of
its separate Quantum Converter substitution in mineral production.

Let S be the smaller of faction mining-satellite count and base population.
If either value is negative, use zero for S. Then calculate:

`intermediate burden = R + truncate_toward_zero((processed minerals - S - C) / D)`.

Processed minerals are gross after ordinary mineral multipliers and incoming
convoys, before the late Space Elevator orbital doubling. Unit support and
outgoing mineral convoys are not subtracted from this gross input. The satellite
deduction uses the capped satellite count, not an independently multiplied
mineral value. The [orbital intake rule](orbital-resource-intake.md) can halve
satellite output before its population cap when neither Aerospace Complex nor
faction-owned Space Elevator applies. This ecology deduction instead uses the
capped count described above; do not substitute that smaller intake amount.
Satellite construction and count lifecycle remain separately researched.

Only the mineral term is divided by D. Residual terraforming R is outside that
division. The mineral term and resulting intermediate burden can be negative;
do not clamp them prematurely. Later ecology code adds other terms, clamps and
scales the result, which is why this value must not be presented as final damage.

## Examples and limits

With H=0, no terraforming, no satellites and no divisor effects, processed
minerals 20 give burden 4. One divisor effect gives 2. All four independent
divisor increments give D=5 and burden 0 after integer truncation.

At processed minerals 11 and D=2, the mineral term is `-5/2`, giving -2 rather
than -3. This differs from the earlier arithmetic-floor division of terraforming
points by eight. With raw terraforming -1, no divisor effects and minerals 20,
the retained residual -1 reduces the intermediate burden to 3.

The [37 acceptance cases](clean-minerals-and-ecology-burden-cases.json) include
construction history, human/AI retention and RNG consumption, clean allowance,
divisor stacking/substitution, satellite cap and signed rounding. The 720-case
grid compares each arithmetic intermediate across history, terraforming,
mineral intake and facility/project combinations.

Final human/faction/global scaling is now specified in
[eco-damage scaling](eco-damage-scaling.md). History initialization, full event
selection and effects, capture/demolition/save lifecycle, extreme overflow,
SMAX and full-game observations remain open.
