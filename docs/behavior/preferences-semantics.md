# RE-SMAC-PREFERENCES-SEMANTICS-001: all created checkbox bindings

2026-10-21. Original terran.exe SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
[Machine-readable self-authored bindings and native scalar receipts](preferences-semantics-cases.json).
The table uses semantic roles rather than copying the original SCRIPT text.
The existing [page inventory/layout](preferences-ui-and-automatic-design.md)
and [pixel contract](preferences-page0-pixels.md) identify visible labels and placement.

## Word numbering and table

Words are zero-based: **W0=Preferences**, **W1=More Preferences**,
**W2=Announce**. Cached addresses are95a48c/490/494; live addresses are
959f98/9c/a0. Every mask below is hexadecimal. **Set** means checked iff the
bit is1; **clear** means checked iff the bit is0. Native initializer4a0040
and collector49f8b0 agree on all67 created controls. Seven controls invert
the bit:13,14,15,42,66,71,79.

| Page | ID | Self-authored semantic role | Word | Mask | Checked iff |
| --- | --- | --- | --- | --- | --- |
| 0 | 7 | tutorial messages | W0 | `00000020` | set |
| 0 | 8 | automatic unit design | W0 | `00004000` | set |
| 0 | 9 | prune obsolete designs | W1 | `00100000` | set |
| 0 | 10 | edge scroll | W0 | `00001000` | set |
| 0 | 11 | turn autosave | W0 | `00000002` | set |
| 0 | 12 | end turn pause | W0 | `00000001` | set |
| 0 | 13 | fast enemy neutral moves | W0 | `00000004` | clear |
| 0 | 14 | fast allied moves | W0 | `00008000` | clear |
| 1 | 15 | single click radio | W0 | `20000000` | clear |
| 1 | 16 | unit click cancels orders | W1 | `00040000` | set |
| 1 | 17 | confirm combat odds | W1 | `00008000` | set |
| 1 | 18 | pause after battle | W1 | `00000010` | set |
| 1 | 19 | fast combat | W0 | `00000008` | set |
| 1 | 28 | fast ordered moves | W1 | `00000040` | set |
| 1 | 20 | fast all moves | W1 | `00000080` | set |
| 1 | 21 | ordered units without centering | W0 | `00080000` | set |
| 1 | 23 | base zoom without recentering | W1 | `00000004` | set |
| 1 | 24 | detailed main menus | W1 | `00200000` | set |
| 1 | 25 | detailed context menus | W1 | `00000400` | set |
| 1 | 26 | right click menu | W1 | `00000100` | set |
| 2 | 43 | facility completion | W2 | `00000001` | set |
| 2 | 44 | civilian completion | W2 | `00000002` | set |
| 2 | 45 | combat completion | W2 | `00000400` | set |
| 2 | 46 | prototype completion | W2 | `00000004` | set |
| 2 | 47 | governor queue completion | W2 | `00002000` | set |
| 2 | 48 | riot start | W2 | `00000008` | set |
| 2 | 49 | riot end | W2 | `00000010` | set |
| 2 | 50 | golden age start | W2 | `00000020` | set |
| 2 | 51 | golden age end | W2 | `00000040` | set |
| 2 | 52 | low nutrients | W2 | `00000080` | set |
| 2 | 54 | stale build orders | W2 | `00000200` | set |
| 2 | 55 | population cap | W2 | `00000800` | set |
| 2 | 56 | delayed transcendence | W2 | `00001000` | set |
| 6 | 57 | starvation | W2 | `00004000` | set |
| 6 | 58 | mineral shortage | W2 | `00008000` | set |
| 6 | 59 | energy shortage | W2 | `00010000` | set |
| 6 | 60 | random events | W2 | `00020000` | set |
| 3 | 29 | air return at fuel limit | W0 | `00010000` | set |
| 3 | 30 | former forests | W0 | `01000000` | set |
| 3 | 31 | former elevation | W0 | `00020000` | set |
| 3 | 32 | former advanced improvements | W0 | `02000000` | set |
| 3 | 33 | former fungus removal | W1 | `00000008` | set |
| 3 | 34 | former sensors | W1 | `00000020` | set |
| 3 | 42 | former transport links | W1 | `00400000` | clear |
| 3 | 35 | pact contact cancels movement | W0 | `00100000` | set |
| 3 | 36 | treaty contact cancels movement | W0 | `00200000` | set |
| 3 | 37 | truce contact cancels movement | W0 | `00400000` | set |
| 3 | 38 | vendetta contact cancels movement | W0 | `00800000` | set |
| 3 | 39 | ignore other unit classes | W0 | `40000000` | set |
| 3 | 40 | wake landed passengers | W0 | `80000000` | set |
| 3 | 41 | automatic monolith | W1 | `00000800` | set |
| 4 | 64 | stop voice on popup close | W1 | `00020000` | set |
| 4 | 65 | project movies | W0 | `10000000` | set |
| 4 | 66 | interludes | W0 | `00040000` | clear |
| 4 | 71 | monuments | W1 | `01000000` | clear |
| 4 | 67 | map animation | W0 | `00000080` | set |
| 4 | 68 | window animation | W0 | `08000000` | set |
| 4 | 69 | scrollbar animation | W1 | `00080000` | set |
| 4 | 22 | unit blink | W1 | `00000200` | set |
| 5 | 72 | fog | W1 | `00000001` | set |
| 5 | 73 | land grid | W0 | `00000100` | set |
| 5 | 78 | ocean grid | W1 | `00800000` | set |
| 5 | 74 | base grid | W0 | `00000200` | set |
| 5 | 75 | base names | W1 | `00002000` | set |
| 5 | 76 | base production | W1 | `00001000` | set |
| 5 | 77 | flat terrain | W1 | `00010000` | set |
| 5 | 79 | active unit path | W1 | `02000000` | clear |

