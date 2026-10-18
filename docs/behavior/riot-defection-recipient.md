# RE-SMAC-RIOT-RECIPIENT-001: defection recipient admission

Status: original-x86 checked for the SMAC fingerprint in this directory's
README. All 22 named, 1,200 sampled and 20 integrated failed-facility-search
checks pass. These stop immediately before ownership mutation. They establish
the recipient decision, not completed defection or its cleanup.

## Post-search gates and former owner

This stage follows an unsuccessful [riot facility search](riot-facility-loss.md).
Proceed only when current POLICE is at most -3 and the current faction is
human-controlled. Otherwise the handler exits this path.

If the base's former owner differs from its current owner, select that former
owner directly. This bypasses the geographic search. The selected value must
be positive to admit transfer. Former owner zero therefore stops this path
without trying nearby factions. A positive former owner can be admitted even
when it currently has no bases; no alive/base-count/relationship test is made
in this local selection branch.

If former owner equals current owner, search the live base list for a
recipient. Current-owner bases are ignored. Other bases, including a supplied
faction-zero record, enter the candidate comparison. The final selected owner
must still be positive. A winning faction-zero candidate can therefore stop
transfer even when a farther positive-faction candidate exists.

## Geometry and strict improvement

Use doubled-column coordinate deltas, with horizontal seam wrapping on a
cylindrical map. As in the [HQ distance rule](energy-waste-and-headquarters-distance.md),
the geometric distance is `floor((3*max(dx,dy)+min(dx,dy)+1)/4)`.

Visit candidates in global base-array order. The initial best score is 9999.
Replace the best recipient only when a candidate's weighted score is strictly
smaller. Equal scores retain the earlier base. A candidate with score 9999
or more cannot become the first best; synthetic large-map cases check 9998,
9999 and a greater score.

## Confirmed weighting dependency

The original uses the current best recipient as the source of relationship
weighting for the next candidate. It does not use that next candidate's own
relationship until that faction has become best. Before any recipient is
selected, weighting instead uses three low-bit conditions of a current-faction
activity stamp. This is confirmed original behavior in both isolated selection
and full riot-handler runs through failed facility searches.

While no recipient has been selected, use these conditions from supplied
`current_faction_stamp`:

- If `stamp & 1` is nonzero, double the distance.
- If `stamp & 2` is nonzero, multiply by three and divide by two, rounding down.
- If `stamp & 4` is nonzero, divide by two, rounding down.

Once a recipient has been selected, use the corresponding conditions from the current best
recipient's directed relationship: pact doubles, treaty multiplies by three
halves, and the resolved third `half_distance` flag halves. Apply those
operations sequentially in that order. The third flag's full diplomatic
identity remains unresolved; the fixture names its checked arithmetic effect.

The activity stamp is a runtime value written by founding and other actions.
Its complete meaning and update lifecycle remain open. The fixture supplies
it explicitly. Preserving this pinned-reference dependency requires those
inputs; replacing it with independent per-candidate diplomatic weights changes
the observed result.

Example: with stamp zero, candidate A has geometric distance 9 and owner 2;
candidate B has distance 7 and owner 3. If the current faction has a pact flag
toward owner 2 but not owner 3, visiting A first stores score 9. B then receives
the current best's pact weighting and scores 14, leaving owner 2 selected.
Visiting B first instead leaves owner 3 selected. Candidate order matters.

## Transfer boundary and scope

Once a positive recipient is selected, the next original stage begins changing
ownership. No further eligibility condition appears between this checked
admission and that first owner write. These tests stop before the write;
assimilation reset, production changes, base counts, units, territory,
facilities, notifications and later cleanup are not established here.

The [22 named cases](riot-defection-recipient-cases.json) cover policy/human
gates, former-owner priority, missing/zero recipients, tie/order behavior,
initial stamp, relationship weighting, wrapping and the score bound. Sampled
cases vary those inputs and candidate lists. Comparisons include every weighted
candidate score and the source recipient used for its weights, not only the
final result. The base record must remain unchanged at the stop boundary.

The isolated selection tests use original instructions without substituted
calls. Twenty integrated cases additionally execute original riot entry and
unsuccessful facility searches with supplied zero RNG returns and omitted
nonmutating notification/map callbacks. Prior riot admission, full RNG,
callback interventions, complete defection and full turns remain outside these
selection checks. The following [initial transfer mutations](riot-defection-transfer-core.md)
now have separate original-x86 coverage, stopping before subsequent helpers.
