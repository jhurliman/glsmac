# RE-SMAC-RESEARCH-SUBMIT-001: per-base research submission

Publication scope: this repository includes this handoff and its literal cases. Other research notes named as unpublished follow-ups below are outside this published evidence boundary and do not expand its claimed scope.

Thirteen controlled contexts pass sixty-five complete original submission-wrapper
returns, with actual research-add calls kept below a supplied cached threshold.
See [acceptance sequences](research-submission-scaling-cases.json). Three additional
original energy-stage prefixes distinguish Skunkworks from Hologram Theatre. This closes
the scaling stage between final base labs and the existing
research-add/completion contract (unpublished follow-up `technology-ownership-and-secrets.md`).

## Order and arithmetic

Begin with the base's final labs output. If Punishment Sphere is present, halve
that value, truncating, before the following gates and scaling.

Suppress this submission's raw labs to zero when either condition holds:

- Global turn is less than minus five times current social RESEARCH.
- Global turn is below five and the faction's stored difficulty is below two.

The first comparison uses the current, unclamped RESEARCH rating. Thus rating
-6 suppresses turn29 even though its later scaling clamps to -5; turn30 passes.
At RESEARCH -2, turn9 suppresses and turn10 passes. Ordinary Librarian
difficulty3 does not take the second suppression. Add the resulting gated raw
labs to the faction's raw-lab statistic before applying the social multiplier.

For nonnegative lab values in these ordinary contexts, let R be current
RESEARCH clamped to -5..5, B the gated labs, and F the faction's retained
fraction in hundredths:

1. Compute `N = B * (10 + R) * 10`.
2. Start the integer increment at `floor(N / 100)`.
3. Add `N % 100` to F. If F reaches 100, subtract 100 and add one to the increment.
4. Store F and submit the integer increment to research-add processing.

The fraction belongs to the faction and carries between base submissions.
Do not round each base's socially scaled labs independently or discard the
fraction at the end of each base upkeep. Allocation rounding and facility lab
multipliers happen earlier; this is an additional social scaling stage.

Five successive submissions of one raw lab give:

| RESEARCH/context | Submitted integer increments |
| --- | --- |
| 0 | 1, 1, 1, 1, 1 |
| +2 | 1, 1, 1, 1, 2 |
| -2 after the early-turn gate | 0, 1, 1, 1, 1 |
| +6, scaled as +5 | 1, 2, 1, 2, 1 |
| -6 at turn30, scaled as -5 | 0, 1, 0, 1, 0 |
| +2 with initial fraction90 | 2, 1, 1, 1, 1 |

Punishment Sphere with three raw labs and RESEARCH +2 gives the same sequence as one
raw lab without Punishment Sphere, proving halving before scaling. Research disabled
by the checked game rule skips the wrapper's processing entirely: no research-add
calls, raw-lab statistic changes or fraction consumption occur. An ordinary
suppressed/zero-output submission still reaches research-add with increment0.

## Scope

Inputs explicitly supply resolved social rating, global turn, stored difficulty,
final base labs, Punishment Sphere presence and fraction. Original facility testing,
wrapper arithmetic, faction statistic/fraction writes and research-add bodies
execute. Progress starts40 with a valid unowned target and cached cost1000;
all cases stay below completion. Award/chooser timing follows the linked
consumer receipt and is not newly exercised. This is not a full new-game
income/research replay or proof of every producer of these resolved inputs.

## Facility identity clarification

These facility IDs use the installed one-based facility table and the original
facility-helper argument directly: Skunkworks10, Hologram Theatre11, Genejack
Factory22 and Punishment Sphere23. Do not shift all IDs to compensate for an
incorrect prose label.

The research-halving helper tests Punishment Sphere23. A separate Genejack22
control with three base labs and RESEARCH +2 submits3,4,3,4,4, so Genejack does
not take that halving branch. The earlier version of this receipt incorrectly
named its measured bit23 control Genejack; the numeric observations are now
correctly labeled and a neighboring control has been added.

For psych, the existing contract/checker use Hologram Theatre11. Direct energy
stages with net energy4, psych5/labs0 tenths give psych2 without either facility,
psych2 with Skunkworks10, and psych3 with Hologram Theatre11. This verifies the
identity and the tested psych multiplier; other Skunkworks effects are separate.
