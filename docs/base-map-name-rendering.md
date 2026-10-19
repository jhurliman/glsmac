# Base map-name rendering

SMAC's ordinary Base names now use regular Arial12 monochrome, with one cached
whole-string bitmap per name and faction text/shadow colors. The source-backed
[plain-name contract](behavior/base-map-name-font.md) distinguishes advance from
ink bounds, includes negative bearings, and establishes floor(width/2) centering,
the zoom0 baseline and a single shadow at (+1,+1). No pair kerning is applied.

The old path used 48px Arial Narrow glyph sprites with a broad outline and
camera-dependent scaling. `RF_PIXEL_SPRITE` projects only the map anchor, snaps
it to the nearest framebuffer pixel boundary, and places the bitmap's vertices
in screen pixels. It preserves horizontal map wrapping through ordinary scene
instances and disables texture smoothing. Generated texture ownership remains
with the sprite manager until its actor retires. Labels acquire shared name
references; destroying the final label evicts its actor and bitmap immediately,
so successive renames cannot accumulate unused scene actors or GPU textures. Name/owner changes use the
existing Base identity refresh path; the legacy ruleset retains its old style.

This implements the checked zoom0 name prefix. The current continuous camera has
no original discrete zoom selector, so names currently retain the checked12px
size at other zooms. The baseline is27px below the current projected Base anchor;
original terrain-aware box-anchor geometry remains a separate integration task.
Optional native Base status/production suffixes and historical Arial raster
parity are not established. Current-Windows offscreen GDI receipts are labeled
as such in the contract; they are not original live screenshots.

Focused regression covers three-pixel narrow advances, negative bearings,
odd-width centering, whole-string shadow order, single-sprite cache reuse,
unscaled geometry, unchanged world anchors, shared-label retirement, final
InstancedText destruction and64successive unique-name evictions. The
existing legacy-spacing and frontend Base owner/name refresh groups also pass
with allocation tracking.

The owned developed-campaign runtime comparison at41293dc0 uses the ordinary
Load Game path, the same1024x768 camera navigation as the317 capture, and a
normal menu Quit. It reported no tracked OpenGL leaks (heap tracking and audio
were not part of that graphical run). Exact foreground masks against the pinned
current-Windows original-path GDI controls differ by0pixels for New Austin,
0for New Chicago and2for Hilton Head (one pixel on the final d moves vertically).
Comparison translates cropped ink bounds only, without resizing or warping;
it does not prove scene-anchor or historical raster parity. That last glyph
raster difference remains unresolved rather than patched with a name-specific
pixel correction. Captures and owned campaign data remain outside the code tree.
