# RE-SMAC-CITIZEN-POLICE-001: police and pacifism

Status: original-x86 checked for the SMAC fingerprint in this directory's
README. All 30 named cases and 1,600 deterministic sampled combinations pass.
Every case executes the current citizen calculation from entry through final
count storage, including original unit lookup, stack traversal and ability
helpers. Consistent unit lists/map markers and the resolved pacifism count
are supplied; their lifecycle and upstream construction are not checked.

## Police slots and strength

Police processing follows the [facility conversion stage](citizen-conversion-stages.md)
and precedes [late projects and suppression](citizen-final-projects-and-suppression.md).
Use the current resolved POLICE rating, not merely a social model's listed
bonus.

| POLICE rating | Maximum police slots | Basic reduction per qualifying unit |
| --- | --- | --- |
| -2 or lower | 0 | 0 |
| -1 or 0 | 1 | 1 |
| +1 | 2 | 1 |
| +2 | 3 | 1 |
| +3 or higher | 3 | 2 |

Non-Lethal Methods adds one more reduction per qualifying unit. Thus its
strength is 2 below +3 and 3 at +3 or higher. Do not multiply its two-point
ordinary strength by two at +3. High supplied ratings +4/+5 do not increase
the three-slot limit or strength further.

Each reduction stops at zero drones. Police directly reduce D, not talents T
or the additional calming burden S. Later normalization handles their
interaction; do not spend police reductions on S first.

## Self-Aware Colony and unit selection

If the faction owns Self-Aware Colony and has at least one police slot, apply
one basic police effect before scanning garrison units, and consume one slot.
Its effect is one drone below +3 and two at +3 or above. It receives no
Non-Lethal ability bonus. This benefit is faction-wide; a rival-owned project
does nothing. At POLICE -2 or lower it supplies no police effect.

Then consider qualifying units physically at the base, with Non-Lethal
Methods units first and other units second. Within each group the existing
stack order is used. A unit consumes one slot regardless of whether its
reduction has already saturated at zero. A project also consumes its slot
even when stronger Non-Lethal units are available.

The original predicate accepts design package/role modes 0..3 and rejects
larger unsigned values. The supplied `package_mode` field in the cases is this
design classification, not weapon strength. Mode 3 includes the installed
Scout Patrol; colony mode 8 and former mode 9 do not qualify. A supplied mode
4 with Non-Lethal Methods still fails the filter. This handoff does not define
how every custom design acquires its classification.

No additional owner, home-base, chassis, movement or order check appears in
this scan. Supplied foreign-owner, unhomed and alternate-chassis units in the
base stack receive the same treatment. These are predicate boundaries, not
evidence that opposing factions may legally occupy one base stack. Off-base
units do not enter the scan.

Example: POLICE 0, one ordinary unit and one Non-Lethal unit gives the
Non-Lethal unit the single slot. Adding owned Self-Aware Colony instead uses
that slot for its one-drone effect, leaving no unit slot. At POLICE +3,
Self-Aware plus two Non-Lethal units can provide up to 2+3+3 reductions.

## Pacifism input

After police, use the already resolved nonnegative pacifism-unit count N:

| POLICE rating | Additional drone demand |
| --- | --- |
| -2 or higher | 0 |
| -3 | max(0, N-1) |
| -4 | N |
| Below -4 | 2N |

This addition can temporarily exceed population. Bound D and T separately to
0..population, preserve S, then apply the shared cancellation rule against
population minus excluded count. Snapshot this police/pacifism result before
the late project stage. Pacifism does not construct new S from its overflow.

The count N is supplied in these cases. Do not replace it with all units away
from the base: ownership, home, location, air and other eligibility rules for
constructing N require a separate upstream trace. A military unit being
eligible for garrison policing does not alone establish its pacifism status.

## Acceptance scope

The [30 named cases](citizen-police-and-pacifism-cases.json) cover slot/rating
boundaries, ability priority, Self-Aware slot consumption, design mode
exclusions, supplied ownership/home/chassis boundaries, pacifism thresholds,
excess calming burden and earlier psych/social/faction interactions. The
sampled cases combine populations 1/3/7/20/127, exclusion 0/half/all, ratings
-6..+5, unit stacks, project ownership and pacifism counts 0/1/2/5.

These execute without modified instructions or substituted helper returns.
Stack repair/rebuild mode is disabled and consistent links are supplied.
Assimilation, local facilities, late projects and suppression are neutral in
these cases; their separately documented rules remain applicable. Project,
unit-stack and social-rating transitions, riot and Golden Age rules, UI and
whole turns remain open. The upstream count producer is now separately
checked in [pacifism-unit count](pacifism-unit-count.md), including direct
producer-to-consumer execution with resolved map/unit inputs.
