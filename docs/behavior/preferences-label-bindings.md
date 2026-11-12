# RE-SMAC-PREFERENCES-LABELS-001: installed caption slots

2026-10-21. Pinned original terran.exe SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
[Machine-readable bindings](preferences-label-bindings-cases.json).

All captions below come from the installed **labels.txt / #LABELS** vector.
Indices are **zero-based**. English is a diagnostic value from this installation,
not a lookup key. Load by slot; do not search for these strings or substitute
SCRIPT page captions for the left buttons.

The native lookup is conceptually `resolve(labelsTokens[index])`:
`[9c15b8]` points to the4-byte token array; the table byte offset is
`4*index`; `62e860` on object9c1598 resolves the selected token to text.
The offset column is relative to that array, **not** a PE virtual address
or a byte offset into labels.txt. `62e8b0` is the original LABELS loader;
its token array excludes the section's count line.

**Port parser boundary:** `TXTLoader::ReadUniqueSection(source, "LABELS")`
retains the count line (`1131` in this installation) at parsed vector index0.
For that unmodified result, use **`rows[nativeIndex + 1]`**:
native101 → parsed102 = OK; native750 → parsed751 = Preferences.
Only a label vector that has already removed the count line can use
`labels[nativeIndex]` directly. All table indices below remain native,
zero-based indices; do not add1 to the native table byte offsets.

## Left captions and common actions

| Role / stock English | Control ID | LABELS index | Table byte offset | Native load instruction |
| --- | --- | --- | --- | --- |
| Preferences | 0 | **750** | `0xbb8` | `0x49bf51` |
| Adv. Preferences | 1 | **751** | `0xbbc` | `0x49bfe7` |
| Warning Preferences | 2 | **752** | `0xbc0` | `0x49c07c` |
| Automation Preferences | 3 | **753** | `0xbc4` | `0x49c11b` |
| Audio/Visual Preferences | 4 | **754** | `0xbc8` | `0x49c1b5` |
| Map Preferences | 5 | **1077** | `0x10d4` | `0x49c24f` |
| Gamma Correction | 6 | **1078** | `0x10d8` | `0x49c2e9` |
| Restore Defaults | 90 | **1079** | `0x10dc` | `0x49ed13` |
| OK | 92 | **101** | `0x194` | `0x49ed3f` |
| Cancel | 91 | **102** | `0x198` | `0x49edc9` |
| More | 94 | **1088** | `0x1100` | `0x49f6fc` |
| Back | 94 | **664** | `0xa60` | `0x49f74f` |

More and Back use the **same control94**, not separate buttons. On warning
page2,49f680 loads More at49f6fc. On continuation page6 it loads Back at49f74f.
The constructor initially loads More through49ef0d, also slot1088/offset1100.
Other pages hide that control. OK is slot101 and Cancel102; their action IDs
92/91 are a separate namespace from label indices.

## Audio playback captions

4a0e00 refreshes these captions. **Off is selected when the relevant state
field is zero; On when it is nonzero.** These are state captions, not translated
Mute/Unmute action names. The constructor initially uses the Off slots.

| Channel | Control ID | State field relative to862180 | Off index / offset / load | On index / offset / load |
| --- | --- | --- | --- | --- |
| Main | 85 | `+15820` | **1080** / `10e0` / `4a0ee8` | **1081** / `10e4` / `4a0ef6` |
| Voice | 87 | `+16eb8` | **1082** / `10e8` / `4a0e2a` | **1083** / `10ec` / `4a0e38` |
| SFX | 86 | `+1636c` | **1084** / `10f0` / `4a0e69` | **1085** / `10f4` / `4a0e78` |
| Music | 88 | `+17a04` | **1086** / `10f8` / `4a0ea8` | **1087** / `10fc` / `4a0eb7` |

Stock slot text is Main Off/Main On, Voice Off/Voice On, SFX Off/SFX On,
and Music Off/Music On. Control order85/87/86/88 differs from slot order;
keep the explicit mapping. Main+15820 is an aggregate playback state,
not an additional packed preference bit. Native621910 replaces the button
caption after62e860 resolves the chosen slot.

Validation:20 exact native table loads are pinned to their byte offsets,
and all20 selected slots match the installed LABELS vector. This stock
file has1131 entries; physical source lines are recorded only as diagnostic
receipts and are not a portable lookup method. No modal capture, production
edit or existing evidence-packet modification was needed. Additive private
packet: `tmp/reference/independent-preferences-labels-evidence-20261021/`.
