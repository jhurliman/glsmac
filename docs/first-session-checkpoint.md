# First-session checkpoint

Next milestone after the current Former/controller/render integration. Gameplay
breadth waits until this path is usable and visually reviewed. This is not a
claim that SMAC or Alien Crossfire is complete.

## Acceptance

Exercise normal new-game setup, starting placement, movement/animation,
selection/stats, the Base screen, production/research, closing and reopening
screens, a new world in the same process, application close/relaunch, and actual
campaign disk save/reload. Retain screenshots or a short recording and compare
the relevant view with the original. Record executable revision, ruleset,
settings and asset identity; a SMAX observation is not SMAC rule evidence.

Use one cached Linux build and affected tests during fixes. Run the platform
matrix and native interactive checks on the combined renderer/session
checkpoint. Existing tests of world snapshot codecs do not establish working
disk saves or menu navigation.

## Concrete gaps from source inspection

| Path | Gap | Required outcome |
| --- | --- | --- |
| Setup | Faction selection and six-difficulty startup/governor policy are merged in PRs #172 and #174. Their affected fixtures and required CI pass. Rendered validation remains. | Every offered selection starts correctly; the selected faction is explicit. |
| Units | Former/Colony use temporary markers; conventional scale/pivot is port policy. | Verify placement, image, scale, movement and selection against reference captures; list remaining visual differences. |
| Unit controls | A visible command panel now exposes currently available Former F/R/O/S/M, cancel C and Colony B actions, plus saved work/order state. Rendered placement remains unverified. | Actions and current orders are discoverable without reading source. |
| Selection | Retention, wrap distance and record-order ties are integrated and pass focused controller tests. | Work continues and selection changes correctly across movement, work and turn boundaries. |
| Base | Treasury and energy allocation now use authoritative values and pass focused UI/native checks. Eco-damage remains unavailable; production offers a limited unit set. | Verify worker edits, quantities and picker confirm/cancel; close/reopen without changing simulation values. |
| Research | Required-choice script passes with UI doubles; native rendered modal is unverified. | Selection, scrolling, confirmation, repeated owner requests and reset behave correctly. |
| Save/load | PR #175 is merged with local SMAC campaign files and menu controls. Serialization now runs at an exclusive owner checkpoint; focused callback/reset/error tests, fresh review and required CI pass. Rendered menu/save/reload validation remains. | A local campaign envelope restores frozen setup, roster/local owner and the complete world without replaying first-turn creation. |
| Lifecycle | Latest retained interactive receipts predate this renderer. | New game, close and relaunch pass with the checkpoint build. |

Movement now uses the existing current-allowance calculation for damaged and
promoted units beside Bases. Ten live command compositions cover Infantry and
the selected Spartan Rover, including Elite/damage thresholds and exhaustion.
The cached Linux Debug build passes 19 affected groups with memory tracking;
targeted clang-tidy 15 passes in 2.15 seconds. These checks compose the recovered
allowance, KEY-001 admission and ordinary movement consumers; they are not new
original-game captures. Conventional attacks remain unintegrated, and the
prepared size-one Colony choice still awaits complete last-base disposition
before default activation.

## Reference and platform routes

Original Windows reference: the installed SMAX PRACX shortcut, with its original
installation as working directory. The October 15 retained attempt crashed
after accepting the first Base name. An October 17 launch initially stalled
with the legacy movie player present. After backing up `Alpha Centauri.ini`
and changing only `DisableOpeningMovie` from 0 to 1, the original menus became
responsive. Make Random Map, Small Planet, Librarian, standard rules and Gaia
reached the faction setup screen. Bringing the still-responsive fullscreen
window forward recovered capture after `FrameArrived` timeouts. Accepting the
leader name, Planetfall and the first Base name rendered a map frame labeled
Gaia's Landing, then the process crashed. Windows Application event 1000 at
15:07:24 local recorded exception `0xc0000005` in `terranx_PRACX.EXE` at module
offset `0x001269e4`. This reproduces the post-first-Base failure; it does not
establish a stable playable map, Base screen or completed visual comparison.

