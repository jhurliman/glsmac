# RE-SMAC-SETUP-PORTRAIT-001: palette, row copying and portrait trim

Static follow-up to [setup continuation](setup-dialog-continuation.md).
The receipt traces the pinned executable's selected-faction graphics loader,
PCX decoder, sprite extractor, generic picture control and keyed sprite copy.
All seven installed portrait crops are decoded independently as indexed PCX
data. No new original rendering/input run or physical framebuffer comparison
is claimed. [Numeric facts and conversion hashes](setup-portrait-render-cases.json).

## Active color authority

`45eb70(slot)` loads `<definition filename>.pcx` with
`5f0be0(filename, 0, 10, 236)` into temporary image `7c9c48`. The second
argument, the palette destination, is **null**. On load failure it tries
`faction.pcx` with the same parameters. This installed seven-profile receipt
covers the successful original sheet loads, not fallback visuals.

PCX decoder `5fb490` does read the sheet's embedded RGB palette into the
temporary image's own palette storage. However, `5fb7bc..5fb7d9` exits when
the passed palette destination is null, before `617880` can install it.
Thus the faction PCX's embedded palette is not the active color authority for
this setup portrait path. Use the already initialized common **PALETTE.PCX**
colors, including their gamma correction, as established by the
[common palette contract](ordinary-land-texture-palette.md).

The prior call to `4297e0(slot)` constructs faction color/look-up data.
The ordinary setup picture copy described below does not consume that
faction tint table or perform nearest-RGB matching against the embedded
faction palette. Its nontransparent pixel store preserves the loaded index.

Embedded faction palettes differ from the common palette even at indices
used by these portraits. For example, Morgan uses source indices129..138
whose embedded RGB triples differ from PALETTE.PCX. Reading the faction
sheet through a generic image decoder's own palette can therefore produce
different portrait colors despite using the correct source rectangle.

## Pixel index rebasing and gamma are separate operations

For each decoded source byte, `5fb6c3..5fb6e3` adds the offset in an
8-bit register and masks to one byte before writing the RLE run:

`loadedIndex = (sourceIndex + 10) & 255`

There is **no clamp of the pixel index at235**. Source235 becomes245;
236 becomes246;245 becomes255;246 wraps to0;254 becomes8;255 becomes9.
The 236 count controls palette entry copying, not an index clamp. The
sprite's key is loaded index9, corresponding to source255 for this load.
Perform transparency before turning indices into RGBA.

The initialized common palette copies source entries0..235 to active game
entries10..245. The existing native Gamma100 evidence uses per-channel
`trunc(235.99 * clamp(pow(channel/235, 1/gamma), 0, 1))`, gamma1. Its
checked result is `min(channel,235)`. That is a **channel** clamp in the
common palette stage, not a clamp on a faction-sheet pixel index. Do not
apply gamma a second time to each portrait after using the corrected table.

The other20 display entries come from palette initialization. The measured
software DIB values and PC_EXPLICIT/device distinction are documented in
[reserved software colors](scout-effect-reserved-rgb.md). This path does
not replace them with the faction sheet's final20 RGB triples. In the seven
actual portrait crops, every opaque source byte is0..235 and the only other
byte is255 at four transparent corners, so none of the opaque portrait
pixels uses a reserved display entry. No reserved-color effect/escape needs
to be synthesized for these crops.

## Scanlines and scaling

Decode the full PCX padded rows using its `bytesPerLine`, then crop at
`(304,457)` with size131×157. All seven sheets have width/stride1024 and
height768. Do not use131 as the source row stride or discard alternating
PCX rows. There is no full transparent portrait row in these assets.

`6203c0` constructs the generic picture member through `62a250`. The
picture's scale numerator/denominator start1/1 (`62a372/375`), and its
optional draw callback starts null (`62a384`). `62ad10` attaches the sprite;
the setup preview does not install a replacement draw callback there.
With the default callback, picture draw `62b080` calls `5fd990`, passing
the sprite's key and its numerator/denominator. `5fd990` temporarily applies
that ratio and calls `5fd9e0`.

At1:1, `5fd9e0` tests each source byte against the key and copies each
non-key index unchanged; the row loop advances the source and destination
pitch every row (`5fdcf8..5fdd0b`). **It adds no alternating dark/transparent
scanline overlay, extra gamma, tint or color matching.** Preserve any stripe
or shading already in the asset's indices. The same wrapper also has
separate integer/rational scaling branches; this receipt does not claim a
new full-resolution UI placement or scaled pixel-parity comparison. Do not
replace the native ratio with an invented scanline effect.

## Trimming and the anchor

Extractor `5fc7d0` records the original requested width/height at sprite
`+18/+1c`, finds fully keyed outer rows/columns, stores trimmed width/height
at `+10/+14` and trim-left/top at `+20/+24`, then copies the retained index
rectangle. At1:1, blitter `5fd9e0` adds those left/top offsets to the
requested destination before clipping. Layout uses the original width and
height, so trimming storage must not move the logical portrait anchor.

For **each of all seven installed setup portraits**, the result is:

- Original and retained dimensions:131×157.
- Trim-left/top:0/0.
- Transparent source coordinates, relative to the crop:
  `(0,0), (130,0), (0,156), (130,156)`.

There is no trimming-induced anchor correction for these seven images.
Custom faction assets can have different trim bounds; retain their original
extent and offset rather than assuming every image has the stock bounds.

## Port conversion and verification

For a stock Gamma100 portrait, keep source255 transparent and map every
other source index directly through the common palette's corresponding
RGB triple with channels capped at235. Output opaque alpha255. The numeric
cases provide SHA-256s for all131×157 row-major RGBA bytes, with transparent
pixels canonically encoded `(0,0,0,0)`. These are independently derived
asset/conversion expectations, **not native framebuffer hashes**. No owned
image pixels or full palettes are placed in public fixtures.

The static extractor checks17 literal instructions, all seven PCX headers,
RLE extents, corner coordinates and allowed portrait indices. It retains
source-byte-range hashes, indexed function disassemblies, source asset
hashes, histograms and differing-palette index counts privately. It can be
reconstructed with `--verify-only`; no production code or installed asset
was changed. System palette mode, display gamma other than the reused
common-palette contract, dynamic palette effects, scaled clipping and
whole dialog rendering remain outside the new verification.