## Acceptance reconstructs words

For each created control, set its mask in a fresh zero word when
`checked XOR inverted` is true. Also include these three independent
Audio/Visual playback states (they are not extra created checkboxes):

| State field relative to862180 | Role | Word / mask |
| --- | --- | --- |
| +1636c | sound effects playback | W0 / `00000400` |
| +17a04 | music playback | W0 / `00000800` |
| +16eb8 | voice playback | W1 / `00004000` |

The Main Off/On button is an aggregate playback control, not a fourth
persisted Boolean. The eight native combinations above round-trip without
coupling one audio bit to another. Original legacy reads for uncreated
IDs61/62/63 do not replace these fields: the initializer handles audio
specially at4a040f..4a049d. Warning ID53 is not created or initialized;
its old W2 bit100 is not represented.

| Word | All67-checkbox coverage | Complete accepted reconstruction mask |
| --- | --- | --- |
| W0 | `fbffd3af` | **`fbffdfaf`** |
| W1 | `03ffbffd` | **`03fffffd`** |
| W2 | `0003feff` | **`0003feff`** |

Thus an unchanged open→OK gives `input & acceptedMask` for all three
words, with audio states initialized from that input. Arbitrary edits use
the control states and playback fields rather than ORing into the old
word. Bits outside the masks are discarded. This happens only on the
accepted path; Cancel does not normalize the words. Opening49ef50 clears
aggregate+a20/a24/a28;49f8b0 clears page contributions;49f330 combines them,
updates cached/live values and writes the profile.

All67 controls unchecked, with audio off, encodes
`(20048004,03400000,00000000)` because of the seven inversions. This is
not the same as three zero words. Decoding zero words checks those seven
inverted controls, and re-encoding returns three zeros.

## Original defaults and Laptop/difficulty variants

The original loader5b5830 derives these defaults, with32-bit arithmetic:

```text
W0 = Laptop == 0 ? bbe1dd96 : a3e1dd16
if signed Difficulty < 2: W0 |= 00000020
W1 = Laptop == 0 ? 003a7168 : 00327168
W2 = 0003c3a9
```

