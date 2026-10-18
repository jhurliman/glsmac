# RE-SMAC-ANIMATION-EXTENSION-001: timing inventory for port tests

Pinned **stock SMAC** only. This extends the
[31-case timing inventory](animation-timing-inventory.md) and
[66-assertion selection/title receipt](selected-unit-blink-timing.md), without
rerunning those measurements. **25 new native controls** add window-slide
pacing, recenter target publication and model-counter distinctions.
[Reproducible cases](animation-timing-followup-cases.json).

A requested timer period, a model's selected frame, an interpolation step and
a compositor-visible frame duration are different quantities. None of these
receipts establishes one universal game FPS or exact live presentation duration.

## Compact inventory

| Named observation | Scheduler / clock | Delayed work and speed | Pause / modal scope |
| --- | --- | --- | --- |
| **SEL-SYNC: selected flag, health and order overlay** | One shared150ms repeating timer; all three use the same blink phase. Nominal cycle300ms. | Pending notifications coalesce; no elapsed-time parity or catch-up. Whole-unit-blink preference changes draw omission, not the timer period. | Base/checked modal visibility, missing command state, shutdown and lifecycle/effect inhibits hold callbacks. Focus loss alone is not an explicit pre-toggle hold. |
| **MOVE-LOCAL: ordinary local facing/translation** | Land minimum8ms facing /16ms translation; sea5/10ms; air2/5ms. | Actual-now timestamp after each accepted wait; one index per step, no skipped indices after overrun. Translation6 normal /9 smooth /3 fast. | Message pumping occurs inside waits. No blanket outer focus/minimize/modal admission is certified. |
| **MOVE-NET-BRANCH: numeric network branch** | Full land/sea/air intervals10/6/3ms; facing integer half. Focused-owner branch can select0. | Four translation steps, including at zero wait. | Branch controls, not a complete network-session observation. |
| **EFFECT-WAIT: hit/effect rendering** | Checked local branch25ms, or50ms for specified effect flags. | Each accepted wait stores actual now. Shift/network select0; Fast Battle selects0 in the ordinary flag case but not every flagged effect. Separate50/200ms tail pauses exist. | Hit/effect pass holds shared timer callbacks. Exact complete death/explosion asset/frame mappings remain open. |
| **CURSOR-PHASE: command cursor / selection** | Shares selected-overlay phase; ordinary redraw reads it. | Redraw does not rearm or toggle it. Ensure-visible can force a phase without rearming, so selection does not promise a fresh150ms interval. | Same measured command/blink gates; hardware/OS pointer refresh is a separate unmeasured path. |
| **RECENTER-TARGET: requested map center** | Checked helper publishes target coordinates immediately, then requests redraw; zero clock reads. | Identical target skips that redraw request; delayed supplied clock values do not create intermediate targets. | This is target publication, with final redraw supplied. Keyboard/drag/outer camera interpolation remains unverified. |
| **MODEL-COUNTER: infantry component frames** | Complete checked component selector reads retained action counter modulo15. It neither advances that counter nor reads a clock. | Separate action-render fragment increments counter once per call. A counter value is not a millisecond period or visible FPS. | Autonomous idle clocks, all native-unit clips and idle pause/resume remain unclosed. |
| **TERRAIN-CLOCK: terrain / Map Animations** | **No terrain frame producer established.** | The100ms view service requests redraws; it does not prove a terrain-frame advance or catch-up rule. | Exact preference gate, water/fungus/native idle scheduling and pause behavior remain unverified. |
| **UI-SLIDE: checked window show/hide helpers** | Six paced steps using a20ms minimum and unsigned32-bit elapsed comparisons. | Fresh start time per step; all six execute under60ms overruns and wrap. Checked preference-off or network flag bypasses the paced path. | Pumps messages; no explicit local blink-modal/effect hold or focus check in these fragments. Full parent/menu admission is separate. |
| **TITLE-STEPS: title opening/closing** | Six opening and six closing steps, minimum10ms each; installed FLC header40ms is not the caller's wait. | Fresh timestamp each step; no decoder-frame skipping/catch-up. Settled menu is frame5 without a continuing title animation loop. | Inner wait spins rather than pumping. Other dialog/movie lifecycles are not inferred. |
| **STATUS-SERVICE: interface/status FLC** | Separate100ms recurring callback. | Pending-message coalescing; callback may advance/rewind an interface FLC. Asset frame speed and total visible cycle remain separate. | Checked shared callback inhibits apply. Not the selected-unit or terrain animation clock. |
| **OTHER-UI: scrollbars, modal effects, movies** | **Unverified except the named title/window paths above.** | A shared20ms constant in other UI code is a static lead, not proof that every popup or scrollbar uses one schedule. | No universal pause/close/restart policy is established. |

