# Terrain admission from the retained landmark word

The 32-bit field at tile byte12 combines low-bit memberships/bookkeeping with
one shared detail byte in bits24..31. Testing the whole word against a short
allowlist rejects ordinary tiles. In particular, `0x00400000` is dirty
bookkeeping, not a terrain deformation membership.

For the existing ordinary geometry implementation, the relevant profile masks
are:

| Consumer | Profile membership mask | Detail use |
| --- | --- | --- |
| Land center/corner heights | `0x00000b03` (`1`, `2`, `0x100`, `0x200`, `0x800`) | Signed upper byte selects profile entries |
| Additional vertex shade | `0x00000203` (`1`, `2`, `0x200`) | Signed upper byte; negative detail skips the extra shade |

Height selection has priority `2`, `1`, `0x200`, `0x100`, `0x800`.
Shade selection has priority `1`, `2`, `0x200`. Do not apply a blanket
high-bit suppression rule to heights: their consumer does not contain the
negative-detail guard used by the shade consumer. Profile-bearing tiles remain
outside ordinary geometry support until those profiles are implemented.

Sea height queries return zero before reading the landmark word. Native sea
face shading also returns zero before the slope/profile path, and the ordinary
sea renderer uses its water-depth values rather than land vertex shading.
Freshwater membership `0x80` adds no geometry or shading profile. Nearby land
still requires the usual checks when it contributes to a coast polygon.

Consequently, values such as `0x00400000` and `0x75400000` do not require a
deformation fallback. `0x01400400` likewise contains no height/shade profile
membership, but its `0x400` Geothermal membership belongs to a separate material
continuation. A geometry admission decision does not close that material.
Jungle `0x4` and Uranium `0x8` also have separate renderer paths. Preserve
existing material exclusions until their respective implementations are ready.

For a conservative ordinary geometry filter, inspect the relevant land tiles
in the existing geometry/shading dependency stencil for `word & 0xb03`, rather
than treating every bit outside a material allowlist as deformation. Ignore
dirty bit `0x400000` and standalone upper-byte values for this decision. Retain
the original word unchanged for all other consumers.

The high byte is meaningful shared detail, not arbitrary unused padding. Its
values can change landmark art and shade admission when a corresponding low
membership is present. See [its lifetime contract](shared-landmark-detail-lifetime.md)
and [ordinary terrain geometry](ordinary-terrain-projection.md). This finding
does not establish parity for landmark artwork, alternate zooms, or the native
dirty-only redraw scheduler.

The [sea material supplement](sea-landmark-material-admission.md) adds27
complete-renderer controls for raw `0x10`, Freshwater `0x80`, dirty bookkeeping
and selected detail-byte combinations in the existing water/Fungus profile.