The private local receipt, original INI backup and setup/first-Base captures are
in `C:/Users/jhurliman/.codex-validation/glsmac-reference-startup-20261017`.
The observed `terranx_PRACX.EXE` SHA-256 is
`84432e3a1465a05f32b5bb70693f5c4099661d5c1511dbbf27233b4890071b1c`.
This is SMAX presentation evidence; it does not establish SMAC rule behavior.

Windows assets: `C:/Program Files (x86)/GOG Galaxy/Games/Alpha Centauri`.
Honey assets: `/Users/jhurliman/Documents/Games/Alpha Centauri`.
Use matching `GLSMAC_data`, an isolated prefix, windowed mode and captured logs.
Start with the normal menus, then controlled Gaia and Sparta quickstart cases
on a 32 by 32 map. A fixed map seed does not pin every gameplay RNG stream.
The combined Windows and Mac checkpoint is now `3f17cb36`. Both native Debug
builds and the focused campaign-file test pass; Windows also passes three
production-picker groups. Honey is open for the requested human session; its
result is pending. Windows normal-menu capture is black at both 1280 by 800
with skipped intro and 800 by 600 with intro, despite successful asset loading
and no logged GL error. The smaller window closes normally. UI composition
versus presentation/capture is being diagnosed; do not count this as a visual
pass. A temporary three-file renderer probe showed a nonempty root cache and
complete FBO. A corrected one-file probe then captured the correctly drawn
800 by 600 menu directly from the backbuffer before swap: 304876 nonblack
pixels, no GL read error. The window capture remained black. This narrows the
remaining issue to presentation/capture rather than missing menu composition;
it still does not establish what a human sees on aleph0. The final diagnostic
closed normally without reported GL leaks. Exact source was restored; the
Windows cache binary remains explicitly diagnostic until rebuilt. Receipts
and private frame captures are under `.codex-validation/render-diagnostic-3f17cb36`.
Honey's earlier user-confirmed checks remain evidence for their old revision only.

The later Linux checkpoint contains gameplay code through `9cd97fac` (its title
still embeds `c14d32c7`). Focused campaign-owner and startup/research tests pass.
Its menu renders through WSLg, but the 1024 by 768 Windows desktop clips the
scaled window. It was closed normally before further source changes; no game
was started and no Linux interactive pass is claimed. The user subsequently
restored aleph0's usual resolution. A Linux `5902aee1` menu launch loaded its
assets and entered the main loop, but the Windows capture helper failed with
`system cursor manager did not become ready` and then `FrameArrived timed out`.
No game input was sent. The test process closed normally after SIGTERM, with
no reported memory/OpenGL resource leaks; allocation tracking was not enabled
for that launch. Its private log is
`C:/Users/jhurliman/.codex-validation/first-session-5902aee1/interactive.log`.
Honey's human result and Linux interactive validation remain pending. These
presentation checks do not trigger another platform build for routine gameplay
changes.

## Visual-defect record

Populate this from actual checkpoint screenshots, not source assumptions. Earlier
observations to recheck include jagged map edges, clipped Base badges and a Mac
texture warning. Do not label them fixed until the current session demonstrates
it. Save each issue with the screen, reproduction steps, expected/reference
behavior and the fixing revision.

## Focused UI integration — 2026-10-17

Source `b238976a` combines faction selection, live treasury, discoverable
Former/Colony commands and Base energy allocation. One cached Linux build passed
12 affected native groups with allocation tracking, including startup owner
selection, treasury restore, command dispatch and the real Base screen path.
The four scripts `faction_selection`, `minimap_treasury`, `unit_commands` and
`base_energy_display` also passed with the same binary and allocation tracking.
Focused Base.cpp LLVM 15 analysis completed with zero failures in 5.52 seconds;
the other new native query implementations passed their focused source checks
before integration. No new full platform or interactive visual receipt is
claimed by this batch.