Any nonzero Laptop value takes the second branch. Relative to Laptop=0,
this clears W0 masks`18000080` (project movies, sliding windows, map
animations) and W1 mask`00080000` (sliding scrollbars). The conditional20 is
the tutorial-messages bit. The initializer/collector does not otherwise
change a bit's meaning by difficulty. Native cases cover difficulties0,1,2,5
for both Laptop branches. This is default construction: a stored valid
format12 preference string can override it as described in the existing
[INI loading/encoding contract](design-workshop-preference-and-picker-layout.md).

| Laptop | Difficulty | Loader W0 | Loader W1 | Loader W2 |
| --- | --- | --- | --- | --- |
| 0 | <2 | `bbe1ddb6` | `003a7168` | `0003c3a9` |
| 0 | >=2 | `bbe1dd96` | `003a7168` | `0003c3a9` |
| nonzero | <2 | `a3e1dd36` | `00327168` | `0003c3a9` |
| nonzero | >=2 | `a3e1dd16` | `00327168` | `0003c3a9` |

**Restore Defaults is slightly different from a fresh loader default.**
Its49f330 sibling callback4a09dc calls5b5610/5b56b0/5b5750. Those helpers
return the base Laptop-dependent W0/W1 and fixed W2, with **no Difficulty<2
OR20**. Reset therefore unchecks Tutorial even on those low difficulties;
it still checks automatic unit design. Reset changes drafts, not the live
words until accepted, and has the audio-preview side effects already documented.

The loader/reset words contain bits that acceptance removes: W0 mask`0x10` and
W2 mask`0x100`. For example ordinary non-Laptop difficulty>=2 default
`(bbe1dd96,003a7168,0003c3a9)` becomes
**`(bbe1dd86,003a7168,0003c2a9)`** on unchanged OK. At difficulty<2,
W0 becomes`bbe1dda6`; Laptop nonzero becomes`a3e1dd06` / `a3e1dd26`.
Do not mistake this normalization for a failed save or restore those dropped
bits unconditionally after acceptance.

## Evidence

The native probe toggles each created checkbox individually against an
all-unchecked baseline, records the changed word/mask and direction, then
cross-checks every control after every one-bit initializer input:
**67 collections plus4489 initializer/getter checks**. It additionally runs
all8 audio combinations,8 original loader/reset variants and4 word-wide
round trips. Original control storage,4a0040,49f8b0 and default helpers run
in the owned executable guest; profile services are memory-only. The code
and source bindings are frozen additively with the button/rail packet.
No production edits or flag-consumer guesses are included.

## Port store and consumer receipt

`config/SmacPreferences.h` implements the67 Boolean bindings, three separate
audio bits, source defaults and accepted reconstruction. The local YAML key
`smac-preferences` contains three unsigned decimal words separated by commas;
this is a port representation, not the original binary-string INI format.
Invalid, truncated or out-of-range words are rejected. Original INI import
is not implemented here.

The wrapped configuration exposes detached `values` objects keyed by native
control ID and three Boolean `audio` entries:0 effects,1 music,2 voice.
`get_smac_preferences(laptop,difficulty)` uses the explicit default variant
only when no packed state exists. `get_smac_preference_defaults(laptop)`
uses the reset variant without the low-difficulty tutorial bit. Acceptance
requires all67 controls and all3 audio entries before reconstructing words.

The old `automatic-unit-designs` Boolean migrates into bit4000 when no
packed state exists. Both APIs then share the packed value; the old setter
changes only that bit and preserves other preferences. A batch save makes
one file-write attempt and restores the complete live settings map if it
fails. This is **not** a crash-atomic filesystem replacement guarantee.

The actual Workshop suggestion gate and frontend edge-scrolling admission
read these shared values. Disabling edge scrolling also stops an existing
scroll timer on the next frontend iteration. The other preference consumers,
normal Preferences window route, audio playback/levels and Gamma remain
open. Storing their source-backed bits alone does not establish their effects.

Focused native checks cover original inversion/default/reconstruction
controls, strict local word parsing, legacy migration, detached edits,
complete acceptance, failed-save rollback, destruction fences and fresh
configuration reload. All three User preferences groups pass with focused
ASan/UBSan instrumentation on configuration, wrapper and GC/object consumers.
