# Apply/Done redesign confirmation and intrinsic warnings

2026-10-21, pinned terran.exe SHA256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
One complete Apply call reuses the frozen occupied-custom fixture, captures
its original prompt inputs and supplies return0. One native499b70 wrapper
then measures that exact dialog at the established1024x768 visible-console
profile. No campaign, pixel/model matrix or additional conversion controls.
[Compact receipts](design-workshop-redesign-confirmation-cases.json).

Checks pass: one native Apply caller, one actual wrapper layout,14 source
literals and four reused conditional cases from the frozen occupied-bank
bundle. New private freeze contains13 payloads at
`tmp/reference/independent-workshop-redesign-confirmation-evidence-20261021/`;
manifest `d7aa5ec704213941ea7756e0d361f53d5ac26fcf0db51bc93bd79a22675be550`.
This provenance paragraph was added after freezing. Prior freezes are unchanged.

## One replacement confirmation: OVERWRITEPROTO

Apply action1 and Done action4 both call441e20. It first reuses an admitted
exact tuple match, or searches for an available custom slot as specified in
the [occupied-bank contract](design-workshop-custom-edit.md). The prompt is
not an unconditional consequence of changing a populated design.

After that search, the OVERWRITEPROTO branch requires all of:

- The final replacement is an active custom slot with a changed component
  tuple (chassis/weapon/armor/ability mask/reactor).
- No inactive or fully unused reusable slot has avoided the live replacement.
- Its owner's **cached unit-count byte is nonzero**.

The final cached count is at `95d9fc + owner*20bc + selectedDesignId`.
Production references use the separate `95dbfc` row. They affect reusable
slot choice, but **positive production references alone do not cause this
confirmation**. The count is not recomputed by this prompt. Exact-match
reuse, a new inactive slot and a changed zero-unit slot can avoid it.

`441e20@4423be..442414` supplies:

| Argument/substitution | Exact source |
| --- | --- |
| SCRIPT section | OVERWRITEPROTO, literal6a6f4c |
| PROTONAME0 | Stored name of the final replacement slot, via633910 index0 |
| NUM0 | Its owner's unsigned cached unit-count byte, via633860 index0 |
| Flags | 0 |
| Illustration | unit_sm.pcx, literal6a6f7c, always in this caller |
| Modal-loop callback | 54e8e0 |

PROTONAME0 is not the edited draft name or a full stats/ability description.
The caller does not choose sea/air illustrations for this warning.
The section has xs440, caption Operations Director, and warns that there
are no empty slots and that all units of the named old type will be disbanded.
Its stock text uses `($NUM0 units)` literally. There is no OVERWRITEPROTO2
or count1 singular branch here, and no production-count substitution.

The bounded ordinary receipt selects owner1/design64, stored name
`Existing Guard`, cached units1, cached production references4. Native Apply
requests exactly this section/image/flags and substitutes `Existing Guard`,1.
Supplied decline0 leaves all Design/Faction/Base/Unit/history arrays unchanged.
The already-frozen41-case bundle covers the accepted retirement/rebuild path
and unit-free/production-only alternatives; those are not rerun here.

## Choices and acceptance

| Value | Stock choice |
| --- | --- |
| 0 | Never mind. |
| 1 | Proceed with redesign. |

Row0 initially has selected state2; row1 has normal state0. The sole button
is localized OK, action-1. No separate Cancel button or Yes/No buttons.
Reuse the [generic modal input contract](design-workshop-modal-input.md):

- Single click selects, staying open. Double-click accepts the selected row.
- Enter13, Shift+Enter0x1000d and OK close with the selected row value.
- **Escape27 also closes with the selected row value**, because flags0
  does not convert command-2 to-1. Escape on default row0 declines; Escape
  after selecting row1 accepts the redesign.

`44241c..44242c` aborts commit only on returned0; nonzero continues.
Confirmed replacement calls595ac0(selectedId,owner) before5bd4f0 rebuilds it
(or the existing network path2415). Reuse the frozen retirement contract
for queue/unit effects; do not replace this with the economic Upgrade conversion.

After a declined confirmation, **Done leaves the edited draft open**.
**Apply always reselects/refreshes the chosen stored record**, restoring old
components and rebuilding the browse list even when441e20 returned0.
On success, Done closes; Apply remains open on the committed design.
This distinction is in4426d6..442724 and the existing Done/Apply receipts.

## Exact ordinary dialog geometry

Measured name/count: Existing Guard /1. Other names/localization/fonts can
change wrapping. Client755x300, screen `[150,255,905,555)`.
The illustration is200x120 at `[28,32,228,152)`, reserving206px horizontally.
Body begins x239, native wrap width449,20px line step:

```
50: We have no empty design slots. If we create this design, all
70: units of our old "Existing Guard" type (1 units) will be
90: disbanded.
```

The journal retains a trailing space on the first two spans. Body and
caption use palette226, RGB92,124,188. Body is Arial Narrow -16 bold/font129;
caption is Arial -14 bold/font149, `OPERATIONS DIRECTOR`, x278/baseline21.

All following rectangles are dialog-local, half-open:

| Element | Rectangle | Style |
| --- | --- | --- |
| Main frame | `[23,0,700,305)` | 9 /7b01c0 |
| Caption outer | `[27,4,696,28)` | 3 /7b00d0 |
| Caption inner | `[30,7,693,25)` | 6 /7b0148 |
| Body inner | `[236,35,693,98)` | 0 /7b0058 |
| Body outer | `[233,32,696,101)` | 3 /7b00d0 |
| Choice-list Window | `[33,164,688,208)` | Child655x44 |
| OK Window | `[33,217,688,237)` | Child655x20 |

There is no combined body/list frame beside the illustration. The image's
120px height controls the list top for this three-line body. Choice rows
are child-local `[0,1,655,21)` and `[0,23,655,43)`, styles29 selected /27 normal;
glyphs x3, baselines17/39, font129, palette0. Final OK glyph x317/baseline16,
font129, palette0. Ignore the preliminary font100 OK paint.

Reuse the [initialized OK sprite contract](design-workshop-ok-button-paint.md),
including its source pieces and states. This geometry probe deliberately
does not export a new button DIB: the old harness fallback border must not
be reintroduced. Reuse the existing illustration asset and font references.

## Source-only conditional messages

These are separate informational SCRIPT sections, not alternative choices
inside OVERWRITEPROTO. All have xs440 and Operations Director caption:

| Section | Trigger / caller | Substitutions and result |
| --- | --- | --- |
| INTRINSICTYPE | Changed intrinsic type cannot obtain a custom slot;44214a..442191 | PROTONAME0 is the intrinsic stored name; unit_sm.pcx, flags0, callback54e8e0;441e20 returns0 |
| INTRINSICNAME | Exact-match built-in with nonempty draft name differing from intrinsic name;442019..4420b4 | PROTONAME0 is the matched intrinsic stored name; unit_sm.pcx, flags0; message return ignored, intrinsic selection succeeds |
| INTRINSICNAME | No-change intrinsic branch with attempted differing name;44219e..442226 | Same stored-name substitution/image, through5d63f0;441e20 returns0 |

INTRINSICTYPE explains that intrinsic types cannot be replaced and no empty
slot is available, suggesting redesign of a non-intrinsic type.
INTRINSICNAME states that intrinsic types cannot be renamed. Their SCRIPT
sections have no choice rows. These variants are source-backed only here;
no additional variant geometry, pixel gate or native modal run is claimed.
Unchanged/no-warning intrinsic selections and silent zero-unit replacements
remain governed by the existing commit contract.
