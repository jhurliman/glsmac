# Base art identity across ownership changes

The base-body renderer's art-family input is **the existing former-owner byte** (Base offset 5). It is separate from current owner at offset 4, but is not immutable founding history. Use the saved conquest/former-owner authority for body-art selection. The port already declares this authority as `SmacConquestState::former_owner`; do not create a second independently evolving founding-art field.

| Transition | Former-owner/art byte after transition | Assimilation timer |
| --- | --- | --- |
| Fresh base for faction A | A | 0 |
| Capture A→B, former F differs from B | A | 50 |
| Recapture/return A→B, former F already B | Preserve B | 0 |
| Riot defection to B | Preserve F | 0 if F=B, otherwise 50 |
| Cede-base helper, native mode 3 | Same conditional rule as capture | 0 if F=B, otherwise 50 |
| Cede-base helper, other native modes | B | Clear if F=B before transfer; otherwise preserve prior timer |
| Positive assimilation timer expires | Current owner | 0 |
| Timer already zero | Preserve existing former owner | 0 |

For example, a newly founded faction-1 base starts `(owner=1,former=1)`. Capture by 2 gives `(2,1)`. Capture by 3 before assimilation gives `(3,2)`: the faction-1 founding art is no longer retained. Return to former faction 2 gives `(2,2)` and clears assimilation. At normal assimilation expiry, art changes to the current owner's family. A synthetic different former-owner byte with an already-zero timer remains different; neither loading nor drawing should invent unconditional normalization from that timer alone.

The native cede-base path has a real mode-dependent distinction. The source-backed receipt records the discriminator numerically (`3` versus another mode); it does not assign unverified UI names such as liberation to those branches. A return to the recorded former owner through the conventional capture common path follows the recapture row above. A separate liberation dialog/admission controller was not executed by these controls.

Actual elimination of former faction 2 does not rewrite a surviving faction-1 base's former-owner byte 2. Existing complete human and AI elimination controls retain `(owner=1,former=2,timer=10)` after deletion of faction 2's last base, disposition, and its faction initializer. The subsequent riot can return that base to inactive faction 2 while preserving former owner 2 and clearing assimilation. The original does not replace every foreign reference to an eliminated faction with the current base owner.

Faction restart removes bases currently owned by the reinitialized faction. Record removal and compaction are lifetime operations, not a semantic reset of every surviving base's art family. Freshly created bases then receive the new current faction ID through the founding initializer. This evidence establishes byte authority and the measured elimination/initialization path; broader scenario editing, arbitrary faction-definition replacement, and asset-bank reload behavior are not certified here.

The explicit scenario editor can also write former owner and its conquest timer. It is separate from ordinary gameplay transitions. Raw base-record loading/copying must preserve the byte; deleting a record ends that base's identity. A retained independent value is required because repeated captures cannot be reconstructed from current owner alone.

Validation: **1,361 original x86 local write-fragment controls** cover seven fresh owners; every distinct old/new pair in factions 1..7 with former 0..7 for capture, riot and both cede modes; and ten assimilation edge controls. No game helper outputs are supplied within these fragments. They stop at explicitly declared boundaries and do not claim full transition-controller execution. Existing full headquarters-capture and human/AI elimination receipts provide integration examples. `base-art-family-transitions-cases.json` pins source hashes and representative numeric cases. The previous [body-art pixel evidence](base-body-variants.md) establishes that this very byte selects the art banks.
