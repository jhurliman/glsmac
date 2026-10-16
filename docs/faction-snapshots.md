# Faction snapshots

Faction records use a typed `GLSMAC_FACTION` string marker followed by integer
version `1`. They contain the ID, display name, flags, ordered land and water
base-name lists, all three colors, and all existing base-render fields. Empty
lists, empty names and repeated names are preserved. These are configured game
data, not names copied into the repository from a purchased installation.

The previous unversioned record omitted flags and both name lists. Readers
reject that incomplete format instead of inventing defaults. Previous readers
also reject the new record at its integer version field. The outer manager's
count/key/record framing is unchanged; each nested record carries its version.

Decoding rejects unsupported markers/versions, unknown flag bits, negative or
impossible counts, render dimensions outside the nonnegative `size_t` domain,
truncated or incorrectly typed Buffer fields, and trailing bytes. Name and
manager counts are conservatively bounded by the remaining input bytes; no
storage is reserved from an untrusted count. Existing Buffer validation handles
frame lengths and checksums. Serialization rejects unknown flags and sizes
outside the signed snapshot-integer range.

An existing Faction changes only after the whole record validates. Its script
wrappers and callbacks retain their existing lifetime; only serialized fields
are replaced. The manager rejects duplicate IDs and disagreement between a key
and its nested record, and publishes each faction only after decoding it.
Manager restore still clears the old roster first, and earlier complete records
remain if a later record fails. This is not a transaction for the whole manager,
State or world, and does not redesign Player-to-Faction pointer ownership.
Allocation-failure recovery and semantic validation of render colors/scales
are outside this change.

Asset-free native GSE tests cover all four supported flag combinations, empty
and distinct name lists, existing fields, manager order and destination lookup,
every truncated record/manager prefix, malformed flags/counts/versions,
duplicate/mismatched IDs, trailing data, and unchanged destination fields after
rejection. They run in the existing full `--gse-tests` / `gse` CTest invocation.
