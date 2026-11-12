# Ordinary Isle sailing and cargo staging

2026-11-11 static caller continuation, pinned stock SMAC. Reuse the executed
[transport boarding rules](transport-mode0-boarding.md), capacity helper and
retained native sea-policy recordings. This packet establishes ordinary
sailing's **mode1 caller**, rather than generalizing the removed-Base mode0
caller. No new sailing/cargo runtime matrix is claimed.

## Successful ordinary sea movement

After movement admission writes the actor's spent byte (`5af342`), the shared
movement path calls **`5d0590(actor,1)` at `5af8a8`**, before final relocation.
It does not require a preceding mode0 call. Mode1 shares the capacity,
two-pass candidate selection and order/payload writes, while also staging the
selected moving group. Failed admission does not reach this call.

For stock Isle9 on ordinary open sea, retained capacity helper `5d9730`
returns stored morale+1 for the qualifying predefined negative-weapon design
with nonzero capacity byte. That reported capacity is distinct from the
transport operation's **owner0 ordinary-cargo budget99**. The override at
`5d07b7`..`5d07bb` is not restricted to mode0; it also applies to mode1.
Do not change design carry capacity to99 or derive this budget from remaining
movement. Ordinary land cargo must itself have zero carry capacity; same-owner
exact match is required. Other Isles and ordinary air units do not become
ordinary ground cargo.

Mode1 first invokes stack carrier-separation helper `5d0230` at `5d0659`.
It temporarily extracts carrier-capable members to `(-3,-3)` and reinserts
them at the original location, rebuilding link order. Mode0 instead begins
at the existing stack head and clears state bit1 across that stack. Do not
assume the two modes have identical traversal starting points or blindly use
dense Unit order. Mode1 starts from its actor in the resulting linked stack;
its traversal follows actual next links.

For modes<=1 there are two passes (`5d07c5`..`5d07cc`): existing order1 cargo
whose payload equals actor consumes capacity first; the second pass can board
new eligible cargo. On an unsafe open sea tile, ordinary owner0 sentry-born
Mind Worms with **order1/payload-1** pass that second native/AI branch. Ordinary
flags0 qualify; no prior mode0 association is necessary. The retained special
state gates still apply: state40000 requires20000 plus region-state7; other
carrier associations can be reassigned by the native/AI branch. Human cargo
eligibility differs and must retain its existing contract.

Each processed cargo calls order reset `5d82f0`, receives order1, payload equal
to the actor's current dense Unit index and cleared order-auxiliary word. In
mode1, unlike mode0, it is unlinked/reinserted at **`(-2,-2)`**, and state
mask10000 is cleared (`5d09e5`..`5d0a06`). The carrier itself is staged at
`(-2,-2)` at `5d0b95`..`5d0ba3`. Unselected source members remain behind;
foreign owners, excluded domains and another unselected carrier are not moved
merely because they shared the source tile. Preserve source stack repairs.

Final movement at `5aff04`..`5aff0f` calls `5d01c0(actor,destX,destY)`.
That helper follows previous links to the head of the **staged moving stack**,
then moves each member through actual unlink/reinsert helpers, saving next
before mutation (`5d01ce`..`5d0226`). Thus both carrier and selected cargo
end at destination with rebuilt links and tile presence; it is not an actor-
only coordinate update, nor a move of every original source-tile member.
Cargo's own spent byte is not charged an independent edge by this transport
path. Actor spending remains the earlier ordinary movement charge.

Temporary negative-coordinate staging is native implementation machinery.
A port may prepare the same final atomic group mutation without exposing
off-map intermediate Units to rendering or AI, but must preserve which Units
boarded, their order/payload/flags and stack-order-dependent selection.

## Observer masks and sight order

Before final relocation, `5af97f` selects owner-dependent visibility handling.
Positive-owner actors call `5cd2c0` to clear carried visible-to bytes over
their linked moving stack. **Owner0 uses the distinct native path**:
`5af99f`..`5af9d5` intersects the **actor's** Unit+`0x27` visible-to mask with
destination explored bits, then the subsequent per-positive-observer checks
can remove actor bits when the nearest-owned-Base/recognized-territory gates
fail (`5af9e1`..`5afa44`). Do not automatically copy the carrier's resulting
mask into cargo or clear every stationary native on the old tile.

Group relocation then updates coordinates, stack links, tile owner/presence
through `5d80f0`/`5d81d0`. Those low-level placement calls are not independent
full sight updates for every cargo member. The ordinary caller subsequently
returns through `5adbc5` and executes actual **`5cdaa0(actor,1)` at `5adc51`**,
after the new stack is in place. Reuse native sight's linked-stack disclosure
and retained-mask semantics: it may update already present destination Units
as well as the actor. Do not grant every cargo an extra full sight/exploration
pass or normalize all masks from current geometric visibility.

The source also retains native origin-fungus repair before that final sight
call (`5adc05`..`5adc48`); it is separate from cargo boarding and promotion.
This ordinary sea→sea/no-combat packet does not establish landing, coast
disembarkation, safe Base/airbase behavior, carrier destruction or air capacity.
Those callers need their own gates; the existing carrier-loss contract remains.

Private source for this caller link and raid-facility clarification is frozen in
`tmp/reference/independent-isle-sailing-static-evidence-20261111/`.
