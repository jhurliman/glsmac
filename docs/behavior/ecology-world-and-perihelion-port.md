# Owned world ecology and live perihelion timing

Simulation V26 owns native-life/cloud categories and cumulative sea offset,
independently of its saved active perihelion bit. The accepted installed Planet
source initializes world values through its signed metadata getters; later
restore reads mutable saved values instead of reloading an unchanged map header.
Only native categories0..2 are admitted; cumulative sea offset preserves signed32.
The port's continuous generated-map settings do not establish native categories
and remain explicitly unavailable. V11–25 restore both new authorities as absent.

Accepted fresh-world initialization clears perihelion. Opening setup is not a
clear instruction, and restoring a campaign never replays this reset. This
follows the [new-world/storage controls](perihelion-new-world-and-storage.md).
The actual Reset/new-simulation regression starts with an active prior world
and verifies the fresh reset alongside retained faction history.

The [native scheduler](perihelion-schedule.md) checks an active ending year
first, then the strict turn>50 starting condition. It uses calendar congruences,
not a reconstructed twenty-turn countdown. Repeated starting-year maintenance
can start again; an already-inactive ending year has no ending transition.
The pure helper exposes that transition without announcements or RNG effects.
Twenty named native controls supply literal expected states/transitions,
including alternate calendar origins, non-maintenance mode and repeated calls.

Both actual Game turn paths preflight the next ordinary mission year, advance
the scalar calendar, and publish perihelion before their upkeep/event consumers.
The ordinary2100 origin matches the existing Game turn event; scenarios/custom
calendars need separate accepted authority. Missing legacy perihelion remains
unavailable rather than inferring its value from today's year. Invalid calendar
arithmetic rejects before turn, state or callbacks change. This follows the
[original global ordering](perihelion-global-turn-order.md).

This is the perihelion state transition slice. It does not implement its original
announcement, the rest of resource-event maintenance/generation, random target
selection, eco-damage posting, sea-step terrain conversion or bloom effects.
Those remain required live integrations. The completed metadata/history/rule
inputs now let the Base ecology calculator consume saved world authority.

Validation: cached incremental build;49 focused native groups. These include
the complete simulation-state module, a real imported synthetic Planet through
claimed Start with independent1/2/12 header canaries, actual turn hooks with
calendar overflow/legacy-absence controls, and actual multi-owner canonical
upkeep at MY2190/MY2210. Both transitions complete Base upkeep; their internal
ecology arithmetic is not claimed because that caller is still forthcoming.
Five changed production TUs pass targeted correctness analysis. Focused
ASan/UBSan on state, scheduling, simulation and canonical-turn TUs passes seven
relevant groups, reusing normal objects elsewhere with leak detection disabled.
