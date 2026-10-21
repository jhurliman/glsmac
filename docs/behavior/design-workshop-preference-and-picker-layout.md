# Workshop automatic-ability preference and generic picker geometry

2026-10-21. Source-only additive caller clarification for original terran.exe
SHA-256 `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
No new component matrices or modal interaction runs. Reuses existing
[components](design-workshop-components.md), [backing](design-workshop-picker-backing.md)
and [15 accepted-picker transitions](design-workshop-interactive-editing.md).
Original source packet is included in the additive private freeze
`tmp/reference/independent-workshop-main-presentation-evidence-20261021/`.

## Automatic suggestions have an original default and persisted preference

The gate is `443e50@443e59..443e61`: test `959f98 & 4000` (hex). Zero skips
the automatic suggestion path, after the ordinary cleanup call `443d20`.
This is user preference state, not a technology, difficulty or faction rule.

The authoritative loader is `5b5830`. Its profile API calls use:

| Input | Original source |
| --- | --- |
| INI filename | pointer `6af310` initially points to `.\Alpha Centauri.Ini` at `6af328` |
| Section | pointer `6af314` points to `Alpha Centauri` at `6af318` |
| Format version key | `Prefs Format`, literal `6af340` |
| First preference word key | `Preferences`, literal `6af3cc` |
| Loaded first word | `95a48c` |
| Live first word | `959f98` |

`5b5ac3..5b5b6d` builds a default bit string, most-significant bit first,
with at least eight characters. With Laptop=0 its base word is `bbe1dd96`;
with Laptop nonzero it is `a3e1dd16` (32-bit addition of `e7ffff80`). If the
separate mode value `95a378` is below 2, also OR bit20. **Every one of these
default variants has bit4000 set**, so automatic suggestions default enabled.
The Laptop setting is obtained through `5b5770("Laptop",0,0)`.

`5b5b6f..5b5c3f` chooses the actual string. A nonzero loader argument uses
the constructed default directly. Otherwise it checks `Prefs Format`:
only decimal **12** admits a read of `Preferences`, using the constructed
bit string as the missing-key default. A missing/other format uses the
constructed default regardless of a stale Preferences key. `6168c0` obtains
the token and `642690` parses consecutive ASCII `0`/`1` characters by
`value=2*value+digit`, stopping at the first other character. It stores the
result in `95a48c`; there is no decimal/hex conversion for Preferences.
In a full 32-character string, bit14 is character17, counting from zero.

`5b67b0` copies cached preference words `95a48c/490/494` to live
`959f98/9c/a0`. The setup/load caller `5a4c50@5a4caf..5a4cb8` calls
`5b5830(0)` then this copy. `53f6e0@53f7e0..53f7f2` also loads the profile
and copies the first word. Preferences-dialog acceptance `49f330` collects
its words into `95a48c/490/494` and calls `5b67b0` at `49f3e0`.

Writer `5b63a0@5b63ae..5b6408` saves `Prefs Format=12`. At
`5b659a..5b6614` it serializes `95a48c` as the same high-bit-first binary
string and writes `Preferences` in that section/file. Thus an explicit
user-selected disabled state must survive across launches. The shared save
payload also begins at `959f98` (`5be250@5be27d`), but that serialization
does not replace this local INI default/load/write contract.

For the port: a fresh preference store can use the source-backed **enabled**
default; retain a boolean so a disabled choice remains disabled. If importing
an original INI, honor its format/version and binary-string representation.
Do not infer a particular user's current choice from the default. This trace
did not read or change the user's installed INI.

## All four generic pickers use the same grid

Caller kind IDs are chassis0 (`443fe0`), weapon1 (`444400`), armor2
(`444860`), reactor3 (`444b50`). All call `43cfc0`. After kind-specific
enumeration, its **unconditional** `43d29f..43d2b5` writes:

* `+2b6c=5` columns;
* `+2b70=4` rows;
* `+2b64=20` visible page slots.

Reactor therefore uses the armor picker's **5×4 geometry**, even with only
four configured reactor choices. No reactor-specific compact grid follows.

For visible ordinal i and current first ordinal p, j=i-p, col=j%5,
row=trunc(j/5). `43cda0@43cdc3..43ce09` computes:

```text
outer = (27+108*col, 4+94*row)-(132+108*col, 95+94*row)
model clip = outer inset 3
anchor = (trunc((clip.left+clip.right)/2),
          trunc((clip.top+clip.bottom)/2))
