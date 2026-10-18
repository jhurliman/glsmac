# First complete visual target: the SMAC title screen

After the installed Planet inspector milestone, focus presentation work on one
settled original SMAC main title screen at native 1024x768, with Start Game
selected. Match the background palette, menu frame artwork and slicing,
button/text blues, font face, glyph rasterization, spacing and placement.
Keep the rendered area 4:3 through the existing presentation viewport.

This is the first whole-screen visual gate, not a claim of SMAX, map or Base
screen parity. Reusable font/palette fixes should subsequently be checked in
their actual in-game consumers. Do not expand this pass into general menu work
before this screen matches.

## Current gate, 2026-10-18

The settled original SMAC title profile now matches all 786,432 pixels at
1024x768. The implemented common palette/tint, openitem.flc frame artwork,
Arial Bold 18 monochrome labels and native placement are described in
[the rendering result](title-screen-rendering.md). The initial diagnosis below
is retained as history; its former console fragments and colors have been
replaced. This completes one settled-screen profile, not all menu presentation.

A fresh actual OpenGL audit [at the recorded PR #283 production head](https://github.com/jhurliman/glsmac/commit/b69d4133856a653f29e18b0f2cdf6403efc88fe3) also exercised Down/Up selection,
Enter into Game Ruleset and Escape back to the title, mouse hover on Load Game,
Up to Scenario, and the real Exit button. Both the Up return to Start Game and
the return from setup reproduced the initial title pixels exactly. The run
used installed GOG assets, a hidden 1024x768 SDL window, WSLg/llvmpipe and
memorydebug; normal Exit reported no tracked memory/OpenGL leaks. No title
production code changed in that audit. Native-platform/font-version captures,
SMAX, other setup menus and original transition-frame/timing parity remain
open gates. The reference's reconstruction/runtime provenance limits remain
in the native receipt; a pixel match does not expand them.

## Capture and compare

Use the user's supplied SMAC title image as the initial visual reference.
Its executable/patch/capture identity has not yet been independently pinned.
Record those identities when obtaining the original executable capture; the
reference investigation must also establish which palette, artwork operations
and font/raster settings produce its pixels. A screenshot match does not by
itself authenticate the reference or prove the underlying implementation.

Capture GLSMAC's actual settled renderer at 1024x768 content pixels. Record its
commit, platform, original installation identity, selected ruleset and selected
button. Keep the pointer outside the menu and exclude the OS window frame.
Extract content explicitly if necessary; do not stretch, realign or color-correct
either capture to conceal implementation differences. Keep captures, overlays,
original artwork and executable evidence outside Git.

With Pillow installed, run from the repository:

```text
python tools/reference/compare_title_screen.py --reference ORIGINAL.png --candidate GLSMAC.png --output-dir PRIVATE_DIAGNOSTICS
```

The command requires two opaque 1024x768 images and exits 0 only for exact
whole-screen RGB equality, 1 for a pixel mismatch, and 2 for invalid input.
It writes `comparison.json` with input hashes and counts, `difference.png`
with absolute channel differences, and a 50% `overlay.png` for alignment
inspection. Background/menu regions are diagnostic only: pixels outside those
regions still count toward the whole-screen gate. No tolerance, mask, scaling
or automatic alignment is allowed. Also exercise keyboard selection, mouse
selection and a menu transition; a static passing picture cannot prove input.

Run the comparator's synthetic acceptance tests with
`python -B -m unittest discover -s tools/tests -p test_compare_title_screen.py -v`.
They require no game installation, graphics driver or platform builds;
the real original-versus-GLSMAC capture remains a separate visual gate.

## Initial diagnosis, 2026-10-18

The supplied reference and retained actual PR #220 GLSMAC backbuffer are both
1024x768. Their exact comparison differs at 183,972 of 786,432 pixels. The menu
diagnostic rectangle (670,240)-(1024,768) differs at 156,264 pixels; the left
background (0,0)-(630,768) differs at 11,305 pixels. These are mismatch counts,
not a percentage estimate of implementation completeness. The original capture
identity and exact selected-state admission still need the reference receipt.

The installed `openinga.pcx` is natively 1024x768. Across the reference's
background-only left region, every one of its 145 used palette indices maps to
one reference RGB value, but 25 mappings differ from the raw PCX palette. For
example, index 80 changes from (248,248,236) to (235,235,235). Thus background
geometry already aligned while palette handling was a real discrepancy.
Do not paint over the screenshot or hard-code these sampled RGB mappings as a
substitute for reproducing the original palette operation.

The initial `sliding.gls.js` used rotated console fragments, green translucent
fills and `arialnb.ttf:20`. Those choices had not been established as the
original title renderer. Resolve their actual original sources before treating
color/font adjustments as correct. The separate SMAX title needs its own
reference and gate after this screen.
