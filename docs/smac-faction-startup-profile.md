# SMAC faction startup source profile, version 1

`FactionTXTLoader::ParseStartupRules` produces an owned seven-faction profile
from explicit host source texts and a supplied SMAC technology catalog. This
first slice provides a pure parser, checked codec and exact equality. It does
not yet connect the profile to frozen settings, packets, world headers or
gameplay. Existing faction base-name loading and its cache are unchanged.

The canonical source bindings are explicit:

| Port identity | File | Section |
| --- | --- | --- |
| GAIANS | gaians.txt | GAIANS |
| HIVE | hive.txt | HIVE |
| UNIVERSITY | univ.txt | UNIV |
| MORGANITES | morgan.txt | MORGAN |
| SPARTANS | spartans.txt | SPARTANS |
| BELIEVERS | believe.txt | BELIEVE |
| PEACEKEEPERS | peace.txt | PEACE |

The parser validates all seven `#FACTIONS` filename/search-key pairs before
requesting a faction file. It also requires the canonical 29 `#BONUSNAMES`
labels and first `#SOCIO` row's eleven axis labels in order; it does not parse
social models. Unknown/custom bindings and lookup tables are unsupported,
not guessed from slot number or display name. The caller-supplied reader
controls file access. No local-client read or SMAX interpretation is added.

Each faction retains every accepted ordered keyword/value pair, its neutral
typed tag, signed32 parameter and SOCIAL/IMMUNITY amount. Raw text means the
fields after the declared file lexical policy: trim outer ASCII spaces, tabs
and CR, remove semicolon comments, skip full `##` comments, then split commas.
Keyword and social-axis labels must have canonical uppercase spelling. Empty
bonus rows are valid and do not consume the following ideology row. The
header is located but its personality/name fields are outside this profile.
Duplicate/missing sections, incomplete or empty pairs, NUL, unsupported keys
and invalid references fail ingestion before a profile is returned.

The admitted record-producing tags are named TECH, SOCIAL, FACILITY, IMMUNITY,
FUNGNUTRIENT and VOTES. `Records()` retains the first eight of these in source
order. All other accepted pairs are metadata-only: numeric TECH, ENERGY,
DRONE, TALENT, POPULATION, COMMERCE, FREEPROTO and FANATIC. They consume no
record slots. The profile retains supported pairs beyond capacity for exact
equality, even though `Records()` omits them. All other original keywords are
explicitly unsupported: skipping one could incorrectly admit a later TECH.

TECH first performs decimal-prefix conversion: ASCII whitespace, optional
sign, then decimal digits until the first other character; no digits means
zero. A nonzero result is the separate signed extra-technology count. The
last such assignment wins, including after eight records. It is never a
technology ID. A zero result uses the existing catalog token resolver,
including its seven-byte stored keys, case-insensitive first match and
trailing-ASCII-space-only lookup. None and Disable remain references -1 and
-2; known IDs 0..88 remain distinct. Unknown tokens, including numeric `0`,
reject instead of silently continuing an original error dialog. Named grant
duplicates are preserved. No ownership or research selection is performed.
`RE-SMAC-START-TECH-001` additionally confirms named references may cover all
89 keys, including disabled definitions and Future Tech. The native fixture
checks that metadata lookup never invokes ordinary availability/production
eligibility. It does not implement that handoff's research or possession resets.

SOCIAL stores its axis and leading-sign sum (zero sum means +1); IMMUNITY
stores its axis and amount 1. Other accepted numeric fields retain the
decimal-prefix value as metadata, without applying their effects. In
particular, FREEPROTO/FANATIC metadata does not define an effect from its
numeric argument. No intrinsic, current or retained social rating is created.

The port's strict ingestion policy is separate from original permissiveness:
decimal prefixes outside signed32 reject rather than relying on overflowing
CRT conversion. All accepted pairs are validated, including those beyond
the eight-record budget. Thus a malformed discarded record can reject the
profile even where the original would not inspect that record's parameter.
No calculation-domain restrictions, ownership defaults or startup state are
inferred from accepted signed metadata.

The typed version-1 codec writes the fixed seven identities followed by each
ordered raw/derived list. Decode checks count, field types, signed ranges,
canonical bindings, exact tag/parameter agreement with raw fields and the
supplied technology catalog, and trailing bytes. Untrusted counts do not
drive a reserve. All decoding is staged in a local value; publication is a
later caller action. This is not whole-world transactional restoration.

## Evidence and reproduction

The semantic basis is `RE-SMAC-START-FACTION-001`: the original bonus loop at
`0x59d3be..0x59db0e`, with supplied token readers and installed lookup tables,
executed with actual CRT comparison/conversion and technology resolution.
Its retained receipt contains sixteen parsed-record/extra-count results plus
twelve unrelated early-reset/difficulty results. This port compares only
the sixteen parser projections. The original unassigned third record fields
are intentionally not assigned gameplay meaning; the port's unused amount
field is canonically zero.

`RE-SMAC-START-FIRST-001` separately establishes the ordering reason for keeping
SOCIAL as metadata: named grants precede automatic designs and first-base
creation; late intrinsic SOCIAL construction follows those operations. Its
nine continuous caller compositions are not rerun by this parser test.
The later numeric-TECH award phase, possession, initial ratings/difficulty,
auto-designs, starting units, first-base production and full startup remain
separate implementation work.

