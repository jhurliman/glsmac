# Native population layout across zoom

Population overlays now request Arial Bold at the original size derived from
`trunc(trunc(50*(zoom+16)/16)/3)`, including the exact10-to11 exception. Their
one/two-digit box measures8/88 and adds5 horizontal pixels; height is the
native logical font height+4 and baseline is logical height+2. The rasterizer
accepts those metrics instead of fixing every box to16 pixels tall.

The layout profile contains160 measured rows for regular/bold Arial sizes1..80,
keyed to the complete font-file SHA256. FontLoader attaches a profile only to
matching loaded bytes in ordinary monochrome mode. Filenames, substituted fonts
and synthetic emboldening do not confer the profile. Cache records own the font
and its optional metrics together; failed registration does not publish a dangling
font. A portable SHA256 helper avoids a new platform crypto dependency and was
compared with hashlib across133 inputs, including padding boundaries and a
million-byte control. Native tests also retain standard empty/short/multiblock
digests and actual installed-font cache registration when assets are supplied.

The immutable source supplement is
`882e83e9c0d26dc7cebceb47e3c8ce6afc23083baf8355f8debfe75f62712dd0`.
The two measured font identities are retained in
[the numeric profile](behavior/arial-native-layout-metrics.json). The earlier
actor caller handoff supplies
[22 population controls](behavior/native-population-zoom-cases.json).
No executable, font files or glyph bitmaps are embedded in those fixtures.

For unmatched font bytes, the captured metrics remain unavailable. The existing
zoom0 fallback layout is retained; other zooms use the loaded glyph's top as a
functional fallback. Native parity for those fonts is unverified. The measured
profile does not prove full glyph-raster or arbitrary-name advance parity even
for matching font bytes; FreeType continues to supply the actual glyph pixels.

## View and rendering integration

Population caches include the zoom, because zooms3 and4 request different font
sizes despite sharing q15 and some box metrics. Sprite world dimensions divide
by8q/4q so the new bitmap remains its intended physical pixel size under the
native camera. The original dispatcher anchor is preserved. View refresh resolves
new counters before publication, removes the previous instance and binds the new
counter without changing the console body mesh or simulation state.

Native population actors bypass the extra world lighting/contrast pass. Their
layer is after the Base body and before Units, matching the original same-tile
Base-before-Unit painter sequence. This does not establish complete inter-tile
painter ordering. Legacy/custom counter presentation remains separate. Marker
views still suppress population drawing below-8 and reuse their existing paths.

## Validation and images

The focused native groups cover digest/profile identity, actual loader cache
registration,22 original font requests/boxes/baselines, real Arial Bold bitmap
extents, zoom3/4/8/16/0 view switching, prior-instance retirement, distinct
same-q entries, camera-projected pixel dimensions and console-body independence.
Existing ownership/deferred UI and Former command-panel checks are included.
The installed-font tests are opt-in via GLSMAC_TEST_ARIAL_PROFILE_ROOT; ordinary
assetless checks remain runnable. Optional GLSMAC_POPULATION_CAPTURE exports
actual population-rasterizer pixels for diagnostic images.

The diagnostic image uses the measured installed Arial Bold file and production
population rasterizer for9/10 at0/3/4/16, with an occupied University-like
white/black style. It is enlarged4x with nearest sampling. These are controlled
counter outputs, not a complete normal-game screenshot or original glyph diff.

## Still required

The ordinary interactive camera and saved-exploration terrain have not yet been
connected to the native discrete view. Base art resampling/indexed shadows,
map-name zoom placement, Unit overlay scaling/fonts and full actor raster order
remain open. This population adapter prepares that integration; it does not
close terrain color, coastlines, unexplored grid or relief acceptance defects.

Validation receipt: cached incremental build;21 focused groups with installed
font inputs and21 asset-independent invocations pass. Seven production units
pass correctness analysis; the final counter-layer producer was rechecked.
Focused ASan/UBSan passes the21 groups with nine affected production/test units
instrumented (other dependencies normal, leak detection disabled). The final
layer-only adjustment reused those ownership checks and passed the native
camera/order checks. No platform matrix or whole-game pixel parity is claimed.
