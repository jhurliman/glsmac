# Coast depths and clipped alternate opaque sampling

Eight new bounded native primitives settle two renderer questions. [Numeric controls and source](survey-coast-clamp-and-clipped-opaque-cases.json) include actual primitive inputs, exact ordered-store/frame hashes, negative-control differences and complete pinned function disassembly. Native receipts and pixels are in `tmp/reference/survey-primitive-discriminators-evidence-20261020`. No foreground game interaction or entity rendering occurs.

## Correction: ordinary coast does not apply terrain caps

Both ordinary full-color coast `6393d0` and alternate `63e880` use supplied vertex depth/light values without reading ordinary terrain clamp globals `6b5564/6b5568`. Full wrapper `63e880(...,0xffffffff)` delegates to `6393d0` at `63e8b1`. Both implementations store the supplied values pointer at `9c447c` and interpolate its values directly. Neither function contains a read of those clamp globals.

The earlier frozen Unity Survey viewport prose incorrectly said full `6393d0` clamps active`[-4,4]`. I inferred a shader contract from the globals captured at entry; those globals were merely measurements. **Preserve the port's already verified ordinary coast behavior.** Existing ordinary valid-range depth controls using broad`[-256,256]` reference limits remain consistent with native code. There is no full-versus-alternate distinction in vertex clamping.

The direct discriminator uses owned water crop/bank`0x786718`, generated coast mask bank`0x784d48`, a synthetic53-entry mapping containing only1 (always choose shaded water), vertices`[(100,100),(156,100),(156,156),(100,156)]`, UV`[(0,0),(55,0),(55,55),(0,55)]` and constant depths`[12,12,12,12]`. It invokes actual wrapper`63e880(bitmap,vertices,depths,mask,mapping,4,uv,flags)` twice per mode with global limits`[-4,4]` and`[-1,1]`.

- Full mode writes source+12 on all covered pixels. Changing those globals leaves the complete3136-store stream and frame unchanged.
- Alternate`0x41` writes65 on even Y and source+12 on odd Y. Changing those globals again leaves its3136-store stream and frame unchanged.
- Both calls retain the input depths and global values unchanged.

Additional full/alternate calls use varying vertex depths`[-12,28,40,-8]`; independent interpolation with uncapped values matches every store. References that apply the captured narrow globals fail all six coast controls. The mapping is synthetic to isolate the shader; it is not a claim that normal material production emits that mapping or arbitrary test depths.

## Clipped opaque lag starts at the clipped span

Alternate opaque`63dcc0` uses its actual left-clipped U/V to initialize the source address (`63e10f..63e125`, then`63e197`). Its initial AL fetch and subsequent fetch-before-source-advance pipeline (`63e221`, `63e23e`, `63e244`) repeat the **first visible texel of each clipped odd-Y span**, then trail ordinary UV sampling by one destination pixel. It does not inherit the pending source texel from the polygon's unclipped left edge. Even Y still fills65.

The control uses the same56×56 quad/UV, owned arid bank`0x7866a8`, constant lights2, globals`[-4,4]`, flags`0x41`, initial backing73 and clip`[113,107,143,145]`. All1140 exact stores match independently generated clipped spans with the first visible sample repeated.

A competing model that renders the lag from the unclipped left edge before applying clipping differs at9 first-visible odd-row pixels, all at x113. For example `(113,107)` is native index60 versus wrong-model56; `(113,109)` is native56 versus wrong-model60. Full indexed clipped-frame SHA256 is `f8795ba30cff3ae17e34ea4850b5fe2636e0768ee1675b1ffa8a979d5ef64a64`.

## Validation

From repository or frozen bundle root, with installed owned assets:

```
python tmp/reference/check_survey_primitive_discriminators.py --verify-only
```

All8 native primitives and23092 exact ordered stores match independent spans/source sampling, with10 literal instruction assertions and owned-art atlas auditing. Wrong narrow coast clamps and wrong unclipped lag are discriminated. The updated working full-viewport checker also removes the false ordinary-coast cap assumption; its eight viewport outputs remain unchanged. Existing frozen archives remain byte-for-byte intact; this correction supersedes their clamp prose.
