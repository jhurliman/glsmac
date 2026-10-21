# Workshop PSI and special strength text

2026-10-20. Original `terran.exe` SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Companion to [interactive editing](design-workshop-interactive-editing.md).
The [scalar reference](workshop_strength_text_reference.py) accepts signed
8-bit strength values and caller-supplied localized text.

## Armor: picker, side card and central stats

Stock armor ID **9**, Psi Defense, has strength **-1**. All three strength
formatters test the signed byte for negativity, then resolve LABELS offset
`0x310` through `62e860`. The installed English text is exactly **`Psi`**.
Do not print `-1` or uppercase the resolved label.

| Surface | Native formatting branch | Display for stock Psi Defense |
| --- | --- | --- |
| Armor picker number | `43c447..43c4b9` | `Psi` |
| Armor side-card Level line | `43ebfa..43ecfb` | `Level Psi` |
| Central armor strength suffix | `4413d3..441471` | `Psi` |

The side card resolves `Level` at LABELS offset `0xa6c`, appends one ASCII
space, then the formatted strength. There is **no colon**. Nonnegative armor
strengths use ordinary signed decimal conversion; there is no weapon-style
99 cutoff. Synthetic strengths -128 and -2 also produce `Psi`; zero produces
`0`. Strength is not multiplied by the reactor rating for these labels.

The central row has a cached label enclosed in literal braces, followed by
one space and the strength. The braces belong to the text styling syntax.
The controls verify its strength suffix; they do not initialize or validate
the cached label/font styling. The armor record's byte at offset +1 is an
unsigned class discriminator: when it is >=3, the side-card Level line and
central armor row are skipped (`43ebed`, `4413dc`). The armor picker also
suppresses its separate Shielding label at this discriminator threshold,
while its number formatter still follows the signed-strength rule.

Probability Sheath is **armor ID 5**, not Psi Defense. Its visual treatment
is documented in [stock composition families](design-workshop-stock-composition-families.md);
this text addition does not change that rendering contract.

## Nearby weapon and noncombat branches

These additional rules are source-traced unless listed as controlled below.

- The **weapon picker** (`43c052..43c09a`) recognizes exactly byte `0xff`
  (-1) as `Psi`. Otherwise mode <3 uses the strength byte as an **unsigned**
  decimal; mode >=3 uses the unsigned record byte at +3. Thus a hypothetical
  -2 combat picker strength prints `254`, despite the side-card behavior.
- The **weapon side card** (`43e7a0..43e7e7`) uses `Psi` for any signed
  negative strength, `*` for signed strength >=99, and decimal for 0..98.
- The **central combat weapon stats** (`441153..4411a3`) use the same
  signed-negative / 99 cutoff rule. Planet Buster's stock strength 99
  therefore displays `*` here, while its picker number follows the picker
  branch and displays `99`.
- Central weapon row dispatch happens first (`441097`): modes >=3 take
  `4411dd..4412d9`, using a different cached label at actor+`144e6` and the
  unsigned record byte +3. Do not run these noncombat rows through the
  combat-strength formatter. Modes below3 use actor+`143d8`.

## Evidence and scope

Additive private bundle:
`tmp/reference/independent-workshop-strength-text-evidence-20261020/`.
Manifest SHA-256:
`e061b28cd2bbd33867eb04fd35dd98ec3f0331ef5204ec9674c5db367fd7477d`.

**21 original x86 fragment controls pass**, with independent scalar reference
comparison: armor picker, full Level line and central suffix at strengths
-128, -2, -1, 0 and 12 (15), plus central combat weapon suffix at -128, -1,
0, 98, 99 and 127 (6). Controls use the installed original text resolver and
integer conversion. Entry registers, stack, component index0, strength byte
and empty output buffer are declared inputs. They stop after text construction,
before layout/drawing or its continuation. Central armor uses stock class0;
class>=3 skipping, weapon picker, weapon side card and noncombat row dispatch
are source-only findings. No whole-window, modal-loop, desktop or new model
matrix was run. The prior browse/reselect/picker freezes remain unchanged.
