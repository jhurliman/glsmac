# Workshop picker backing, borders and Silksteel table

2026-10-20. Additive clarification of the frozen c626 component evidence;
that bundle is unchanged. No desktop, modal loop or parent painter was rerun.

## Silksteel selects the second tint table

The existing `alltech_armor_picker` ordinal 3 control's actual `6867f0`
second table is SHA-256
`459eed0309d2bc0bda3ccb0f8e27b3e183b1266f1e6b35e71be651633bec51c3`.
All 6,144 bytes equal **zero-based table 1**, cache offset `0x1800`, in
`armorgrads.tmp` (whole-cache SHA-256
`8e23ce5cbea562bdc3d6b190b8221a6c1777b968b5b93937322cb07af58a9126`).
The original assembly selects `7a9298` at `4d9170`, then passes it to
`631be0` at `4d933c`. This confirms the existing table choice, not a new
material source. Table 1 uses SYSTEM.TXT relative tint 233, loaded index 243.
The cache is shade-major: `table[shadeRow*256 + voxelColor]` after the first
lighting lookup. Its source shades come from VBP.CVR, not vipta00.cvr.

### Tint palette is the initialized Gamma100 palette

The c626 palette `c7129c7433e5d3028e4e3d7990e696fd9a5b321ebf24694cee6316e5d9a32ba6`
is exactly the established [terrain active palette](ordinary-land-texture-palette.md),
not raw PALETTE.PCX RGB. The producer is general gamma routine **`5e1020`**:
it reads the palette pointer from `this+0xa64`, processes 236 entries starting
at byte offset `0x28` (index 10), and updates all three channels. Formula:
`trunc(235.99*clamp(pow(channel/235,1/gamma),0,1))`.
At Gamma Correction 100 (`gamma=1`) this is `min(channel,235)` for the checked
8-bit inputs. Reserved entries 0..9 and 246..255 remain outside that loop.
Calls to `617900` and `617690` publish the changed palette afterward.

The Workshop harness inherits this exact initialization from
`probe_title_native.py`: load PALETTE.PCX into `959aa8`, set the object's
`+a64` pointer, call `5e1020(1.0)`, then `617690`. This happens before Workshop
construction; it is **not a Workshop-specific clamp**. Existing title and
terrain contracts already establish it. An additive comparison of the installed
raw PCX with c626 matches all **708 common RGB channels**, with exactly **26
channels changed** by the cap. Private receipt:
`tmp/reference/workshop-gamma100-existing-check-20261020.json`.

Use this active palette both for tint multiplication and nearest-color searches
in the native cache derivation. Applying the clamp only at final RGB conversion
cannot repair already different indexed tint-table choices. A port may isolate
this correction to Workshop to preserve current map behavior, but that scope
would be a port integration choice rather than the original game's policy.

## Cell frame and indexed destination

`43cda0` selects small-frame style **0 when unselected, 2 when selected**
(`43ce58..43ce6f`), then calls `5fbbd3` before the model/text callback.
The style table is `7b0058 + 40*style`. These are INTERFACE.PCX slices loaded
by `45eea0`; they are not CVR assets or FRAME.PCX images.

Use the exact [small-frame composition rules](small-dialog-frame-pixels.md)
and [atlas crop contract](generic-dialog-fonts-and-frame.md). For style `i`,
atlas origin is `x=1+39*(i%3), y=229+39*(i/3)` with integer division.
Corners are 6 by 6 at origin and offsets 32. Horizontal edges are 24 by 6
at x+7; vertical edges are 6 by 24 at y+7. Center is 24 by 24 at x+7,y+7.

| State | Atlas origin | Center crop | Loaded center rows, even / odd |
| --- | --- | --- | --- |
| Unselected, style 0 | `(1,229)` | `(8,236,24,24)` | **196 / 65** |
| Selected, style 2 | `(79,229)` | `(86,236,24,24)` | **191 / 83** |

Each listed row is constant across its 24 pixels. Decode the owned asset at
runtime; the row values are verification expectations. The existing bounded
theme-initializer receipt confirms both complete center buffers. Their hashes
are `966f54c285391cfbe12046e6cd897cdf2614b48ca737b5ee5292b65abd591db3`
and `7c7a951ccb064c37dedf455ab4354e822bfd58c65985aded925afd43616b6e6c`.

Loaded indices are `(raw+10)&255`. Each corner/edge sprite skips source 9,
then the center pattern replaces destination bytes equal to 7 in that piece.
Final interior `[L+6,T+6,R-6,B-6)` copies the pattern literally. All center
sampling uses absolute destination-bitmap phase `center[y%24][x%24]`.
Edge sprites repeat from their own piece rectangle origin. Preserve clip
intersection and operation order from the linked compositor contract.

For Probability Sheath, compose this indexed backing first. Its model's
source values 0..23 select translucency rows; each lookup uses the **current
destination index**, including these alternating backing indices and any border
pixels under the model clip. Source 24 leaves that destination unchanged.
An RGB fill or a flat index cannot reproduce that operation.

## Blank cells and unavailable text

For ordinal >= configured count, `43ce13..43ce2a` calls `5f37f0` with sprite
`6d30e0`. The initialized object has key 9, width 0 and height 0. The primitive's
zero-dimension guard exits without writes: ordinals 10..19 retain existing
background and receive **no card border, model or labels**.

The constructor separately allocates a 10 by 10 temporary bitmap, attempts
to blit that same empty sprite (`43d02c`, `5fd990`), and installs the temporary
bitmap as its outer picker frame's center (`43d06a`). The offscreen fixture's
new DIB allocation is zero-initialized; this is a declared service boundary,
not proof of an arbitrary real-session allocation's prior bytes. The per-cell
INTERFACE.PCX backing above does not depend on that allocation assumption.

Unavailable cards still draw their selected/unselected frame. Their model uses
silhouette index 185, and `43c100` returns before name/stats: there are **no
unavailable glyph baselines**. For available cards, c626 records No Armor name
baseline `(51,14)`, Shielding `(30,89)`, defense `(122,90)`; Synthmetal name
lines `(150,14)` and `(168,27)`, Shielding `(138,89)`, defense `(230,90)`.
The later [weapon-picker outer-clip trace](design-workshop-weapon-picker.md#underlay-source-crop-key-and-clip)
clarifies that model clipping uses the outer cell rectangle. The inset3 rectangle
is the callback layout argument.

## Receipts

The additive private bundle is
`tmp/reference/workshop-picker-backing-evidence-20261020/`. Its check script
compares the frozen Silksteel kernel table with all four frozen cache slices,
checks the initializer's complete center patterns and blank sprite dimensions,
and records the c626 manifest hash. It does not execute the game. The bounded
theme initializer had stopped immediately after fonts/INTERFACE.PCX loading;
its source and receipt are retained with the source disassemblies.

Additive manifest SHA-256:
`31d33779a9a867de02ced321808f4f06ce217e93e89aaa255b0e7ad27f9d0f07`.
