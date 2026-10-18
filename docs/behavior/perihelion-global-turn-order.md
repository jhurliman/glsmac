# Perihelion maintenance precedes the new turn's owner passes

2026-10-18, stock SMAC. **Ordinary global advance increments the turn and
updates its calendar before event maintenance. That maintenance finishes
before the following owner Base/research passes.** Consequently MY2190's
first measured owner pass already sees active perihelion.
[Literal boundary observations](perihelion-global-turn-order-cases.json).

One admitted local-human controller path is supplied turn89/MY2189 immediately
before entry, perihelion inactive, and an existing Bumper Crop with timer1.
New random events are disabled. The actual global advance and maintenance run:

| Boundary | Turn / stored year | Perihelion | Bumper Crop / timer |
| --- | --- | --- | --- |
| Controller and global advance enter | 89 /2189 | Inactive | Active /1 |
| Turn increment completed, calendar update pending | 90 /2189 | Inactive | Active /1 |
| Maintenance enters with mode0 | 90 /2190 | Inactive | Active /1 |
| Maintenance returns | 90 /2190 | **Active** | **Expired /0** |
| Owner1 upkeep enters | 90 /2190 | Active | Expired /0 |
| Base0 upkeep, both mineral/ecology passes, then Base research entry | 90 /2190 | Active | Expired /0 |

The native perihelion-start announcement is requested once; only its presentation
is supplied. The actual maintenance expires the existing local event and sets
perihelion before returning. Both complete mineral/ecology bodies run with that
new state before the probe stops at Base research entry. It does not complete
research, later ecology-event processing or the controller return.

Source/caller order is full global ranking update → turn increment → calendar
update → intervening global maintenance → resource-event maintenance(mode0) →
global advance return → owner-upkeep selection. Inside the observed owner pass,
Base processing precedes its research entry. Existing
[perihelion transitions](perihelion-schedule.md) establish the maintenance
routine's start/end tests, and
[resource-event lifetime](resource-event-lifetime.md) establishes all-Base aging.
The new observation closes their placement for this ordinary controller path.

Apply this global maintenance once at the global boundary; do not defer it until
after the owner Base passes or age local events separately for each owner.
The source ordering also places the existing ending transition before owner
processing when its maintenance condition matches; a separate MY2210 integrated
path is not newly executed here.

This is one controlled turn-boundary replay using an already admitted Gaians
startup state, not a naturally progressed MY2190 campaign or a replay of all
factions. Inherited headless startup/UI/history/cleanup services remain declared.
No turn, calendar, expiry, perihelion or resource helper result is substituted.
No new random-event selection, bloom behavior, renderer or ecology damage-value
matrix is investigated. Standing research remains paused.
