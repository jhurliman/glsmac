# Full inspector feature schedule at flags0x10621

For the measured **known raw Planet scene**, enabling the inspector's explicit
flags0x10621 and executing every tile through its real return preserves the
[core viewport geometry schedule](viewport-geometry-lifecycle.md). All four
query streams and complete native12-byte cache hashes equal the corresponding
frozen first/repeat/east-pan/back-pan controls. The core selector can therefore
be used for this bounded inspector configuration without inventing feature-stage
geometry queries.

The [cases](viewport-feature-schedule-cases.json) retain full ordered outer calls,
every nested height call, entry cache bytes, native tile arguments, draw-boundary
observations and name/overlay phase observations. Run
`tmp/reference/check_viewport_feature_schedule.py --verify-only` with the
emulation venv and installed Planet asset. Earlier frozen controls remain unchanged.

| Control | Complete tile returns | Outer geometry queries | All height calls | Feature pixel calls |
| --- | --- | --- | --- | --- |
| First22,38 |288|6408|10738|1014|
| Repeat |288|6408|5796|1014|
| Pan24,38 |63|1805|1829|208|
| Pan back22,38 |50|1343|1220|189|

Independent replay checks15,964 outer query returns,19,583 height returns and
115,200 height/shade cache-byte positions across four snapshots. Native-to-native
comparison additionally checks complete cache hashes, including status/packed
slope bytes. There are2,425 feature draw-boundary calls and4,120 earlier terrain
draw-boundary calls; pixels themselves are observation sinks. Native TEXTURE,
TER1 and generated coast-mask initialization executes with owned assets.

## Later passes and the -1 sentinel

The complete name routine is entered and returns once per control. It makes no
projector call here. Flags0x10621 lack the Base-name bit0x800, the extra overlay
loop bit0x2000, and the later0x20000/0x100000 loop bits. Thus no later name/overlay
projection loop is enabled in this configuration. The object tile loop still
projects its admitted tiles, even with no Base/unit drawing enabled.

No height request occurs after the feature boundary within these measured tile
passes, and none of the19,583 height returns is-1. There is consequently no new
within-feature -1 recomputation rule demonstrated by this scene. Preserve the
established rule: height byteFF is cold, including a prior computed signed-1.
Do not convert it to a reusable cached result or suppress native repeated
primary-geometry queries. This receipt does not extend that conclusion to
unknown/fogged tiles, other flags or synthetic landmark inputs.

## Production-facing anchors

Use a submission carrying tile coordinates, the selected visible world copy,
the viewport state, flags and **the projector result already produced by the
scheduler**. Keep the geometry cache with that view. The UI should consume the
submission rather than call the projector again to infer an anchor.

For this zoom0 scene the native terrain call receives
`(bitmap,view,0,viewer0,-1,0x10621,x,y,Px,Py)`. Preserve the fifth auxiliary input
as-1; these live caller arguments differ from some standalone art fixtures.
Their semantic label is not needed to translate the measured schedule.

Terrain processing still makes its native S/E/N/W height requests after the
scheduler's projector. At its flat-center construction point,
`F=(Px+48,Py+(sumOuter>>2)+24)`, using the outer heights queried at that point.
For candidate placement this equals the unwrapped visible-copy center. Preserve
those native query touches; a fresh projector call is not a substitute for them.
Carry F/P explicitly into draw-plan generation instead of deriving one from a
rounded screenshot coordinate or a separately precomputed height array.

The subsequent object loop performs its own projector request. Its dispatcher
receives `(Px,Py-24)` at zoom0; at other scales use one native vertical step.
The established Base/Scout control reuses that dispatcher position. Do not reuse
the terrain loop's projection implicitly, omit the object projection loop, or
let each UI widget perform additional projection queries. Snapshot inspection
should read retained results without warming the view cache.

## Bounds

This supplement uses80x80 installed Planet,1024x511, zoom0/default relief25,
camera22,38 then24,38 and back, observer0 exploration on every tile, explicit
active game and empty Base/unit rosters. Full terrain/features and the full name
routine execute; final draw calls and generic presentation services are sinks.
Owned art pixels are external and no new whole-scene pixel parity is claimed.
It does not certify partially known geography, other flag combinations, late
name/overlay geometry when enabled, arbitrary pan histories or a natural faction
start. The general cache/selector bounds in the core handoff still apply.