```

For the first card: clip `(30,7)-(129,92)`, anchor **(79,49)**. This is the
generic picker's center; main component-card anchors are separately measured
and must not be substituted. All three newly traced model callbacks use this
center for both normal and unavailable rendering.

`43ce58..43ce6f` selects theme small-frame style0 unselected, style2 selected,
then insets the rectangle before dispatch. `43ceaf` maps kind0→`43b8d0`,
kind1→`43bc40`, kind2→`43c100`, kind3→`43c510`. Out-of-range visible slots
attempt the blank-sprite path and retain the background. Reuse the existing
small-frame/backing composition and palette contracts.

## Scales and render calls, without repeating the model matrices

All use the generic scene initialized at `43d128..43d1ac`, including fixed
yaw bits `3f05f92d` (about .52333 radians), not main-preview rotation.
Scales below are binary32 arguments to `6324f0`:

| Kind / callback | Scale bits | Approximate scale / source branch |
| --- | --- | --- |
| Chassis `43b8d0` | `3d75c28f` | .06, unconditional at `43b96b` |
| Weapon `43bc40`: Hand Weapons ID0 | `3e4ccccd` | .20, direct `632060(7af880,0)` |
| Weapon: Planet Buster ID13, conventional payload ID20 | `3d75c28f` | .06, direct payload scenes |
| Other weapon modes below3 | `3e4ccccd` | .20 |
| Other weapon mode9, terraforming | `3da3d70a` | .08 |
| Other weapon mode11, probe | `3e4ccccd` | .20 |
| Other weapon modes | `3dcccccd` | .10 |
| Armor `43c100`, nonzero model | `3dcccccd` | .10, existing verified contract |
| Reactor `43c510` | `3e4ccccd` | .20, unconditional at `43c5bb` |

Chassis composition `4d88e0(owner,ch,0,0,draftReactor,scene)` uses the
selected card chassis and draft reactor. Generic weapon fallback calls
`4d8e40(owner,-1,weapon,scene)`, so it requests an independent weapon part,
not the whole draft unit. Reactor calls
`4d95f0(draftChassis,draftWeapon,cardReactor,draftMask,scene)`; use the
already documented context-sensitive reactor composition. No Armor has no
model. Do not replace these calls with generic whole-unit thumbnails.

Available callbacks draw `631370(canvas,anchorX,anchorY,6)`. Unavailable
callbacks draw `631760(canvas,anchorX,anchorY,185,6)` and return before normal
name/stats: the existing silhouette-table contract applies to every kind.
Weapon modes8/9/11 also have the original sprite-underlay branch
`43bd70..43bdf8`; do not mistake it for a different model anchor or scale.

## Enumeration limits and scrolling

`43d20f..43d29d` counts definitions with signed prerequisite>=-1;
prerequisite<-1 excludes the definition. Unknown-but-configured technology
does not remove a slot: it gets availability rendering and selection guards.

| Kind | Source prerequisite addresses / stride | Maximum definitions |
| --- | --- | --- |
| Chassis | `9b7186` to before `9b7696`, stride90 hex | 9 |
| Weapon | `9b6fb4` to before `9b7104`, stride10 hex | 21 |
| Armor | `9b6f14` to before `9b6fb4`, stride10 hex | 10 |
| Reactor | `9b6c40` to before `9b6c70`, stridec hex | 4 |

Armor alone has the existing chassis-byte guard (`43d24b..43d265`) that
forces count1 for special chassis. Reactor card mapping uses rating1..4,
while ordinals are zero-based; keep the accepted-picker transition contract's
initial-highlight and return mapping, including filtered-definition cases.

Scroll control creation threshold is **count>18** (`43d31a`), not count>20.
Its upper range expression is `trunc(count/5)-4+2`; its `+1454` field is3
(`rows-1`). The picker still paints 20 slots. Initial highlighting starts
first ordinal p=0 and advances p by5 until the initial ordinal is below
`p+20`, updating scroll position to p/5 (`43d38a..43d3d4`). It then paints
`p..p+19` and stores the initial ordinal at `+2b80`. The late additional
scroll integration call `46ac10` is conditional on count>20 (`43d413..430`).
These two thresholds are distinct original branches. With the fixed tables,
only the weapon picker can actually need a second page.

## Addendum: chassis, reactor and armor picker text

2026-10-21, source-only caller clarification. No new component matrix.
These callbacks receive the **inset model clip**, e.g. first card
`C=(30,7,129,92)`. Shared small font `picker+45f4` is initialized by
`43d0e5..43d0f2`: global face `6affac`, size10, normal. Numeric font
`picker+461c` is initialized at `43d0f7..43d107`: the same face, size12,
bold. Existing component receipts identify the face as Arial. These fonts
are independent of the generic ability modal's MainFontSize/Arial Narrow font.

Let H be small font's effective height: first DWORD + field10 if first>=0,
otherwise fieldc. After creation these small fonts retain first=-1. In the
existing Arial profile H=13, with small-font ink height7 and numeric ink
height9. Each callback puts its header at the clip top, and reserves a bottom
row **[C.left,C.bottom-H,C.right,C.bottom)**. The left bottom caption uses
`5f5d40`; the right value uses `5f6250`. Both center vertically in that row,
while aligning horizontally left and right respectively. With those metrics:

* Header first baseline14; subsequent wrapped baseline27, etc.
* Bottom caption baseline89; bold value baseline90.
* Add108*column to X and94*row to Y for later cells. Left caption starts at
  C.left; numeric text ends at C.right, after measuring the actual string.

These are the existing profile's metric-derived positions, not new raster
measurements. Header names are uppercased before the centered wrapping helper
`5f68b0`; chassis uses `5f4a00`, which temporarily selects the passed small
font and calls that same helper. Long names can wrap; bottom text positions
continue to come from the original clip bottom.

| Callback | Header localization token | Bottom left | Bottom right |
| --- | --- | --- | --- |
| Chassis `43b8d0` | `9b70f8+144*chassis` | resolved triad token at `9595c4+4*triad` | unsigned raw speed byte `9b7140+144*chassis` |
| Reactor `43c510` | `9b6c2c+12*rating` (rating1..4) | LABELS offset1e4, **Power**, installed line132 | decimal reactor **rating1..4** |
| Armor `43c100` | `9b6f08+16*armor` | LABELS offset1f8, **Shielding**, line137, only armor class byte<3 | signed raw strength, or LABELS310 **Psi**, line207 for negative strength |

Chassis source is `43ba55..43bc1d`, reactor `43c6b3..43c859`, armor
`43c30e..43c4ef`. All tokens resolve via `62e860`; the per-definition token
addresses are not LABELS-array offsets. The triad labels are the loaded domain
names (Land/Sea/Air), not hard-coded chassis descriptions.

The chassis picker prints raw speed: a Needlejet prints **8** under stock
rules, not the main stat formatter's reactor-adjusted Fission10/Fusion12.
Reactor prints **Power 1**, **Power 2**, **Power 3**, **Power 4** across the
two aligned bottom fields; no colon, `*2` suffix or multiplication by10 is
present. Its header comes from the reactor definition, not the Power label.
Armor prints raw0 as0 rather than main-stat clamping it to1, and negative
strength as localized Psi rather than a question mark. Reuse the existing
[strength-text contract](design-workshop-strength-text.md) for these distinctions.

The small header uses theme text pair `8d0a44/48`; bottom text uses `8d0a4c/50`.
Unavailable callbacks take the silhouette return path before these headers and
bottom labels. No Armor is text-only and still gets its normal label/value.
All of this is caller presentation; it adds no new composition or availability
rules to the already accepted component matrices.
