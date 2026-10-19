# Conventional Scout playback development checkpoint

The actual admitted conventional attack now prepares an owned presentation
request from the resolved round history before committing its result. The
backend still commits casualty, surviving state, hostility, palette and RNG in
one transaction. Playback never calls back to advance combat or consumes RNG;
normal save/Start retains the committed result, without replaying presentation.

The frontend queues subsequent requests behind the battle, keeping its two
stable Unit display identities available until death frames/tail finish. Health
snapshots precede grouped hit effects; all16 frames render in order. A late
presentation tick schedules the next frame from actual now rather than skipping
or catching up. Audio actors finish independently of frame tails. World cleanup
retires queued payloads and stops its audio; retired world queues are not drained.
Game input is inhibited during the presentation while mouse-button bookkeeping
remains balanced.

The source frame rectangles, row0 hit/death, sound bindings, grouped damage and
normal minimum waits are connected. The pure clock also supports the measured
Fast Battle waits, but the current frontend uses normal mode; preference wiring
is still required. Ordinary local assets are supplied by the installed game.

Validation: the original19-round seed3 timeline, six grouped hits, death and112
frames match the numeric receipt. Exact normal/fast and late-clock completion
controls pass, as does deep-copy ownership of the frontend payload. Actual
hostile Scout and defended-Base commands emit their timelines while existing
results, RNG, casualty wrappers and save/Start checks pass. Five changed
production TUs pass correctness analysis. Three focused groups pass ASan/UBSan
with five production and two test TUs instrumented, remaining current normal
objects, leaks disabled. This does not cover graphical playback/cancellation.

Remaining before review: actual graphical battle capture and cancellation,
destination-dependent indexed CLUT conversion (hit bank3/death bank0, including
253-to255 substitution), verified projection/zoom alignment, sound setting and
channel behavior, and preference/skip policy. The current ordinary texture
loader is a provisional display path; its PCX palette alone is not the original
compositor. No pixel fidelity, hardware audio or frame-rate claim is made.
Separate CVR body/facing clips and broader combat domains remain open.

[Original audiovisual contract](scout-combat-audiovisual.md).
