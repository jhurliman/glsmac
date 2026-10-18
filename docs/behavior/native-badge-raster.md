# Owner0 native badges: gamma100 palette and transparency

This is a renderer-ready recipe for the fourteen original23x30 UNITS.PCX badge templates, seven effective levels times two variants. Fourteen original sprite extracts/remaps and28 original software blits match an independent player-PCX decoder. The fixture contains indexed and canonicalRGBA SHA256 for every output; original pixels/assets are not embedded.

## Exact recipe

1. Decode installed UNITS.PCX indexed pixels, retaining indices. Convert each file index to game index `(file_index+10)&255`; do not clamp at235 or discard the wrapped/system indices.
2. Original builder replaces game7 with9 and game6 with9 before extraction. Extract level m/variant v at `(1+24*m,700+31*v)`, width23,height30.
3. For owner0 replace game4 with249 (primary bright red), then game3 with1 (secondary dark red). These are index replacements, not RGB matching.
4. Game9 is this sprite's transparent key. For other actual badge indices, resolve RGB through the active main SOFTWARE palette and use alpha255. For canonical export/hash, key9 becomesRGBA(0,0,0,0).

```text
g = (file_pixel + 10) & 255
if g == 6 or g == 7: g = 9
if g == 4: g = 249
else if g == 3: g = 1
RGBA = (0,0,0,0) if g == 9 else (main_palette[g].RGB,255)
```

At gamma100 the main palette factor is1.0. Construct it from the original20 reserved software entries plus PALETTE.PCX file RGB entries0..235 installed at game indices10..245. The complete twenty-entry reserved table is in native-badge-raster-cases.json, reused from the original constructor receipt. Those constants take precedence over a generic OS/system palette and over UNITS.PCX local palette values. The independently constructed1024-byte RGB+entryflag table matches the original active palette hash `bea9081f0b2fae7b300871cbfb2ae9a7fcec8a2f13f43e5706971723575db30e` and the earlier fresh gamma100 initialization receipt.

Palette entry flags are NOT alpha. In particular reserved key9's storedRGB(9,0,0)/entryflag2 does not make it a dark-red opaque pixel in this sprite. Transparency comes from the sprite key.

| Final game index | RGB at gamma100 | Badge treatment |
| --- | --- | --- |
| 249 | 255,0,0 | Opaque primary |
| 1 | 128,0,0 | Opaque secondary |
| 65 | 0,0,0 | Opaque black |
| 72 | 173,196,192 | Opaque variant0 detail |
| 155 | 216,224,244 | Opaque variant0 detail |
| 44 | 226,203,176 | Opaque variant1 detail |
| 46 | 204,163,137 | Opaque variant1 detail |
| 9 | StoredRGB9,0,0 | Transparent key, canonicalRGBA0,0,0,0 |

File249 wraps to game3 and then becomes owner0 index1. File250 wraps to game4 and then becomes249. File255 wraps to transparent9. Direct use of local PCX RGB is not the general original palette recipe, even where a particular owner0 color happens to coincide.

## Holes, black and shadows

Every actual output pixel belongs to the table above. Actual fourteen rectangles contain no decoded6/7 before preprocessing; the whole asset preprocessing still occurs exactly as shown. There is no additional alpha cutout, health-bar hole or fractional shadow opacity in these captured templates. **Do not make index65 black transparent or semitransparent.** The native software blit copies every non-key pixel and leaves every key9 destination byte intact, checked on both key9 and arbitrary diagnostic backgrounds.

No index8 occurs in these fourteen outputs. Its reservedRGB is recorded for palette completeness; this handoff does not define a universal shadow rule for other assets or other sprite/render routines. Dynamic health-fill, order overlays, body shadow/CVR and the whole composed Unit draw remain separate from this badge texture.

## Variant used by ordinary unselected natives

The live map draw caller initializes variant0. In ordinary local mode it only queries movement allowance when the Unit owner equals the current viewer. A native owner0 shown to a normal positive human viewer therefore keeps **variant0**, regardless of remaining movement. Six original decision fragments cross designs8/9/10 and spent0/255: all choose0 and never enter the allowance query. Do not choose native variant1 merely because the native cannot currently move or is not controlled by the human.

This is source-traced live caller behavior plus isolated native decision controls, not a new full-window capture. Network mode and owner-equals-viewer use different allowance gates; this ordinary-local result is not generalized to them. Existing selected-unit blink can omit overlays; it does not redefine this native bank variant.

Use bank index `14*Unit_owner + 2*effective_level + variant`. Owner0 is the actual Unit owner, independent of ecological attribution. Effective native level uses current viewing faction context as already established, not stored morale alone.

## Output convention and limits

Each indexed hash covers690 bytes, row-major top-down23x30, key pixels9, no row padding. Each canonicalRGBA hash covers2760 bytes in the same order, zeroRGB/alpha0 for key9 and active-paletteRGB/alpha255 elsewhere. Native code produces indexed software pixels, not RGBA; the alpha encoding is an explicit equivalent export convention.

Actual original palette constructor/PCX decoder, preprocessing, sprite extraction, recoloring and software raster execute. Heap/software bitmap backing, rectangle OS service and UI polling are supplied; no pixel/palette result is supplied. Original whole badge-bank constructor, full map body composition, scaled/filtered presentation, live window/hardware palette and non100 gamma are not newly established. Prior default-palette/gamma100 source evidence is reused. No terrain/animation campaign or installation writes occurred.

Cases and all14 hashes: native-badge-raster-cases.json. Frozen supplement: tmp/reference/native-badge-raster-evidence-20261018. Broad RE goal remains paused.
