# Preferences initialized shared theme correction

2026-10-21. Original terran.exe SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
[Scalar native receipts](preferences-initialized-theme-cases.json) and
[self-authored extraction/composition recipe](preferences_initialized_button_reference.py).

**The prior null-skin procedural button target omitted initialization.**
Use this binding for initialized normal-game Preferences. The older frozen
pixel/button packets remain immutable evidence of an incomplete-startup
fallback; their appearance and pressed/focus effects must not be ported as
normal-game behavior. The67 preference mappings are a separate contract.

## Startup and inheritance

The source path `53f6e0 ->5a59c0` calls `460f70`, whose `460f7c` calls
**`460720(0)`**, before `59f880` initializes the font/default bank. Later,
`5a57e0@5a5853` calls the Preferences constructor `49bde0`.

`460720` first loads `iface.pcx`, samples ten palette roles at x931,
y`2+11*i`, and installs generic window-frame slices. It then installs button
slice pointers in `9c1344/48/4c` (middle), `9c135c/60/64` (left) and
`9c1368/6c/70` (right). Its later interface loader creates the pointed-to
sprites. It finishes with `45eea0`.

An early button object's null pointers are not decisive: **`621620` calls
virtual+168 (`6221f0`) when creating the control**, refreshing every state
from these shared globals. Instance middle pointers are `+af8/afc/b00`,
left pointers `+b10/b14/b18`, right pointers `+b1c/b20/b24`.
The whole-sprite and frame-bank pointers remain null. The initialized painter
therefore takes **`6223ba..62266a`**, not procedural branch `62266f`.

## Owned-asset extraction and exact painting

All rectangles below are `(x,y,width,height)` in **`interface.pcx`**,
not `iface.pcx`. Native extraction is in `460df7..460ec9`.

| State | Meaning | Left cap | Repeated middle | Right cap |
| --- | --- | --- | --- | --- |
| 0 | ordinary | `(64,127,3,20)` | `(68,127,67,20)` | `(136,127,3,20)` |
| 1 | hover | `(64,148,3,20)` | `(68,148,67,20)` | `(136,148,3,20)` |
| 2 | pressed or selected | `(64,169,3,20)` | `(68,169,67,20)` | `(136,169,3,20)` |

Native PCX loading adds10 modulo256 to source indices; loaded indices6/7
are replaced with key9 before extraction. Paint onto a key9 child plane:
left at `(0,0)`, right at `(W-3,0)`, then tile the middle from `(3,0)`
with67-pixel period, clipped to `(3,0)-(W-3,20)`. **Do not stretch it.**
For W214, native middle draws start at x3,70,137,204; the final draw clips
at x211. All native sprite primitives use transparency key9. This recipe
covers the20px-high Preferences controls with nonoverlapping3px caps.

Select state as follows: explicit override `+ab8` in0..2 wins; otherwise
pressed `+a9c` or selected `+a18` chooses2, then hover `+a74` chooses1,
otherwise0. A selected page keeps state2 after the mouse leaves.

**No caption shift, bevel reversal or extra focus outline occurs in this
skin branch.** Those effects belong to the skipped procedural branch.
All eight native measurements keep the OK caption at x98/baseline14,
Arial height−12/plain, foreground palette0 and shadow−1. Its black ink is
legible against the authored blue skin. Normal equals explicit0 and focused;
hover equals explicit1; pressed equals selected and explicit2.
The focus Boolean was admitted at helper60bb20, not through an OS message loop.

The initializer also changes the otherwise unused fallback color properties:
+a84=19, +a88=73, +a8c=147, +a90=2, +a94=0, +a98=207.
Do not substitute those strokes for the actual initialized artwork.

## Explicit image seeds and loader scope

The checked image sets **live `(W0,W1,W2)=(0x4000,0,0)`** immediately before
native `49ef50(0)`; the unchecked image sets **`(0,0,0)`**. Normal open then
initializes its drafts from those words. **Neither image is a fresh-profile
default capture.** The full preference profile loader/copy sequence was not
executed for these images. The earlier description of them as stock profile
defaults was incorrect.

Consequently checked ID8 coexists with unchecked IDs9/10/11 and checked
inverted IDs13/14. A fresh desktop default with difficulty>=2 is instead
`(bbe1dd96,003a7168,0003c3a9)` before OK normalization; use the separate
[loader/default contract](preferences-semantics.md), not these screenshots,
to seed a fresh profile.

## Completed verification and limits

The corrected guest executes the **complete native460720** before59f880,
the source-equivalent final Arial bootstrap constructor, Preferences49bde0,
normal open49ef50 and native software composition6101a0. It uses owned
PCX files and Windows GDI through memory Win32 services, with blank-map
backing. It stops at60708b before the OS input pump. This is an initialized
shared-theme native offscreen target, not a screenshot of a fully executed
application startup. Unrelated tech/model stages of460f70 were not run.

An independent source-PCX recipe matches **all4280 pixels of each of the
three native214×20 skin-only planes**. Eight native captioned cases confirm
the state/caption rules. A separately constructed native parent plus the
captioned normal plane matches **all4280 pixels** of the corrected full-frame
OK crop `(195,487)-(409,507)`.

Compared with the old checked frame, exactly40637 indexed pixels change,
all within the ten visible214×20 button rectangles. The checkbox, label,
frame and console-rail indexed pixels remain unchanged. Toggling ID8 still
changes exactly324 pixels at `(418,187)-(436,205)`.

New checked indexed frame SHA-256:
`67f75b7871842620b9fbe32753a80e854808129b806b08cbce0461f7c5e0e04a`.
New unchecked indexed frame SHA-256:
`ec849310d57532bb6e4610356bfec8ec6e625fbbed89bb6a437ccaa2f2cd3642`.
Original pixels, glyphs, buffers and assembly receipts remain private under
`tmp/reference/independent-preferences-initialized-theme-evidence-20261021/`.

The same missing initializer may limit earlier offscreen targets that inherited
the old harness. This packet establishes Preferences; it does not retroactively
certify unrelated dialog appearances.
