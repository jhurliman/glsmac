# Host-authoritative SMAC base-square rules

Explicit SMAC session initialization now reads `alpha.txt` through the existing
resource resolver (mod, data and installed-resource lookup) and parses the
three RESOURCEINFO triples. Reading and parsing finish before rules are
published or the session is locked. Failed reads/parses leave selection
retryable. Repeating initialization of a locked session does not reread files.
This is a settings-state retry guarantee: the resource resolver retains its
existing path cache, including failed resolutions. It does not promise hot
reload or discovery of a newly created file at a previously missing path.

SMAX initialization does not read `alpha.txt` and carries no SMAC numeric
profile. This means that profile is unavailable, not that SMAX uses neutral
values or that its arithmetic matches SMAC. Joining clients do not read either
rules file; they clear previous identity/rules and accept the host's values
through the existing State/Packet handshake before state callbacks run.

`GlobalSettings` owns a private optional `BaseSquareRules`, exposed by const
reference only. Selection/reset clears it; SMAC cannot lock or serialize without
it. The source callback in `Settings::PrepareRuleset` is the same seam used by
normal GLSMAC initialization and asset-free tests. No defaults are substituted.

Global-settings format version 2 stores the explicit identity, numeric-profile
presence and nine signed int32 values using the existing integer frame codec.
Old/unknown versions, mismatched presence, out-of-range values, truncated data
and unknown trailing fields are rejected. Parsing stages the map and remaining
fields before changing live settings. The currently empty default-rules block
must remain empty. These checks do not add gameplay bounds to map/economy data.

A locked session requires exact equality of identity and all nine rule values;
ordinary lobby updates with unchanged rules still work. State and Packet
serialization preserve signed endpoints. Native tests use synthetic source
text and the real Client dispatcher, including rejected host packets and
callbacks observing complete host rules. Protocol-error tests suppress only
network disconnect I/O; the actual decode, validation and error path run.

This step does not activate the SMAC center calculator or claim SMAX parity.
It does not yet add a rules header to the separate world-download snapshot.
Full world/save binding and supplying every calculator input remain separate
prerequisites.
