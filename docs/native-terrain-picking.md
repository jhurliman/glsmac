# Native terrain picking

Committed native view surfaces now route tile requests through the original
relief-aware edge walk and their owned shared-corner height cache. They no
longer query the legacy GPU data mesh. The response still uses the frontend's
pending/result interface, including cancellation, replacement and miss delivery.
Queued native results are discarded on world retirement and terrain teardown.

The walk alternates W/N and N/E edges. Integer division truncates toward zero;
strict signed cross-product comparison advances on equality. Products explicitly
preserve the native 32-bit result without C++ signed-overflow UB. Direct height
requests outside the world return zero before valid corner aggregation wraps
its neighboring contributors, including on a wrapping world.

The result preserves native output semantics: early viewport/offset rejection
writes nothing; a completed walk writes its candidate before checking world
bounds. This permits a failed result with newly written invalid coordinates.
The frontend admits only a successful tile. Unknown terrain is still pickable
geometrically; action/inspection and actor-disclosure rules remain separate.

Picking uses the last committed frame's raw zoom, layout and relief. Pending UI
zoom/relief changes do not replace them. Zooms 3 and 4 share q15 but have
different height steps, so their selected tiles can differ at the same pixel.
The session retains the committed relief alongside its bitmap/layout publication.

| Controlled input | Original selected tile |
| --- | --- |
| Hill, pixel (500,275), zoom 3 | (21,37) |
| Same hill/pixel, zoom 4 | (20,38) |
| Slope, pixel (720,301), zoom 3, relief 8 | (24,38) |
| Same slope/pixel/zoom, relief 25 | (24,40) |

Source is frozen picking/zoom evidence manifest
`29dd703a8393d1fa79d11a06104325c59b3e29c19aac3f0afd9e6a456b47b612`;
all 23 files were verified. [The public numeric receipt](behavior/native-terrain-picking-cases.json)
retains original pixel inputs and hashes of every result, direct-height query
and edge-cross-product event. It contains no executable, game art or fonts.

The production picker matches all 72 profiles: 9,955 points, 56,196 direct
height queries, 28,098 edge comparisons and 2,204 equalities. Profiles cover
all integer zooms -14 through 16, plain/hill/slope/coast, round/flat boundaries,
small-world centering, odd viewport dimensions, low relief and 24 equal-q
discriminators. The standalone comparison passes normally and with ASan/UBSan.
The defensive unsupported-walk guard is 10,000 edges, matching the frozen
reference guard; the largest captured walk uses six.

```sh
python3 tools/reference/verify_native_terrain_picking.py \
  --output-directory /tmp/glsmac-terrain-picking
```

Add `--sanitize` for the instrumented probe. No purchased assets are needed.
Native application validation includes 36 selected groups, also passing under
focused ASan/UBSan with six affected production/test units instrumented and
other dependencies normal. Leak detection is disabled. The three new groups
also pass without installed-asset overrides. Four production consumers pass
correctness analysis, with the final retirement consumer rechecked. Cached
incremental builds were reused; no platform matrix was repeated.

The frontend fixture checks actual request/result methods with an absent legacy
data mesh: native misses cannot fall through to it. It covers cancellation,
replacement/purpose, same-q frame changes, pending UI state, retirement and
teardown. Picking and view preparation remain on the existing MAIN input/task
path; backend map pointers do not cross into the picker.

These are terrain-only controls with ordinary retained bands 2..5; authored
landmark and high-altitude saturation picking scenes still need additional
native evidence. Actor sprite hit testing, overlap priority and higher-level
selection/disclosure remain separate. The normal main-map gate, discrete key
zoom and complete layer connection remain open. The live inspector uses this
route when a committed native surface is visible. GPU presentation timing and
native Mac interaction were not newly validated by these CPU/caller checks.
