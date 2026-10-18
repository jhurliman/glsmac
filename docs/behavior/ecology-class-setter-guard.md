# Class-only setter: exact rockiness guard and landmark word

RE-SMAC-ECOLOGY-CLASS-SETTER-GUARD-001, 2026-10-18. Clarifies the abbreviated
rockiness condition in the frozen [sea terrain handoff](ecology-sea-terrain-mutation.md).
[Eleven small native helper controls](ecology-class-setter-guard-cases.json).
No earlier fixture or frozen evidence was changed.

## Exact lowering predicate

On the same land/sea-category path used by the ecology target-class2 shaper,
the lowering rockiness draw requires **all** of:

```text
currentTurn != 0
oldClass >= 3 && newClass >= 3
newClass < oldClass
R > 0
R > newClass - 3
```

Thus class0/1→1/2 with R1 consumes **no rockiness draw**. The setter first
compares old/new land-versus-sea categories; on the same-category branch it
skips the entire rockiness block when the new category is sea. The strict
lowering comparison is another required gate. The earlier two rockiness tests
alone were not the complete predicate.

When admitted, one existing CRT draw modulo `(R+1)` is consumed. Remainder0
preserves R; nonzero reduces R by one, ORs landmark metadata `0x00400000`, and
sets global dirty mask1. The final class store sets dirty mask4 independently.
No difficulty/faction/rule condition is added to this same-category lowering
branch. Cross-category conversion has separate effects and is outside this
clarification and the earlier target2 terrain controls.

For completeness, **raising land** has a different source branch: nonzero turn,
same land category, `newClass > oldClass`, `R < 2`, and `R+1 <= newClass-3`.
It draws modulo `(R+4)` and increases R by one only on remainder0, with the same
metadata/dirty markers. That raising branch is source-backed here, not a new
runtime case or an ecology target2 path.

## Exact landmark-word transformation

On a nonzero turn, a **changed** class calls the landmark invalidator before the
category-specific effects. Its exact unsigned32 word operation is:

```text
invalidate(w):
    if w == 0: return 0
    low = w & 0x00ffff7f     // preserve low24 except clear bit0x80
    high = (w >> 24) & 0xff
    if high < 0x80: high = high ^ 0xff
    return low | (high << 24)
```

This is **not** universally `0xff000000 | oldLow24`. A nonnegative signed detail
byte is complemented; an already-negative detail byte is preserved. The zero
word is special-cased **before** clearing bit0x80, so word0x80 becomes0xff000000,
whereas word0 stays0.

| Before | After invalidation |
| --- | --- |
| `0x00000000` | `0x00000000` |
| `0x00000080` | `0xff000000` |
| `0x01000082` | `0xfe000002` |
| `0x7f000082` | `0x80000002` |
| `0x80000082` | `0x80000002` |
| `0xff000082` | `0xff000002` |

These six exact words were checked through complete native class1→2 calls.
If rockiness subsequently changes, the final word is this invalidation result
**OR `0x00400000`**. If the rockiness draw leaves R unchanged, that marker is
not added. Other low membership/detail bits are not replaced with a guessed
constant. Landmark-cache/view bookkeeping inside the invalidator is separate
from the terrain word and does not consume RNG in these controls.

## Unchanged class and turn zero

An unchanged-class call skips invalidation and skips both lowering/raising RNG
branches. For example class3→3, R2, word0x01000082 retains R2 and the entire word,
including bit0x80, and consumes no draw. It still writes the same class bits
back and sets **global dirty mask4**: the helper is not wholly side-effect-free.

Turn0 skips the pre-store invalidation/category/rockiness work even when class
changes. The class store and dirty mask4 still occur. The literal class6→3,
R2, word0x01000082 control preserves rockiness/word and consumes no draw then.

The eleven controls execute the complete original class-only helper and actual
CRT, with runtime TLS and zero-flag view backing. They cover sea/no draw,
unchanged class, turn0, land lowering with zero/nonzero remainder, and the six
landmark words above. No broad climate, UI or new terrain-shaper investigation
was performed; the standing research goal remains paused.
