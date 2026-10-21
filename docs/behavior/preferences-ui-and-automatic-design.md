# RE-SMAC-PREFERENCES-UI-001: shared Preferences and automatic unit design

2026-10-21. Pinned original `terran.exe` SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
[Scalar cases and semantic control inventory](preferences-ui-and-automatic-design-cases.json).
This extends the existing [format12, binary encoding, defaults and Workshop
gate trace](design-workshop-preference-and-picker-layout.md); that contract
remains authoritative for importing/loading the original preference file.

## Port behavior to connect

**Game → Preferences… / Ctrl+P** opens the shared Preferences window on page0.
Its second checkbox is **“Design units automatically.”** Native control ID8
maps directly to **first live preference word `959f98`, mask `0x4000`**:
checked enables automatic suggestions. The Workshop consumes this live bit
at `443e50@443e59..443e61`. Keep the setting in the common preferences UI and
store; the original does not add a Workshop-only checkbox.

Edits to this checkbox stay local until **OK or Enter**. Changing pages retains
the edit without applying it. **Cancel or Escape** discards it. Acceptance
updates the live engine state and the local INI; reopening initializes from
the accepted live state. Restore Defaults checks this option in the draft;
Cancel can still discard that reset. These transitions have native receipts,
including disabled→enabled and enabled→disabled acceptance.

## Routes and page inventory

`528d50` calls `49ef50(0)` on the singleton `862180`. The other menu wrappers
enter the same object with different page indices. Installed `menu.txt`
supplies these routes; the shared native constructor is `49bde0`.

| Page | Menu route / shortcut | Native group offset | Visible checkbox IDs, in order |
| --- | --- | --- | --- |
| 0 | Preferences / Ctrl+P | `+5974` | 7, **8**, 9–14 |
| 1 | Advanced Preferences / Ctrl+O | `+649c` | 15–19, 28, 20, 21, 23–26 |
| 2 | Warning Preferences / Ctrl+W | `+8614` | 43–52, 54–56 |
| 3 | Automation Preferences / Ctrl+A | `+6fc4` | 29–34, 42, 35–41 |
| 4 | Audio/Visual Preferences / Ctrl+I | `+7aec` | 64, 65, 66, 71, 67, 68, 69, 22 |
| 5 | Map Display Preferences / Ctrl+M | `+9c64` | 72, 73, 78, 74–77, 79 |
| 6 | More on Warning Preferences | `+913c` | 57–60 |

There are **67 created checkbox rows**. The JSON lists every row's semantic
role, control ID, ordinal and constructor call return. Full original labels
remain in the private receipt. Page indices and IDs are different namespaces:
left button ID6 opens Gamma Correction; page6 is the warnings continuation.

The visible main-page sequence is tutorial messages, automatic unit design,
obsolete-design pruning, edge scrolling, turn autosave, end-turn pause,
fast enemy/neutral movement, fast allied movement. Their source bindings are:

| ID | Word | Mask | Checked meaning |
| --- | --- | --- | --- |
| 7 | 0 | `0x20` | bit set |
| 8 | 0 | `0x4000` | bit set |
| 9 | 1 | `0x100000` | bit set |
| 10 | 0 | `0x1000` | bit set |
| 11 | 0 | `0x2` | bit set |
| 12 | 0 | `0x1` | bit set |
| 13 | 0 | `0x4` | **bit clear** |
| 14 | 0 | `0x8000` | **bit clear** |

Use actual constructor inventory rather than displaying every SCRIPT row:
the Audio/Visual constructor skips the template's first three checkbox rows,
and the warning constructor creates no ID53. The audio page instead has four
level sliders and four playback controls. This trace does not establish the
meaning of every unused/reserved preference bit.

## Shared geometry and visual bindings

Coordinates below are native constructor arguments, relative to the window
unless identified as screen coordinates. Rectangles use right/bottom bounds;
the JSON's `xywh` entries use width/height.

