# Phase1 Scout: first and second full paints

2026-10-21. Pinned terran.exe SHA256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
[Hashes and per-painter scalar receipts](design-workshop-phase1-two-paints-cases.json).
Exactly two native441ba0 calls extend the existing shared-position fixture;
there is no new design/material/render matrix and no port-pixel input.

Native610d10 reports **visibility false**. Model phase8f3c60 is seeded to1
once, then observed without resetting it between paints. The same owner1,
Scout2, all-tech mask2 and declared Spartan-material/Gamma100 inputs are used.
The first call starts with constructor-zero position; the second retains all
shared position and natural renderer state from the first.

For the fixed-pose frontend comparison, main yaw is explicitly supplied as0
before each call. Native43f8c0 still increments its stored next yaw to
3d4ccccd afterward. Hidden visibility does not itself freeze native yaw;
these are controlled yaw inputs, separate from the observed model phase.
No asynchronous timer callback is delivered.

**Every component/main painter and every scene draw retains phase1.**
Chassis → Weapon → Shielding → Reactor → Ability1 → Ability2 → Main is the
actual full-paint order. Both full paints end with main-centered position
words **`40a4c540,42706428,c3826823`**, matching the reported live run's words.
The JSON retains phase and exact position words before/after every painter,
including no-model Shielding/Ability calls.

| Expectation | First full paint | Second full paint |
| --- | --- | --- |
| Chassis incoming position | zero | `40a4c540,42706428,c3826823` |
| Chassis99×85 SHA256 | `995994109caecde8d02dd3f40088e06368b5ed18d77b13f7593ff7174fd00654` | `be27d0ef064d09c00ece9b83ee594a2f526f622535f8985b3dd07f98581751cc` |
| Main256×256 indexed SHA256 | `ed4e467cae80982699d17acb47ad551b06847722851d7fc9a3a9a77a096d0d0b` | same |
| Main256×256 depth LE-u16 SHA256 | `5dbabddb82d11dc50222d2b375189663edc3c9bbe2b859f0000476d9cd24bb92` | same |

The two complete8415-byte chassis crops differ at **194 pixels**, within
card-local `(47,18)-(56,53)`. Both complete main color and depth planes are
identical. Main planes are captured at631613, before keyed copy/cleanup.
Color is65536 row-major indexed bytes. Depth is131072 row-major little-endian
unsigned16 bytes, pitch256 samples, without headers or padding.

Each main has65 raster-kernel/group calls and59 geometry entries. Its raw
kernel-count operands sum to47548. That number is labeled an **operand sum**;
this recorder did not retain kernel flags and does not claim a processed-loop
count. The [terminal-count audit](design-workshop-kernel-count-semantics.md)
corrects the separate phase0 chassis expectation to42810 processed records,
preserving its42874 inclusive operands and all pixel hashes.

The Workshop is constructor-initialized and its full native painter executes
offscreen; no normally booted campaign, OS message loop, or full-screen live
port pixel comparison is claimed. Private crops, complete model planes,
scene journals and scripts remain in
`tmp/reference/independent-workshop-phase1-two-paints-evidence-20261021/`.
Public data contains only hashes/scalars. Earlier freezes are unchanged.