The full movement parameter/flag predicates remain in the preceding inventory;
the table is an index to those contracts, not a replacement with rounded values.

## New window-slide controls

The checked show/hide loops have a native interval value20ms. Each step:

1. Samples a fresh start timestamp.
2. Performs that step's copy/update work and pumps messages.
3. Samples now and waits until unsigned elapsed reaches20ms, pumping again
   while early. Rendering/work time counts toward that minimum.
4. Advances one interpolation step and starts the next step from a new sample.

The source is **now-relative per step**, not an absolute six-frame deadline
grid. It does not run a backlog of elapsed intervals or skip indices to get
back on time. Normal early samples19/20ms distinguish the boundary. A60ms
overrun still visits countdown6,5,4,3,2,1; wrap from a start near uint32 maximum
also respects the unsigned difference. Nominal six-step minimum120ms excludes
setup/final copy work and is not a measured120ms of visible animation.

The closing helper can perform an extra completion copy after its paced loop
when interpolation has not reached the final position. That copy is not a
seventh established20ms wait. Preference-off and the checked numeric network
flag bypass these paced loops. They do not establish network admission or
behavior for every window class.

Tests with Base-modal visibility/effect-hold fields set still execute all six
admitted slide steps. This establishes the absence of those local checks in
the measured fragment. It does **not** establish that the full input/controller
admits a slide behind another modal, nor that pumped messages cannot interrupt
or change a real window lifecycle.

## Camera and model findings

Four complete recenter-helper controls use a supplied final redraw: changed,
unchanged, stalled clock and wrapped destination. The target fields already
contain the final requested coordinates when redraw is entered. No clock is
read, and the unchanged request skips the redraw. This rules out adding a
timer-based interpolation **inside that helper**. It does not rule out smooth
copying/scrolling elsewhere in the map-view/input stack.

Four complete infantry component-selection controls supply retained counters
0,1,14,1000 and obtain selected component frames0,1,14,10. The selector preserves
the counter and makes no time query. Three admitted action-render increments
produce2,15,1001 from1,14,1000, again without a clock. Scene assembly is supplied
in these controls; complete earlier Scout receipts separately establish actual
frame0 composition/raster output in their bounded idle profile.

Do not turn the modulo15 into15 FPS, feed the150ms blink phase into CVR frames,
or let arbitrary redraws advance model animation. These observations distinguish
selection and action advancement; they do not yet supply an autonomous idle
timer. The ordinary renderer's cleanup-mode argument0 is also **not a frame
number** and is not used as idle-clock evidence.

## Reusable port-test observations

- **Coalesced selection:** four timer notifications before one dispatch toggle
  once; flag/health/order share the result. Redraw alone preserves phase.
- **Stalled movement:** admitted facing indices0..2 and translation0..5 all
  execute under40ms overruns, preserving each accepted actual timestamp.
- **Effect settings:** test ordinary25ms, flagged50ms, Shift0, Fast Battle's
  ordinary0/flagged50ms split, and network0 separately from asset frames.
- **Stalled UI slide:** six steps remain six at60ms per-step overruns;19ms is
  early,20ms satisfies the wait; short unsigned wrap works.
- **Recenter publication:** target changes before redraw; no fabricated camera
  tick or intermediate destination in this helper.
- **Model selection:** retained counter14 selects14;1000 selects10; neither
  selection advances the counter. Advance only at the traced action producer.
- **Title asset/pacing split:** FLC header40ms does not override explicit10ms
  caller waits; opening decodes0..5 and closing6..11 without skipped frames.

The new case file contains literal clock lists and expected indices, positions,
read/pump counts and selected frames. Referenced earlier case files retain their
existing native observations; those are reused rather than counted as new runs.

## Evidence limits and remaining questions

No live OS timer precision, vsync/compositor-visible duration, minimized-window
schedule, whole game animation coverage or end-to-end input/UI rendering was
measured. Terrain's actual frame producer, autonomous CVR/native idle clocks,
full keyboard/drag camera interpolation, sliding-scrollbar paths, other modal
transitions and movie presentation remain named gaps, not guessed defaults.

Neither a SMAX executable nor PRACX source/runtime was examined. Their differences
are **unverified**, not assumed equal or different. Existing original asset
metadata is kept distinct from measured caller pacing. All reference execution
is private and read-only with explicit external clock/presentation services;
no original binary, port UI or live user window is modified.
