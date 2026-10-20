# Initialized Scout bodies in the lower console

The original initialized Infantry card dispatcher preserves the Unit's facing.
The old facing-0 console receipt establishes one fixture, not a rule that every
card faces zero. Neither the transparent256-square work surface nor the native
body scale should be fitted or enlarged to compensate for a different silhouette.

The [six original controls](initialized-console-scout-cases.json) execute actual
`56fd50` dispatch through `4d7860`: selected flags`0x48` at zoom−4 and roster
flags`0x400` at zoom−8. The helper reads signed Unit byte`+0x29` at`4d791f`;
the dispatcher preserves supplied facings0,2,5. The existing initialized factory
receipt asserts a fresh design2 Scout has facing2 before any drawing override
(`capture_initialized_infantry.py`, described in `../cvr-initialized-infantry.md`).

Initialization executes original constructors, palette binding/graphics prefixes,
107 original model loads, `data_setup(0,1)` and the design2/owner3 Unit factory.
The actual SYSTEM token-reader/normalizer fragment supplies color light. Source
assets and complete original buffers remain external to Git. The pinned executable
SHA256 and each complete indexed/depth/RGBA digest are in the cases file.

| Facing | Selected opaque pixels / bounds | Roster opaque pixels / bounds |
|---|---|---|
|0|212 / `[118,102,133,135)`|113 / `[121,110,132,133)`|
|2|244 / `[117,97,135,128)`|126 / `[121,107,133,129)`|
|5|267 / `[121,98,143,131)`|136 / `[123,108,139,130)`|

Bounds are half-open in the256-square work surface. Native wrapper anchors remain
selected`(71,103)` and roster`(181,222)`, with scales`0.06562500447034836` and
`0.04374999925494194`. All59 object frames and65 kernels execute in every control.
Every byte of all18 production surfaces matched these original buffers, including
depth and transparent pixels. Both RNG streams stay unchanged during drawing.

`UnitDef::GetCardSprite` now preserves the initialized presentation descriptor's
retained facing, while keeping each card's native zoom, scale and translation.
The actual caller regression uses facings2 and5, so replacing zero with a global2
does not satisfy it. Preinitialization Scout and Rover fixture views are unchanged.
The opt-in `CVR initialized console cards match six complete original body draws`
group verifies complete digests with `GLSMAC_SCOUT_ASSETS` pointing to owned assets.

This is a controlled flat-world/body-at-copy-entry receipt, not an original full
console framebuffer. Destination copying reuses `scout-target-copy.md`. Counter0
selects animation frame0; a nonzero-counter experiment did not return and was
stopped, so it supplies no animation evidence. Original initialized light and
native-lifeform bodies are separate profiles.

Live conventional Unit state still has no facing field or movement-facing write
authority. The detached `SmacNativeUnitRecord` does retain that byte, but is not a
substitute for conventional state/save/publication. Honoring the already retained
render descriptor fixes fresh initialized cards; it does not complete orientation
changes after movement or arbitrary combat animation.
