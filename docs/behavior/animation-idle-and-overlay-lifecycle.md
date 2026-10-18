# Idle drawing and selected-overlay lifecycle

RE-SMAC-ANIMATION-LIFECYCLE-001, 2026-10-18. Pinned stock SMAC. This extends
[animation timing](animation-timing-followup.md) with **57 new native controls**.
[Literal cases](animation-idle-and-overlay-lifecycle-cases.json). No SMAX or
PRACX executable/source was examined; no difference or equivalence is asserted.

## What advances an idle model

Three consecutive complete ordinary idle-map calls for the Scout and Rover
preserve the retained action counter and read no clock, including supplied
100,000 ms stalls between calls. With retained counters 0, 1, 14, 15 and 1000,
the Scout body selects frames 0, 1, 14, 0 and 10 respectively on every redraw.
A separate component submitted by this profile selects frame 0. The checked
Rover scene has no calls to that Scout animated-component submission path.
The numerical idle caller and native component selectors execute; component
assembly, final raster/cleanup and morale results are supplied services.
These tests do not revalidate model pixels or installed component decoding.

The complete native-unit component selector also reads the retained counter
without advancing it or reading time. Mind Worms and Isle of the Deep use
frame `30 + (counter % 5)`; Locusts use the counter directly. These statements
cover the nonnegative supplied counters in the cases. They do not establish
the final asset decoder's normalization of an arbitrary frame number.

The direct executable-operand inventory finds three writes to the shared
model counter: the already traced action setup initializes it to 1; the
action-render path increments it once; a Gravship component path increments
it once when the Design Workshop window is visible. Native controls confirm
the last branch selects frame 14 from counter 14, then leaves counter 14 when
hidden or 15 when visible. This is a draw-driven workshop exception, not
evidence of an autonomous map-idle timer. The inventory is not an indirect
alias proof. It found no additional direct writer for an elapsed-time idle
producer in this stock executable.

For the port, preserve the distinction between the retained frame selector,
action advancement, workshop preview and selected-overlay blink. Do not advance
the Scout merely because its square is redrawn, normalize its retained frame
to zero on every idle draw, or infer 15 FPS from modulo 15. An independent
autonomous idle clock remains unestablished beyond these traced paths.

## The 100 ms map service is a different counter

The complete map-service callback decrements a separate unsigned 32-bit counter
once per admitted invocation; zero becomes 4,294,967,295. Its traced draw
consumers use that counter as a phase for patterned route/order lines. It
does not change the model counter or read elapsed time.

The callback invokes its subordinate interface redraw and considers eight map
views. The primary nonnull view is eligible regardless of its active field;
other views must be active. Each also needs at least one of the three relevant
route/order display flags. Only eligible views receive the supplied redraw
service with its update argument true. Thus this is not a whole-map terrain
frame advance every 100 ms.

Native lifecycle-inhibit, effect-in-progress, visible Base window and the
second checked modal window each return before decrement/redraw. Focus
mismatch, the shutdown field and absent command-loop state do not hold this
callback in the checked controls. Those last gates differ from the selected
blink callback; do not merge their admission logic into one generic pause flag.
The underlying timer registration/coalescing contract is reused from the
[selection receipt](selected-unit-blink-timing.md), not measured anew here.

## Pending selection notifications and transitions

These cases run real timer notification and dispatch around a transition:

| Transition after a notification is pending | First dispatch | Following admitted notification |
| --- | --- | --- |
| Base modal opens, effect begins, or lifecycle inhibit sets | Clears the pending latch; callback holds phase 1 | After the hold clears, phase 1 toggles to 0; no replay of the held tick |
| Primary ensure-visible selection path forces phase 0 | The already-pending dispatch toggles that current phase to 1 | Next dispatch toggles to 0 |
| Whole Unit Blinks / Map Animations preferences change | Phase 1 toggles to 0 | Next dispatch toggles to 1 |
| Several delayed/wrapped-time notifications coalesce | One dispatch toggles phase 1 to 0 | Next admitted dispatch toggles to 1 |

All controls retain the same timer handle. The selection test executes the
native primary ensure-visible phase-change path with rectangle query/recenter
presentation supplied; it is not a full user click or unit-selection command.
The setting change does not rearm the timer or promise a fresh 150 ms interval.
Flag, health and order presentation continue to share the established phase;
this receipt adds lifecycle sequences, not a new health-bar raster claim.

## Preferences and terrain boundary

The actual Audio/Visual checkbox projection confirms distinct controls for
Map Animations, Sliding Windows, Sliding Scrollbars and Whole Unit Blinks.
Map Animations and Whole Unit Blinks are in different preference words. Four
native projection controls distinguish their on/off states. Installed dialog
text order and its construction associate the Map Animations checkbox with
the former setting; it must not be confused with the Monuments control.

Changing Map Animations does not change either the tested complete idle calls
or the map-service callback. The latter's decrement and eligible view dispatch
are identical with the setting off/on. This does **not** establish that the
preference is globally unused, nor that stock terrain can never animate.
The direct-reference and named-renderer inspection has not established an
actual water/fungus/terrain frame producer or its preference consumer. Do not
assign that service's 100 ms cadence, the shared 150 ms overlay phase, or the
model's modulo 15 to terrain. A terrain-specific animation needs its own
producer/asset/presentation evidence before a parity claim.

No live OS precision, compositor-visible frame duration, minimized-window
scheduling, complete pause command, all modal classes or shortcut behavior
was measured. These cases use explicit private original execution with
supplied external/UI services. Existing measured movement/effect/title/window
waits remain unchanged; requested waits are not measured visible frame times.
