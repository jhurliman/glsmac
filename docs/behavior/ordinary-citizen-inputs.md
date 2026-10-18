# RE-SMAC-CITIZEN-INPUT-001: ordinary and faction citizen inputs

Status: original-x86 checked for the SMAC fingerprint in this directory's
README. Twenty-one named cases and 2,700 additional combinations execute the
complete citizen prefix through ordinary and faction input calculation.
The separate base-local timed modifier is held at zero and bureaucracy input
is zero. Later facilities, psych, conversion, calming and riots are unexecuted.
These outputs are intermediate inputs, not final visible citizens.

## Difficulty-based ordinary input

For a human faction, use its stored difficulty index `D`. For AI, use 3
regardless of its stored difficulty. The ordinary content allowance is `6-D`.
For population `P`, the initial ordinary drone input is:

`ordinary = clamp(P - (6-D), 0, P)`

| Difficulty index | Human content allowance |
| --- | --- |
| 0, Citizen | 6 |
| 1, Specialist | 5 |
| 2, Talent | 4 |
| 3, Librarian | 3 |
| 4, Thinker | 2 |
| 5, Transcend | 1 |

AI's allowance is always 3 in this prefix. For example, at population 7 a
Transcend human starts with ordinary input 6, while an AI starts with 4.
This is before other drone and talent effects, not a final content/drone split.

## Faction definition parameters

The faction's explicit `DRONE` and `TALENT` parameters are processed separately
from the current social-engineering `TALENT` rating.

For positive `DRONE = K`, add `floor(P/K)` to ordinary raw drone input.
For positive `TALENT = L`, create a separate talent input `ceil(P/L)`.
Zero disables that parameter's term. Recompute these from current population;
do not grant one fixed point per base independently of population.

Examples:

- `DRONE 4`: populations 3/4/7 give extra inputs 0/1/1.
- `TALENT 4`: populations 1/4/5 give talent inputs 1/1/2.
- Population 7, human difficulty 5, `DRONE 4` and `TALENT 4`: raw drone
  input after the faction term is 7 and separate talent input is 2.

Do not immediately subtract that talent input from drone demand. The original
retains them separately for subsequent processing. The faction drone addition
has no immediate population cap; e.g. `DRONE 1` can push raw demand above P.
The current social `TALENT` rating does not replace either faction parameter
or alter these checked prefix outputs. Its later effects are not specified here.

## Negative custom parameters

For any nonzero parameter the original literal arithmetic is:

- Drone term: `trunc_toward_zero(P / K)`.
- Talent input: `trunc_toward_zero((P + L - 1) / L)`.

The talent expression is a ceiling division only for positive L. For example,
P=1 and L=-4 produce talent input 1, while P=10 and L=-4 produce -1. Negative
drone parameters can reduce raw demand below zero. No clamp occurs at this
parameter stage. These are custom arithmetic boundaries, not evidence of
negative visible citizen counts after later processing.

## Neutral-modifier scope

The [conquest assimilation input](assimilation-timer.md) is calculated between
ordinary difficulty and faction parameters. It is explicitly zero in these
cases, as in a freshly initialized base record. Its contribution and local
aging are now separately checked; capture initialization remains open. Do not
use this neutral-case formula to omit that modifier when active.

[Bureaucracy input](bureaucracy-drone-input.md) is also retained separately and
added later. The current tests use a one-base faction and global position zero
on Standard, giving zero bureaucracy input. They do not establish complete
ordinary-plus-bureaucracy-plus-facility citizen output.

## Acceptance scope

The [21 named cases](ordinary-citizen-inputs-cases.json) cover difficulty
boundaries, human/AI differences, faction floor/ceiling thresholds, zero and
negative parameters, separate current social rating and uncapped raw demand.
The grid combines human/AI, all six difficulties, nine populations, five drone
parameters and five talent parameters, cycling seven playable faction slots.
Population zero and negative parameters are supplied arithmetic boundaries.

Map scale is supplied as the already-established Standard value 40. No gameplay
helper return is substituted and no original instruction modified. The base
record remains unchanged; prefix outputs are passed to later citizen stages.
Timed modifiers, later facilities/psych/specialists, drone conversion/calming,
state-transition timing and original-game observations remain open.
