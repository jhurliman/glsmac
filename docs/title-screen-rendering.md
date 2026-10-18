# Original SMAC title rendering

The settled 1024x768 original SMAC title profile now passes exact whole-screen
RGB comparison against the supplied original capture: 786,432 pixels compared,
zero changed pixels. This is the opening menu with Start Game active, Gamma
Correction 100, openinga.pcx and openitem.flc frame 5. It is one fidelity gate,
not a claim of complete SMAC/SMAX presentation or gameplay.

The candidate was captured from the actual GLSMAC OpenGL back buffer on the
cached Linux Debug build under WSLg/llvmpipe. A small external SDL interposer hid
the window to prevent desktop window-manager resizing; it did not replace game
rendering, assets or pixels. The capture reports a 1024x768 drawable/viewport and
GL error zero. A real mouse click on Exit Game completed normal shutdown with
memorydebug enabled and no tracked memory/OpenGL leaks. Captures and original
assets remain outside Git.

Reference PNG SHA256:
`732ec16f1bed508f0953ab3e97ed68432c66b9b6893ded2571d525ca6602aa3e`.
Candidate PNG SHA256:
`b0b85c5839f1d0455290fdb9d9b3cc7ad7629b4023542ad400be221260d9a9fa`.

## Rendering contract

The [native title receipt](behavior/smac-title-screen.md) defines the indexed
palette, menu geometry and text metrics. The FLC decoder retains game indices.
The compositor repeats a frame over seven 75-pixel rows at the lower right:
index 9 is transparent, index 0 applies the original 20/80 blue lookup to the
background index, and ordinary artwork indices use the gamma-corrected common
palette. Lookup generation occurs after gamma correction, preserves native
integer truncation, and selects the first game entry on equal RGB distance.
Unsupported reserved indices are rejected rather than assigned invented colors.

`openitem.flc:smacmenu(openinga.pcx,5,7):stretch()` requests that composed texture.
The source operation must come first. Decoded source assets and frames are
cached; each surface owns only its current composed output, so resizing does
not retain an RGBA texture for every previously visited window size. Optional
fourth/fifth arguments specify the presentation viewport width/height; omitted
dimensions use the current viewport. The title refreshes its current frame on
resize. Tint sampling maps displayed pixel centers into the stretched PCX,
and display-palette textures retain nearest-neighbor sampling so the tint
lookup addresses actual palette entries rather than interpolated colors.
Native opening frames 0..5 and closing
frames 6..11 use six minimum-10ms steps; file header speed is metadata, not the
caller's presentation cadence. The settled screenshot does not certify every
transition frame or host presentation timing.

Fonts accept `<filename>:<size>:mono` as an opt-in monochrome raster mode.
Default fonts retain grayscale rendering. FreeType expands packed mono rows
using their pitch; native title labels use Arial Bold at 18 pixels with no
shadow. `text.measure()` returns glyph-advance width and text height so labels
can be placed using the original centering rectangle and baseline. Measured
text bounds and button hit rectangles are separate.

Arial Bold is resolved from game/custom data first, then conventional system
locations on Windows, macOS and Linux (including the Windows installation when
running under WSL). A system without that font remains usable with the existing
Arial Narrow Bold asset and logs the fallback, but that fallback **does not pass
this golden profile**. Exact rendering requires an owned Arial Bold font in an
admitted location; no font file is redistributed here. Native macOS/Windows
visual validation and other font versions remain separate gates.

The comparison exposed a shared UI cache bug: right/bottom geometry edges are
exclusive, but viewport clipping used width-1/height-1 and removed the final row
and column. Cache clipping now admits width/height. A native regression covers
both a full viewport and its one-pixel bottom-right rectangle.

## Verification and remaining gates

The native FLC tests cover palette updates, BRUN/SS2 state, malformed bounds and
compositor transparency/tint/background sampling. Focused ASan/UBSan checks run
the production decoder/compositor with the same synthetic cases. Targeted C++
correctness analysis covers changed production translation units and affected
geometry consumers. The exact comparison uses the unmodified
`tools/reference/compare_title_screen.py`: no masking, tolerance, alignment or
rescaling.

A hidden-window OpenGL resize sequence (1024x768, 1280x960, 800x600, 1024x768)
also checks the displayed background/tint mapping independently against the
indexed assets. At each size all 13,958 checked blue pixels match exactly.
Transparent pixels match nearest background samples; exact texel-boundary
ties may select either adjacent sample through interpolated GPU coordinates.
This resized check is separate from the strict golden comparison: both native
1024x768 captures still have zero differing pixels across the whole screen.
Live GL texture count stays at 28 across the sequence, and clicking Exit Game
completes normal memorydebug shutdown without tracked memory/OpenGL leaks.
These are WSLg/llvmpipe rendering checks, not native-platform certification.


A later current-integration audit [at the recorded PR #283 production head](https://github.com/jhurliman/glsmac/commit/b69d4133856a653f29e18b0f2cdf6403efc88fe3) repeated the strict whole-screen
comparison with zero changed pixels. Real SDL Down selected Quick Start and Up
returned to the exact initial Start Game pixels. Enter opened Game Ruleset;
Escape returned to the exact same title. Mouse hover selected Load Game, then
Up selected Scenario. The real Exit button completed normal memorydebug
shutdown without tracked memory/OpenGL leaks. All captures retained the actual
1024x768 drawable/viewport with GL error zero; baseline live texture count was 28.
This checks these input/return states, not original hover-state pixel references
or transition cadence. Evidence is retained externally under
playable-planet-inspector/title-audit; the unchanged title production path was
validated from that named binary, not a screenshot reconstruction.

Remaining presentation gates include native-platform captures, original hover/keyboard
selection and return-to-menu states, transition frames, resizing/letterboxing,
other setup menus, SMAX-specific artwork, and in-game screens. The broader
faithful 1.0.0 roadmap remains unchanged.

## Transition label ordering

The native title caller draws labels after its six opening frames. The port now
keeps those text objects hidden through frames0..5 and the final10ms timer
delivery, then reveals the selected and inactive labels together. Closing hides
them before frame6 and invokes the selected action only after frame11's final
delivery. Native-title keyboard events are consumed during either transition,
matching the original's non-message-pumping title loop and the port's existing
mouse guard; queued input cannot change the action or history mid-transition.
The generic sliding menus keep their existing behavior.

`GLSMAC_data/tests/title_transition.gls.js` executes the real sliding module and
history manager with explicit timer deliveries and DOM doubles. It covers every
opening/closing frame, label visibility, input suppression, deferred action and
cancellation/reopening. The original ordering is documented in the
[native title receipt](behavior/smac-title-screen.md). This checks sequencing;
real presentation intervals and original transition screenshots remain separate
acceptance gates.

A focused WSLg/llvmpipe capture also records all twelve real OpenGL
transition backbuffers. Each matches an independent indexed-asset composition
across all786,432 pixels, with no label pixels on the artwork frames. The
initial settled screen and the Down/Up return still match the supplied original
reference exactly. Readback instrumentation delays presentation, so these
checks do not establish native frame durations or original live-loop parity.
The real Exit button completes memorydebug shutdown without tracked leaks.