Pinned provenance:

| Input | SHA-256 |
| --- | --- |
| SMAC executable | `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01` |
| alpha.txt | `38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4` |
| Semantic fixture | `b0b44e3fbb72012be8ac11f48216b867412aa140b3fb64054aab355c22204485` |
| Original receipt | `068bd93903781b0bdf4a06d25ad6f5b5f39cbac0e48bf66b79f01e06998648af` |
| Original probe source | `7db43d7ab6e12700ff39bb4c30052d1f4d7485f06bde47f8d59e3e2142dc9387` |

The receipt also pins each of the seven installed faction files. No original
files or executable bytes are distributed here. The test fixture contains
reduced semantic bonus pairs and synthetic headers/catalog metadata.

An opt-in local probe uses the actual production parser, without graphics:

```sh
clang++-15 -std=c++17 -O2 -DNDEBUG -ffunction-sections -fdata-sections \
  -Wl,--gc-sections -Isrc src/game/backend/settings/SmacFactionStartupRules.cpp \
  src/game/backend/settings/SmacTechnologyRules.cpp src/loader/txt/FactionTXTLoader.cpp \
  src/loader/txt/SmacRulesTXTLoader.cpp src/loader/txt/TXTLoader.cpp src/util/String.cpp \
  tools/tests/smac_faction_startup_probe.cpp -o /tmp/faction-startup-probe
python3 tools/reference/compare_faction_startup.py --probe /tmp/faction-startup-probe \
  --install /path/to/installed/SMAC --report /path/to/original-receipt.json \
  --fixtures /path/to/startup-faction-tech-and-social-inputs-cases.json
```

The comparator pins evidence and installed inputs, then compares exact ordered
record tuples/counts from the C++ parser. It does not execute the original or
claim its downstream ownership/social results. `--installed` reads the seven
canonical files; both tools require exactly one case-insensitive filename
match, rejecting missing/ambiguous files. `--lines` projects explicit synthetic bonus lines from stdin.
Probe argument errors exit 2; file/parse errors exit 1 without partial output.

Native regression command in the integrated development branch (asset-free):

```sh
./build/bin/GLSMAC --gse-tests --gse-tests-native-filter '^SMAC faction startup' --memorydebug
```

The focused publication base predates that filter and the later native tracking
repairs. On that branch, run `./build/bin/GLSMAC --gse-tests --quiet` for its full
suite. Its isolated validation uses a copied test-registration translation unit
containing only these seven cases; no production path or test body is changed.

Local validation used Linux x86-64, Clang/LLVM 15.0.7 and source parent
`445ee2094331babdab7e82528eb00f028a65ff26`:

- Full normal CTest: 8/8, 47.40 seconds. Subsequent test-only assertions
  strengthened empty-list roundtrips, valid-but-inconsistent tags and all 89
  named references with disabled definitions; all seven
  focused groups then passed again with allocation tracking.
- The final seven groups passed ASan/UBSan/LSan with allocation tracking and
  `detect_leaks=1`, using the existing local non-PIE sanitizer configuration.
- The Release C++ probe matched 16/16 retained original parser projections
  and 7/7 installed source bindings. Six argument/file/parse rejection
  controls produced their documented exit code without partial output. Both
  tools also passed mixed-case installed symlink lookup, ambiguous-case
  collision rejection and directory-only rejection on the Linux filesystem.
- LLVM's curated correctness policy reported zero failures for the two
  affected production translation units in 2.00 seconds.
- Three separately copied production translation units deliberately changed
  eight slots to nine, assignment to additive numeric TECH, and removed
  raw/derived validation. Each real native regression exited 1 at its intended
  assertion; the production source was never mutated for these controls.

These are local receipts, not cross-platform runtime promises. Shared frozen
settings and world-header integration must follow the separately owned
world-size profile change before clients can consume this metadata.

## Focused publication receipt

The publication is based on technology-catalog PR #114 at
`714bb937f8c7a4e47a5b0bd687ea38aee20483ef`. Applying source
`f99b43fd6b1ceddf86c37df7a72d86dfcd2be232` required only the existing test-list
union: one include, one CMake entry and one registration. Production files,
new tests, semantic fixture and probe/comparator are unchanged. Source patch
commit `b813c18ac988695ba2f7965f6d6bd77ca91c614c` passed:

- Actual publication CTest: 5/5, 27.06 seconds.
- All seven cases through a registration-only copied translation unit, then
  the three copied-production negative controls, each failing as intended.
- Release probe: 16/16 original parser projections and 7/7 installed bindings.
- Full Linux production analysis: 293 translation units, LLVM 15.0.7, four
  jobs, zero failures in 107.10 seconds. The explicitly Windows-only unit is
  outside that Linux database. The earlier two-unit check was focused evidence,
  not a substitute for this complete header-change scope.

The publication parent lacks later tracking/sanitizer repairs, so the integrated
source sanitizer receipt above is kept separate; this paragraph makes no full
publication LSan or allocation-tracking claim.
