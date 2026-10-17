# Ordinary Colony production

The live conventional identity resolver now accepts the explicit Infantry /
Colonist mode8, offense0 / Scout armor / reactor1 / plan8 tuple with no abilities.
This is a separate Colony package. Support, saved home, morale, movement, sight
and restored records use the existing owned Unit machinery. A distinct cyan C
GLSMAC marker and fourth configured presentation role avoid borrowing Combat or
Former artwork; no original visual-fidelity claim is made.

Full `Base::ProcessSmacUpkeep` prepares the actual homed Pod while the producing
Base still has its old population, then debits the candidate population before
queue advancement, baselines and the population step. Housing is evaluated
against that post-debit population. The first nutrient surplus remains the
population-step input until the required internal or unconditional outer
calculation. Thus food20 can regrow a size-two Base after its Pod debit, while
food0 leaves population1. Final worker repair and Unit/Base publication remain
one prepared transaction. Empty human queues repeat the current Pod target.

The queue permits Colony only in the existing ordinary generated local human
campaign. Scenario/mapfile no-colony restrictions require their own producer
before admission. Size-one completion now checks the owner's stored difficulty:
Citizen and Specialist complete without population debit, including when the
production governor is active. Larger Bases still lose one population. At other
difficulties, governor activation plus production-management bit `0x1` defers
completion: minerals post, the queue stays unchanged, baselines update and the
ordinary food/growth tail runs. Neither a presentation resolver nor a Unit ID is
needed by deferral. Growth cannot retroactively complete that deferred attempt.

An explicit player choice (Delay, Base Screen or Accept abandonment) remains
unconnected and rejects before publication. The production-only intermediate
entry also rejects Colony; only full upkeep handles its population effects.
Automatic governor target selection is not implemented: the existing empty
human queue keeps its target, without inferring a universal Former replacement
from the captured Citizen case. No UI choice, saved pending-choice record,
network branch or protection setting is introduced by this automatic-branch fix.

Ordinary nonfungus movement uses the existing edge-cost and command pipeline.
Its validator now admits the Colony tuple; fungus encounters and foreign
noncombat ZOC remain unsupported. Actual movement checks distinguish forest6
from flat3 with allowance3. Saved continuation preserves the Pod's identity,
home, marker descriptor and spent byte without replaying manufacture.

The source semantics come from `docs/behavior/colony-production-loop.md`, the
ordinary land-training and support contracts, and the existing movement kernel.
Native tests are synthetic live compositions: two population/food cases, an
actual move/save/restore/next-upkeep continuation, atomic size-one/presentation
rejection, explicit default script role resolution and owned marker lifetime.
Sixteen affected groups passed before the discovered Colony edge-admission
fix; all three Colony and five edge groups pass after it. Twelve changed
production TUs plus the final edge TU analyze clean. No platform matrix or
new original-executable run was performed. Running second-site founding and
Pod consumption use their separate transaction.

The automatic branches use `docs/behavior/colony-choice-continuation.md` and its
13 semantic contexts. Three new live groups check stored difficulty 0/1 versus
global 3, population-one versus population-two debit, independent governor bits,
queue retention and no factory during deferral. The repeated governor-deferral
test composes the measured automatic branch with the established population
ordering: food19/minerals27 becomes size1/food20/minerals30, then
size2/food0/minerals33, then a Pod and size1/food0/minerals5. The original
three-upkeep replay supplied explicit Delay; this composition does not claim a
separately captured three-turn governor run. Both changed C++ translation units
pass focused syntax checks. Linked validation of this amendment is pending the
shared affected-test batch; its native filter is `^SMAC Colony` (six groups).
