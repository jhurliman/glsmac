# RE-SMAC-CITIZEN-CONVERT-001: initial, psych and facility conversion

Status: original-x86 checked for the SMAC fingerprint in this directory's
README. Twenty named cases, 1,600 deterministic sampled combinations and 144
complete-prefix combinations pass. This specifies the first three conversion
stages. Police, later project effects, final citizen state and riots remain
outside the executed scope.

## Inputs and quantities

Use current population `P`, an already resolved excluded-population count `X`,
and ordinary citizen capacity `W = P-X`. The tests cover P=1..127 and X=0..P.
The upstream construction of X and of resolved psych points is outside this
handoff; do not infer specialist assignment or energy allocation from it.

Three values are retained: talents `T`, drones `D`, and additional drone
calming burden `S`. S is not a disjoint population group: do not subtract
T+D+S from population to obtain content citizens. The cancellation algorithm
below establishes its arithmetic role; visual classification of this burden
as particular citizen icons is not checked.

The other inputs are [ordinary/faction inputs](ordinary-citizen-inputs.md),
[bureaucracy input](bureaucracy-drone-input.md), current social TALENT rating,
resolved psych points, and the [facility/Paradise inputs](citizen-facility-project-inputs.md).
Use nonnegative ordinary, bureaucracy and faction talent inputs in this
contract. Negative custom faction parameters require separate integration
work; their earlier arithmetic alone does not prove later conversion behavior.

## Shared cancellation rule

At each stage below, normalize the three values against W as follows:

1. While T+D+S exceeds W, if T is nonzero, spend one talent. If S is nonzero,
   reduce S by one; otherwise, if D is nonzero, reduce D by one. Repeat.
2. If T is zero and D exceeds W, reduce D by one and repeat.
3. If T is zero and D is at most W, bound S to 0..D and stop.

If T+D+S is already at most W, leave all values unchanged. The third branch
can stop with T+D+S greater than W. That is intentional: S is additional
burden associated with drones, not another group occupying population slots.
Do not replace this procedure with a single clamp or subtract talents directly
from ordinary drones before accounting for S.

## Stage 1: initial conversion

Start from ordinary raw demand R, bureaucracy input B and faction talent
input F. Let A be the current social TALENT rating:

- `Q = clamp(R - min(A,0), 0, P) + B`
- `S = clamp(Q-P, 0, P)`
- `D = clamp(Q, 0, P)`
- `T = F + max(A,0)`

Apply the shared cancellation rule. Negative social TALENT adds demand before
the ordinary-demand cap; positive social TALENT adds talents separately.
Bureaucracy enters after that cap, allowing Q to exceed population and create
S. Excess ordinary demand alone does not create S.

For P=7, X=0, R=14 and B=0, the initial result is T=0, D=7, S=0.
Changing B to 3 produces T=0, D=7, S=3. Adding F=1 instead produces
T=0, D=7, S=2: the talent removes one extra step before reducing D.

## Stage 2: psych

Add `trunc_toward_zero(psych_points/2)` to T. Bound T and D separately to
0..P, retain S, then apply the shared cancellation rule again.

Thus 1 point adds zero talent input; 2 and 3 points each add one. This is a
stage input, not a guaranteed extra visible talent. For the default named
case P=7, R=6, B=0, 2 psych points result in T=1, D=6, S=0; the same points
against R=7 produce T=0, D=6, S=0 after cancellation.

Negative supplied psych values use signed truncation, then the same clamps.
The checks include -3 giving an input of -1. This is an arithmetic boundary,
not evidence that ordinary gameplay can allocate negative psych.

## Stage 3: local facility effects

Add the signed facility drone adjustment to D and Paradise talent input to T.
Bound D and T separately to 0..P, retain S, then normalize again.
The full facility/project input handoff defines how these inputs are built.
The later Human Genome Project/Clinical Immortality input is not included here.

The retained S matters when a facility reduces D: S is not recomputed from
the reduced D at stage entry. Normalization decides whether to spend talents
against it or reach the final S-to-D bound. Preserve the sequence, including
clamps at stage boundaries; merging psych, facilities and project talents
into one net adjustment is not equivalent.

## Evidence and remaining scope

The [20 named cases](citizen-conversion-stages-cases.json) provide three
expected snapshots each. The 1,600 sampled combinations use populations
1/2/3/7/20/127, zero/partial/full exclusion, demand/bureaucracy/talent boundaries,
signed social ratings, psych rounding and signed facility adjustments.
These execute the contiguous original conversion blocks with stage inputs
supplied. The private report additionally checks 144 complete-prefix runs
combining population, social rating, exclusion, psych, bureaucracy, faction
TALENT, Commons and Paradise Garden. The actual facility helpers execute in
those runs. No instructions or helper returns are modified.

These snapshots precede police processing and the later project/final stages.
The complete routine can change them again. Punishment Sphere is detected
earlier but its suppression is applied later; this handoff does not yet claim
its final effect. Full turn timing, excluded-population/psych construction,
citizen icons, riots and Golden Ages remain open. The subsequent project and
suppression stages are now checked separately in
[late projects and suppression](citizen-final-projects-and-suppression.md),
including complete current calculations at POLICE -2. The excluded count is
now identified as specialist count by the separately checked
[psych allocation/specialist-output loop](psych-allocation-and-specialist-output.md),
which also supplies psych to the original citizen consumer. Specialist
assignment/availability and earlier net-energy construction remain open.
