# Defense and reactor component tables

`UnitRulesTXTLoader::ParseDefenses(source, ruleset_id)` and
`ParseReactors(source, ruleset_id)` add two pure component catalogs beside the
[weapon/package decoder](unit-weapon-table-parser.md). Each requires an explicit
`smac` or `smax` identity and returns value-owned metadata in source order.
There is no file selection, cache, live unit activation, technology resolution,
asset loading, or design construction.

The read-only audit used the same installed `alpha.txt` and `alphax.txt` hashes
recorded in the weapon parser document. The actual schemas are:

| Section | Columns | Observed SMAC/SMAX rows |
| --- | --- | --- |
| DEFENSES | Full name, short name, rating, mode, cost factor, prerequisite | 10/14 |
| REACTORS | Full name, short name, power, prerequisite | 4/4 |

The reactor comment lists only three fields, but the records contain a short
name before power. It must be preserved. Defense mode 0 means projectile,
1 energy, and 2 binary; a separate enum prevents conflating binary defense with
missile weapon mode 2 or accepting noncombat package modes as armor modes.
The installed `No Armor`/`Scout` record has rating 1, mode 0 and cost factor 1;
it is the defense referenced by all three initial conventional designs. The
installed `Psi Defense` record retains rating -1. Both reactor catalogs list
Fission, Fusion, Quantum and Singularity in that order with power 1 through 4.
These observations do not establish automatic reactor selection for `#UNITS`,
which has no reactor column.

## Parsing contract

The existing weapon field/section helpers now serve all three tables. Existing
weapon regressions are retained. Required sections must occur exactly once
and be nonempty; unrelated duplicate sections remain permitted. LF/CRLF, line
whitespace, blank lines, full-line `;`/`##` comments, inline semicolon annotations,
EOF termination and one optional trailing comma retain the same behavior.

Rows require exactly six or four fields respectively. Names and prerequisite
tokens must be nonempty and NUL-free; exact short-name duplicates within a table
are rejected. There is no shared ID registry across catalogs, case folding, or
display-name uniqueness requirement. Full names, short names and prerequisites
are preserved after whitespace trimming. Complete signed decimal int32 fields
are required; explicit plus/minus signs and representable endpoints are accepted.
Defense modes must be 0, 1 or 2 before enum conversion.

Rating, cost and reactor power are structural signed metadata. Zero/negative
power and values outside the installed 1..4 range are retained by this decoder;
that does not make them eligible for a playable design. Future design assembly
must establish and enforce its original-backed power/cost constraints before
using these fields in calculations. No observed table counts or default values
are imposed here. A malformed table throws `std::invalid_argument` with field
section/row/column context, and no partial catalog is published.

## Validation and next boundary

Synthetic native fixtures exercise real production parsing for both rulesets,
including distinct rating/cost fields, every defense mode, psi rating, reactor
short names, preserved order, independent catalog identities, signed endpoints,
truncated/extra columns, invalid text, duplicate IDs and malformed numbers in
every numeric column. A local Release probe reads the installed 10/14 defense
and 4/4 reactor records without redistributing the files. Tests specify GLSMAC's
strict ingestion policy; they do not reproduce original malformed-file behavior.

CHASSIS has nineteen fields: six name/gender pairs, speed, triad, range, missile,
cargo, cost and prerequisite. It remains a separate parser slice before a
cross-table design resolver. No current `StaticDef`, ownership/snapshot format,
production cost, conventional art or gameplay behavior changes in this slice.
