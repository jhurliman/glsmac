# Upgrade confirmations, affordability notices and completion dialog

2026-10-21. Pinned terran.exe SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Four bounded original504fe0 calls capture exact prompt substitutions from
the retained engine fixtures; five native499b70 wrapper/layout controls cover
UPGRADEPROTO, UPGRADEPROTO2, UPGRADEPROTOCOST, UPGRADEPROTOCOST2 and YESUPGRADE.
Two further wrapper controls correct the earlier image-free Retire layouts.
Original text/font/palette/PCX loading and painting execute; no models,
campaign, real modal loop or new conversion matrix.
[Compact receipts](design-workshop-upgrade-confirmation-layout-cases.json),
[bounded geometry reference](workshop_upgrade_prompt_geometry_reference.py).
All four substitution calls, five Upgrade layouts, two Retire wrapper controls
and five caller-source literals pass independent checks. Additive private
freeze contains40 payloads at
`tmp/reference/independent-workshop-upgrade-prompt-evidence-20261021/`;
manifest `502750b15467ec38d3114b7e303c3571095e55b577101c47b8a1a18490dd9c92`.
This provenance paragraph was added after the immutable freeze.

## Exact substitutions and caller conditions

Original504fe0 produces these strings through stored record names,5935c0
stat formatting and its ordered ability-name suffix logic:

| Parameter | Retained fixture value |
| --- | --- |
| UNITTYPE0 | `Upgrade Source (1-2-1)` |
| UNITTYPE1 | `Upgrade Target 65 (2-3-1)` |
| NUM0 | Total energy quote:100 prototyped /640 unprototyped |
| NUM1 | Available credits:2000 for confirmation /0 for affordability notice |
| NUM2 | Cached own source-unit count:2 |

Numeric substitutions are at `9c39a4+4*index`, strings at
`9c39f4+256*index`. They are not plain unsaved draft names. Prototyped controls
reuse native upgrade_one_accept / upgrade_insufficient fixtures; unprototyped
confirmation reuses upgrade_unprototyped. The unprototyped affordability case
uses that same stored fixture with declared available credits0. Prompt return0
is supplied so no additional engine conversion executes during this capture.

| Condition | Template | Choices / purpose |
| --- | --- | --- |
| Affordable, target prototype flag4 set | UPGRADEPROTO | Ask to pay quoted total |
| Affordable, target prototype flag4 clear | UPGRADEPROTO2 | Ask despite unprototyped target |
| Unaffordable, target flag4 set | UPGRADEPROTOCOST | Informational insufficient-credit notice |
| Unaffordable, target flag4 clear | UPGRADEPROTOCOST2 | Same notice plus prototype warning |
| Successful action with cached own source-unit count0 before upgrade | YESUPGRADE | Informational `Upgrade complete!` |

YESUPGRADE has no string/numeric substitutions; its provenance reuses the
already executed no-unit dispatcher case and `442f7d..442f9c` caller. It does
not ask to retire the old design. Source ability abbreviations, pricing,
skip-confirmation latch and engine conversion remain in the
[existing action contract](design-workshop-explicit-upgrade-and-retire.md).

Stock confirmation templates have exactly two rows, values0 and1:

| Value | Text |
| --- | --- |
| 0 | No, we can't afford that. |
| 1 | Yes, we need to modernize our weaponry. |

Initial row0 is selected (state2), row1 normal (state0); the only button is
localized OK (LABELS byte offset `0x194`, zero-based token101), action -1.
This is `labels[102]` in an array that includes the count at index0;
decimal index194 selects a different label. There is no visible Cancel button or
third Yes-to-All row. SCRIPT's `##Yes to ALL.` is a comment; the engine's
answer>1 latch branch must not cause the stock UI to invent another choice.
The two affordability templates and YESUPGRADE have **no choice rows**, only OK.
Affordability SCRIPT templates also carry wave9; no audio playback is measured.
NUM2 drives the stock singular/plural text branches: count1 selects `our single`
and `unit`; the captured count2 selects `all of our` and `units`.

## Include the wrapper's illustration

Unlike the multiple-target chooser, these dialogs pass an illustration to
499b70. Both economic prompt branches select unit_sm.pcx for land,
navun_sm.pcx for sea and air_sm.pcx for air (`5054eb..505533`,
`5055d2..505619`). YESUPGRADE always passes unit_sm.pcx in its caller.
The retained fixtures are land. Original499b70 successfully loads the
200x120 unit_sm.pcx, sets dialog+2144 to the loaded bitmap and includes its
206px horizontal reservation (image width plus6) and120px height in layout.
The image appears at dialog-local28,32; body text starts at x239.

