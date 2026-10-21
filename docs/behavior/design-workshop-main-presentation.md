# Workshop main COST, stats, placement and timer integration

2026-10-21. Original terran.exe SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Source-derived caller contracts plus 24 complete native stat-format calls,
23 native human mineral-factor calls, five actual cost-calculation/display
fragments, one declared minimum-cost branch and four stat-layout fragments.
These are text/state checks, not new model/pixel matrices. Public executable
reference: [workshop_main_presentation_reference.py](workshop_main_presentation_reference.py).
Native scalar receipts: [cases](design-workshop-main-presentation-cases.json).
Additive private freeze:
`tmp/reference/independent-workshop-main-presentation-evidence-20261021/`.

## COST reads a cached faction mineral factor

Open `449630@4496fe..449713` refreshes faction state through `577140`, then
calls `4f9d60(owner,1,-1)` and stores its result at Workshop `+14214`.
The arguments mean minerals and no particular Base. The Base inspector's
selected Base must not supply this value. Current faction INDUSTRY still
applies: it is read from `95d418 + owner*20bc`, not definition/personality
fields. See [input authority](design-workshop-input-authority.md).

For a human faction, initial factor is 10 regardless of global difficulty.
Apply mineral rule multiplier `9ba444` (normally 10), then resolved map-size
category `9b9528`: Tiny 0 multiplies by 8/10, Small 1 by 9/10, categories
2 and above do not discount. Truncate each step separately. Then apply
current INDUSTRY, rounding the positive factor upward:

| INDUSTRY | Multiplier |
| --- | --- |
| -7 through -3 | 13/10 |
| -2 | 6/5 |
| -1 | 11/10 |
| 0 | 1 |
| +1 | 9/10 |
| +2 | 4/5 |
| +3 | 7/10 |
| +4 | 3/5 |
| below -7 or at least +5 | 1/2 |

The out-of-range default is the original unsigned jump-table behavior, not
a recommendation to permit those social ratings in normal play. This table
assumes positive ordinary rule multipliers. AI factions take difficulty and
adaptive deductions before rule/map/INDUSTRY adjustments; reuse the existing
[cost-factor contract](nutrient-cost-factors.md), not the human-only reference.

Main painter `43f8c0@440548..4407db` recomputes raw mineral rows through
`5bd1e0(chassis,weapon,armor,abilities,reactor)`. It does not read a stored
design's old cost byte. Display quantity is
`max(rawRows,reactorRating) * cachedFactor`. If rawRows is below the reactor
rating, the caption uses `Cost: (Min Cost: quantity)`; LABELS offset `1e8`
resolves to **Min Cost**, not Reactor. The normal raw markup is
`{Cost:  }quantity`. The minimum branch was isolated with declared rawRows=1,
reactor=2 and factor=8; its native text is `{Cost:  }(Min Cost: 16)`. That
fragment does not establish constructibility of that tuple under stock costs.

The caller appends `!` if all three hold: land chassis with base speed >1,
signed armor strength >1, and weapon **cost byte** `9b6fb3+16*weapon` >1
(`4407db..440826`). This also selects warning colors. It then uppercases
the buffer before drawing. A native Impact/Synthmetal Rover displays 60 and
takes the marker branch. The cost routine already owns the cost calculation;
the marker does not independently multiply the displayed quantity.

A Scout is one raw row: Standard/INDUSTRY 0 gives 10, Standard/INDUSTRY +2
gives 8, and Tiny/INDUSTRY 0 also gives 8. Morgan's name alone cannot resolve
an 8-versus-10 discrepancy. Log actual current INDUSTRY, resolved map category
and cached factor. A control deliberately leaves current INDUSTRY at +2 while
supplying cached factors 10 and 8: the display follows the cache in both cases.
Do not recalculate the cache from social state on every animation paint.

The later prototype-cost warning is a separate line. `44091e..440aa0` starts
with selected ID>=64 and scans already prototyped/available designs for
chassis/domain/speed, weapon-strength and armor-strength coverage. If coverage
removes the prototype requirement, the warning is skipped. The obsolete branch
also precedes it. At `440c62..440cbe`, when admitted, surcharge is
`trunc((5bd1e0(ch,we,ar,0,re)*5d9790(selectedID)+50)/100)*cachedFactor`,
added to the displayed ordinary total. The surcharge cost call explicitly
uses ability mask zero. Reuse [prototype rules](unit-design-and-prototype-costs.md)
for the percentage; do not fold this warning into the primary COST line.

## Complete main stat formatter

`5932d0(dst,designID)` appends to the existing string. The main caller clears
the buffer first and writes draft mask/chassis/weapon/armor/reactor into
scratch design 63 (`43fdd6..43fe18`); saved selected-design fields are not the
formatting source. No synthetic committed design is needed in the port.

Output grammar is `attack-defense-movement`, then `*reactorRating` when the
rating exceeds 1. Rating 2 prints `*2`; it does not print power 20.

Attack uses signed raw weapon strength. Negative prints `?`, >=99 prints `*`,
and both branches skip every attack decoration. Otherwise `592f10` calls
`5d91c0(id,-1,0)`: raw offense, or trunc(3*offense/2) when chassis definition
byte `9b7145+144*ch` is nonzero. Zero remains zero. Append decorations in order:

