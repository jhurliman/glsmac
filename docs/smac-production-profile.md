# Frozen SMAC production configuration

An explicit SMAC host now reads `alpha.txt` once through the existing resource
resolver and stages both the base-square rules and a production profile before
publishing either. Joining clients accept the complete host profile through the
existing State/Packet/Client path; they do not read local rules files. SMAX reads
neither SMAC file nor profile, and no SMAX equivalence or neutral values are implied.

This supplies authoritative configuration for later production admission and
arithmetic. It does not yet initialize a base's production, resolve a buildable
target, post minerals, complete a facility, or change the UI. Existing scripted
`FacilityDef` costs are unchanged and are not the parsed unscaled catalog costs.

## Source contract and provenance

The read-only source audit used the installed GOG SMAC `alpha.txt`, 53,073 bytes,
SHA-256 `38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4`.
No installed file or human-owned research document is modified or committed.
The parser does not require this hash: the resource resolver may supply modified
rules, which must satisfy the declared layout.

`SmacRulesTXTLoader::ParseProductionRules(source)` requires exactly one requested
`#RULES` section with 70 data rows and one `#FACILITIES` section with 71 rows.
The shared section reader supports LF/CRLF, whitespace and full-line semicolon
or `##` comments; duplicate unrelated sections are irrelevant. Identity is
positional, as in the observed source, rather than inferred from annotations or
display names. Missing/extra rows are rejected rather than silently renumbered.

| One-based RULES row | Retained value |
| --- | --- |
| 7 | Mineral cost multiplier |
| 43 | Retool loss percentage |
| 44 | Retool strictness |
| 45 | Retool mineral exemption |

Each selected row contains a complete signed decimal int32, an optional trailing
comma and optional semicolon annotation. All other RULES values are outside this
parser's contract. Explicit plus/minus signs and signed endpoints are retained;
empty, fractional, hexadecimal, partial and out-of-range numbers are rejected.

Each facility's fixed five-field prefix is name, unscaled cost, maintenance,
prerequisite token and obsolete Free token. All five must be nonempty and contain
no NUL; cost and maintenance are complete signed int32 decimals. Only the first
four fields are retained. The delimiter after Free is required; the remaining
effect text and any project AI suffix are uninterpreted. Effects can contain
commas, so the parser does not impose an incorrect six-column count.

The resulting owned records contain source IDs 1 through 71 in exact order.
SMAC slot ranges are local facilities 1–33, orbitals 34–37, Stockpile 38 and
projects 39–71. These are source catalog identities, not port `Def` or
`ProductionItem` string IDs. Display names may repeat or be renamed. A later
explicit adapter must map supported port IDs and reject unknown/unresolved IDs.

Prerequisites remain exact unresolved tokens, including `None` and `Disable`.
The installed Headquarters and Stockpile rows have `None`; this does not grant
build eligibility. Technology ownership/enabled definitions, duplicate-facility
checks and other original admission gates still require real authoritative inputs.

All signed32 numeric metadata is preserved, including negative costs/multipliers,
out-of-domain strictness and loss percentages. This is an ingestion/storage
domain, distinct from the supported arithmetic and admission domains. Existing
calculators reject unsupported inputs; no clamping, positive-cost assumption or
default gameplay context is introduced here.

The field meanings connect to original-execution handoffs RE-SMAC-MINERAL-COST-001,
RE-SMAC-RETOOL-001 and RE-SMAC-FACILITY-ELIGIBLE-001 for pinned `terran.exe` SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
This slice reads the installed text and exercises the port parser; it does not
rerun those executable routines or reproduce the original loader's malformed-file
policy. Its exact row/prefix requirements are explicit GLSMAC ingestion policy.

## Host and wire ownership

`GlobalSettings` owns the private optional `SmacProductionRules` by value and
exposes only a const reference. Selecting/resetting a ruleset clears both profiles.
SMAC cannot lock or serialize without both; initialization of an already locked
session does not reread files. File or late-row parsing failure leaves identity,
profiles, lock and all other settings unchanged. Existing resource-path caching
is unchanged; this is not a hot-reload guarantee.

Global-settings **version 3** replaces V2. Its SMAC presence flag governs both
the existing nine base-square integers and a framed production-profile string.
That string has its own **version 1**, four rule integers, exact count 71, and
71 records of source ID, name, cost, maintenance and prerequisite. Every numeric
frame is checked in int64 before narrowing; source IDs must match their positions.
Unknown/old versions, wrong frame types, counts, IDs, empty/NUL text, truncation
and trailing fields are rejected. No untrusted count is used for allocation.

Decode stages the entire profile, map and remaining settings before publication.
A locked session compares every modeled production field exactly, along with
the existing ruleset and base-square values. Ordinary lobby updates with an
identical profile still work. Wire failures use the real Client protocol-error
path, and rejected settings do not release callbacks or reach faction decoding.
This is atomic global-settings parsing, not whole-State transactionality: a later
faction-body failure remains governed by the existing State decoder.

This publication ancestry has no rules header in the separate world download.
Binding both profiles into a versioned world header is a required separate
follow-up before production activation. No full snapshot, replay or
base-production rollback claim is made.

## Validation

Native asset-free tests exercise both parser and codec, signed endpoints,
canonical row identities under renamed/duplicate names, unresolved prerequisites,
all numeric/text wire fields, every locked catalog field, and early/late rejection
without settings mutation. Shared host fixtures now include synthetic complete
production data; the real State→Packet→Client cases check callback visibility,
joining/rejoining, changed locked catalog fields and malformed production payloads.

An opt-in local probe reads only the explicitly supplied file:

```sh
clang++ -std=c++17 -O2 -DNDEBUG -ffunction-sections -fdata-sections \
  -Wl,--gc-sections -Isrc src/game/backend/settings/SmacProductionRules.cpp \
  src/loader/txt/SmacRulesTXTLoader.cpp src/loader/txt/TXTLoader.cpp \
  src/util/String.cpp tools/tests/smac_production_profile_probe.cpp \
  -o /tmp/smac-production-profile-probe
/tmp/smac-production-profile-probe /private/alpha.txt
```

It prints the four rules and all modeled catalog records as tab-separated output;
it does not authenticate the file, create definitions or modify an installation.
Local validation used Clang/LLVM 15 on Linux through WSL:

- The final full normal suite passed 8/8 in 41.76 seconds. The full
  ASan/UBSan/LSan suite passed 8/8 in 54.68 seconds with `detect_leaks=1`.
  Twelve selected host/profile/protocol cases also passed with allocation
  tracking in both builds, with no tracked memory/OpenGL leaks. The sanitizer
  build retained the existing local non-PIE workaround; repository build policy
  is unchanged.
- Full production correctness analysis covered 306 translation units with zero
  failures in 108.88 seconds using four jobs. These are local measurements, not
  cross-platform runtime promises.
- The Release probe's four rules and all 71 modeled facility records matched
  independent field extraction from the hash-pinned installed file above.
  Missing arguments/file and malformed-source controls produced nonzero exits
  without successful output. No original executable was run for this slice.
- Three copied-production controls each returned exit 1 at the intended native
  assertion: reading the wrong RULES position, accepting changed frozen rules,
  and publishing base-square rules before a late production parse failure.
  The intended checkout source was never replaced by these controls.

Receipts remain under `/home/jhurliman/src/glsmac-production-profile-audit`.
The first final-script attempt omitted the managed worktree's local Git metadata
override and failed during version-header generation; the corrected invocation
produced the final normal result above. CI requires no original assets.
