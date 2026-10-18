# RE-SMAC-ECO-SCALE-001: final eco-damage scaling

Status: original-x86 checked for the SMAC fingerprint in this directory's
README. Thirty-four arithmetic cases, two acquisition-counter transitions and
576 additional combinations match original fragments. The arithmetic executes
from the clean-allowance stage through the stored eco-damage result without
substituted helper returns. Full-game observation remains pending.

This completes the calculation from resolved inputs. It does not complete
technology-score initialization, atrocity recording, climate/event scheduling,
bloom selection or consequences. The result is a stored ecology value; do not
infer a complete event probability or UI behavior without those later rules.

## Inputs

Begin with intermediate burden B and ecological divisor D from
[clean allowance and mineral burden](clean-minerals-and-ecology-burden.md).
Other inputs are human/AI status, faction and global difficulty, major-atrocity
counter A, cumulative sea-level offset W, active Worldbuilder Sea Level Rises
parameter Q (installed default 5), native-life setting L (0/1/2 for sparse/
average/abundant), effective PLANET, active perihelion, whether Voice of Planet
exists anywhere, and whether Transcendence victory is enabled.

The [perihelion schedule](perihelion-schedule.md) now establishes its calendar
activation and expiry, including compositions through this scaling stage.

W and Q use the original climate system's common units. Their update and
conversion to visible sea-level measurements remain separate research; do not
substitute an independently converted altitude value in this calculation.

The technology inputs are stored acquisition score S and future-technology
counter F. **Do not replace S-F with a count of owned technology records.**
The examined acquisition fragment adds 2 to S and, for Transcendent Thought,
adds 1 to F. Its full initialization/setup and award-entry lifecycle remain
open. Selected editor/award transitions are now specified in
[technology-score transitions](technology-score-transitions.md).
The subtraction therefore does not prove that future-tech acquisitions
have no ecology effect. Fixtures supply these stored values explicitly.

## Human term and nonnegative burden

For humans only, calculate:

`Z = floor(clamp(W,0,100) / clamp(Q,1,100)) + D`

`B = B + truncate_toward_zero(5*A / Z)`.

AI skips this addition. The original uses the faction's major-atrocity counter;
what actions record an atrocity and whether they change that counter are not
specified here. D also reduces this human term. A negative or zero sea-level
offset behaves as zero; the offset and rule parameter are bounded separately.

After the human term, floor B at zero. Do not clamp the earlier intermediate
burden first: the human addition can offset a negative intermediate.

## Global and faction scaling

1. If Voice of Planet exists anywhere **and** Transcendence victory is enabled,
   double B. This test does not require faction ownership of the project.
2. During active perihelion, double B again. Both effects together multiply B
   by four before the subsequent scaling and rounding.
3. Select difficulty factor K:

| Context | K |
| --- | --- |
| Human Citizen (index 0) | 2 |
| Human Specialist, Talent or Librarian (indices 1..3) | 3 |
| Human Thinker or Transcend (indices 4..5) | 5 |
| AI | `clamp(6 - global difficulty index, 1, 3)` |

Human difficulty comes from the faction's stored setting. AI uses global
difficulty. Thus normal global indices 0..3 give AI factor 3, index 4 gives 2,
and index 5 gives 1.

4. Clamp PLANET to -3 through +2 and calculate the integer product:

`X = (S-F) * (3 - clamped_PLANET) * (L+1) * B * K`.

5. Compute `Y = truncate_toward_zero(X / 6)`.
6. Store `truncate_toward_zero((Y + 50) / 100)` as eco-damage.

Retain this operation order and intermediate rounding. For ordinary
nonnegative inputs, the last step rounds to the nearest hundredth of Y with
half rounding upward. There is no final 100 cap: stored eco-damage can exceed
100. The PLANET multiplier saturates at 6 for PLANET <=-3 and 1 for PLANET >=2;
high positive PLANET does not make this multiplier zero.

## Examples and evidence boundaries

At B=1, S-F=49, PLANET=0, sparse native life and human Citizen difficulty,
X=294, Y=49 and final damage is 0. Raising S-F to 50 gives X=300, Y=50 and
final damage 1. Global doublings occur before these rounding steps.

The [fixture file](eco-damage-scaling-cases.json) has 34 complete-input
arithmetic cases and two separately scoped acquisition-counter transitions.
It compares intermediate burden, human addition, global doublings, difficulty
factor, division-by-six result and final stored value. The 576-case grid varies
human/AI, all normal difficulty indices, PLANET clamp boundaries, native life,
Voice of Planet and perihelion.

All inputs in the arithmetic contract are resolved state. The following remain
open: exact score/atrocity initialization and all writers, global climate units
and updates, full perihelion turn placement and initialization, full bloom/native
consequences, UI display,
extreme signed overflow, SMAX and controlled full-game observations.
