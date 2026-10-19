# Original lower-console terrain-preview source map

Executable SHA256: `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.

- `4cc023..4cc03c`: width minus four becomes `6b0114`; `6b0118=100`.
- `4cc04a..4cc079`: anchor is rectangle left+2/top+1; call `46fd00(bitmap,0,0,viewer,0,0x621,x,y,anchorX,anchorY)`.
- `4cc082..4cc08a`: caller sets both scale globals to -1 after the terrain call.
- `46fe29..46fe62`: signed division computes the geometry quarter-height from scale. Positive override path is `trunc((trunc(50*N/D)+1)/4)`.
- `46fe68..46fec1`: null view bypasses four native height requests and sets average height to zero.
- `4709e6..470a64`: null view bypasses corner height subtraction and face shade queries, writing zero vertex light values for ordinary land.
- `470c64`: opaque base-triangle painter `63dcc0`, four calls in measured order.
- `471a00`: keyed unlit fungus painter `63e2e0`.
- `47263d`: road keyed shaded quad painter `636990`.
- `472712`: resource sprite shadow painter `5fe680`.
- `473226..473259`: only a nonnull view can receive the scaled-view invalidation sentinel; generic sprite scale resets to 1/1 and shader limits reset to -256/256. Caller scale override remains until its own reset.

Producer: `tmp/reference/probe_console_terrain_pixels.py`. Native receipt: `tmp/reference/console-terrain-pixels-original-x86-20261019.json`. Checker: `tmp/reference/check_console_terrain_pixels.py --verify-only`. Public numeric receipt: `docs/behavior/lower-console-terrain-preview-cases.json`. Private pixels and normalized ordered CPU stores: `tmp/reference/console-terrain-pixels-private-20261019/`.

The producer reuses the v2 viewport machine initialization solely for native atlas/memory/OS services, removes no raster behavior, supplies a console bitmap header/clip, and directly invokes the exact previously observed console terrain call. All seven complete frames and 13,137 stores match independent polygon and rational sprite-scale replay. Geometry cache before/after files are identical. Caller postdraw reset is source-confirmed; the direct terrain-entry controls correctly leave override85/100 intact. No claim that the entire console UI executed in these pixel controls.
