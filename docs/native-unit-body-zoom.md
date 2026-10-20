# Native Unit body placement across zoom

The native camera quantizes terrain diamonds, while CVR model generation uses
an independent art ratio. Fixed96x48 world sprite dimensions only agree at
zoom0. The Unit body adapter now uses the shared quantized camera unit for
world dimensions and the model's returned anchor for placement.

## Contract

For ordinary zoom -14 through16:

- `q = max(1, (50*(zoom+16)/16+1)/4)`, with integer divisions.
- Terrain diamonds span8q by4q. World camera scale is proportional to q.
- The CVR renderer receives the current zoom, without a positive identity
  scale override freezing its model at zoom0. Its art ratio remains
  `(zoom+16)/16`, independently of q.
- The existing final model canvas stays256x256. World dimensions become
  `256/(8q)` by `256/(4q)`; offsets use the returned target anchor relative to
  the native dispatcher. The dispatcher lies4q left and4q above the
  frontend object center in screen coordinates.
- View-owned projector heights divide by4q when converted back into world
  coordinates. This avoids multiplying elevation by camera zoom a second time.
  Backend/canonical picking metadata is preserved and restored on clearing.

Zooms3 and4 both have q15, but their art ratios are19/16 and20/16. They must
share diamond dimensions without sharing body pixels. The zoom0 actor and
independent console-card descriptors retain their prior identities.

The source contract is the frozen `native-zoom-known-render-evidence-20261020`
handoff, manifest SHA256
`f674cb59801d950f3b34714275876f11a9344cfea2468e4043fb8cb17dcb215c`.
Its four original actor controls establish caller/scale interaction, not full
actor framebuffer parity. Existing original-backed CVR rendering controls cover
model composition separately; these new placement checks exercise production
frontend camera matrices and generated meshes.

## Live refresh and ownership

`UnitManager::SetNativeBodyZoom` resolves all required bodies before publishing
its new zoom, removes each old rendered instance, updates preview mesh bindings,
and restores admitted visibility. It does not invoke the general gameplay
refresh, cancel work/selection visits, or replace console-card descriptors.
Repeated requests reuse scene-owned immutable textures and actors.

The cache reserves837 possible canonical native views (31 zooms, nine facing
values including the sentinel, three current compositions), plus the existing
64 independent custom/card views. Failed renders consume no slot. Custom
inputs cannot consume the extra native allowance. Allocation is lazy; no cache
is prefilled. A fully populated901-image cache can retain about225MiB of RGBA
CPU storage, plus GPU copies and model/actor overhead. This is a finite safety
bound, not a performance target or a measured normal-game footprint. Eviction
must first account for the actors and previews retaining texture ownership.

## Validation and remaining work

Focused native tests exercise Scout, initialized Scout and Spartan Rover at
zooms -14,-1,0,3,4,8,16, checking actual rendered body creation, projected
256-square bounds, dispatcher anchors, stable cache/card identities, and
registered-unit switching3 ->4 ->0 without stale instances. Additional tests
cover same-height changes across different q values, clearing/restoring tile
metadata, and native allocation after the independent cache budget fills.
Targeted correctness analysis and focused ASan/UBSan cover affected producers,
body/cache ownership, renderers and frontend test consumers.

This is the Unit-body adapter for the camera bridge. The ordinary map camera
has not yet been switched to this zoom API. Base art, population/name glyphs,
Unit order/health/stack overlays and ordinary saved-exploration presentation
still require their corresponding live integration. No complete interactive
zoom or gameplay screenshot parity is claimed by this change.