The root is **778×390**, from width constant700 plus78, height390.
For screen width other than800, screen x is `trunc(W/2)-350` and y is `H-618`.
The 1024×768 receipt gives **(162,150)**. At width800 the special branch gives
**(50,0)**, retains 778×390, sets vertical adjustment50, and a separate audio
checklist reduction80. Do not infer uniform scaling from the 800-width case.
The SCRIPT `#xs 440` declaration is not the root window size here.

| Control | x | y at1024 | Width | Height |
| --- | --- | --- | --- | --- |
| Main page buttons, IDs0–5 | 33 | 10 +22×page | 214 | 20 |
| Gamma Correction, ID6 | 33 | 164 | 214 | 20 |
| Restore Defaults, ID90 | 33 | 314 | 214 | 20 |
| OK, ID92 | 33 | 337 | 214 | 20 |
| Cancel, ID91 | 33 | 360 | 214 | 20 |
| Warnings More/Back, ID94 | 256 | 360 | 214 | 20 |

At800, Restore Defaults/OK/Cancel move up50; page/Gamma/More buttons retain
their coordinates. Page0/1/3/5 checkbox groups use `(256,10,457,376)`.
The warning group height is321; its continuation height281. Audio/Visual
height is281 at1024 and201 at800. These are group extents, **not row heights**.

Audio playback buttons IDs85/87/86/88 are Main/Voice/SFX/Music at x259,
y290/312/334/356, size120×20; at800 each moves up50. Slider IDs80–83 are
Main/Voice/SFX/Music, x382, y288/310/332/354, length328; at800 each moves
up50. `61ed90` receives x/y/length/parent/ID; `61ee70(0,127)` sets each range.
Button captions change with playback state; constructor “Off” labels are
not a static statement that the corresponding engine setting is disabled.

`49ef50` paints two nested **style9** frames at `(23,0)-(723,390)` and
`(27,4)-(719,386)`, then **style18** at `(30,7)-(250,383)` for the left rail
and `(253,7)-(716,383)` for the page area. Style pointers are `7b01c0` and
`7b0328`. Audio page painting adds another style9 region. Reuse the existing
[small-frame pixel rules](small-dialog-frame-pixels.md), including center
pattern phase; these are the shared original frame bank, not CSS borders.
The checkbox groups explicitly select sprites `7c68d4` / `7c6900`.

Font initialization follows the existing [shared font-bank contract](generic-dialog-fonts-and-frame.md).
The Preferences constructor reserves bank requests10/plain,12/plain,
12/italic,12/bold,14/plain,14/italic,14/bold at `+1b3b4..+1b3cc`.
Above12, plain/bold bank requests both resolve to bold Arial Narrow; 12 and
below use Arial. **These reserved requests alone do not identify each visible
text role.** Checkbox font tuples inherit through `623430` from
`9c13a8/ac/b0`, populated by `59f240→6241c0` from `94f434/420/424`.
The stock MainFontSize16 selection is16 at1024 and14 at800. Do not hardcode
the reserved10/plain request as the checkbox font. Exact glyph masks,
per-row hit rectangles and all button-state font bindings are not newly
admitted by this packet.

## Draft, tab, accept and close mechanics

`49ef50@49ef6a..49eff6` clears aggregate result words `+a20/a24/a28`, copies
the live words into per-page draft fields, then `4a0040` initializes native
checkbox groups through `628ef0(id,state)`. In particular, ordinal1 on page0
uses ID8 and mask4000 directly. `628d80` toggles the control's local state;
neither operation writes the live preference word.

Page selector `643c30` sends old/new IDs through `49f4b0`, which updates
`+a18` (old page) and `+a14` (new page). The callback and painter hide the old
group and show the new one (`49f4f0`, `49f680`, `4a1130`). They do not collect,
persist or reload checkbox values. Native page0→3→1→0 retained the unaccepted
ID8 edit and produced zero profile writes. `49f230` maps **Up38** to previous
and **Down40** to next, wrapping0↔5; Left/Right are no-ops in this handler.
Page6 is reached by More/Back, not included in that six-page wrap.

The action and key switches have distinct accept/discard entry points:

