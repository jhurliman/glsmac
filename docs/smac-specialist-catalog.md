# Frozen SMAC specialist catalog

`SmacSpecialistRules` owns the seven positional `#CITIZENS` specialist rows and
the configured minimum specialist size from `#RULES` row 11 (zero-based 10).
`GlobalSettings::InitializeHostRules` invokes the production parser on the same
host-supplied `alpha.txt` already used for the other profiles. Joining clients
and snapshot restoration use the transmitted value; they do not read local
rules files. SMAX carries explicit absence.

Each specialist retains its source ID, singular/plural names, prerequisite and
obsolete tokens, signed32 economy/psych/labs outputs, and opaque trailing token.
Duplicate display names are valid. Identity remains the positional ID 0..6.
`TechnologyReferences` resolves both owned tokens through the supplied frozen
technology catalog, preserving None, Disable, known ID and Unknown as distinct
results. This is a catalog query, without ownership or eligibility inference.

The last three ordinary-citizen name pairs validate the supported text shape;
they are not additional specialist records. The trailing specialist token has
no demonstrated stored semantic field in the original loader and is not used
as an ability or availability mask. The minimum size is metadata for later
consumers, not a universal prohibition on specialists below that size. The
automatic selector, manual replacement, and tile competition have different
conditions; none is activated here. No worker allocation, PopDef mapping,
specialist selection, or default Doctor choice changes.

## Text and wire policy

The strict port ingestion accepts one unique 70-row RULES section and one
unique ten-row CITIZENS section. The configured rule is selected by section
position, never its physical file line or an English comment. Each specialist
has exactly eight comma-separated fields; the last three rows have two names
and an optional trailing comma. Numeric fields are complete signed32 decimal
integers, including one optional leading plus. Quoted fields, missing/extra
columns, empty metadata, embedded NUL, oversized strings and out-of-range
integers reject. These are GLSMAC ingestion policies, not claims about the
original malformed-input reader. Names and tokens are trimmed by the existing
section/field reader; no original display-string localization semantics are
inferred.

Specialist V1 writes version, minimum size and count, followed by seven records
in source order. Every record writes ID, four strings, three output integers,
and the opaque string. Strings are at most 256 bytes. The exact maximum is
9,604 bytes: three 14-byte integer frames plus seven records of four 14-byte
integer frames and five strings with six-byte framing and 256-byte payloads.
Decode checks the complete budget, then each string bound before copying and
each integer bound before narrowing. Count/order/trailing-byte validation
completes before returning an owned value.

Global settings V11 appends explicit presence and the bounded specialist frame
after the movement profile. Host preparation stages the complete set of
profiles and existing aggregate packet admission before publishing it. Decode
stages the specialist value and the existing mutable tail, and locked sessions
compare every raw field before publication. The existing complete
`PT_GLOBAL_SETTINGS`/State/FactionManager framing ceiling remains authoritative;
the per-profile limit does not exempt any packet from that ceiling.

World V17 appends the same presence/frame after movement. The writer validates
and pre-encodes it before appending any header bytes. The real decoder bounds
the outer frame and checks full equality before map allocation or body
installation. Old/unknown global and world versions reject explicitly. The
simulation payload is unchanged by this slice (source ancestry uses V5).
This is prefix rejection, not transactional rollback for arbitrary later world
body or setup failures.

## Original comparison

The opt-in `tools/tests/smac_specialist_probe.cpp` executes the actual installed
parser, owned codec roundtrip and frozen technology resolver.
`tools/reference/compare_smac_specialists.py` consumes literal retained output
records; it does not import the original checker or calculate a substitute
oracle. It pins the executable, source, fixture, report and installed alpha
fingerprints, requires the normal loader return/read counts, and compares all
seven ordered rows plus the configured minimum.

The separate read-only evidence is
`docs/behavior/installed-specialist-catalog.md` and its machine-readable
supplement. Its fresh complete original rules-loader mode0 capture produced
seven stored name/reference/output records and minimum 5. Names and plural
names, 14 reference IDs and 21 numeric outputs are captured results. Source
keys and opaque trailing tokens are separately retained source metadata.
The last three ordinary names are validated context, not additional compared
capability rows. This does not rerun original code or reproduce dynamic
specialist selection or a complete first-base operation.

Pinned inputs:

- Original executable: `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
- Installed alpha: `38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4`.
- Original checker: `4a0e02937cc2cef8efaedbeb5e80ce248f3422f2bc22a9626fbe76ce366eb405`.
- Public fixture: `ef6a3611670cf27280afb99c637be9d57b70df7c835d0bae3c308f92f08c9797`.
- Retained original report: `9a0e4203d9bb96de02adf3519ec68bd03cd5cab4b49cbbf0a12ff150648486d1`.

No installed source or private report is committed. The original receipt's
minimum is the automatic selector's configured threshold; it does not replace
the separate fixed population-five tile-competition gate.

## Validation boundaries

The five dedicated native groups exercise ownership/copy/move, duplicate names,
reference distinctions, signed endpoints, malformed section shapes and every
numeric field, every codec truncation, ordered identities, independent frame
bytes, exact-cap admission and nested checksum/version failures, individual
string bounds, all 57 mutable metadata fields, host retry after a late source
failure, and writer preflight against deliberately corrupted test-owned state.

The existing actual Client settings dispatcher now includes 20 additional
specialist mismatch/malformed/version scenarios, alongside callback-visible
accepted metadata and SMAX absence. Actual immediate/queued final download
ingress exercises specialist prefix rejection while retaining the prior map,
rules and callback counts. Large download fixtures preload prior chunks and
dispatch the real final packet; they do not claim transport coverage of those
preloaded chunks. Existing actual writer/body/InitComplete tests roundtrip the
new frozen profile. These tests use synthetic source/map setup and do not
claim rendered startup or socket interoperability.

The source checkpoint is based on
`2f3b707bc431310d34c82912dda2ab5bcea5b4cf`. Final normal CTest passes 8/8 in
97.50 seconds; the full leak-enabled ASan/UBSan/LSan suite passes 8/8 in
205.72 seconds. The 26 focused catalog/host/header/actual Client groups pass
with allocation tracking in both the normal and ASan/UBSan/LSan builds.
The full 339-production-TU LLVM15 correctness scan reports zero failures in
171.26 seconds. The optimized UBSan installed probe matches all seven records
and the minimum, including the actual codec roundtrip.

Three separately compiled copied-production controls fail their intended
assertions: selecting RULES row12, omitting trailing-token equality, and
omitting the outer world-frame bound. The source checkout was not mutated for
these controls. Earlier full-suite attempts exposed old host/header fixture
walks that assumed movement was the final profile; they were corrected while
retaining their cursor assertions. An initial CTest invocation also lacked the
local dependency library path and exited before tests; that log is retained.
Final receipts use the corrected environment and fixtures. No findings from
those attempts required a production correction.

Logs, the original-comparison receipt, copied controls and final hash manifest
are retained under `/home/jhurliman/src/glsmac-unit-weapon-parser/` with the
`specialist-` prefix. The source is independently reviewed; cross-platform
integration and publication remain separate parent-task gates.
