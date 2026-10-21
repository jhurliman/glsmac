# Arial10 card-font compatibility

The Workshop card receipt requests regular Arial at logical height -10. The
resolved owned-host font SHA256 is
`b3658eadae55e682b5f69eb64c439c1ecc8f196c0bb8d4756d145d13bc86476a`.
The [95 ASCII glyph receipt](design-workshop-arial10-cases.json) retains the
original LOGFONT, integer advances/bearings, complete64x64 mask hashes,
string extents and kerning observations. It is a bounded offscreen GDI API
receipt using the native request, not a new x86/whole-parent execution.

Raw monochrome FreeType matches91 of95 masks for this face at10px. The four
remaining glyphs are @, K, V and Z, with thirteen differing pixels in total.
Its lowercase c advance is6 while GDI uses5, shifting the rest of
`Attacks air units`; GDI measures that string at72 pixels, `Attacks ` at35,
`air ` at15, and `ck` at9. There is no ck kerning pair; whole-string GDI
pixels match additive glyph composition.

The loader now applies the measured95 advances only for the identified face,
10px size and ordinary monochrome mode. Four complete input-mask hashes guard
thirteen scan-conversion deltas to the loaded glyphs. Already-correct or unknown
raster masks are left intact. Other faces, sizes and raster modes keep their
existing path. No TTF or full glyph bitmap is embedded or redistributed.

The focused production-raster test compares every printable ASCII advance,
nonempty glyph bearing and complete4096-byte mask, then checks the diagnostic
string extents. Set `GLSMAC_WORKSHOP_REGULAR_FONT` to the matching owned face;
an absent variable means that asset-dependent check is not exercised. The
existing16px synthetic-body and14px caption GDI checks remain separate.
Unknown font hashes/raster versions do not acquire a pixel-parity claim.

Private immutable bundle:
`tmp/reference/independent-workshop-arial10-evidence-20261021`.
Manifest SHA256:
`e9e3938c5566a1642bfe6d1e9f3a03d4f2e1f39738e61ca680bcf15ee3a05f77`.
The [local reference reader](workshop_arial10_glyph_reference.py) validates
private compressed glyph masks against the public hashes. Original full font
and glyph buffers remain private.
