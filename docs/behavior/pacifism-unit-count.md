# RE-SMAC-PACIFISM-COUNT-001: building the pacifism count

Status: original-x86 checked for the SMAC fingerprint in this directory's
README. All 29 named cases and 1,400 deterministic sampled combinations pass.
Each runs the complete original support calculation twice, then the current
citizen calculation, with actual territory and ability helpers. The map's
resolved ownership/base markers and unit/design state are supplied.

## Unit admission

Rebuild the current base's count from zero by scanning live units in global
array order. Only units owned by the base's faction and homed at this base
can enter its pacifism count. Units homed elsewhere or without a home do not
enter, even if they are physically at this base.

For each matching owned/homed unit, clear its previous pacifism markers before
checking its current state. Count it only if:

- Its design package/role mode is at most 9 as an unsigned value.
- Its weapon's signed attack value is nonzero, including negative psi attack.
- It passes the location/air test below.

Mode 9 can count, while modes 10 and higher do not. A zero-attack weapon
cannot count regardless of location or chassis. This is a literal resolved
design predicate: do not substitute a broad combat-unit label. Custom
positive-attack colony/former classifications are supplied boundary cases,
not claims about ordinary workshop legality.

Clean Reactor does not exempt a unit from this count. Neither does the
built-in native/fungus mineral-support exemption. Support cost and pacifism
count are separate outputs: free mineral support does not imply no pacifism.

## Location and air test

A location passes the protected-location test if either:

1. It has a base marker and the resolved base-owner classification is in 0..7; or
2. The original territory lookup returns the unit/base faction.

The first branch does not require that marked base to belong to the unit's
faction. A valid foreign base or faction-zero base bypasses territory lookup.
An invalid base-owner classification falls back to territory. These tests concern
supplied resolved tile state; legal access to foreign bases is separate.

If the location is unprotected, count any unit that passed admission. This
includes ground, naval and air units, including air design mode 4.

If the location is protected, count only units whose chassis triad is air
and whose design package/role mode is not 4. Thus ordinary air units can
count at a friendly base or inside friendly territory; air mode 4 is exempt
there. Ground and naval units at protected locations are exempt. Do not apply
the air-mode-4 exemption outside protected locations.

No movement, damage, order or distance check is part of this local predicate.
Territorial ownership generation, diplomatic access, home reassignment and
capture transitions remain separate research work.

## Count and per-unit markers

Increment the count for each qualifying unit regardless of the current POLICE
rating. Then assign its marker according to that rating:

| POLICE rating | Marker on qualifying units |
| --- | --- |
| -2 or higher | None |
| -3 | First qualifying unit marked as exempt; subsequent units marked as penalized |
| Below -3 | Every qualifying unit marked as penalized |

The first exemption follows global unit-array order after all ownership,
home, weapon, mode and location tests. A skipped unit does not consume it.
These are per-unit accounting markers; their UI rendering is not checked.

The routine clears markers only on matching owned/homed units. Units skipped
by ownership or home selection retain supplied stale markers during this
base's recalculation. Do not infer global cleanup on rehoming/capture from a
single base calculation. Repeating calculation replaces the count and
rebuilds selected markers, rather than accumulating them.

The citizen routine consumes this resulting count using the
[pacifism rating formula](citizen-police-and-pacifism.md): at -3 it exempts one,
at -4 it adds the count, and below -4 it adds twice the count. The tests run
that consumer after the producer and compare both raw demand and final drone
count. A positive or neutral police rating can retain a nonzero count while
adding no drone demand.

## Acceptance scope

The [29 named cases](pacifism-unit-count-cases.json) cover ownership/home
selection, attack signs, support exemptions, protected/foreign/invalid base
tiles, air mode 4, mode boundaries, first exemption order and stale markers.
The sampled cases combine populations 1/7/20/127, ratings -6/-5/-4/-3/-2/0,
mixed units and ownership/location/chassis/attack states. Original instructions
and helper returns are unmodified. No convoy yield substitution is used.

The original terrain helper executes against supplied tile ownership. This
does not establish map-border generation, unit legality, complete diplomacy,
unit deletion/rehoming, global base processing order or full turns. Other
citizen inputs are neutral so producer/consumer results remain attributable;
combined facility/project/social interactions still need broader coverage.
