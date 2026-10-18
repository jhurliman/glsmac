# RE-SMAC-CITIZEN-FAC-001: facility and project citizen inputs

Status: original-x86 checked for the SMAC fingerprint in this directory's
README. All 27 named cases and 3,510 additional combinations pass. These are
three intermediate inputs constructed by the citizen routine, not final
visible talent/drone counts. Full conversion, psych, calming and riots remain
open.

## Facility drone adjustment

Start this signed adjustment at zero and add each applicable row:

| Condition | Adjustment |
| --- | --- |
| Genejack Factory present | Configured Genejack drone rule; default +1 |
| Recreation Commons present | -2 |
| Hologram Theatre present, OR Network Node present and faction owns Virtual World | -2 once |
| Faction owns Planetary Transit System and population is at most 3 | -1 |
| Research Hospital present | -1 |
| Nanohospital present | -1 |

The Hologram Theatre and Virtual World/Network Node routes share one reduction.
Having both still gives -2. Recreation Commons stacks with that reduction,
and both hospital reductions stack independently. Virtual World without a
Network Node gives no reduction here. A rival's Virtual World does not enable
the Node reduction.

Planetary Transit System applies at population 3 but stops at population 4.
Its ownership benefit is faction-wide, including bases other than the project
location. The same ownership test applies to the other projects below.

Use the configured Genejack rule, not a fixed one-drone constant. Checked
custom values 0, -2 and +4 are added literally. Preserve a negative net
adjustment at this construction stage; no zero or population clamp occurs
here. For example, all seven local facilities and all four owned projects at
population 3 produce facility adjustment -6, Paradise talent input 2 and
project talent input 3 under the default Genejack rule.

## Separate talent inputs

| Input | Construction |
| --- | --- |
| Paradise talent input | 2 if Paradise Garden is present, otherwise 0 |
| Project talent input | +1 for owned Human Genome Project; +2 for owned Clinical Immortality |

Both project terms stack, giving project talent input 3. Projects owned by
another faction contribute nothing. The project may be in the current base
or another base owned by the faction.

Keep the three outputs separate. Static tracing places facility adjustment
into raw drone demand, then Paradise input into talents; project talent input
is added at a later stage of the routine. The checks execute input construction
only, so they do not establish the intervening conversion rules or final
visible effect of Clinical Immortality's two-point input. Do not combine all
talents early or immediately cancel them against the drone adjustment.

## Presence and scope boundaries

These checks use completed local facility presence. A facility merely listed
in the production queue does not count. Project ownership is supplied from
the current owner of its location; project construction, capture and
destruction transitions are outside this specification.

The [27 cases](citizen-facility-project-inputs-cases.json) cover each local
facility, ownership and population boundaries, stacking, queues and custom
Genejack values. The additional grid covers every subset of seven facilities,
populations 2/3/4 and three ownership states for Virtual World and Planetary
Transit System, plus talent-project ownership/Paradise combinations at
populations 1/3/7. Playable faction slots are cycled across the grid.

The original complete citizen prefix and its actual facility-presence helpers
execute without modified instructions or substituted helper returns.
Assimilation and faction DRONE/TALENT parameters are neutral; resolved map
scale and base count give zero bureaucracy input. This is not a complete
combined citizen result. Punishment Sphere, later project effects, specialists,
social ratings, psych and riot/Golden Age transitions remain outside these
input-construction checks. See the subsequent
[early conversion](citizen-conversion-stages.md) and
[late project/suppression](citizen-final-projects-and-suppression.md) handoffs
for their separately checked arithmetic and remaining scope.
