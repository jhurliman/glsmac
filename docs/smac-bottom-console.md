# Functional bottom console progress

The gameplay reference in `gameplay-map-and-console-fidelity.md` uses SMAC's
`console2.pcx`. The previously unconditional `console2_a.pcx` has different
metalwork and pipes. The live ruleset now selects the appropriate frame. The
SMAC resource filters its magenta transparent pixels; the expansion retains
its original dark-purple transparency filter.

The console is 257 pixels high. At 1024x768 its frame starts at y=511, and the
three background pieces preserve their native widths. The tile pane ends
above the stack strip, which begins below that pane instead of at x=245.
Selected-object and terrain text use tighter spacing and monochrome raster.
The original executable's precise lower-console font contract remains to be
verified; this does not establish text pixel parity.

The Base preview now reads the current retained production target and mineral
store. When an owned cost is available it displays `(stored/cost)`; absence
of that private calculation leaves the stored amount alone. Unsupported
production is explicitly unavailable. The preview wraps using actual font
measurements, refreshes on committed Base updates, and clears on removal.
It never switches production, posts minerals or forecasts using a made-up
surplus. Current queue editing remains in Base Control.

Validation: a native-backed production/refresh/removal test, five existing
production and Scout/Rover preview groups, two focused GSE scripts, and
correctness analysis of ResourceManager, TextureLoader and the SDL2 texture
decoder pass. The callback test uses real Base wrappers and real event
registration, with DOM/font measurement doubled. It checks unchanged world
and RNG snapshots while displaying current stores.

A real GOG-assets capture restored the pending research question through the
title's Load Game command, accepted priorities, selected a Scout, and exited
through Menu / Game / Quit with exit status zero. At 1024x768, all 55,300
unobscured opaque frame-art pixels matched the installed `console2.pcx`
exactly; this comparison excludes transparent panel interiors and the two
menu-button rectangles. This validates asset selection and frame placement,
not parity with the reference's different developed map. OpenGL retirement
reported no tracked leaks; heap allocations and audio were not checked.

Remaining: faction/leader/integrity/might bindings, retained dominance history
and graph, political/terrain minimap modes and the original mode-button row,
faithful unit statistics and icons, exact console fonts/colors, and integration
against the same original saved scene. The current minimap mode buttons still
only show/hide the map. No decorative history or unsupported command is claimed
as implemented by this first slice.
