# Workshop saved-strip ordinary-unit raster

The [saved-strip layout](design-workshop-saved-strip.md) call boundary now has
bounded original pixel evidence for Scout Patrol, Scout Rover, armed Foil and
armed Needlejet. The full original `56fd50` wrapper takes the ordinary idle
`4d7860` path under these inputs. This closes the thumbnail boundary separately
from the main Workshop model and whole-parent console composition.

## View and copies

The first card calls `56fd50(7cbc04,2048,42,207,-10,0x1048,0)` after original
`5d9ca0` scratch-unit construction. Scratch Unit2048 has facing **2** in these
controls. Scene8f2790 calls `631370(canvas,60,225,1)`. The 18-pixel displacement
comes from truncating `50*(zoom+16)/16`. Its 256-square surface is copied at
**(-68,97)**, under the card's half-open clip **(29,197)-(95,243)**.

Ordinary scale is binary32 `0.07*(zoom+16)/16`, giving
`0.026250001043081284` at zoom-10. This chassis0/weapon0 Scout applies another factor1.25, yielding
`0.03281250223517418`. The [exact stored-component multiplier predicate](ordinary-actor-scale-predicate.md)
distinguishes Infantry weapons, noncombat designs and Speeder Probes. Reuse the checked ordinary view and shadow-camera
references; the main Workshop camera scale0.14 is a separate path.

The original copies shadow first (`5f1ce0`, source9c1ba8, key0), then body
(`5f12c0`, source9c1600, key9). A nonzero shadow stamp replaces the existing
target index with `shadowLookup[existingTargetIndex]`. Its stamp value128 is
not an alpha amount or a palette-table index. Color9 remains transparent
independently of depth. Both source surfaces are 256x256 in these controls.

Body lighting normalizes (6,4,15). Shadow direction is a separate scene field:
approximately (-0.5773500204,-0.2109061331,-0.7887869477). Animation phase0,
flat slope and the standard ordinary camera inputs are explicit fixtures.
Owner1 is supplied the independently checked **Spartan material tint**; owner
number alone does not establish a faction's art colors.

## Initialized lookup precision

The original initializer fragment42a042..42a05a calls617fa0 with brightness-43
to produce the 256-byte table at71db18. It must run before initialized-shadow
expectations are used. A zero-filled table darkens shadowed target indices to0
and is retained only as an initialization control.

At the same Gamma100 RGB palette, x87 control word **027f** produces table hash
`70b3ab988991323661c52a73dd8459d6b06202a9e8c6fec329e4f12b0fb7aa49`;
**037f** produces
`0229080d2b3b2e56a39523f3ced54fcbcbe8bc8dea8a8548d5fbe75820cd6ef3`.
Only entry6 differs:177 versus178. Repeating037f is stable. The earlier apparent
before/after-draw distinction was inherited x87 precision, not RGB palette
mutation. The renderer controls use037f; the initializer precision is recorded
separately. Whole-parent initialization has not established which state reaches
this initializer in a complete live session.

Eight original copy-only controls apply both tables to the four checked model
surfaces and match independent keyed/clipped copy replay. Scout's final target
changes between the two precision states; the other three card targets do not.
The private receipt names every source and target hash, making either explicit
precision profile reproducible without another model draw.

## Validation and frozen handoff

Additive bundle:
`tmp/reference/independent-workshop-strip-raster-evidence-20261021/`.
Manifest SHA-256:
`7fc13d4a01cad80d397a0e0ed446cdf7fcecc9e521e1933aa3d89621f21a10ff`.

- Four independently reconstructed component compositions,31 operations.
- Nine full original wrapper draws covering four designs and initialization
  controls; independent whole color/depth buffers match all nine.
- Independently derived ordinary views and projected shadow masks match all
  nine; final whole indexed targets match, including clipping and copy order.
- Three isolated initializer controls establish the precision distinction;
  eight additional original copy controls cover both precisions for all four
  designs without repeating model rasterization.

The replay deliberately accepts original decoded geometry, prepass and kernel
streams as declared stage inputs. It does not claim complete asset decoding
through whole-window regeneration. Original model pixels and palette bytes
remain private in the bundle. Its per-root `checked-contract.json`,
`checked-view-shadow-copy.json`, `checked-composition.json`, and copy-precision
`controls.json` provide the port's exact stage expectations. Whole-parent UI
composition, other animations, abilities and faction color profiles remain
outside this bounded addition.
