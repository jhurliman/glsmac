# Resource, Supply Pod and Monolith display

Implementation checkpoint: the C++ selector matches all 52 captured caller
contexts using the production resource/site resolvers, plus two zoom gates,
under ASan/UBSan. Owned live snapshots now carry the current retained native
resource/site flags and live Monolith presence. The renderer composes the three
stages and independently replaces/restores legacy resource sprites. All 46
affected native groups, targeted analysis and focused eleven-TU ASan/UBSan pass.
Site pickup state changes remain separate gameplay work; this presentation pass
reads current retained policy flags and does not implement pickup outcomes.
Runtime capture and full live-assembler comparison remain pending. The source contract below is not a production-completion claim.

The ordinary renderer's flag `0x200` enables this display continuation.
The [cases](resource-pod-monolith-art-cases.json) and
[reference](resource_art_reference.py) match52 native producer contexts,
274 polygon commands,39 sprite commands and56 complete indexed prefix frames:
5,144 spans,179,842 attempts and154,972 stores. Four replays use smaller clips.
The actual resource and site helpers run; their results are not supplied.

## Admission, authority and order

All three displays require flag `0x200` and zoom greater than -8. Their item
authority is **live tile state**, independently of remembered improvement bits.
Live-only and remembered-only controls prove this for resource presence,
Supply Pods and Monoliths.

After terrain/coast/River/Road/Tube, the renderer draws:

1. The effective nutrient/mineral/energy resource icon, if nonzero.
2. The existing common developed-improvement group.
3. The separately gated named-landmark sprite continuation.
4. A Supply Pod when the live resolved site query is nonzero.
5. A Monolith when live item bit `0x2000` is set.

Resource icons and Monoliths can coexist: Monolith suppresses the site query,
but does not suppress the resource resolver. The Monolith overlay is last.
Supply Pod art does not distinguish resolved site class1 from class2.
None of these callers adds a land-only condition; the resource resolver itself
rejects altitude bands0/1. Forced Pods and Monoliths can still draw there.

## Resource icon selection

Use the complete [effective resource resolver](resource-bonus-resolution.md)
with live items, retained pattern offset and automatic-land-resource setting.
Forced resource presence is `0x400`; type override is `0x20000000`; energy and
mineral fallback markers are `0x10000` and `0x20000`. Do not draw directly from
the presence/type bits without resolving terrain and coordinate rules.

Let `type=1/2/3` for nutrients/minerals/energy, `sea=(band<3)`, and
`parity=(floor(x/2)+y)&1`. The native sprite index is
`4*(type-1)+2*sea+parity`. TER1 crop size is100x62:

| Type | Crop y | Land parity0/1 crop x | Sea parity0/1 crop x |
| --- | --- | --- | --- |
| Nutrients | 253 | 203 / 304 | 1 / 102 |
| Minerals | 316 | 203 / 304 | 1 / 102 |
| Energy | 379 | 203 / 304 | 1 / 102 |

These icons use the destination-lookup shadow sprite kernel. Key9 skips,
source0 maps the destination through the initialized shadow lookup, other bytes
copy literally. Use the active-palette lookup and nonstandard native color
conversion already established in
[developed overlays](ordinary-developed-overlays.md).

The caller also contains a type4 plain-sprite branch, but the measured complete
original resource helper only returns0..3. That branch is source-mapped and not
promoted to an independently reachable game behavior.

## Supply Pod selection

The live resolved site query follows these rules, in order:

1. Live `0x2000` or `0x100000` suppresses sites.
2. With automatic-land-resource option enabled, return class2 only for live
   `0x04000000` or `0x08000000`; otherwise none. This branch ignores the forced
   secondary marker and ordinary pattern placement.
3. With that option disabled, live `0x10000000` forces class1, even in deep sea.
4. Otherwise pattern offset0 disables placement. With nonzero offset, calculate
   `u=floor((x+y)/2)`, `v=x-u`, `phase=(u&3)+4*(v&3)`.
5. On land, matching `phase == (-5*floor(u/4)-3*floor(v/4)+pattern)&15`
   gives class2.
6. Otherwise class1 requires
   `phase == (pattern+11*trunc(u/4)+61*trunc(v/4)+8)&31`.
   The coefficients11/61 are measured retained globals; the exact reference
   accepts them as arguments. Signed truncation here differs from floor.

The complete site-query audit already covers synthetic combinations and the
installed Planet map. This continuation checks its actual display caller.

For either nonzero class, choose `variant=(7*x+5*y)%3`; index is
`2*variant+sea`. Each TER1 crop is100x62 at y379:

| Variant | Land crop x | Sea crop x |
| --- | --- | --- |
| 0 | 519 | 418 |
| 1 | 721 | 620 |
| 2 | 923 | 822 |

Pods use the **plain** keyed sprite kernel: source0 is a literal index0 store,
not a destination shadow. Monolith uses the shadow kernel and TER1 crop
`(304,1,100,62)` on both land and sea.

All use the same terrain projector anchor P, native transparent trim and
axis-aligned retained-offset placement as developed sprites. Sprite pixels
receive no terrain lighting. Raised/coastal contexts and clip controls match.

## Scope

Measured zoom0, viewer3, default relief and gamma100, 1024x511 indexed viewport.
The prefix stops before optional moisture/rockiness diagnostic displays and
later actors/fog. Named-landmark art is a separate handoff; its place in this
ordering is source-traced here. Alternate zoom pixels, arbitrary overflow inputs
and repaint scheduling are not claimed. No production code changes or commits.
