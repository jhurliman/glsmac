# Workshop DATA rows, status state and color operands

2026-10-21. Pinned terran.exe SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
This completes the DATA/status portion of the
[main presentation contract](design-workshop-main-presentation.md).
The [executable reference](workshop_data_status_reference.py) and
[compact cases](design-workshop-data-and-status-cases.json) accompany it.
Scope: bounded native formatting/status fragments, static caller traces,
independent theme-sample decoding, and reuse of the three retained native
Morgan-material parent glyph journals. No new live desktop or model matrix.
Validation passed 11 native DATA fragments, nine native status transitions,
six decoded theme pairs and three retained frame journals. Additive private
freeze `tmp/reference/independent-workshop-data-status-evidence-20261021/`
contains 15 payloads; manifest SHA-256
`92d5a04d299620a805bf710345bc6fdc6e50079dbf8e095c6e3888958dc47d07`.
This provenance paragraph was added after the immutable freeze.

## DATA reads the current draft

Main painter `43f8c0@441087..441b88` reads draft chassis, weapon, armor,
reactor and mask at actor `+141fc,+14200,+14204,+1420c,+14208`.
SP1 and SP2 read explicit ordered slots `+14218,+1421c` respectively.
The mask must not be sorted or compacted to reconstruct those two slots.
The current draft is also copied into scratch design 63 before this code;
movement resolves that scratch record, not the stored selected design.

| Row | Exact stock raw markup / source value |
| --- | --- |
| W / E | Mode byte `9b6fb2+16*weapon` <3: `{W:} value`; otherwise `{E:} cost` |
| W value | Signed strength byte `9b6fb0+16*weapon`: negative = `Psi`, >=99 = `*`, otherwise decimal including zero |
| E value | Unsigned raw equipment cost byte `9b6fb3+16*weapon`; neither offense nor total mineral cost |
| A | Armor class byte `9b6f11+16*armor` >=3 omits the entire row; otherwise `{A:} value` using signed strength `9b6f10+16*armor`, negative = `Psi`, otherwise decimal |
| C | `{C:} movement`, from `5d9400(63)/[9ba428]` |
| R | `{R:} reactorRating`, e.g. `2`, not `20` |
| SP1 / SP2 | Negative slot = `{SP1:} -` / `{SP2:} -`; nonnegative follows the truncation below |

No reactor/ability strength multiplier is applied to W or A. In particular,
A zero remains zero; do not reuse the main stat formatter's minimum-one rule.
All negative signed strength bytes select Psi, not only `ff`.
An omitted A row still consumes its vertical step.

Case is preserved for all six DATA rows: stock `Psi` stays `Psi`, and SP
suffix bytes retain the full name's original case. Only the DATA heading
receives the uppercase call, at `441004`; rows call `5f5ca0` directly at
`4412fc,441471,441637,441775,44197d,441b83` without that conversion.
`5f5ca0` bounds text length and forwards to `5f58e0`; it does not uppercase.
The native Psi fragment receipt contains `{W:} Psi` and `{A:} Psi`.

`441548..441637` briefly appends a triad label and then writes NUL at absolute
buffer offset four. For the stock `{C:}` prefix this removes that triad text.
What survives is `{C:} ` followed by movement, not a triad/chassis name.

Movement `5d9400` starts from unsigned chassis speed `9b7140+144*chassis`.
Apply these steps in order:

1. Air triad 2 adds twice the reactor rating.
2. Weapon mode 7 subtracts one; ability `800000` subtracts one.
3. Ability `400` adds twice reactor for air, otherwise one.
4. Air with ability `20`: truncate `value*4/5` toward zero.
5. Air with mode 7: truncate `value/2` toward zero.
6. Clamp to 1..99. Native returns this times movement quantum; DATA divides
   by that same quantum.

