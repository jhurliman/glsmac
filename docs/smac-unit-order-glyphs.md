# Live conventional Unit order glyphs

The backend publishes a viewer-specific order glyph on actual Unit spawn and
refresh. Owned ordinary orders use `-`, `H`, `f`, `R`, `F`, `S`, or `M` for idle,
Hold, Farm, Road, Forest, Solar, or Mine. Foreign human viewers receive `-`.
Retained status bits whose override rendering is not implemented suppress this
ordinary path. Native and legacy script Units retain their previous badge path.

The [original overlay contract](behavior/map-unit-and-population-overlays.md)
and [seven-faction color controls](behavior/ordinary-hold-glyph-colors.md)
define the zoom0 order box and baseline. The renderer uses monochrome Arial Bold12,
centers each glyph using its own advance, and paints transparently into an owned
23x30 flag copy. Cache identity includes glyph, faction foreground, morale and
active state. The canonical frozen faction section preserves original logical
filename identity: exact `SPARTANS` uses palette44, the other stock factions use
black. Palette44 channels receive the original gamma100 cap235.

The map and retained Unit previews consume the same composite. Hold/Activate
refresh changes its glyph without modifying the shared atlas. Exhausted Unit
refresh stops blinking and keeps its stationary flag, order and health bar visible.

Validation: twelve affected native groups pass, including real deferred Hold /
Activate and Scout/Rover preview teardown. Targeted correctness passes six changed
production translation units. Focused ASan/UBSan instruments SlotBadges, BadgeDefs,
frontend Unit, Texture and both affected renderer/preview test units; other objects
reuse the normal build, with leak detection disabled. Live software-rendered
1024x768 capture verifies H, real Activate replacement, movement exhaustion and
normal Quit0. This is not a hardware performance or audio check.

This does not establish complete original framebuffer parity. Other zooms, status
and automation overrides, editor reveal, fog admission, original flag artwork,
and historical Windows font raster differences still need dedicated matching.
