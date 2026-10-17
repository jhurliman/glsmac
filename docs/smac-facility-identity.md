# Positional facility identity

`FacilityDef` now owns an optional `smac_source_id` identifying a position in the
frozen SMAC `FACILITIES` catalog, 1 through 71. Port IDs and display names remain
independent. This gives resource consumers an explicit way to recognize an
installed facility even if its text or port ID changes. It does not establish
eligibility, construction cost, maintenance, project ownership, orbital counts,
or a facility grant at founding.

The native constructor, GSE binding, and decoder check the range before narrowing.
Manager admission requires locked SMAC rules and the owned production catalog.
Each source position can bind to only one registered definition; duplicate port
IDs also reject at runtime. Population and facility IDs occupy independent
namespaces. An absent source remains a legacy/unavailable identity and is never
inferred from names.

The default definition producer binds Headquarters to position 1 and Recycling
Tanks to position 3 when the selected ruleset is SMAC. It leaves identity absent
for SMAX. No other facility definitions or automatic grants are added here. The
catalog loader already preserves the distinction between local facilities,
orbital entries, Stockpile Energy, and projects; a source identity alone gives a
consumer no permission to treat these categories as equivalent.

The BaseManager snapshot marker advances to `GLSMAC_BASES_V6`, following the V5
base-context/population-role change. Facility records append strict presence and
position fields. Unknown/older manager versions, truncated suffixes, invalid
booleans, invalid positions, and trailing definition bytes reject. Restored Base
facility pointers refer to the restored manager definitions with the saved source
identity. The schema is GLSMAC's format, not an original-game save format.

Definition creation and decoding retain candidate ownership until successful
registration. Removing a definition now deletes the owned object, rejects unknown
IDs, and refuses to remove a definition referenced by a registered Base. This
closes the existing facility-undefine leak and prevents installed dangling
pointers. It does not claim a manager-wide transactional restore.

Five dedicated native groups exercise all 71 catalog positions and explicit
absence, malformed framing and wide integers, profile/mode rejection, duplicate
identity ownership, failed later definition records, independent population IDs, actual GSE binding, the default
definition producer and event handler, and actual manager/Base restoration.
Only synchronous delivery replaces the event queue in the producer test; the
production script, validation/apply handler, and native manager execute unchanged.
The restore test verifies that its manager-only operation preserves the target's
existing gameplay RNG; world RNG restoration is a separate responsibility.

Final validation receipts are recorded with the integration checkpoint. Initial
test-only corrections fixed assertion-macro string concatenation and a test that
incorrectly compared two independently initialized world RNG streams during a
manager-only restore.
