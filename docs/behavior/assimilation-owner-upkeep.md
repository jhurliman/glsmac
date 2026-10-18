# RE-SMAC-ASSIMILATION-OWNER-UPKEEP-001: assimilation in ordinary upkeep

Status: two complete original SMAC owner upkeeps returned normally, with two
owned Base upkeeps each. Fingerprint: this directory's README.
[Semantic cases](assimilation-owner-upkeep-cases.json).

The captured population-one human Librarian Base uses its incoming conquest
assimilation timer in both citizen calculations during ordinary upkeep. After
both resource passes, production and population processing, the Base's final
view/cache checks run; then assimilation ages once before normal Base return.
Neither checked case recalculates citizens after that decrement, including
the remainder of owner upkeep. The subsequent faction summary sees the aged
value.

| Incoming timer | Current / former owner | Both citizen inputs | Timer at return | Former owner at return |
| --- | --- | --- | --- | --- |
| 50 | 1 / 2 | 0 | 49 | 2 |
| 1 | 1 / 2 | 0 | 0 | 1 |

The zero inputs follow the existing [assimilation formula](assimilation-timer.md):
population one and difficulty three cap the contribution at zero even while the
timer is positive. Expiration changes former owner after citizen processing;
it does not trigger another citizen pass in this caller. Both cases preserve
population one and nutrient storage five, and post one mineral from zero.
The other owned Base also returns normally; the defender's surviving Base is
not processed by this owner's upkeep. Neither control Base's timer changes.

The final common view/cache check invalidates remembered Base context before
aging. Pending Base zoom is supplied as zero, and the captured Base does not
match the finished-view Base, so neither its conditional redraw nor the modal
Base zoom and FINISHED log executes. Static ordering places decay after that
optional zoom/FINISHED path too; these runs do not dynamically prove that UI
branch. The aging block consumes no randomness. Each full owner upkeep uses
two CRT draws elsewhere, one in each owned Base's ecology processing; shared
RNG is unchanged.

The initial timer-50 Base state is produced by the actual
[undefended capture](undefended-base-entry.md), replayed to its Base-screen-entry
boundary. These are independent next-turn owner-upkeep invocations on that
state, with active owner one and a closed headless Base view supplied. They do
not continue the capture screen, capture return or outer movement return.
Timer one is supplied aged history, not a replay of 49 turns. No gameplay
calculation return or branch is supplied; inherited presentation/runtime
services remain explicit in private evidence. General capture duration,
matching-owner decay and early deletion exits retain their separate evidence
and scope in the linked timer contract.
