# RE-SMAC-ANIMATION-INVENTORY-001: bounded timing inventory

Profile: pinned stock SMAC. There is **no universal animation rate**: timer
requests, model frames, interpolation steps and visible presentation differ.
[The follow-up inventory](animation-timing-followup.md) adds25 native controls
for20ms window slides, immediate recenter publication and model-counter selection,
with an explicit cross-category pause/stall/asset-timing matrix.
[31 native scheduling controls](animation-timing-inventory-cases.json) supplement
the [blink/title receipt](selected-unit-blink-timing.md). No original timing is
inferred from GLSMAC/WSLg captures.

| Path | Established scheduling | Limits |
| --- | --- | --- |
| Selected Scout flag/health/order overlay | Shared150ms timer; nominal300ms cycle; pending deliveries coalesce. | Separate from CVR sprite animation. Exact reset/modal/focus gates are in the blink receipt. |
| Focused unit movement/facing display | Local ground: minimum8ms per admitted facing frame,16ms per translation step. | Outer visibility/quick-movement gates can bypass this path. Actual facing-frame count remains asset-dependent. |
| Sea/air display variants | Local full intervals10/5ms; facing uses integer half,5/2ms. | Native parameter controls, not live sea/air captures. |
| Mouse-edge map scroll | Runs on the150ms callback after blink, with a focus/instance gate and viewport recenter call. | Keyboard/drag scrolling and smooth-scroll frame timing remain unmeasured. |
| Combat hit/effect display | Measured branch waits25 or50ms; Shift/network can select0. Fast Battle Resolution has flag-dependent effects. | Not every death/effect ID or asset frame count is mapped. |
| Effect/battle tail pauses | Separate conditional50ms effect-tail and200ms battle-call-site delays. | These are extra pauses, not sprite frame durations. |
| Recurring interface/status FLC | Separate100ms callback invokes interface work that can advance or rewind a FLC. | Asset identity/header timing not newly closed; this is not a unit animation rate. |
| Periodic minimap/flagged map refresh | Another100ms callback requests interface and eligible map-view redraws. | Redraw frequency does not establish terrain frame cadence. |
| Ordinary idle Scout model | Blink's model-visible off phase retains ordinary idle rendering. | No new idle CVR frame clock; other units' idle clips remain separate. |
| Title opening/closing | Six explicit steps each, minimum10ms per step, despite FLC header40ms. | Other popups, sliding GUI controls and movies are separate unknowns. |

## Unit display pacing

The shared display routine is reached from movement-wrapper and focused-combat
callers. Its normal local full interval is16ms for land, integer `2*16/3=10ms`
for sea and `16/3=5ms` for air. Facing waits integer half; translation waits the
full interval. For ordinary0/1 configuration settings:

| Configuration | Translation steps | Land translation nominal minimum |
| --- | --- | --- |
| `FastUnitAnim=0`, `SmoothUnitAnim=0` | 6 | 96ms |
| Smooth on, Fast off | 9 | 144ms |
| Fast on | 3 | 48ms |

These totals exclude facing, setup, rendering/copy work and caller pauses. They
are not measured screen-visible durations. A numeric network-mode branch uses
4 translation steps and full intervals10/6/3ms by domain; its focused-owner case
sets the wait to0. This is branch evidence, not a complete network-session test.
Zero waiting still advances the admitted steps individually.

The loops use local `timeGetTime` timestamps, signed32-bit elapsed comparisons
and message pumping. Each step replaces its prior timestamp with **actual now**.
Overruns therefore do not preserve an overdue absolute deadline, drain missed
intervals or skip interpolation indices. Native controls retain facing0,1,2
and translation0..5 with40ms overruns; short32-bit wrap also passes.

No explicit focus check occurs in the measured loop bodies. Their message pump
can run other UI logic; full outer focus/modal/minimization behavior remains
unmeasured. Do not transfer the scrolling focus gate to every movement loop.

## Hit/effect pacing and shared services

In the measured local branch, effect flags intersecting18 (masks2or16) select50ms;
the other ordinary case selects25ms. Shift selects0. Fast Battle Resolution
selects0 for the ordinary case but leaves the measured flag2 case at50ms.
Network mode selects0. This is not a mapping of every effect ID to a death or
attack animation.

The wait uses signed elapsed comparisons and message pumping, then stores the
accepted clock value. The renderer holds an inhibit during its animated pass,
which holds blink and100ms callbacks. **An earlier label was too broad:** the
traced inhibit writer is the hit/effect routine, rather than a general movement
pause. Blink cadence/coalescing conclusions are unchanged.

The separate delay helper pumps messages and compares **unsigned**32-bit elapsed
milliseconds; a50ms wrap control passes. The title's10ms inner wait instead
spins on the clock. Shared clock source does not mean shared phase or deadline.

The100/150ms timers use the verified pending-message mechanism: one pending
application callback per timer, no elapsed-time catch-up. Base/checked modal
visibility and lifecycle/effect inhibits can hold them. Blink toggles before
its later scrolling focus check; this does not establish visible unfocused
rendering for every path.

## Explicit unknowns

No recurring terrain-frame producer was established here. The follow-up adds
specific20ms sliding-window and recenter-target controls. Map Animations,
terrain variants, autonomous idle clips, keyboard/drag/camera interpolation,
other sliding windows/scrollbars and movies need their own caller/asset traces.
Do not borrow one of the rates above for them.

No SMAX binary or PRACX source/runtime was analyzed: differences are
**unestablished**. These controls bind stock SMAC scheduling fragments with
supplied clocks, keyboard/message services and empty view-copy work. They do
not certify live timer precision, vsync/compositing, alt-tab/minimization
scheduling, whole-action frame counts or end-to-end animation pixels.