| Ability bit | Suffix |
| --- | --- |
| 10 | `^` |
| 8 | `~` |
| 100000 | `x` |
| 800 | `e` |
| 20000 | `!` |

Bits here and below are hexadecimal. Surround the attack and these suffixes
with parentheses when offense>0, armor strength>=0, and either triad is sea
or triad is land with ability 8000 (artillery). Air never gets parentheses
from this helper. Ability 20 surrounds the resulting attack with `<` and `>`.
Example combined decoration: `<1~xe!>`. A sea attacker with PSI armor has
`1-?-4`, with no attack parentheses. Noncombat equipment prints `0`, not a
blank or dash; any admitted numeric-attack ability decorators still apply.

Defense uses signed raw armor strength: negative prints `?` and skips all
defense decorators. Otherwise `5d92f0(id,-1,0)` clamps strength to at least 1.
Append `+` for bit 200, then `t` for bit 40000. Bit 100 wraps the decorated
defense in `<...>`. Thus `1-<1+t>-1` is native, while double PSI with those
decorators remains `?-?-movement`. These question marks differ from the
localized PSI label used by component-strength captions.

Movement is `5d9400(id)/movementQuantum(9ba428)` with signed integer division:
start with chassis speed; for air add 2*reactor; subtract 1 for weapon mode 7
(transport); subtract 1 for bit 800000 (Slow); bit 400 (Antigrav) adds
2*reactor for air or 1 otherwise. For air with bit 20, truncate 4*movement/5;
for air transport, then truncate movement/2. Finally clamp to 1..99, multiply
by movement quantum, and divide by that same quantum for display. In the
checked stock rules quantum is 3. A Fusion Needlejet is `1-1-12*2`, with
Air Superiority `<1>-1-9*2`, with Antigrav `1-1-16*2`, and as a transport
`0-1-5*2`. The transport decrement precedes the halving.

## Main placement uses font fields, then actual text centering

Preview rectangle is `(140,9)-(451,320)` in the 623x387 Workshop canvas.
At `43fe1f..43fe79`, effective line height H is:

* if font first DWORD>=0: first DWORD + font field `+10`;
* otherwise: font field `+c`.

`630450@6305ae..6305d6` derives font `+10` as ascent minus internal leading,
and `+c` as measured text height plus external leading. Constructor
`446a2c..446a3e` initializes Arial size16 bold and sets first DWORD=4.
In the independently captured parent: ascent15, internal leading3, so H=16,
even though measured text height is 19. **Do not substitute 19 into H.**

The stat draw rectangle is
`(preview.left, preview.bottom-4*H, preview.right, preview.bottom-3*H)`:
**(140,256)-(451,272)**. Four native caller fragments produce exactly that
rectangle for Scout, double PSI/Fusion, air/SAM and sea transport. It does
not move down for reactor or PSI strings. `5f5f30` centers measured text
horizontally and the font's `+10` vertically within that rectangle. The full
initialized-parent Scout glyph receipt is x275, baseline270, Arial -16 bold,
clipped to the preview rectangle. Parent origin (216,168) makes its full
1024x768 reference-frame baseline **438**, not a second canvas coordinate.

After `5932d0`, `43fea9..43ff76` walks all 24 ability bits in ascending order,
resolves each nonempty short-name token at `9b6c70+28*bit`, joins them with
`, `, and appends a space plus that list. The trailing space exists even for
zero abilities. Example main text: `<1>-1-9*2 SAM`. This caller contains no
stat-line wrapping or second-line relocation; the centered text uses the
preview canvas clip. Transport mode7 adds a separate localized capacity line
below it: `440038..44010a` shifts the rectangle by trunc(3*H/2), switches to
font `+14568` and gets capacity from `592ec0(chassis,mask,reactor)`.

In the same native parent, the primary cost line uses Arial -12 bold at
baseline69: `COST:  ` starts x268 and `10` starts x309. The full glyph journal
and fonts remain in the existing [native-parent evidence](design-workshop-native-parent.md).

## Timer and rotation: existing proof plus initialized-parent registration

Reuse the 24 controls in [animation controller](design-workshop-animation-controller.md).
The independently initialized whole-parent receipt additionally records actual
`timeSetEvent(40,5,630330,7509ac,1)`: 40ms period, 5ms requested resolution,
periodic callback adapter. Its host adapter intentionally delivers no timer
events; it is not an OS timing measurement.

Notification/dispatch coalesces pending events. Admitted callback `4468a0`
repaints the main preview, invalidates its rectangle, and calls shared UI
maintenance; it does not repaint all four component cards or replay missed
intervals. Main 3D yaw uses the old angle then stores binary32(angle+0.05f)
per main paint, without angle wrap. Thus extra paints also rotate it; elapsed
time alone does not. The cached ordinary-unit branch skips that yaw update.
Shared model phase is separate: Copter body rendering advances it only while
Workshop is visible through its parent chain; other model paths select phases
without this increment. A full refresh can render both chassis and main Copter
body and advance twice. Preserve the documented shared-phase/frame rules rather
than introducing a timer-owned animation frame counter.