This same illustration argument exists on Retire. The earlier direct-template
Retire757x300 layouts omitted it. **Use the actual wrapper coordinates below
for Retire**, superseding those image-free diagnostic coordinates. The
multiple-target Upgrade chooser remains image-free and retains its prior
757x300 measurements; it directly loads WHICHUPGRADE2 with flags40.

## Captured1024 profile geometry

Mode0, MainFont16, native visible-console rails, stock assets and the exact
substitution strings above. All dialogs have height300 and bottom screen555.
Longer names, localization or other fonts can change wrapping/layout; these are
measured profile gates, not universal fixed-size rules.

| Template group | Client | Screen Window | OK rectangle, dialog-local |
| --- | --- | --- | --- |
| UPGRADEPROTO / UPGRADEPROTO2 | 755x300 | `[150,255,905,555)` | `[33,217,688,237)` |
| UPGRADEPROTOCOST / UPGRADEPROTOCOST2 / YESUPGRADE | 758x300 | `[149,255,907,555)` | `[33,167,691,187)` |
| Retire wrapper, zero or2 units | 755x300 | `[150,255,905,555)` | `[33,217,688,237)` |

Confirmations and Retire list child: `[33,164,688,208)`,655x44.
Child-local rows: `[0,1,655,21)` and `[0,23,655,43)`,22px stride.
Selected row style7b04e0/index29; normal7b0490/index27.
Retire values remain0 Never mind /1 Retire design, with no Cancel button.
For these short bodies, the120px illustration controls list/button vertical
position: adding a fourth body line does not move them down another20px.
Do not reuse the image-free combined body/list outer frame.

Frame rectangles for a dialog width W and measured body line count L:

| Frame | Rectangle | Style |
| --- | --- | --- |
| Main | `[23,0,W-55,305)` | 7b01c0 /9 |
| Caption outer | `[27,4,W-59,28)` | 7b00d0 /3 |
| Caption inner | `[30,7,W-62,25)` | 7b0148 /6 |
| Text inner, to right of image | `[236,35,W-62,38+20*L)` | 7b0058 /0 |
| Text outer | `[233,32,W-59,41+20*L)` | 7b00d0 /3 |

| Template | Body line baselines | Caption x, baseline21 |
| --- | --- | --- |
| UPGRADEPROTO | 50,70,90 | 278 |
| UPGRADEPROTO2 | 50,70,90,110 | 278 |
| UPGRADEPROTOCOST | 50,70,90 | 280 |
| UPGRADEPROTOCOST2 | 50,70,90,110 | 280 |
| YESUPGRADE | 50 | 280 |
| RETIRETYPE2 | 50 | 302 |
| RETIRETYPE, count2 | 50,70 | 302 |

Caption is OPERATIONS DIRECTOR for Upgrade, CHIEF ENGINEER for Retire.
Font is Arial -14 bold, palette226 (RGB92,124,188). Body is Arial Narrow -16 bold, x239,
20px line step, palette226. The native journal color `0x10ff00e2` has low
byte `0xe2` =226 decimal; the earlier prose conversion to222 was incorrect.
The `{not yet been prototyped,}` span in the two
unprototyped templates uses **palette157**, font record130; its resolved
face/size/weight are still Arial Narrow -16 bold, not an inferred italic font.
Its wrap splits `not yet been ` on baseline50 and `prototyped,` on70;
the exact mixed-color glyph spans and trailing spaces are in the receipts.
The first emphasized span starts x528; the second starts x239, followed by
ordinary text at x319 on that second line.

Choice glyphs use palette0, Arial Narrow -16 bold, child x3 and baselines17/39.
Final OK glyph baseline16, button-local x317 for755-wide dialogs, x318 for758.
Ignore the earlier default-font preliminary button glyph pass when matching
the final raster. Font129/130 metrics are height20, ascent16, internal-leading4,
external-leading0 in this captured environment.

Exact body wrapping and generic key/click acceptance are pinned in the
[modal input clarification](design-workshop-modal-input.md).

## Owned-DIB exports and limitations

Private directory: `tmp/reference/workshop-upgrade-prompts-layout-v2-private-20261021/`.
Each case exports `ID-own-0.png` for the dialog-owned image/caption/body/frame
surface and further `ID-own-N.png` files for separately owned OK/list surfaces.
The receipts give dimensions and hashes for each. The parent bitmap includes
the illustration but **does not contain the child-owned OK and choice pixels**.
They are not full composed screenshots and make no blank-map assumption.

The font stays registered through every paint/export. The probe pauses at
61caf9, exports owned surfaces and then completes original dialog destruction
with a supplied modal return0. Confirmation callback0 and completion/Retire
callback54e8e0 are retained as caller arguments but are not delivered to a real
message loop. This packet adds presentation/substitution evidence; the earlier
33-case engine contract continues to establish economic mutations and cancels.
