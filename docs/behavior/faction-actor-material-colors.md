# RE-SMAC-ACTOR-COLOR-001: faction vehicle material colors

Status: **15 original-x86 material cases and 20 composed Spartan Recon views
pass**, against [the ledger's fingerprint](README.md). Original PCX decoding,
software palette construction/installation, faction sampling, material arithmetic
and nearest-color search execute. An independent model matches all6,144 indices
in every material case. [Acceptance cases](faction-actor-material-colors-cases.json).

This extends [Spartan Recon composition/view](spartan-recon-composition-and-view.md).
The earlier zero-remap fixtures remain frozen geometry/view controls. The new
fixtures supply separately produced material inputs and colored indexed-buffer
hashes. Installed map light remains6/4/15 with original normalization.

## Palette and sampling

The traced main palette filename is `PALETTE.PCX`. These checks execute the
original software/default palette constructor, decode that file and install236
colors starting at index10. Reserved indices0..9 and246..255 retain original
constructor colors. Hardware/system-palette mode is excluded. Faction PCXs
decode without replacing the active game palette.

The decoder uses an index offset10 on these loads. Sample decoded bitmap
indices, then resolve RGB through the active game palette. Unadjusted file
indices or the faction file's palette can produce different colors.
Coordinates below are zero-based:

| Pixel | Produced field |
| --- | --- |
| (161,747) | Name/UI color index |
| (161,757) | Blend control: subtract10 from index, multiply100, divide255 and truncate |
| (435,745) | Vehicle material tint index |

Name color and blend control are separately sampled fields. The checked body
material calculation uses only the vehicle tint RGB. Installed samples pass
the caller's validity checks; invalid/missing samples and fallback are excluded.

## Material arithmetic

The source is **VBP.CVR**, specifically its decoded palette chunk's24-by-256
shade lookup. It is not selected anew from each tinted vehicle part. The
[input-provenance fixture](faction-actor-color-inputs-cases.json) binds the
asset identity/hash and verifies original loaded bytes against the independent
[CVR shade decoder](cvr-palette-and-shade-decode.md).

Use the common loaded body model's24 shade rows, each containing256 source
palette indices. Resolve each source index and vehicle tint index through the
same active palette. For each RGB channel compute:

`channel = truncate(source_channel * tint_channel * 0.003921568859368563)`

The factor is the original float32 approximation to1/255. Checked channels
are nonnegative; truncation is toward zero. Search all256 palette entries for
the smallest unweighted squared RGB distance to this triple. Equal distances
keep the lowest index. Store the chosen index at the corresponding shade and
material position, preserving24-by-256 order for the6,144-byte output.

This nearest-color mode includes reserved colors. The checked table does not
incorporate the name color, blend percentage or HSV arithmetic.

| Faction PCX | Tint index | Active tint RGB | Distinct output indices |
| --- | --- | --- | --- |
| gaians.pcx | 152 | 116,156,56 | 50 |
| hive.pcx | 4 | 0,0,128 | 12 |
| univ.pcx | 72 | 173,196,192 | 131 |
| morgan.pcx | 251 | 255,255,0 | 52 |
| spartans.pcx | 80 | 64,76,76 | 46 |
| believe.pcx | 229 | 208,112,24 | 38 |
| peace.pcx | 237 | 116,116,200 | 78 |

These are faction identities, not fixed owner slots. All seven files are checked
at owner1; Spartan additionally runs at owners1..7 and repeats at owner1.
All Spartan rows match SHA-256
`cf8960d9612c082f18e5411cc03809523c4641361f7ea5a366cebddc9165932b`.

## Reserved colors and shade asset

Extension **RE-SMAC-ACTOR-COLOR-INPUTS-001** captures the original software
constructor's20 reserved entries and verifies that actual PALETTE.PCX
installation preserves all four bytes of each. These are constructor constants,
not colors to extract from the PCX. In particular, do not substitute a generic
system palette for indices8,9,246,247. Entry flags are retained here for exact
provenance; they are not an alpha channel or an input to the squared-RGB search.

| Index | RGB | Entry flag |
| --- | --- | --- |
| 0 | 0,0,0 | 4 |
| 1 | 128,0,0 | 4 |
| 2 | 0,128,0 | 4 |
| 3 | 128,128,0 | 4 |
| 4 | 0,0,128 | 4 |
| 5 | 128,0,128 | 4 |
| 6 | 0,128,128 | 4 |
| 7 | 192,192,192 | 4 |
| 8 | 8,0,0 | 2 |
| 9 | 9,0,0 | 2 |
| 246 | 245,0,0 | 2 |
| 247 | 246,0,0 | 2 |
| 248 | 128,128,128 | 4 |
| 249 | 255,0,0 | 4 |
| 250 | 0,255,0 | 4 |
| 251 | 255,255,0 | 4 |
| 252 | 0,0,255 | 4 |
| 253 | 255,0,255 | 4 |
| 254 | 0,255,255 | 4 |
| 255 | 255,255,255 | 4 |

VBP.CVR SHA-256 is
`00eedcd3a2a666d704dffc42551be497150b666343cc10819ae5a66a1fae9c8b`.
Its installed version is10.100000381469727. Outer chunk3 contains nested
palette chunk513 (`0x201`); decode the modern palette payload using the linked
contract, skip the two range bytes and768 RGB bytes, and take the remaining
6144 shade bytes. Their SHA-256 is
`7ec7220b339f81a78ddb0ff5570f97cd8dc25f7a0e9eb88ca610326b98125e1c`.
The original model loader's named VBP object supplies exactly those bytes.
No source shade or player palette payload is embedded in the public fixture.

## Recon composition and limits

The new20-view checks retain the earlier actual startup-produced Spartan unit/
design, complete107-asset initializer, ten-part idle scene, camera, light,
indexed background9 and cleared depth buffer. The body remap is now produced
immediately before drawing; VGMCP and VBP consume it. All20 depth hashes match
the prior controls. All19 visible indexed hashes change. Visible views retain24
kernels each; the off-target view has0. Each view retains ten frame/draw/lighting
operations and intact buffer guards.

Memory-backed file/bitmap allocation and reached text/rectangle services are
supplied. Decoded pixels, palette colors, sampled tint, material results and
nearest-color returns are not supplied. Outer faction file-cache wrappers,
missing-file recovery, complete cache invalidation, OS/GDI allocation, hardware/
system palette, final target copy/native RGB/window and live observations remain
excluded. This is a staged routine composition.

PALETTE.PCX SHA-256 is
`54341b1bee31736997950e432ff3312a620f7cf722016c154098a4c78d28f866`.
Fixtures bind individual faction-PCX, material and view hashes. No proprietary
palette/model/material payload is distributed; derive tables from player data.
