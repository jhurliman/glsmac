# Founding on an existing worker square

2026-11-11 static caller audit, pinned stock SMAC. Reuse
[unavailable squares](worker-square-unavailability.md),
[worker orchestration](worker-allocator-orchestration.md), and
[complete manual founding](manual-founding-continuation.md).
No new runtime donor-founding fixture is claimed.

An existing worker assignment is **not a founding refusal** in these native
founding/creator paths. Do not reject legal human founding merely because
another Base currently selects that square. Preserve the separate terrain,
spacing, territory/diplomatic, capacity and Colony admission gates.

The source order is important:

1. Creator `4fa490` sets the center Base marker at `4fa4c4`..`4fa4e0`, sets
   occupying owner through `5a8f60`, and updates memory through `5a9290`.
   This is before new Base count/record initialization and worker allocation.
2. It initializes the new record, facilities and production, updates its
   radius via `4f9c80`, runs regional/territory rebuild `538e80`, then runs
   **the new Base's** support → intake → nutrients → minerals → energy at
   `4faa9e`..`4faab2`. This is not an explicit resource pass over every donor.
3. Intake starts with unavailable-square builder `4fc140`. Any valid Base
   center is unavailable to ordinary radius workers (`4fc2cb`..`4fc2df`),
   regardless of center owner. The evaluating Base's own mandatory center
   remains special; it is still collected by center/repair logic.
4. Whenever an existing donor next runs actual intake, its selected mask is
   intersected with the complement of unavailable squares and center bit1 is
   forced on (`4fd6c9`..`4fd6de`). Thus the founding square drops out of the
   donor's ordinary workers. The existing allocator retains legal choices,
   fills the freed population slot or uses its specialist fallback, commits
   selection/intake and creates the normal resource snapshots. Do not use an
   arbitrary nearest replacement square or decrement population.

Same-owner versus foreign distinctions belong to the existing reservation and
territory builder, not a special donor-stealing refund. Other same-owner Bases
reserve their selected squares **and their centers**. Other-owner reservations
apply to sea workers, while ordinary foreign land workers do not reserve land
through that pass (`4fc56f`..`4fc5a1`). Nevertheless a **new actual Base center**
is blocked for both owners by the earlier universal Base-marker test; recognized
foreign territory may exclude further squares independently. The new Base's
initial allocation can see still-retained donor reservations until that donor
is actually reprocessed. Preserve the native pass order when matching choices.

The audited admitted action `4df560` and creator `4fa490` have **no explicit
synchronous donor resource/production transaction** around this operation.
Do not claim they immediately reallocate every donor from this source audit.
Port cache invalidation should make the changed center/territory visible to
the next actual donor allocation. If the port eagerly refreshes donors for UI
consistency, that is an integration policy and may change order-sensitive
allocations; it is not established as the native founding caller's behavior.

No donor food/mineral-store, production queue, production-loss baseline or
completion/refund adjustment is made merely because this worker became
unavailable. Later donor resource calculation changes derived intake/balances;
normal upkeep remains responsible for advancing production/stores. The new
Base's own production initialization remains its separate retained contract.

Private causal source is frozen in
`tmp/reference/independent-first50-dependencies-static-evidence-20261111/`.
