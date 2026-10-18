# Original-aspect presentation

The rendered game area is fixed at 4:3. The OS window remains resizable and
fullscreen uses the selected display; unused space is cleared to black. This
preserves the original title artwork's proportions without cropping. It does
not establish menu, movie, or gameplay rendering parity by itself.

## Rendering and input

`graphics::PresentationViewport` fits the largest centered multiple of 8x6 in
the available area. Both dimensions remain even, retaining the renderer's
existing texture-tiling constraint. A window narrower or taller than 4:3 gets
letterboxing; a wide window gets pillarboxing. At odd sizes bars can differ by
one pixel. SDL has a 320x240 minimum window; minimized/empty resize notifications
leave the last valid internal dimensions intact.

UI layout, camera projection, cached textures and picking buffers use the
content dimensions. Default-framebuffer rendering uses a separately fitted SDL
GL drawable rectangle, supporting drawable/window pixel differences. Offscreen
framebuffers use a local origin and their own texture dimensions; window bars
must never become part of a cached widget or picking buffer.

Mouse coordinates are translated from SDL window space before dispatch.
Presses and wheel events in bars are ignored. Moves outside clear hover, and a
release outside still unwinds a press/drag that began inside. Outside positions
are never clamped onto a control. Leaving the content also stops map edge
scrolling. Keyboard and fullscreen commands retain their existing behavior.

## Reference discrepancy, 2026-10-18

The user supplied a 1024x768 SMAC title-screen screenshot. Its executable hash,
patch configuration and capture process have not been independently recorded;
it is a supplied visual reference, not a fully pinned executable observation.
The actual GLSMAC Windows renderer capture preceding that report was 1920x1080.
Source inspection found these distinct issues:

- `mainmenu.gls.js` stretches `openinga.pcx` across the rendering area. A 4:3
  viewport fixes the resulting widescreen distortion.
- `sliding.gls.js` builds frames from `console_x.pcx` and `interface.pcx`, uses
  custom green RGBA fills, and requests `arialnb.ttf:20`. The supplied SMAC
  screen has blue buttons and visibly different lettering and spacing.
- The button dimensions/spacing are fixed pixels; the background scales with
  the viewport. Matching the aspect ratio alone does not align the menu at
  every resolution. SMAC/SMAX asset selection is not yet separately verified.

Keep original screenshots and assets outside Git. For presentation acceptance,
capture each original executable and GLSMAC at the same content resolution,
record executable/asset identities and selected mode, then compare background
landmarks, button bounds, fonts/colors, highlighted state, and keyboard/mouse
responses. First establish native 1024x768 parity; separately evaluate scaling.
Movies must preserve their authored aspect when playback is implemented and
validated. Do not call a successful backbuffer capture proof of visual parity.

## Focused validation

The native test `4:3 presentation fits windows and maps only the rendered area`
checks known native/wide/square cases, first/last pixel and all four outside
boundaries over a grid of sizes, centering, exact aspect, even dimensions and a
2x drawable case. Renderer smoke uses actual backbuffer pixels with a disclosed
private SDL-event/readback harness; it does not alter menu/world rules.

Linux WSLg smoke at 1920x1080, 1000x1000 and 1024x768 produced GL viewports
(240,0,1440,1080), (0,125,1000,750) and (0,0,1024,768), respectively, with no GL
errors. A bar press left the menu pixel-identical, and the actual final Exit
button closed the process with exit0. The current button styling remains a
known discrepancy. Hosted Windows/macOS checks and native fullscreen/Retina
interaction must be assessed separately before claiming platform validation.


Review follow-up: an out-of-bounds mouse-up alone cannot reach the pressed DOM
control. Input now establishes the hover path for a press without prior motion
and sends mouse-out before a bar release. Root hover cleanup also runs when the
pointer leaves the root. Scrollbar captures retire in their global left-release
handler rather than waiting for a geometrically relevant slider event. Global
handler dispatch snapshots IDs and copies the current callable so self-removal
cannot invalidate iteration or destroy the executing callable.

The private real-DOM SDL/GDB regression exercises a button press without motion,
release in a bar (no click), a subsequent genuine click (one click), and repeated
scrollbar drags released in bars. Handler retirement is observed at the real
UI::RemoveGlobalHandler breakpoint, before widget destruction. Private controls
exercise the production renderer/event path without changing gameplay rules.
