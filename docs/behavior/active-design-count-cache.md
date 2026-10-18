# Active counts by faction and design

Research ID: RE-SMAC-ACTIVE-DESIGN-COUNT-001. Original SMAC only.
This supplies the retained quantity contract for
[ordinary ranking scores](ordinary-ranking-score.md).
[Evidence cases](active-design-count-cache-cases.json) distinguish static
transaction rules from one ordinary native save/read discriminator.

## Storage and arithmetic

Each faction has **512 unsigned 8-bit active counts**, indexed by the actual
catalog design ID 0..511. These are distinct from the queued/production counts,
loss-history counts and the separate active-plan tally. Do not interpret the
active-count byte as signed or use a negative sentinel for its high values.

Native increments/decrements are byte operations with **modulo-256 wrap**:
creation 255→0; removal 0→255. They do not saturate, clamp at 250 or repair a
stale count before modifying it. Those boundary arithmetic rules are static
instruction semantics; no 256-unit world was created here. The ranking scorer's
separate count>250 exclusion is a consumer rule, not a storage or factory cap.

The ordinary factory has a separate global Unit-capacity refusal at 2048.
That refusal occurs before incrementing the design cache. It does not limit
one design's cache to 255. Invalid Unit/design/faction inputs are outside this
contract.

## Transaction updates

| Transaction | Cache update |
| --- | --- |
| Successful ordinary factory allocation | Increment `(new owner, new design)` once. |
| Native removal | Decrement the removed Unit's `(owner, design)` once, before unlink/registry compaction. Moving surviving Unit indices does not require further design-count changes. |
| Ownership conversion, design unchanged | Decrement `(old owner, design)` and increment `(new owner, design)`. |
| Ownership conversion with design remapping | Decrement `(old owner, original design)`; after the actual remap result, increment `(new owner, resulting design)`. |
| Design replacement with owner unchanged | Decrement the old design and increment the actual replacement design for that owner. |

The original has inline updates in several conversion/upgrade paths, rather
than one universal ownership setter. Preserve the old owner and original design
until their debit is applied. Updating only the Unit's owner/design fields
leaves the cache wrong until a later recount.

The previously executed riot conversion pass
illustrates an important boundary: its custom remap helper is attempted for
source IDs **greater than 64**, not ID 64. A negative remap result preserves
the original design; the recipient count then increments for that preserved
ID. Other conversion admission/design-selection policies are not generalized
from this one path. The count rule follows the **actual result**, not an assumed
recipient design block.

Base capture alone does not imply ownership conversion of every homed Unit.
The established capture/restart callers rehome or remove Units as documented;
their ordinary removals/factories update counts through the paths above.

## Clear, recount and restart lifetime

The faction initializer clears all 512 active-count entries when it reaches
its count-initialization loop. That clear is **not restricted to turn zero**:
the nearby turn test gates loss-history clearing, after the active-count store.
An admitted restart therefore starts its replacement grants from a cleared
active-count row; actual factories increment for the granted designs. Do not
preserve active counts merely because retained loss history survives restart.

The ordinary owner summary pass also clears the full active-count row, then
walks the live Unit registry and increments the entry for each Unit whose owner
matches the requested faction. That accumulation uses the same wrapping byte
arithmetic. It does not exclude Colony Pods or Formers from active-by-design
counts. The separate plan tally has its own classification rules.

This recount is later than Base processing in faction upkeep. The owner summary
also produces the [cached population aggregate](faction-population-aggregate.md).
It does not run after every factory/removal/conversion, and a pending unprocessed
ending can skip it in the ordinary faction-upkeep tail.

## Save/read authority

The complete ordinary native writer and reader preserve the cache as saved
faction state. The focused fixture deliberately has **zero Units and zero Bases**,
but saves these stale counts:

| Faction / design | Saved and restored count |
| --- | --- |
| Native faction 0 / design 2 | 255 |
| Faction 1 / design 2 | 251 |
| Faction 2 / design 128 | 201 |
| Faction 7 / design 511 | 17 |

After saving, all four live values are changed to 99. The full ordinary reader
restores the four saved values exactly, without replacing them with zero from
the empty Unit registry. Both file-body calls return normally; the established
FILE/allocator/view services are memory-backed and no original save file is
written.

For the port: save/restore this retained cache, maintain its transaction updates,
and perform a recount at the native-equivalent owner-summary point. Do not
silently rebuild it during ordinary load/Start or after every mutation. This
receipt closes the file-body result; complete frontend resume-to-command
scheduling and other serializer modes remain outside scope.

## Exact weapon-reference scan and technology gate

The ranking reference scan visits catalog design IDs **64 through 511 inclusive**:
the seven custom blocks `64*f .. 64*f+63`, for faction blocks f=1..7. It does
**not** visit block 0, IDs0..63, which includes predefined native-life designs.
Native faction 0 is not an additional iteration. Membership/inactivity of the
design's faction does not otherwise filter this scan. Its active+prototyped and
positive power<99 predicates remain those in the score contract.

Predefined counted-design scoring calls the **same technology-validity helper**
used for valid owned technologies. For ordinary faction IDs1..7:

- Prerequisite `None` (-1) succeeds without an ownership bit; `Disable` (-2)
  and other negative IDs fail.
- Ordinary technology IDs0..87 require ownership and a valid definition.
  Definition prerequisite A must be at least -1; prerequisite B must be at least
  -1 unless A is -1. This asymmetric definition guard does not test whether
  the faction owns those prerequisite technologies.
- Future Technology ID88 and higher IDs fail this helper even if their ownership
  bit is set. Faction0 is rejected before the `None` exception.

This is not full design buildability, workshop eligibility or unit-factory
admission. In the ranking counted-design loop, IDs64..511 bypass this particular
predefined prerequisite gate. The global weapon-reference scan performs no
technology availability query at all.
