# RE-SMAC-FACILITY-ELIGIBLE-001: facility/project build eligibility

Status: original-x86 checked for the SMAC fingerprint in the
[research ledger](README.md). All **9,315** scenarios pass, including
[49 named fixtures](facility-project-build-eligibility-cases.json). The complete
eligibility routine and its actual facility, queue-presence, project-owner and
coastal helpers execute without supplied return values. Base records and
project locations remain unchanged in every check. These are synthetic queries,
not observations of a complete construction or victory workflow.

The query receives an item, requesting faction, optional base, and preceding
queue depth. Item numbers below are the positive IDs in the SMAC facility rules
list; production queues represent these choices with negative IDs. Tested inputs
cover every item 1 through 71, plus no-item zero. Runtime prerequisites are
explicit inputs and need not equal the installed prerequisite for a named item.

## Guard order

Apply these decisions in order:

1. No-item zero rejects.
2. Skunkworks rejects at **global difficulty zero or one**. The requesting
   faction's stored difficulty is not used by this gate.
3. Secret Projects, including Ascent, reject when the resolved project-building
   disable rule is active. Its setup/user-interface identity remains unresolved.
4. Ascent to Transcendence takes the separate special path described below.
5. Other items require a positive requesting faction and an available prerequisite.
6. Ordinary Secret Projects accept only when their global project location is
   exactly the unbuilt sentinel. They return here, before local or queue checks.
7. Other items queried without a base accept here. Local duplicates, dependencies,
   project substitutes, coast and the local free-prototype restriction are skipped.
8. With a base, local facilities 1 through 33 reject if already present locally or
   present in the specified preceding queue positions.
9. Apply the item-specific local rules below. Other items accept.

For step five, prerequisite None is unconditional. Disable, other negative
references and technology references outside 0 through 87 reject. A technology
must be owned and its definition must be enabled. A first prerequisite below
None disables the definition. A second prerequisite below None also disables
it, except when the first prerequisite is None. Thus an owned synthetic
technology with first prerequisite None and second Disable passes this query;
an ordinary first prerequisite and second Disable does not.

These are the query's guards, not the complete legality rules for every picker,
scenario, command or construction lifecycle.

The [token and prerequisite contract](technology-token-and-production-prerequisites.md)
executes the full original loader/resolver before ordinary prerequisite queries
and specifies unknown-token diagnostics and the distinct loader-count predicate.

## Ascent to Transcendence

After the project-disable gate, Ascent accepts precisely when Higher Goal
(Allow Victory by Transcendence) is enabled and Voice of Planet's global location
is **not exactly the unbuilt sentinel**. Voice may belong to another faction.

This early decision bypasses the ordinary positive-requester guard, Ascent's
configured technology prerequisite, its own existing global location and local
or queue duplicate checks. Named cases demonstrate acceptance with requester
zero, disabled prerequisite, foreign Voice, or an already recorded Ascent.
These are explicit query boundaries, not claims that a player can win repeatedly
or bypass other victory-controller admission.

A synthetic other-negative Voice location also passes the non-unbuilt test.
For ordinary projects the same other-negative location fails the exactly-unbuilt
test. Keep these two predicates distinct. The port should preserve coherent
project state; the unusual sentinel inputs are dependency-isolation cases.

Voice of Planet itself follows the ordinary project rules here: an unbuilt Voice
with available prerequisite can pass even when Higher Goal is disabled.

## Local presence and queue depth

For a required or forbidden local facility, presence means either actually
installed or found in the first `depth` queue positions. Position zero is the
current target; later positions are pending targets. A depth of zero ignores
the queue. Depth one considers only the current target; depth two considers
the current target and first pending target. It does not inspect future entries
beyond that prefix or check whether a promised prerequisite is itself eligible.

The same prefix rule rejects duplicate local candidates 1 through 33. Orbitals
34 through 37 and Stockpile 38 do not use that local duplicate guard. Ordinary
projects return earlier, so their earlier occurrence in a queue does not itself
reject them. An installed/queued facility is not implicitly granted by every
project that can substitute for it: apply only the explicit substitutes below.

