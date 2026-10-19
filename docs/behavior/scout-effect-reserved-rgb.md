# Reserved display colors for Scout effects

Pinned stock SMAC, Gamma100, software palette mode. Supplement to the [initialized index252 receipt](scout-effect-index252.md). [Numeric controls](scout-effect-reserved-rgb-cases.json); [private source map](../re-smac-scout-effect-reserved-rgb.md).

The measured software DIB color table supplies these **RGB** values. They are not taken from `PALETTE.PCX`, which installs only game entries10..245. The reserved values survive that installation and Gamma100 unchanged.

| Index | Software DIB RGB |
| --- | --- |
| 0 | `(0,0,0)` |
| 1 | `(128,0,0)` |
| 2 | `(0,128,0)` |
| 3 | `(128,128,0)` |
| 4 | `(0,0,128)` |
| 5 | `(128,0,128)` |
| 6 | `(0,128,128)` |
| 7 | `(192,192,192)` |
| 8 | `(8,0,0)` |
| 9 | `(9,0,0)` |
| 246 | `(245,0,0)` |
| 247 | `(246,0,0)` |
| 248 | `(128,128,128)` |
| 249 | `(255,0,0)` |
| 250 | `(0,255,0)` |
| 251 | `(255,255,0)` |
| 252 | `(0,0,255)` |
| 253 | `(255,0,255)` |
| 254 | `(0,255,255)` |
| 255 | `(255,255,255)` |

For the measured index252 escape, backgrounds `[0,13,44]` become indices `[0,1,6]`, then RGB `[(0,0,0),(128,0,0),(0,128,128)]`. A port's canonical RGBA representation uses alpha255 for these resulting pixels. The DIB table's fourth byte0 is reserved padding, not an alpha channel. Transparency is the earlier source-key comparison; output index0 does not become transparent.

Sixteen reserved colors come from fixed executable color tables. The constructor explicitly overrides logical entries8/9/246/247 with low-word values8/9/245/246 and flag2. The other reserved entries carry flag4 in the common palette. The original software bitmap publisher converts the palette to BGR0, discards these flags and submits all256 RGB entries through [SetDIBColorTable](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setdibcolortable). Thus the table above is verified at the DIB publication boundary, including the four unusual red values.

Do not confuse that boundary with logical-palette realization on a palette-managed device. Flag2 is `PC_EXPLICIT`: in that separate GDI interpretation the low word identifies a hardware-palette index, rather than literal RGB. [Microsoft's flag definition](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-wmf/45e5c320-55b4-4f3a-af55-40f9e0f6e3ac) supports this distinction. The original also has a distinct system-palette initialization mode that requests entries from the OS. Neither that mode nor actual logical-palette/device realization is measured by this software receipt.

The new control executes original palette construction, PCX installation, native Gamma100, lookup initialization, bitmap palette conversion/publication and indexed compositing. The supplied memory GDI service captures the submitted table; it does not invent or remap its colors. All20 reserved entries match their constructor values and actual published BGR0 entries. No physical monitor/framebuffer, device quantization/color management or new SMAX display behavior is claimed. No production changes were made.
