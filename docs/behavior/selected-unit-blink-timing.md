# RE-SMAC-BLINK-001: selected-unit blink and title timing

Profile: pinned original SMAC, ordinary local-human command mode, selected
conventional Scout. [Numeric cases](selected-unit-blink-timing-cases.json).
This closes the code-level timing contract. Controlled original-code execution
checks timer registration, delivery, phase changes and draw decisions; it does
not measure a live Windows compositor, minimized-window scheduling or an entire
original gameplay window loop.

## Cadence and clock

The selected-unit blink uses a shared repeating **150 ms Win32 timer**.
Each admitted application callback toggles one global phase exactly once.
Normal delivery therefore gives a **150 ms half-period / 300 ms full cycle**.
The provisional 500 ms half-period is incorrect for this path. A separate
500 ms timer services network-related UI work.

This is a timer-message clock, not a per-unit timestamp or a calculation from
the system wall clock. The blink callback does not read elapsed time and the
timer trampoline ignores its supplied tick timestamp. Requested milliseconds
are not a guarantee of exactly timed presentation under load.

The ordinary visible phase draws both model and overlay. During the other
phase, with Whole unit blinks disabled and the mouse sufficiently far away,
the same idle Scout model remains drawn while **flag, health bar and order
overlay are omitted together**. This does not select another CVR animation
frame or run the movement/combat animation routine.

## Delivery and missed frames

The OS timer trampoline posts a main-window application message and sets a
pending latch. Further notifications while that message is pending do nothing.
The main-window dispatcher clears the latch **before** calling the registered
callback. Four notifications before one dispatch produce one toggle, not four.
An arbitrarily large or wrapped callback timestamp changes nothing.

There is no elapsed-time parity calculation, skipped-frame counter, draining
loop or per-unit deadline to catch up. A callback rejected by a gameplay/UI gate
does not advance phase. The next admitted delivery toggles the retained phase
once. A dispatcher-wide inhibit can discard a pending callback while clearing
its latch; it does not defer that toggle for replay.

For a port, preserve phase across redraws and coalesce delayed work. A steady
clock is suitable for scheduling requests, but computing phase solely from
absolute elapsed time or draining every missed interval changes the measured
behavior. Resetting phase on every Refresh also changes it.

## Selection, refresh and phase reset

The phase is shared by map views and command-cursor presentation, not owned by
an individual unit. Its zero-initialized state is the ordinary unit-visible
phase. Starting or stopping the shared timer service does not itself reset it.

Ordinary selected-tile redraw and UI refresh read the existing phase. Repeated
redraws do not toggle it or restart the timer. Selection is **not an
unconditional new full-duration blink cycle**. The traced ensure-visible/
recentering operation forces the visible phase when the requested point is
outside the primary view's accepted rectangle and that view is in ordinary
unit-control mode. If the point is already inside, phase remains unchanged.
A special view mode bypasses this reset. In command-cursor rather than unit
control, the same operation selects the opposite phase.

That reset does not rearm the shared timer: the next already-scheduled accepted
tick may arrive soon afterward. Do not promise a fresh 150 ms visible interval
for every selection. Other explicit editor/lifecycle initialization is outside
the ordinary selected-Scout profile.

## Visibility gates and mouse edge cases

The ordinary selected-unit off phase requires a valid selected human unit on
the drawn square, unit-control mode, a pending command, no assigned order and
no draw suppression. Otherwise the ordinary overlay remains visible, subject
to the surrounding normal visibility rules. Network special-display gates also
exist; multiplayer behavior is outside this profile.

In the admitted off phase:

- **Whole unit blinks enabled:** omit the entire selected unit draw.
- **Preference disabled, mouse nearby:** also omit the entire selected unit
  draw, rather than only its badge.
- **Preference disabled, mouse farther away:** draw idle model without its
  flag/health/order overlay.

Mouse proximity is calculated in the game's parity-map coordinate units.
Compute absolute x distance, replacing it with map width minus that distance
when the world wraps and it exceeds half the width. Add absolute y distance,
clear the low bit of the sum, and treat a result **at most 2** as nearby.
Distances 0 and 2 omit the whole draw; distance 4 preserves the model and omits
the overlay. A controlled odd projection distance 3 also rounds to 2; this is
an arithmetic discriminator, not a claim that such coordinates identify a
normal parity-map tile. Flat worlds do not use the horizontal wrap correction.

## Focus, modal windows and pause

Focus loss alone is not an explicit blink pause in the traced native path.
The window's activation handler neither resets phase nor stops the shared
timer. Inside the blink callback, the explicit focus/instance check occurs
**after** phase toggle and selected-square redraw; it gates subsequent
mouse-edge scrolling. Controlled active/inactive and mismatched-focus cases
still toggle the blink phase.

There are separate real holds: global lifecycle/animation inhibits, a displayed
Base window or another checked modal window, shutdown, and absence of the
required ordinary command-loop state can return before the toggle. The
hit/effect-animation caller explicitly holds a timer-callback inhibit while it
runs. These holds retain phase and do not accumulate missed toggles.

The main timer stop operation destroys the three 150/100/100 ms timers and
clears their registrations; it leaves the separate 500 ms network timer.
Restart registers the timers again while retaining phase. This is distinct from
a callback hold. No blanket rule that every popup or every game pause stops
the timer has been established. Actual OS throttling while minimized and the
entire host focus/message-pump lifecycle were not newly measured.

## Shared services and title animation

The shared timer wrapper uses the multimedia periodic timer service below
50 ms and the window timer service at 50 ms or above. Both post through the
same pending-message mechanism. Startup registers 150 ms blink, two separate
100 ms UI services, and the separate 500 ms network service; these are not
interchangeable animation defaults.

The separate delay helper and the title caller use **unsigned 32-bit
millisecond `timeGetTime`** differences. The delay helper pumps messages while
waiting. The title transition's inner wait spins on the clock instead.

For the installed `openitem.flc` used by the
[title-screen receipt](smac-title-screen.md), metadata is **12 frames,
354×75 pixels, 40 ms header speed**. Each native decoder call advances exactly
one frame. The title caller explicitly performs six opening calls (frames
0–5), then later six closing calls (frames 6–11). It records a fresh timestamp
for **each** step, draws that frame across the menu rows and presents it, then
waits until unsigned elapsed time reaches **10 ms**.

Both opening and closing use that same minimum-10ms rule. Processing time is
included: elapsed 9 ms waits again; 10 or 40 ms proceeds immediately. An overrun
does not skip or repeat decoder frames. There is no automatic 40 ms or 100 ms
wait in this caller. Its six waits establish at least 60 ms of step time when
the clock advances normally, plus any setup/other work. They do not certify
60 ms of compositor-visible animation. The settled open menu is frame 5 and
has no continuing FLC frame clock in this path.

## Evidence limits

66 assertions pass against the pinned executable with OS timer/message/clock
services and unrelated presentation work supplied at declared boundaries.
Selected-tile tests enter an explicitly admitted branch and execute the native
unit-validity helper, proximity arithmetic and draw-flag selection. A separate
native model/overlay fragment verifies idle-model dispatch and overlay omission.
The title tests execute the actual six-step caller loops with supplied decoder
and presentation; previously frozen title evidence independently verifies all
12 actual decoded frames. No new blink sprite pixel comparison, live stopwatch
measurement, broad CVR timing survey or production implementation is claimed.