The [queue advancement helper](production-queue-and-build-eligibility.md) calls
this query with depth zero. Its now-current entry is consequently not rejected
merely for occupying the current position, and promised queued prerequisites
do not qualify during that call.

## Additional rules with a base

Every row remains subject to the preceding common guards. "Present" includes
the supplied queue prefix as above. "Own project" means its host base belongs
to the requesting faction, rather than merely that the project exists globally.

| Candidate | Additional acceptance requirement |
| --- | --- |
| Recycling Tanks (3) | Pressure Dome (26) must not be present |
| Perimeter Defense (4) | Must not own Citizens' Defense Force (44) |
| Tachyon Field (5) | Perimeter Defense (4) present, or own Citizens' Defense Force (44) |
| Skunkworks (10) | Requester must not have the resolved free-prototype capability |
| Hologram Theatre (11) | Recreation Commons (6) present, and must not own Virtual World (45) |
| Hybrid Forest (14) | Tree Farm (13) present |
| Quantum Lab (16) | Fusion Lab (15) present |
| Nanohospital (18) | Research Hospital (17) present |
| Nanoreplicator (20) | Robotic Assembly Plant (19) or Genejack Factory (22) present |
| Quantum Converter (21) | Robotic Assembly Plant (19) present, and must not own Singularity Inductor (67) |
| Punishment Sphere (23) | Paradise Garden (12) must not be present |
| Habitation Dome (25) | Hab Complex (24) present |
| Command Center (27) | Must not own Command Nexus (40) |
| Naval Yard (28) | Must not own Maritime Control Center (49), and adjacent sea required |
| Bioenhancement Center (30) | Must not own Cyborg Factory (56) |
| Temple of Planet (32) | Centauri Preserve (31) present |
| Orbitals (34 through 37) | Aerospace Complex (29) present, or own Space Elevator (66) |
| Stockpile Energy (38) | No further item-specific local requirement |

The exclusions are asymmetric. Pressure Dome can pass with Recycling Tanks
present, and Paradise Garden can pass with Punishment Sphere present. Quantum
Converter specifically needs Robotic Assembly Plant; Genejack Factory alone
qualifies Nanoreplicator but does not qualify Converter. Virtual World's
Hologram Theatre exclusion applies without checking for Network Node here.
Foreign-owned projects do not provide these own-project substitutions or exclusions.

Skunkworks' global difficulty guard still applies without a base, but its
free-prototype exclusion does not. Free-prototype capability is a resolved
input; faction token decoding remains open, as in the
[unit/prototype cost contract](unit-design-and-prototype-costs.md).

Naval Yard uses the same actual eight-neighbor sea predicate specified under
[unit sea adjacency](production-queue-and-build-eligibility.md#sea-adjacency).
The center does not count; a synthetic isolated sea-base tile surrounded by
land fails. Horizontal wrapping/flat-map clipping and vertical clipping apply.

## Evidence and remaining scope

The [completion-grant contract](production-completion-grants.md) separately
connects actual grants to this query during the next queue decision. Admission
and mutation are distinct stages; those admitted grant cases do not execute
all earlier race/victory checks or later project effects.

The 9,315 checks comprise 49 named cases, 2,272 general item/prerequisite/
difficulty/context combinations, 4,368 local dependency/prefix combinations,
1,008 project-ownership combinations, 1,152 Ascent combinations, 426 duplicate
combinations and 40 technology-definition cases. All queries preserve the two
test base records and the complete project-location array.

The fixtures describe explicit runtime inputs, including synthetic disabled
definitions, requester zero, nonstandard sentinels, contradictory installed
facilities and isolated sea centers. They establish predicate behavior rather
than normal reachability. Complete chooser/queue-edit admission, maximum queue
length, rule/capability token decoding, construction/grant effects, project races,
victory execution, save/load, scheduling and live-game confirmation remain open.
