# Ability metadata table

`UnitRulesTXTLoader::ParseAbilities(source, ruleset_id)` reads one `#ABILITIES`
section into an owned `AbilityTable`. The caller explicitly supplies `smac` or
`smax`; the parser does not select or authenticate a rules file. The vector keeps
source row order. Names and abbreviations are display metadata and may repeat;
the vector position is the ruleset-local source identity.

| Column | Retained value |
| --- | --- |
| 1 | Required name |
| 2 | Signed int32 cost metadata |
| 3 | Required unresolved prerequisite token |
| 4 | Abbreviation, including an empty string |
| 5 | Nonempty binary flags, preserving width and leading zeros |
| 6 | Required description, including embedded semicolons |

All values own their storage after the source is changed or destroyed. Copies
and moves preserve each record's contents independently. Neither the flags nor
source positions are mapped to runtime abilities or `#UNITS` ability bits.
Signed costs are preserved even outside the installed range; interpreting a
negative cost code, ordering surcharges, resolving prerequisites and enforcing
unit legality require separate contracts. No gameplay definition is activated.

## Deliberate ingestion policy

The existing section collector skips empty lines, full semicolon comments and
`##` translator comments, accepts LF/CRLF, and rejects a missing or duplicate
exact section. It preserves semicolons within a record. The ability-specific
splitter also preserves them: using the other component tables' inline-comment
splitter would truncate a legitimate description.

Records have six comma-separated fields, with surrounding whitespace trimmed
and at most one optional trailing comma. Quoting and escaped commas are not
supported. There is no inline-comment suffix: text after a semicolon in the
description remains description text. Additional comma columns are rejected.
Every field except abbreviation must be nonempty, and all record fields reject
embedded NUL. Costs require a complete signed decimal int32, including supported
leading plus/zeros. Flags accept only `0` and `1`; no installed width is imposed.

Missing/empty/duplicate sections, unknown ruleset identities and malformed
records throw `std::invalid_argument` with section/row/column context where
applicable. No partial table is returned if a later record fails. Standard
container ownership releases earlier records during unwinding. These strict
rules are GLSMAC's ingestion policy, not a reproduction of the original reader's
malformed-input behavior or a full rules-file grammar.

## Installed-file evidence

The opt-in probe below runs the actual production parser on the purchased local
GOG files. It prints aggregate metadata, not asset text. The checked files are:

| File | Bytes | SHA-256 | Rows | Flag width |
| --- | ---: | --- | ---: | ---: |
| `alpha.txt` | 53,073 | `38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4` | 24 | 11 |
| `alphax.txt` | 62,778 | `bb01e3ac23793da0a46713f4851d65e63b599c24aef8300ffe45e7e57b332c42` | 29 | 12 |

Each table retains one empty abbreviation and one semicolon-containing
description. Both have observed costs from -7 through 2. These are observations
of the two pinned inputs, not hardcoded row counts, width requirements, default
costs, original-reader execution or SMAC/SMAX gameplay equivalence.

On Linux/WSL with Clang 15, compile the standalone Release probe using dead-section
linking to omit unrelated TXTLoader/String application services:

```sh
clang++-15 -std=c++17 -O2 -DNDEBUG -ffunction-sections -fdata-sections -Isrc \
  src/loader/txt/UnitRulesTXTLoader.cpp src/loader/txt/TXTLoader.cpp \
  src/util/String.cpp tools/tests/ability_table_probe.cpp \
  -Wl,--gc-sections -o /tmp/ability-table-probe
/tmp/ability-table-probe smac '/installed/Alpha Centauri/alpha.txt'
/tmp/ability-table-probe smax '/installed/Alpha Centauri/alphax.txt'
```

Exit 0 means parsing succeeded; 1 reports file or parser failure; 2 reports
incorrect argument count. No original files are committed or downloaded.

## Regression scope

Four new native test groups cover complete semicolon descriptions and optional
abbreviations through the real section collector, input/copy/move ownership,
duplicate display metadata and source order, signed endpoints and binary width,
section/profile validation, and malformed late fields. They run with the existing
normal `--gse-tests` suite and need no original assets. This focused publication
base predates the native-filter CLI.

Local Linux/WSL validation:

- Full normal CTest passes 4/4 in 17.15 seconds, including the four new groups.
- The four groups also pass in a focused executable using copied registration
  translation units linked to the unchanged production/application objects.
  The same four groups pass ASan/UBSan/LSan with leak detection enabled in
  1.07 seconds. The sanitizer cache retains its local `-no-pie` startup
  workaround; this does not change the repository build policy.
- Two separate copied-production controls truncate descriptions at semicolons
  or omit binary validation. They fail two and one native assertions respectively
  and both return exit 1; the working production sources and normal executable are
  unchanged throughout.
- Clang 15 correctness analysis of the production translation unit in explicit
  Debug and Release modes reports no failures (3.97 / 3.67 seconds).
- The standalone Release probe parses both complete installed tables above.

Commands, logs and the installed-file manifest are retained locally under
`/home/jhurliman/src/glsmac-ability-table-audit/`. This parser makes no runtime
rendering, multiplayer, ability-cost arithmetic or full-game fidelity claim.
