# RE-SMAC-NATIVE-BIRTH-PRODUCER-001: initialized native birth and effective level

Status: **111 creation requests and 12 following attacks pass through normal
return** on the pinned SMAC image in [the ledger](README.md). Of these worlds,
109 execute the complete native-faction initializer first; two deliberately
retain uninitialized runtime coordinates as controls. These are controlled
original-routine executions, not live-game or full elapsed-turn observations.

Use the [component-complete input chain](native-component-combat.md). Its
earlier 43 cases supplied zero-valued native runtime reference coordinates.
They do not establish the runtime state produced by actual native-faction
initialization. This extension supplies that producer and distinguishes its
birth RNG behavior from those controls. No frozen older fixture is changed.

## Native faction initialization and birth state

The actual new-game faction loop includes native faction 0. Its complete
initializer, called here in the ordinary new-game mode, resets poisoned AI
reference coordinates to **-1/-1** and returns normally. Localized text is
supplied; gameplay initialization is not skipped. The existing human Rover
record remains unchanged. These faction reference coordinates are distinct
from a unit's home-base identity.

Creation then uses the actual Mind Worm factory and placement path. The normal
returned birth state in the checked fresh-image profile is native owner 0,
healthy, reactor 1, no abilities, stored morale 1, spent movement 0, home none,
singleton stack, at the encounter destination. The discovering Rover stays
at its source without spending movement, and the movement request returns 0.

Stored morale is initialized from the low byte of the native faction's direct
morale scalar, plus one with byte wrapping. That scalar starts at zero in the
fresh image. The native runtime initializer does **not** reset it: explicit
scalar diagnostics -2/-1/256 survive initialization and produce stored bytes
255/0/1 respectively. These diagnostics do not describe stock faction data or
establish a normal route for changing the native faction definition.

## Exact creation RNG boundary

With the produced inactive reference coordinates, the factory skips its later
AI-assignment stage. The encounter command consumes **one CRT draw**, the
encounter admission roll; native creation consumes no additional CRT draw.
No shared movement-lottery draw occurs. This holds across all checked turn
and native-life boundaries with the initialized reference coordinates retained.

The earlier extra initialization draw is not a random morale or birth-level
roll. With the supplied zero-coordinate controls, the factory instead enters
an optional regional AI-assignment branch. It consumes a second CRT draw,
uses raw output modulo 5 in those controls, and selects different assignment
metadata for zero versus nonzero remainder. It does not change stored morale
or the effective level in either control. Full AI assignment behavior is open.

Do not hardcode either one or two draws for every possible later game state.
One is established for this actual initialized runtime; additional draws
depend on reaching the later assignment stage. Later writers and lifetime of
the faction reference coordinates are not simulated here.

The native-faction initializer itself consumes CRT values in the supplied
setup: eight advances in each checked initializer invocation. The test records
those separately and supplies its explicit command
seed **after initialization**. It does not claim a continuous process-start
RNG history. There is no reseeding between encounter creation, returned-state
queries and the following attack.

## Stored morale is not wild-native effective level

For native-owned Mind Worms with neutral additional level markers and a neutral
opposing-faction context, the effective-level query uses the **current global
turn counter**, independently of the stored morale byte:

| Current turn counter | Base effective level |
| --- | --- |
| Below 45 | 0 |
| 45 through 89 | 1 |
| 90 through 169 | 2 |
| 170 through 249 | 3 |
| 250 through 329 | 4 |
| 330 and later | 6 |

The base schedule skips level 5. These are turn counters, not an inferred
calendar-year conversion. Stored morale remains 1 at every checked birth turn.
The same existing unit changes effective level when the counter crosses a
threshold, without a unit-record mutation or RNG advance. Do not freeze its
effective level at birth or derive it directly from stored morale.

The main grid checks 16 turn values against native-life settings 0/1/2 and
two encounter seeds: 96 complete births. Native-life changes encounter chance
under the admission contract, but does not change this base level schedule.
All four combinations of the tested morale-query mode and neutral/native or
human opposing context agree. Nonzero atrocity counters, special level markers,
other native designs and ownership transfer remain outside these level checks.

## Continuous combat checks and promotion distinction

Twelve sequences continue the initialized birth into actual component-complete
combat without reseeding. Eight Rover victories and four native victories pass
through normal return, with 179 rounds. These retain the original two-command
behavior, exact stream order, damage, spending, rewards and casualty handling.

At battle turns 1 and 15, native effective level is **0**. Its defense-strength
consumer has a separate minimum of 1; that explains the checked native strength
448 before the early-turn halving. It does not mean the native's effective
level query returned 1.

On the four native victories, promotion changes stored morale from 1 to 2,
but the subsequent effective-level query still returns 0. Automatic promotion
admission at these levels is compatible with that result. A portable unit
model must keep the stored byte and wild-native level calculation distinct.

The [111 fixtures](native-birth-producer-cases.json) label the unreset-runtime
and scalar diagnostics separately. The 12 battle continuations use initialized
runtime and retain both wins and defeats. Visible presentation, native offense,
retreat, capture/release, later AI-reference changes, opposing atrocity and
special-unit level modifiers, general reward tables, repeated-game definition
initialization, ownership transfer and complete turn scheduling remain open.
