# RE-SMAC-ASSIM-TIMER-001: conquest assimilation contribution and decay

Status: original-x86 checked for the SMAC fingerprint in this directory's
README. Sixteen named cases and 6,144 additional combinations execute the
citizen prefix with an active conquest assimilation timer, its local upkeep-tail
decay and a same-population recalculation. Capture initialization, other upkeep
phases and final citizen processing have not been executed.

Follow-up: [ordinary owner upkeep](assimilation-owner-upkeep.md) now checks two
complete owner upkeeps on an actual capture-produced population-one Base,
including timer50→49 and expiration1→0. It establishes the ordinary caller's
two citizen passes before one tail decay and normal return. The original
fragment/grid scope and other lifecycle limits below remain unchanged; the
optional modal FINISHED view path was skipped in the follow-up.

## Raw citizen contribution

Let P be population, C the stored conquest assimilation timer (0 through 255),
and D the human faction's difficulty index, or 3 for AI. The contribution is:

`min(max(trunc_toward_zero((P + D - 2) / 4), 0), ceil(C / 10))`

For the nonnegative timer, calculate the second term as `floor((C+9)/10)`.
This contribution is added after the ordinary difficulty input and before the
explicit faction DRONE parameter. TALENT remains a separate input. See
[ordinary citizen inputs](ordinary-citizen-inputs.md) for those adjacent stages.

The timer bands alone are zero at C=0, one at C=1..10, two at C=11..20, and
so on, but the population/difficulty term can cap the contribution more tightly.
For example, population 7 at difficulty 5 caps this
input at 2 even with a much larger timer. A small low-difficulty base can
have zero contribution while its timer is still positive.

Former-owner identity does not enter this contribution formula. A positive
timer can still contribute when former and current owner match. Matching
owners changes the decay rate below; it does not immediately zero the input.
These are raw drone inputs, not final visible drones or riots.

## Local upkeep-tail decay

Each execution of the checked upkeep-tail block applies:

1. If the timer is already zero, skip this block, including former-owner changes.
2. Otherwise subtract one.
3. If former owner equals current owner and the timer is still positive,
   subtract one more.
4. If the timer has now reached zero, set former owner to current owner.

Thus a positive timer decreases by one for different owners and by up to two
for matching owners, without underflow. A matching-owner timer of 1 ends at
zero. A different-owner timer of 1 ends at zero and updates the former owner.
A synthetic stale different-owner identity with timer already zero stays
unchanged in this block; do not add unconditional normalization.

At population 7/difficulty 5, C=11 gives input 2. Ordinary different-owner
decay changes C to 10 and same-population recalculation gives input 1.
Matching-owner decay changes 11 to 9 and also gives input 1. The tests hold
population constant to isolate this transition; other upkeep phases can
change population or other inputs.

## Identification and lifecycle limits

The original editor labels the fields “Former Owner” and “conquest assimilation
timer,” supporting their identity. This does not establish the initial timer
for every capture, liberation, transfer, recapture or scenario. In particular,
an editor default must not be treated as a universal capture duration.

The decay is statically located near the tail of base upkeep after its finished
base-view refresh path. The harness executes that block directly, not a full
turn. Exact refresh timing, every call frequency and interactions with capture
or subsequent population changes remain open.

## Acceptance scope

The [16 named cases](assimilation-timer-cases.json) cover timer bands, 1/2/0
expiration, matching/different owners, stale zero identity, human/AI difficulty
and high unsigned timer values. The grid covers every C from 0 through 255,
human/AI, stored difficulties 0/5, populations 1/7/20 and both owner relations,
cycling all seven faction slots. It verifies the whole base record changes
only in the timer and, when required, former-owner identity.

Faction DRONE/TALENT and bureaucracy are neutral in these checks. No arithmetic
helper return is substituted or original instruction modified. Full capture
initialization, subsequent citizen conversion/calming, complete upkeep/turn
sequencing and original-game observations remain open.
