# Human Workshop normal-open with active PSI designs

Active custom designs containing negative weapon or armor strengths are now
admitted by `workshop_human_producer_reference.proposals` and `normal_open`.
The original blanket conventional-only rejection has been removed. The
[existing human producer evidence](design-workshop-human-producer.md) remains
unchanged; this is an additive extension for reopening after PSI saves.

## Port changes and input authority

The focused port changes are: admit active PSI records, retain signed prefix
arithmetic, preserve the existing human preview availability gates, and pass
the real PSI bonus/project context into capability refresh. Human preview does
not need an additional PSI-strength veto or a Scout fallback.

`normal_open` adds optional `psi_percent`, `ratio`, `dream_twister_owner` and
`neural_amplifier_owner` arguments to the existing refresh composition. Defaults
retain the prior neutral-project fixture behavior. These are current engine
inputs, not properties inferred from component names or faction names.

| Source authority | Meaning |
| --- | --- |
| Definition95a9c4 + owner*59c, definition offset4fc | Innate PSI percentage bonus |
| Rules9ba48c : 9ba498 | Land PSI offense:defense ratio, installed3:2 |
| Faction95d9d0 / 95d9d4 + owner*20bc | Retained foreign available attack / defense maxima |
| Project host index95a068 / 95a040 | Dream Twister / Neural Amplifier; resolve host Base owner |

Keep project host indices distinct from owner IDs. A zero host index means
Base0, which can be owned; it does not mean no project. The focused fixture
explicitly sets absent hosts to-1 and separately declares owned Base0 hosts in
the project control. This avoids silently enabling project bonuses.

## PSI evaluation and human scoring

Original597390 evaluates a negative proposal weapon/armor before scanning the
pool, after its early category-filter rejection. For an existing candidate it
evaluates strengths only in the same-chassis, same-reactor comparison branch
whose ability XOR has no `0x48841a` bits. Other candidates can still reach the
ordinary replacement score without taking that comparison branch.

For any signed negative component power, use the current retained foreign
maxima at this stage:

1. Weapon: truncate `foreignDefense * offenseRatio / defenseRatio` toward zero.
   Armor: truncate `foreignAttack * defenseRatio / offenseRatio` toward zero.
2. Apply `trunc(value * (100 + psiPercent) / 100)` separately.
3. An owned Dream Twister for weapon, or Neural Amplifier for armor, adds
   `trunc(value / 2)` to that already rounded result.

Nonnegative powers return their signed raw value. The comparison does not use
reactor, morale, PLANET, damage or combat odds. The independent audit helpers
`strategic_strength` and `psi_bonus` in the preview-policy reference implement
these stages. This is the same established
[PSI capability algebra](psi-capability-estimates.md), but cache timing differs:
577140 uses the **previous** foreign maxima for rebuilding own capabilities;
597390 subsequently reads the foreign maxima rebuilt by that refresh.

Crucially, the strength-comparison rejection at597839..59788e is **AI-only**.
The owner bit in959ff0 sends human callers directly to the ordinary score
block. PSI estimates therefore do not change the human mode1..3 candidate
existence gates. The existing `submit` implementation remains valid for PSI.
Do not import the AI comparison veto into the human preview policy.

For an active own nonmatching record, after earlier shortcuts:

- Excluded with no units reaches score1024, even if in production.
- Different category with any units or production does not score.
- Otherwise start at576 without units, or64 with units; OR256 if excluded,
  OR128 if not in production; subtract `clamp(costByte*unitCount + productionCount,0,63)`.

Consequently every reached ordinary score is positive: its minimum is **1**.
Inactive own records score2048. The earlier excluded/no-units/no-production
shortcut scores8192 for matching category or4096 otherwise. Exact matching
and intrinsic admission retain their previously documented precedence and
unexclusion side effects; an equal PSI estimate does not make different
component IDs an exact match. `human_score` documents the final score block;
`submit` uses its equivalent candidate-existence gates.

