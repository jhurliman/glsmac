# Retained inputs for ordinary riot defection

RE-SMAC-RIOT-DEFECTION-AUTHORITIES-001, 2026-10-18. Pinned stock SMAC.
[Literal storage/source supplement](riot-defection-retained-authorities-cases.json).

## Relationship weighting and the shared history word

The recipient selector's `half_distance` is **Truce, primary relationship bit4**.
Pact1, Treaty2 and Truce4 act in that order: double, multiply3/2 with truncation,
then halve with truncation. Preserve the existing selector's unusual dependency
on the **current best recipient's** directed relation, rather than the next
candidate's relation. This names the third flag in the frozen
[recipient receipt](riot-defection-recipient.md); it does not change its arithmetic.
The identification reuses the actual [diplomatic primitive](diplomatic-relation-primitives.md)
and silent-Truce receipts without rerunning them.

Before any best recipient exists, the same indexed code reads the word just
before that relation row. This is **exactly the existing saved
`last_base_expansion_or_advice_turn[faction]` authority**, not a new riot field.
Its low bits1/2/4 receive the same arithmetic tests; those history bits are not
the faction's current Pact/Treaty/Truce state.

Reuse [the established lifetime](leading-human-victory-pressure-shortcut.md):
fresh initialization writes current turn0; admitted nonzero faction initialization
writes that current initialization turn; successful founding, capture/acquisition
and gated SECOND BASE/BUILD MORE tutorial paths update it. Ordinary movement,
factory creation and nonlast Unit removal preserve it in the existing controls.
The original whole-world path initializes all eight faction slots. Ordinary
save/load preserves all eight words without recomputing them.

The defection core writes the current turn to the **recipient's same word**.
The completion fixture observes actor777 preserved and recipient888→6. Do not
create another saved `current_faction_stamp`, reset this word each turn, or infer
it from the current Base count. The original name remains unknown; the port's
existing descriptive name is appropriate.

## The cleared Base reference is a battle-plan target

The faction Base-reference slot cleared by defection is a retained **battle-plan
target Base**. Static producer callers use the original BATTLEPLANS, BATTLEPLAN
and BATTLEYES paths. The plan-selection path stores its selected Base; the
acceptance path stores the requested Base for the responding faction. AI
production, regional planning and Unit planning subsequently read this field.
This is a target, not Headquarters, a home Base or the current Inspector Base.

The ordinary faction initializer sets−1. Capture and defection scan playable
factions1..7 and clear matching references to the transferred Base. Base deletion
clears an equal reference and decrements references above the removed Base index.
Ordinary file bodies retain the field; the new canary pair below confirms that
even deliberately stale values are not rebuilt on load. Preserve it as a signed
Base-reference authority with absent−1. A stable-ID port must translate target
identity across its own Base removal/ownership transaction.

The battle-plan UI's complete admission, negotiation and AI strategy are not
newly replayed. A fresh campaign without those operations starts from the actual
initializer's−1, rather than an invented Base0 target. A restored nonnegative
target must not be replaced with−1 merely because the port does not yet expose
that UI. The source survey is direct-reference evidence, not an indirect-alias
exhaustiveness claim.

## Per-Unit pacifism markers

The original Unit state word owns separate accounting bits: **2097152
(`0x200000`) penalized,4194304 (`0x400000`) first-unit exemption**. Reuse the
[complete support producer receipt](pacifism-unit-count.md):

- Each support calculation resets the current Base's pacifism count and clears
  both old marker bits, plus the distinct paid-support bit16, only on matching
  owner/home Units before admission tests.
- Qualifying Units count regardless of POLICE. At−3, the first qualifying Unit
  in global record order gets exemption; later ones get penalized. Below−3 all
  get penalized; at−2 or above neither marker is set.
- Wrong-owner, other-home and unhomed Units retain their supplied marker bits
  during that Base calculation. Repeated resource queries rebuild selected
  markers; they are not globally cleared whenever a view opens.

New Unit initialization zeros state, hence both markers. The defection radius
conversion mask does **not** clear these two bits by itself. Converted Units get
the new owner/home; the final real support pass then clears/rebuilds their bits
under that Base's new authority. A Unit outside the processed home set can retain
stale markers. Keep these bits in saved Unit state, independently of the derived
Base pacifism count. The completion receipt records their transfer/query timing;
it does not infer a global cleanup from a single Base calculation.

## Native active-plan tally width and retention

The separate owner0 active-plan tally is **one32-bit word**, independently of
the512 design-count bytes and the global Unit count. The original factory loads
the whole word, increments a32-bit register and stores it; deletion decrements
and stores it when the removed design's unsigned plan byte is at most7.
Arithmetic wraps modulo2³²: increment4294967295→0, decrement0→4294967295.
There is no saturation or special zero repair in these instructions.

Storage/increment/decrement do not establish a universal signed-comparison
interpretation for every consumer. Retain all32 bits; use unsigned/wrapping
arithmetic for these producer operations. The ordinary factory's separate
2048-Unit admission gate does not turn this retained tally into a registry count.
Native removal updates owner0's tally; deleting another owner's Unit does not.

The already measured native factory early exit still reads signed referenceX<0,
with the initialized retained pair−1/−1. The direct-morale definition scalar0 is
another independent input. None of these authorities is derived from this tally.

## New ordinary storage discriminator and limits

One actual ordinary save/load pair preserves two complete Unit records104 bytes,
including penalized/exempt and paid-support marker canaries, all eight shared
history words, all eight battle-plan Base-reference words and the native tally
bit pattern4294967295. After save, Unit bytes and named words are poisoned; load
returns0 and restores them exactly, without RNG. Deliberately stale references
and tally are storage controls, not legal gameplay/query inputs or a recount.

This is file-body evidence with supplied FILE/TLS/allocator/view services, not
frontend resume or owner upkeep. Earlier producer/consumer and lifetime receipts
are reused unchanged. No production or installed assets changed; broad research
remains paused.
