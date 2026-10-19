# Turn Complete control integration

The lower console now uses the full233x23 original `turn.flc` artwork at
console-local[783,61,1016,84), Arial bold14 with baseline78, and the active
common software palette. Index0 leaves the retained console backing visible.
The original asset and palette remain external; the embedded animation palette
is deliberately ignored. The decoder admits only the measured19 declared frames
and empty physical ring; menu playback is unchanged.

The real console consumes backend `get_turn_status()` and its notifications.
`turn_complete` advances once per delivered100ms callback and hover enter/exit;
other local states rewind to frame0. The timer reschedules from delivery time,
without elapsed catch-up. Repeated hover-inside events do not advance phase.
Initial readiness paints once, and the existing sound plays only on a transition
into ready. The actual Cancel/Confirm and complete/uncomplete event flow remains.

The real popup manager suppresses timed advances while its modal is shown.
The engine has not exposed all original interface/effect/global-hold admission
conditions to this script, so this is not full native timer-gate equivalence.
Likewise, existing port PLEASE WAIT / WAITING FOR PLAYERS transitions remain:
they are functional engine states, not invented native network masks. Native
WAITING FOR OTHERS selection requires actual network-mode and completion-mask
authorities that are not published here. Other modal surfaces, native whole-game
command-loop equivalence, localized metrics and historical glyph pixels remain
outside this focused slice. See the original receipt in
[behavior/turn-complete.md](behavior/turn-complete.md).

Recurring drawing reuses one owned texture and retained decoded RGBA frames;
it does not allocate/decode or resolve file paths every100ms. Surface tests
verify repeated uploads update the UI actor revision and preserve key alpha.