## Upgrade prefix stays signed

The597de0 upgrade detector compares **raw signed** component strengths, not
the strategic PSI estimates above. Candidate weapons still require0<power<99
and exclude weapon20; candidate armor still requires power>1. Existing negative
components can establish that a positive candidate is an upgrade. A PSI weapon
can therefore be proposed for replacement by a conventional weapon while PSI
armor is retained, according to the existing plan-specific branches.

The hybrid-plan1 armor-budget division is signed. A retained PSI weapon can
remain the divisor when no replacement weapon candidate was found. The
reference now uses truncation toward zero rather than asserting positive power;
the existing minimum-budget rule then yields2 for a negative divisor. A
declared minimal-registry stage control keeps weapon12 and upgrades armor to1
(Synthmetal), producing `[Infantry, PSI, Synthmetal, abilities0, Fission]`.
Its retained plan override and reduced registry are explicit stage inputs, not
a claim about a natural initial game. The12 ordinary reopen controls separately
exercise populated installed registries.

## Native controls and frozen handoff

Additive bundle:
`tmp/reference/independent-workshop-psi-reopen-evidence-20261021/`.
Manifest SHA-256:
`10b7250e197c36848842feaebe3a9a85470d95f5a53757bfad79d223675e7860`.

The15 focused controls consist of12 full4490b0 normal-New opens, two full-bank
597de0 boundaries, and the one negative-divisor prefix boundary. They match:

- All **84 ordered submissions, return values and preview state transitions**.
- Complete Design and Faction pools; Base/Unit/history buffers remain unchanged.
- Final draft fields and selector slots for all12 normal-New opens.
- **1,023 exact score updates**, including the full-bank score1 boundary.
- **404 preview PSI evaluations** with independently derived ratio inputs and
  **412 total515e10 bonus-stage checks**, including capability calls.

The controls distinguish weapon PSI, armor PSI, both, sea, air, excluded unused
and live records, positive and negative upgrade paths, retained plans, foreign
maxima, PSI percentage, owned projects and full-bank category admission.
The full-bank inputs retain plan3 to skip repeated upgrade-prefix requests;
they test candidate existence without inventing an inactive UI slot.

The prior22 producer controls/414 submissions and15 normal-open controls/71
submissions still pass independent replay from their frozen observations.
No prior native matrix or renderer matrix was repeated. The public
[case index](design-workshop-psi-reopen-cases.json) summarizes inputs and
expectations; complete pools, source disassembly, scalar-stage observations and
frozen references remain in the private bundle.

This extension covers stock human preview modes1..3 and the declared PSI
refresh inputs. AI replacement behavior, alternate tables/rules, zero ratio
denominators, overflow/extreme strategic states, and full-parent Workshop
initialization/compositing remain outside this evidence.


## GLSMAC producer integration

The C++ human producer now admits active PSI custom records and retains the
raw signed hybrid-plan denominator. Zero divisors still fail explicitly.
No PSI estimate or AI comparison veto is added to human candidate admission.
Fifteen declared caller contexts cover all84 original submissions and states;
context fields are independently refreshed from the input Faction pools.
The12 normal-open captures also contain4490b0's subsequent selected-record
bit80 clear: the test composes that separate caller effect for the full-pool
comparison and separately verifies the producer does not consume that flag.
One original first-call controller reactor is stale zero; the port supplies a
valid Fission draft at the detached boundary. The first admitted proposal
replaces it, and every captured published tuple remains checked unchanged.
This is producer validation, not a complete live normal-open implementation.

Private bundle manifest SHA256:
`10b7250e197c36848842feaebe3a9a85470d95f5a53757bfad79d223675e7860`.
The complete focused group includes the prior414 producer and23 filtered
submissions, for521 native expectations. The C++ checks do not reproduce the
reference's1023 individual scoring writes or404 PSI helper evaluations; human
preview uses their equivalent positive-candidate existence rule.
