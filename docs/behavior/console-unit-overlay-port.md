# Selected and roster Unit overlay port

The supported conventional card body remains the measured native work-surface
translation: selected Scout15×33 pixels/212 opaque pixels, roster11×23/113.
The previous tiny-body/decimal-movement screenshot predates those corrections.
No model enlargement or tuple/movement change is part of this slice.

The remaining actual mismatch was the overlay: the card reused `FLAGS.PCX`,
RGB recoloring/transparency fixes and atlas UV coordinates divided by texture
dimensions plus1. It now composes the original indexed `UNITS.PCX` template,
health and order glyph into an independent23×30 texture with exact0..1 UVs
and nearest sampling. Main-map badge artwork, placement and body geometry are
unchanged.

## Source and identity

- Template top-left is `(1+24*effectiveLevel,700+31*variant)`, size23×30.
- Rebase file bytes by10 with byte wrap; replace game6/7 with transparent9.
- Replace game4 with owner's primary and game3 with secondary palette index.
- Primary/secondary come from indexed `PALETTE.PCX` samples
  `(580+8*originalLoadedSlot,23/31)`, rebased by10 with byte wrap. These are
  the same metadata producers consumed by the graph palette loader.
- Resolve through the active Gamma100 software display palette: common
  channels cap235; reserved white/health colors keep their full values.
  Key9 is transparent; other black pixels remain opaque.

Original loaded faction slots differ from dense simulation/controller slots.
The owned `FR_SLOT_DEFINE` value carries a separate palette slot, default255
for unavailable. It is admitted only for a local single-player SMAC world
whose locked, frozen startup catalog matches the seven original bindings in
order and whose occupied profiles are known and unique. Any saved current or
pending native identity table must agree. Custom/reordered catalogs, conflicting
saved keys, duplicate profiles, networking and legacy absence retain fallback;
no filename, controller index or RGB reverse lookup supplies an identity.

The current port's canonical source catalog does not admit arbitrary original
custom ordering. The original does support such ordering and keeps slot colors;
the conservative fallback must not be described as completing that feature.

## Composition and live state

Selected overlay anchor is console-local14,66; roster anchor145,197. Both
retain zoom0 and remain independent of the body's−4/−8 views. Original flags
0x1800 bypass body anchor adjustments and body painting; they preserve the
standard bank/effective-level/fresh-spent selection.

The card uses the existing published effective morale, authenticated ownership,
remaining-movement admission, health denominator, and already-masked order
glyph/color. Only the owning viewer can select the spent variant. Health uses
the existing canonical2×21 black backing at6,6 and integer top fill. The order
uses Arial bold logical−12, native centered box and baseline16. No blink phase
is borrowed from a main-map actor for this explicit console overlay draw.

The Unit caches only its current composed value. DOM actors retain the texture
independently through Unit/scene retirement; replacing/removing the card retires
those actors before releasing their retained image.

Authority: [original card callers](lower-console-unit-card.md),
[badge metadata](native-display-identity.md),
[Unit overlay source/raster](map-unit-and-population-overlays.md), and
[ordinary Hold masking/color](ordinary-hold-glyph-colors.md).
These are bounded original offscreen/caller controls, not full original-game
framebuffer or whole-console parity.
