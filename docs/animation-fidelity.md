# Animation fidelity and live caller gaps

The original has distinct clocks for selection, unit display, effects, window
slides. Movie timing remains unverified. The [timing inventory](behavior/animation-timing-inventory.md)
and [25-control follow-up](behavior/animation-timing-followup.md) distinguish
requested intervals, model-frame indices and visible presentation. All original
evidence here is stock SMAC; SMAX/PRACX remains unverified.

The subsequent [57-control lifecycle receipt](behavior/animation-idle-and-overlay-lifecycle.md)
closes pending-notification transitions and distinguishes the 100ms route/order
service from model frames. Idle Scout/Rover draws preserve the retained model
counter, including after supplied long stalls; native-life component selectors
also consume retained counters. The Gravship workshop preview has a separate
draw-driven increment. No autonomous terrain/water frame producer is established.

## Current port callers

This is a source audit of the port, separate from the native reference controls.

| Path | Current implementation | Remaining work |
| --- | --- | --- |
| Selected-unit overlays | Shared150ms timer phase, synchronized flag/health and redraw preservation in `UnitManager`/`Unit`. | Complete original lifecycle/effect/modal and preference gates; ensure-visible behavior. Existing measured scope is documented in `installed-planet-campaign.md`. |
| Canonical conventional movement | `UnitManager::RelocateConventionalUnit` changes the displayed tile immediately. | Add the measured facing/translation presentation, preferences and visibility admission while keeping simulation completion independent of animation. |
| Legacy movement | `Unit::MoveToTile` uses `Scroller` with125ms requested duration. `Scroller` uses10ms fixed-grid ticks; `Unit::Iterate` drains them. | Replace this path with the appropriate measured display schedule when admitting legacy/native units; changing a duration constant alone cannot reproduce the step count or stall behavior. |
| Generic sprite effects | `Animation` divides declared duration by sprite count and drains its repeating timer. | Bind actual effect identity/flags, input preferences, asset frame count, now-relative pacing and tail pauses. Do not apply25ms to every effect or derive pacing solely from asset metadata. |
| Settled SMAC title | Original artwork/frame and font palette were validated at1024x768. | The settled screenshot does not prove six-step opening/closing timing; retain separate presentation acceptance. |
| Idle model / terrain | No complete original autonomous frame producer established by the published timing receipts. | Trace actual frame advancement and its gates before assigning a clock. Counter modulo15 is a frame selector, not15FPS. |

Callback admission and draw visibility are separate decisions. A Base/effect/
lifecycle hold consumes a pending selection notification without changing its
phase or replaying it later. Ensure-visible can change that phase before an
already-pending dispatch; it does not rearm the timer. Neither Map Animations
nor Whole Unit Blinks changes this callback cadence, and the settings occupy
distinct preference words. Do not turn the current port's selection eligibility
check into a universal modal-pause or model-animation clock.

`util::Timer` and `util::Scroller` are shared by unrelated UI code. A blanket
change to their catch-up policy would alter those consumers without proving
their original contracts. Implement each game's display scheduler at its actual
caller, preserving the existing timer services for consumers with different rules.

## Implementation and acceptance order

1. Connect normal local ground movement to a presentation-only sequence: real
   source/destination and facing frames, normal/smooth/fast step counts, then
   actual-now waits. Backend commands, saves and turn progress must remain
   independent of renderer delivery. Cover queued moves, Unit removal and map
   teardown so a canceled presentation cannot retain a dead Unit/Tile.
2. Bind actual hit/effect identities and frame assets to the measured flag and
   preference branches. Add their inhibit writer and tail delays, then connect
   the shared overlay callback gate. Verify normal, flagged, Shift and Fast
   Battle branches separately.
3. Connect measured window/title transitions through their actual UI callers;
   continue terrain/idle and SMAX evidence work without borrowing another clock.

Use literal native clock/index cases for each admitted scheduler, including
short clock wrap and delayed deliveries. In actual rendered scenarios check
all intermediate frames, final position, input and teardown. Record minimum
scheduler waits separately from compositor-visible intervals. Keep vsync,
audio continuity and hardware frame-budget measurements as independent
performance gates; the original's low animation rate is not a rendering-FPS target.

The published native controls are reference evidence, not passing port scheduler
tests. None of this inventory enables the unfinished movement/effect paths or
claims full animation fidelity.