| Input | Target | Result |
| --- | --- | --- |
| Enter13 | `49f24f` | set `+a5c=1`, then `49f330` |
| Escape27 | **`49f259`** | call `49f330` without setting acceptance flag |
| OK92 | `4a09ca` | set `+a5c=1`, then close |
| Cancel91 | **`4a09d0`** | close without setting acceptance flag |
| Restore Defaults90 | `4a09dc` | reset draft controls, remain open |
| More/Back94 | `4a0991` | switch2↔6, remain open |

The mid-block Escape/Cancel targets matter: both share the close routine
with OK but bypass its acceptance store. `+a5c` is an **acceptance flag**,
not a dirty flag set by checkbox edits.

When accepted, `49f330` calls `49f8b0`: it clears per-page draft words and
rebuilds mapped bits from every group's controls, then ORs page contributions
into the three aggregate words. It writes cached words `95a48c/490/494`,
calls `5b67b0` to copy them live, and `5b63a0` to persist the profile. It also
writes the four audio level keys via `5b62c0`, then clears the acceptance flag.
This is a reconstruction of represented bits; **unknown bits are not shown
to be preserved simply because the opening draft copied the whole words**.

When not accepted it skips that whole block. Both paths reach cleanup
`49f443`: restore console attachment, hide the window, return map focus,
end the modal loop. The supplied probes stop at this cleanup boundary;
they do not claim a complete foreground modal run. The constructor creates
the app's custom panel and the close controls listed above; it creates no
separate Preferences X button. OS title-bar close, Alt+F4 and application
shutdown were **not exercised**. Do not label an unverified OS-close route
as acceptance. A port-added dismiss/X should explicitly choose the discard
behavior rather than accidentally reuse OK.

## Persistence and immediate-control exceptions

The native writer receipts for the isolated automatic-design word are:

* disabled: `Preferences=00000000`;
* enabled: `Preferences=100000000000000`;
* both: `Prefs Format=12`, section `Alpha Centauri`.

These show the writer's minimum-eight-digit, otherwise significant-bit
representation; a fixed32-character string is not required. Reopening the
dialog reads live words, not the INI again. The established loader/copy path
handles a new launch; reuse its format12 and default rules rather than
inferring that a missing key means disabled. The writer also saves other
profile settings; it is not a one-key write routine.

Restore Defaults calls `5b5610/5b56b0/5b5750` for draft words and
`4a0040` for controls. Native reset checked ID8 while leaving a disabled live
word unchanged; Cancel and reopen restored disabled. The reset also previews
audio levels127/127/95/63 (Main/Voice/SFX/Music).

The full window is **not wholly transactional**: slider callback `4a0860`
passes levels straight to audio services, playback callbacks `4a0f30`
immediately start/stop channels, and Gamma action6 calls `5e08a0` and
`5b62c0("Gamma",...)` directly. The music preview path also temporarily ORs
live word0 bit800 before invoking its player. The ordinary Cancel path has
no general audio/Gamma rollback block. Preserve the checkbox staging rule
without promising rollback for those separate preview controls.

## Evidence and validation scope

Private packet: `tmp/reference/independent-preferences-ui-evidence-20261021/`.
It contains the pinned source bindings, exact original constructor labels,
probe, receipt, dependencies, document snapshots and a SHA-256 manifest.

**17 focused native state observations** cover opening, local toggle,
Escape/reopen, Enter save/reopen, Cancel/reopen, OK save/reopen, three page
switches, Up/Down wrapping, Restore Defaults and cancellation of that reset.
Both1024 and800 constructor argument inventories are captured. Original
checkbox allocation, toggle, lookup, initialization, collection, key
dispatch, page selection and profile writer code execute in an offscreen
guest. Win32 profile reads/writes use an in-memory service; the installed INI
is neither read nor modified. Widget window creation and painting are
explicit boundaries; button identity/parent fields are supplied at that
boundary. OK/Cancel and reset use admitted callback fragments, excluding
temporary dialog construction. No complete Preferences screenshot, actual
OS-close run, new-launch process run or glyph-perfect UI claim is made.

No production source, GSE script or Git state was changed for this handoff.
