# RE-SMAC-RIOT-UNIT-001: defection radius and unit conversion

Status: original-x86 checked for the SMAC fingerprint in this directory's
README. All 19 named and 1,000 sampled cases pass. They execute the complete
radius/unit pass with actual map setters and scoped prototype-helper paths.
The checked transfer is old faction 1 to recipient 2. Later homed-unit cleanup,
territory rebuilding and new prototype-slot creation remain open.

## Tiles visited versus units converted

Visit the standard 21 base-radius offsets, including the center. Bounds-check
vertical coordinates; horizontally wrap on a cylinder or clip on a flat map.
For each valid tile, mark the recipient's visibility bit and update the
resolved attribution cache described below.

Only the center and eight immediately adjacent tiles admit old-owner unit
conversion. The twelve outer work-radius tiles receive the map updates but
their old-owner units remain unchanged by this conversion predicate. Units
outside the 21 visited tiles are not processed.

Within each tile, scan the global unit array backward. A unit owned by the old
faction on one of the inner nine tiles changes to the recipient. Its previous
home does not affect admission: units homed elsewhere or unhomed also convert.
Third-owner units do not convert. Units already owned by the recipient keep
their owner, design, home and order during this pass, although a separate
runtime-state marker is cleared.

The [fixture](riot-defection-unit-pass-cases.json) includes the ordered offsets
and compares conversion order explicitly. This is an array/coordinate scan,
not a test of which stack is currently selected in the UI.

## Home, orders and active-design accounting

For every converted unit:

- Set its home to the defected base.
- Preserve Hold order; replace every other checked order with No Orders.
- Decrement the old faction's active count for the source design.
- Increment the recipient's active count for the resulting design.

The count update applies even if the unit was homed elsewhere. Multiple units
of one design update counts separately. This is active-unit accounting,
separate from the queued-unit accounting cleared by the earlier transfer core.

The code also clears several unit-state flags and conditionally resets a byte
according to a chassis property. Their exact semantic identities are not yet
mapped. Private checks verify their literal mutation; the public output cases
cover the named owner/design/home/order/count effects. They are not complete
unit-record specifications.

## Prototype handling and the design-64 boundary

Design IDs 0..64 inclusive skip the prototype request in this caller. Design
65 and greater request a recipient-side design using the source chassis,
weapon, armor, abilities, reactor and source name. Preserve the literal
inclusive boundary at 64.

If the helper returns a nonnegative design ID, use it for the converted unit.
If it returns a negative result, retain the source design ID while still
changing owner and home and applying the other conversion updates.

The original helper is executed for these checked paths:

- A supplied custom design with the Scout component combination is recognized
  as the existing Scout design, ID 2.
- A supplied matching recipient design is reused for an AI recipient; the
  test's existing design ID is 129.
- With a human recipient and the checked exact matching design state, the
  original helper returns a negative result, so the caller retains source
  ID 65. This is a specific tested helper path, not a claim that every human
  prototype request fails.

No prototype return is substituted. New slot allocation, generic workshop
legality, other component/ability combinations, naming collisions and complete
design migration require further research.

## Map updates and unresolved cache meaning

Every valid visited tile gets the recipient's visibility bit and a recipient
value in an attribution cache. Only tiles containing converted units get
their occupant-faction cache set to the recipient during this pass.
Unrelated packed portions of those fields are preserved.

The attribution cache's full role remains unresolved. In particular, the
main territory-owner field read by the territory resolver remains unchanged
through this pass. Do not use the cache update alone as evidence that all
21 tiles change territorial ownership. The subsequent territory-rebuild path
is outside the checked boundary.

## Acceptance scope

The [19 named cases](riot-defection-unit-pass-cases.json) cover inner/outer
radius, homes/orders/owners, reverse scan, designs 64/65 and actual helper
outcomes, cylinder/flat edges and vertical clipping. The sampled cases combine
those inputs with mixed unit lists. Supplied off-map or mixed-faction stack
states are local predicate boundaries, not legal coexistence rules.

Comparisons check unit semantic outputs, prototype request order, active-design
counts and changed tile sets. Actual map setters, prototype helper and its
duplicate validator execute without instruction patches or substituted
returns. State/chassis cleanup identities, new prototype creation, later
unit deletion/reassignment, full map ownership rebuilding, final base refresh
and whole turns remain open.