SP rendering deliberately preserves an original truncation quirk
(`441888..441953`, mirrored `441a90..441b59`). Resolve the ability's **full
name** from token `9b6c68+28*slot`, append it to `{SP1:} `, then NUL byte six.
Find the first literal space in the full name. If it has a nonempty suffix,
append that suffix, then NUL byte seven. With the stock six-byte brace prefix,
only the first byte after the first space survives, without a value separator:
`Super Former` -> `{SP1:}F`, `Deep Radar` -> `{SP1:}R`.
No space in the name, or a trailing first space, leaves the brace label alone.
Consecutive spaces can produce a single space. These are absolute byte cuts;
localized longer labels do not receive a smart abbreviation algorithm.
None remains the spaced hyphen. Swapping slots swaps the displayed fragments;
an empty first slot does not pull the second slot upward.

## Placement and measured baselines

For the retained 1024x768 profile, parent-local main clip is
`[140,9,451,320)`. DATA text starts x160. Font is Arial -12, weight700.
Its native effective height is 14: object first5 plus ascent-minus-leading9.
Heading Y is midpoint(main.top,main.bottom) minus four effective heights,
108 here; the GDI baseline is nine pixels below it.

| Text | Baseline | Label x | Value span x in retained stock frames |
| --- | --- | --- | --- |
| DATA | 117 | 160 | — |
| W: / E: | 131 | 160 | 175 / 170 |
| A: | 145 | 160 | 171 |
| C: | 159 | 160 | 171 |
| R: | 173 | 160 | 171 |
| SP1: | 187 | 160 | 186 for ` -` |
| SP2: | 201 | 160 | 186 for ` -` |

Value span x is the measured label width, not a separately fixed column.
Preserve the leading space in normal values and None. The retained Scout
shows W/A/C/R =1; Colony shows E10, A/C/R=1; Unity Rover shows W/A/R=1, C2.
These are actual draft values. DATA is not a placeholder ID display.

## Console status is also a selection transition

Port integration: the live parent now renders DATA from current draft values,
using the explicit ordered slots and the movement helper shared with the main
stat formatter. `GLSMAC_data/tests/workshop_data.gls.js` checks the eleven native
fragments. `workshop_parent_pixel_gate.py --region data` compares the stock
heading/six-row rectangle `[376,275,460,373)` without rescaling or tolerance.
The live Tiny Morgan Colony capture matches all 8,232 pixels; Scout differs
by one foreground pixel in W at `(383,292)`. This is a remaining glyph-mask
difference, not whole-screen parity. The source reference parents use Standard
map size, so COST 30 versus the live Tiny COST 24 is expected. The separate
opaque shell gate matches all 3,552 selected pixels. Models, other text,
controls and the console are outside those two pixel gates.

The status transition and console layout below are now connected. The backend
projects the provisional selection without changing the saved registry, count
caches or supplied draft. The UI adopts that selection for subsequent edits
and Apply/Done, updates the strip highlight without recentering, and renders
the status in an independent original-size console above the modal dimmer.
The console clears unrelated Base-screen content, uses the native198/65
scanline backing and restores the blank left panel at `[7,59,242,189)` with
style3 then style0 inset3 (native callers448908/448949). Presets and the
remaining main actions are still pending.

The `status` pixel gate covers the complete `[266,581,759,679)` screen panel,
including border gaps and both text rows: **48,314/48,314 pixels match** the
retained native Scout reference at1024x768. No shift, resize, tolerance or
foreground-only mask is applied. Centering explicitly truncates integer
coordinates; fractional UI centering otherwise moves both native glyph runs
one pixel right. The live Colony edit changes the message to NEW DESIGN
BASED ON OLD; Apply restores EXAMINING BASIC UNIT and again matches the same
complete panel. Done removes the console and the process exits cleanly.
This gate does not cover the remaining console/preset controls or prove a
whole Workshop match. Multi-line layout has source/metric and UI-test
coverage, not a new native multi-line image.

`43d730` compares only chassis/weapon/armor/reactor/mask against the selected
record. Draft name, ordered ability slots, named flag and cost do not enter
that equality test.

