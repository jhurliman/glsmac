# Owned specialist catalog prerequisite

`SmacSpecialistRules` owns the seven positional SMAC `CITIZENS` records and
the minimum-size value from `RULES` row 11. Each record retains its source ID,
singular/plural display names, prerequisite/obsolete tokens, signed32
economy/psych/labs values, and opaque trailing token. Names do not establish
specialist identity. The three ordinary-citizen name rows establish the admitted
section shape and are not additional specialists.

This publication extracts the parser and owned value from source
`af35415aaa1791029bee59503bff0546a4a0d481`. The production header, implementation,
parser function, installed probe, and comparator are unchanged. Host settings,
world transport, Base population roles, eligibility, selection and allocation
are later consumers. This PR changes no Global/World version or live packet path.

## Admission and codec

The parser requires 70 `RULES` rows, seven eight-column specialist rows, and
three ordinary two-name rows. It rejects missing/extra fields, quotes, empty or
NUL-containing text, text exceeding 256 bytes, and malformed/out-of-range
signed32 decimal metadata. This is a strict port ingestion policy; the retained
original capture establishes installed outputs, not arbitrary malformed-input
equivalence. Duplicate display names are valid.

The independent V1 codec writes version, minimum size, row count and all seven
records in order. It retains signed values and source spelling. The exact
maximum is 9,604 bytes: three integer frames plus seven rows of four integer
frames and five bounded strings. Decode checks the aggregate bound, version,
count, integer domains, individual string bounds, positional IDs and exact end.
An outer caller must also bound its frame before copying it into this decoder.

`TechnologyReferences` resolves the two tokens per row through the existing
frozen technology catalog. `None`, `Disable`, a concrete ID and `Unknown` remain
distinct. This method does not test possession, obsolescence or specialist
eligibility, and it does not grant technology. The opaque trailing token has no
inferred effects.

## Evidence and publication validation

The opt-in `tools/tests/smac_specialist_probe.cpp` runs the actual parser,
standalone codec roundtrip and technology-reference resolution against a
caller-supplied local `alpha.txt`. `tools/reference/compare_smac_specialists.py`
pins the original loader report, literal fixture, original executable and
capture source fingerprints, and the supplied rules file. It compares seven
records plus the minimum-size value. Original reference IDs, names and outputs
are distinguished from retained source token/opaque metadata. No game assets
are committed, and this comparison does not rerun original code.

The original fixture SHA-256 is
`ef6a3611670cf27280afb99c637be9d57b70df7c835d0bae3c308f92f08c9797`;
the original report is
`9a0e4203d9bb96de02adf3519ec68bd03cd5cab4b49cbbf0a12ff150648486d1`.
The current publication's consistently Release-compiled (`-O2 -DNDEBUG`)
UBSan probe matched all seven records and the minimum, with exact codec restore.

Three native groups cover owned/copy/move metadata and token distinctions,
strict text shapes and signed boundaries, independent wire framing, every
truncation, version/count/domain failures, exact-cap and per-field bounds.
The pure parts of the source tests are retained. They use an isolated synthetic
RULES/CITIZENS fixture; source host/world checks remain with the deferred
transport integration instead of being represented as exercised here.

On the actual publication checkout based on
`7a5ce16af546004e316eced2788906e289e14d37`, all three groups passed with memory
tracking in normal (1.066 s) and ASan/UBSan/LSan (1.067 s, leak detection enabled)
Linux x86_64 builds. LLVM 15 analysis of the two changed production TUs completed
with zero failures (1.62 s analyzer time). Peer review checked the source
extraction and additive registrations without rerunning validation. These are
focused publication results, not a live consumer or cross-platform claim.

The same publication source also passed the full normal CTest suite, 8/8 tests
in 75.82 s. No full sanitizer suite or all-production-TU analysis was repeated
for this extraction.
