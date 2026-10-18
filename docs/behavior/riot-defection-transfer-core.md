# RE-SMAC-RIOT-TRANSFER-001: initial defection mutations

Status: original-x86 checked for the SMAC fingerprint in this directory's
README. All 14 named, 1,100 sampled and 42 gate-to-transfer faction-pair checks
pass. These execute the contiguous initial transfer block, stopping before
the first subsequent helper. Unit/territory changes, later facility/status
cleanup and full completion remain open.

## Ownership and assimilation

Given an admitted positive recipient distinct from the old owner:

- Change the current owner to the recipient.
- Preserve the existing former-owner identity.
- Replace the assimilation counter with 0 if the recipient equals that former
  owner; otherwise replace it with 50.

The old assimilation counter is not added, capped or preserved. Returning to
the former owner clears it even when it was previously 255. A transfer to a
different owner sets 50 regardless of the previous amount. These are defection
rules for this block, not an initialization rule for military capture.

The recipient's activity stamp becomes the current turn index, including zero
when supplied. Other factions' corresponding stamps remain unchanged here.
This is a verified writer of the stamp implicated in the preceding
[recipient-selection weighting](riot-defection-recipient.md); its complete
meaning and update lifecycle remain open.

## Minerals and production

Clear both accumulated mineral stock and the mineral baseline. Set both the
current production item and remembered previous item to Stockpile Energy.
Remove all additional active queued items by resetting queue length.

Before that reset, remove each current/queued unit item from the old owner's
per-design queued-unit accounting. Process every occurrence, including repeated
entries for the same design. Facilities, projects and Stockpile entries do
not decrement unit-design counts. The recipient's queued-unit counts are
unchanged at this stage; the old queue is discarded rather than transferred.

For example, three queued copies of design 64 decrement the old owner's
design-64 counter three times. A supplied stale zero counter wraps to 255
when decremented; the original accounting field is an unsigned byte. That
case is an arithmetic boundary, not a claim that consistent queue construction
normally produces a zero count for a queued unit.

Resetting active queue length does not erase every inactive backing entry.
The checks verify those tail values remain unchanged while the active current
item becomes Stockpile and the additional queue is empty. The port can use
its own queue representation while matching these active semantics.

## Faction bookkeeping

Decrease the old owner's base count by one and increase the recipient's by
one. Across the seven playable factions, clear an existing base-reference
slot if it points at this transferred base; leave nonmatching references
alone. The complete role and lifecycle of that reference field remain
unresolved bookkeeping work.

Tests vary current base identity to distinguish clearing matching references
from clearing every slot or assuming the transferred base always has index
zero. Other factions' activity stamps and unaffected queue counters are also
checked.

## Exact stop boundary

At the end of this checked block, population, former owner, nutrients,
facilities and status have not otherwise changed. Whole base-record
comparison permits only current owner, assimilation, mineral stock/baseline,
current/remembered items and active queue length to change. Later code performs
more transfer work; these observations do not assert that those other fields
remain unchanged through the completed defection.

The [14 named cases](riot-defection-transfer-core-cases.json) cover return to
former owner, other assimilation comparisons, mineral discard, mixed and
duplicate queue entries, stale accounting, cache identity and recipient/turn
boundaries. The sampled cases cover all faction slots and varied queue/state
inputs. The 42 integrated checks execute the preceding human/POLICE/former-owner
gate for every distinct faction pair, then this transfer block.

Original instructions execute without patched bytes or substituted returns;
no external helper is called within the checked core. Inputs to isolated core
cases are conditional mutation boundaries and can be broader than combinations
admitted by the earlier recipient decision. No whole-turn, completed transfer,
unit ownership, territory, final facility reconciliation or notification
behavior is claimed. The subsequent [replay/policy/grant phases](riot-defection-policy-and-facilities.md)
now have separate original-x86 coverage, including runs from riot entry;
the intervening territory/unit processing is still omitted there.
