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
The source operation must come first; animation path, background path, frame and
row count identify its cached result. Native opening frames 0..5 and closing
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

Remaining presentation gates include native-platform captures, hover/keyboard
selection and return-to-menu states, transition frames, resizing/letterboxing,
other setup menus, SMAX-specific artwork, and in-game screens. The broader
faithful 1.0.0 roadmap remains unchanged.