| State | LABELS offset | Exact stock message |
| --- | --- | --- |
| Selected ID <64, tuple unchanged | e44 | EXAMINING BASIC UNIT |
| Selected ID <64, tuple changed | e54 | NEW DESIGN BASED ON OLD |
| Active custom, not excluded for owner, tuple unchanged | e50 | EXAMINING PREVIOUS DESIGN |
| Selected custom inactive or excluded | c34 + e48 | DESIGNING NEW UNIT (SLOT #n) |
| Changed active custom, replacement candidate accepted | e54 | NEW DESIGN BASED ON OLD |
| Changed active custom, no candidate accepted | c38 | MODIFYING PREVIOUS DESIGN |

Slot number is selectedID modulo64 plus one, rendered uppercase with literal
` (` + localized `Slot` + ` #` + decimal + `)`. The original signed remainder
also exists for invalid negative IDs; ordinary custom IDs are nonnegative.

For changed active custom designs, `43dae0..43dbf8` scans this owner's 64-slot
bank ascending. First inactive record immediately becomes selected and stops
the scan. Otherwise only owner-excluded records with cached own unit count
zero qualify. Zero cached production count qualifies regardless of role and
can replace an earlier accepted candidate. Nonzero cached production count
qualifies only before any candidate has been accepted and when role matches
the **currently selected** record. Every accepted candidate sets the same
accepted flag. Thus a queued excluded record can produce e54; it is not c38.
The last eligible fully unused record wins unless a later inactive slot stops
the scan. This changes actor selectedID, not the design pool or draft tuple.
Use the own count caches at `95d9fc+owner*20bc+ID` and
`95dbfc+owner*20bc+ID`. Do not replace this status scan with the Apply/Done
[commit/match scan](design-workshop-custom-edit.md).

Status uses console bitmap `7cbc04`, region actor `+a4c`, measured
`[266,70,759,168)`. It draws style3 outside, inset3 style0, then another
three-pixel text inset. Both title and body use Arial -18 bold, primary color
operands `(224,-1,1,1)` at `43d824..43d83a`.
Title `e4c` = DESIGN WORKSHOP is centered, measured x421 baseline94.
Body uses `5f5160` wrapped height to bottom-align its rectangle, then
`5f68b0` centers/wraps it. Retained EXAMINING BASIC UNIT is x407 baseline154.
Do not fix every status string's x coordinate to407 or ignore wrapping.
It invalidates the console region and restores its full clip afterward.

For multi-line status bodies, the effective text width is **481**: outer
493 minus the two three-pixel insets on each side, text x272..753.
`630450` leaves the local font's first word -1 and stores TEXTMETRIC
height plus external leading at font+0c (`6305ae..6305c0`). The retained
Arial -18 bold metrics are height21, external leading1, ascent17,
internal leading3, so effective wrap line height is **22**, not21.
Both height measurement `5f5160@5f51c6..5f51e8` and actual wrapped drawing
`5f68b0@5f6969..5f6989` use that same height. With N measured lines,
the first text origin is innerBottom162 minus22*N; glyph baseline adds14.
Thus two body baselines are132/154; three are110/132/154. Each line is centered
independently. This is a source/retained-metric derivation, not a new captured
multi-line frame. Status role is record byte43 (`9b0213`); do not confuse it
with the explicit Upgrade target scan's separate plan byte42.

## Exact palette operands for COST and DATA

`4d01f0` loads text.pcx with index offset10, reads paired samples and converts
second sample index9 into -1. Independent stock asset decoding gives:

| Purpose | Primary pair | Brace/secondary pair | Stock values |
| --- | --- | --- | --- |
| Ordinary COST | 8d0a0c/10 | 8d0a04/08 | both `(224,-1)` |
| COST fast-land `!` branch | 8d0ac4/c8 | same pair | both `(249,-1)` |
| DATA heading | 8d0a24/28 | literal `(222,-1)` | primary `(224,-1)` |
| DATA rows | 8d0a34/38 | 8d0a2c/30 | both `(224,-1)` |

All calls add operands `(1,1)` after the pair: primary `5f3ab0`,
secondary `5f3ae0`. These are palette operands, not RGB hex colors.
The `!` branch changes **both** text colors for the whole COST line
(`440850..440882`), not only the exclamation glyph. The Min Cost branch only
changes the caption/value calculation; it has no independent color change.
Without the `!` condition it retains ordinary colors.
Theme sample coordinates: ordinary pairs x371/381, y52/68;
DATA heading y116, row secondary y132, row primary y148;
warning pair x205/215,y260. Native glyph journals retain the resolved color
`10ff00e0` for the ordinary DATA/COST/status spans in the measured profile.
