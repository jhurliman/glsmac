# Interactive native inspector zoom

The revealed Planet inspector now dispatches native zoom commands from the live
keyboard handler. Lowercase `z`/`x` step by two, uppercase `Z` resets to zero,
uppercase `X` selects overview -6, and Ctrl+Z/Ctrl+X select 16/-14. The existing
ordinary continuous zoom handler no longer consumes inspector keys. Wheel input
remains a no-op, matching the measured original ordinary-map input path.

The command policy preserves explicit primary-view camera coordinates and
repaints at unchanged limits. Its automatic-target policy separately supports
the measured overview admission, old negative-zoom cursor selection and eligible
auxiliary views. The inspector uses explicit target zero; it does not expose an
auxiliary-view UI. Literal translated uppercase input is distinguished from
lowercase; the captures do not establish every Caps Lock or focus behavior.

Command checks are preconditions, not output clamps: retained odd states 15 and
-13 can produce 17 and -15. The pure policy preserves those measured results;
the current live raster profile admits -14..16. Fresh inspector keyboard states
remain even. This is not a campaign view-state loader or support for rendering
the two out-of-profile odd results.

Terrain preparation, committed-frame picking and actor roots share the new raw
zoom and layout. Tile render roots carry the selected horizontal wrap offset
without changing their canonical coordinates. The inspector renders one GPU
world, hides actors outside the native object window, suppresses all replaced
legacy props and hides the old terrain mesh. A separate explicit revealed canvas
owns opaque black backing outside the small zoomed-out world. Diagnostic
transparent inspection and ordinary observer preparation retain separate modes.
Legacy camera movement is converted to a native tile view; this adapter is not
proof of every original higher-level recenter or click behavior.

Numeric input comes from the frozen `ordinary-map-picking-zoom-evidence-20261020`
handoff, manifest SHA256
`29dd703a8393d1fa79d11a06104325c59b3e29c19aac3f0afd9e6a456b47b612`.
All 23 entries were verified before import. The test fixture contains 250 original
command/camera/repaint controls, six key controls and 14 wheel controls, with no
executable, original map or purchased art payload. The complete input contract
is described in [ordinary map picking and zoom](behavior/ordinary-map-picking-and-zoom.md).

Validation: cached incremental build and 55 affected native groups pass. The
actual camera matrix projects wrapped object roots onto native canvas positions
at zoom -14, 0, 3, 4 and 16 near three world boundaries. Real Tile tests preserve
canonical coordinates through positive/negative wraps; visibility tests cover
full legacy suppression and restoration. Nine production translation units
pass correctness analysis, with the final mesh suppression consumer rechecked.
The same 55 groups pass a focused 11-TU ASan/UBSan build; other objects are
uninstrumented and leak detection is disabled.

A hidden Linux SDL/OpenGL run loaded the existing Morgan 2105 campaign
(SHA256 `aed85217e7684412e0de762d4ccf23df5892f20fc0355a8b2fa15ea55580ae92`),
exercised all six keyboard commands, both zoom limits, terrain selection after
zoom changes and normal Quit. It exited 0 with no tracked OpenGL leaks. Audio
and allocation-debug tracking were disabled. These are actual 1024x768
framebuffer captures, not original-game comparisons or performance measurements.

![Default live inspector zoom](https://raw.githubusercontent.com/jhurliman/glsmac/6dee3e941a94e6f12e1b324cf222e9aaedaaf0eb/native-live-zoom/default.png)

![Minimum live inspector zoom](https://raw.githubusercontent.com/jhurliman/glsmac/6dee3e941a94e6f12e1b324cf222e9aaedaaf0eb/native-live-zoom/minimum.png)

![Maximum live inspector zoom](https://raw.githubusercontent.com/jhurliman/glsmac/6dee3e941a94e6f12e1b324cf222e9aaedaaf0eb/native-live-zoom/maximum.png)

Ordinary main-map observer integration remains gated. Base art resampling,
name font/baseline scaling and complete unit flag/health/order scaling still
need their dedicated original-backed rendering pass. A working zoom control
does not establish whole-screen pixel matching, normal-game fog, general map
support, combat animation fidelity or saved view restoration.
