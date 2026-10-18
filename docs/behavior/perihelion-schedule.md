# RE-SMAC-PERIHELION-SCHEDULE-001: perihelion schedule

Status: original-x86 checked for the pinned SMAC reference. Twenty named
fixtures and 336 calendar combinations execute the complete event routine and
calendar helper, followed by complete center yields and the final ecology
scaling stage. Announcements and a later random target draw were supplied.
Full-turn scheduling, native-life consequences and live-game observations
remain open.

## Calendar and state transitions

The calendar helper computes mission year as `startYear + turnIndex`. On the
maintenance entry to the event routine, process the following in order:

1. Advance solar phase state, dust and existing local resource-event timers
   using the [existing maintenance rules](resource-event-lifetime.md).
2. If perihelion is already active and `(missionYear - 2210)` is divisible by
   80, clear perihelion, request its ending announcement and **return**.
3. If the routine has not returned, and `turnIndex > 50` and
   `(missionYear - 2190)` is divisible by 80, set perihelion, request its
   starting announcement and **return**.
4. Otherwise proceed to [random-event admission](resource-event-admission.md).

The transitions precede both the random-events-disabled gate and the random
event's first-eligible-turn test. Disabling new random events therefore does
not disable perihelion on this path. A start or end also prevents the random
target draw and event-generator reseeding in that call. Maintenance in step
one still occurs before either return.

With start year 2100, normal transition boundaries are:

| Mission year | Turn index | Result after maintenance |
| --- | --- | --- |
| 2189 | 89 | No scheduled transition |
| 2190 | 90 | Perihelion active; start announcement; return |
| 2209 | 109 | Active state retained |
| 2210 | 110 | If active: clear it; end announcement; return |
| 2270 | 170 | Next start |
| 2290 | 190 | Next end if active |

The natural interval is twenty years, with starts eighty years apart. These
are results when event maintenance is invoked for the listed year, not a
measurement of its placement relative to every faction's upkeep or UI refresh.

## Exact boundaries

The start guard is strictly `turnIndex > 50`; a custom calendar aligning a
start year with turn 50 does not start it. Turn 51 can start it. The ending
test has **no** corresponding minimum-turn guard: a supplied active state
can end at its matching calendar boundary even on turn 50.

The year tests use divisibility, with no separate lower bound on mission year.
A custom start year can therefore admit an earlier congruent year after turn
50; a checked example starts perihelion in MY 2110 at turn 110 with start year
2000. Do not replace these tests with a fixed countdown from game start.

The non-maintenance entry mode skips both transitions and leaves the active
state unchanged at either boundary. It can still reach random-event admission.

The start test does not require perihelion to be inactive. Repeating maintenance
in a start year repeats its starting announcement and early return. Conversely,
an end-year call with perihelion already inactive does not announce or return
for that end; it can proceed to random selection. Two calls at an end boundary
therefore have different behavior: the first clears active state and returns,
and the second can draw a random target. The transition routine is not
idempotent across repeated same-year calls.

An active state supplied after a missed ending boundary is not repaired simply
because the mission year is later. It remains active until another matching
ending year or a separate state writer changes it. This is an isolated-routine
boundary, not evidence that ordinary play skips scheduled maintenance.

## Yield and ecology composition

Perihelion is separate from the solar event that triples energy. In the checked
neutral non-HQ land center, perihelion starts and ends leave default yields
at **2/1/1**, in both ordinary and faction-upkeep calculation contexts.
Do not infer an energy tripling from the perihelion announcement.

Local effects, dust and solar phases may change in the same maintenance call,
before the perihelion transition. One connected case expires Bumper Crop and
dust, moves solar from its ordinary phase to its upkeep phase, then starts
perihelion. Its subsequent center output is 2/1/1 in ordinary context and
2/1/3 in upkeep context. The energy difference comes from solar phase selection.

The resulting perihelion state is consumed directly by
[final eco-damage scaling](eco-damage-scaling.md): it doubles the nonnegative
burden **before** later difficulty/faction scaling and rounding. It need not
double the final rounded value. With post-human burden 1, technology score
125, no future-tech subtraction, PLANET 0, sparse native life, human Citizen
difficulty and no Voice of Planet, the actual scaling stage stores eco-damage
1 outside perihelion and 3 during it. The scheduling checks carry the actual
event-written flag into this original scaling stage without supplying a
replacement perihelion input.

## Evidence scope

The [20 named fixtures](perihelion-schedule-cases.json) cover start/end neighbors,
later cycles, disabled random events, non-maintenance mode, the strict start
guard, an early ending guard, repeated same-year calls, custom calendars and
the combined local/dust/solar transition. All include subsequent original
center-yield and final ecology results. Another 336 cases vary start year
2000/2100/2200, fourteen turn boundaries, incoming active state, entry mode
and the new-random-events option.

The final ecology stage starts from the explicit resolved inputs in the fixture;
upstream ecology burden production is not rerun in these compositions. Later
random selection is ended with a supplied out-of-list target, so no unrelated
random-event outcome executes. Periodic state initialization, all alternative
writers, native spawning/combat, complete world scheduling and save/load remain
separate research.
