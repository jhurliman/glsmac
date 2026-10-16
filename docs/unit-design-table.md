# Unit design records

`UnitRulesTXTLoader::ParseUnits(source, ruleset_id)` decodes one `#UNITS`
section into a value-owned `UnitTable`. The caller explicitly supplies `smac`
or `smax`. Records retain source order; display names may repeat. Parsing does
not resolve components, choose a reactor, calculate costs, interpret abilities,
load art, or create playable definitions.

The source audit used the installed files and hashes recorded in the
[weapon parser document](unit-weapon-table-parser.md). The inspected `alpha.txt`
has a count of 14 and fourteen records; `alphax.txt` has a count of 23 and
twenty-three records. Each record has ten columns:

| One-based column | Retained metadata |
| --- | --- |
| 1 | Display name, including any leading `*` |
| 2 | Chassis reference |
| 3 | Weapon/package reference |
| 4 | Defense reference |
| 5 | AI plan |
| 6 | Cost |
| 7 | Carrying capacity |
| 8 | Prerequisite token |
| 9 | Icon |
| 10 | Raw binary ability string |

Although the source comment calls components indices and names an `ARMOR`
list, the inspected records contain textual references. All 111 references
across the 37 installed records match exact keys in the same rules file:
the first chassis name, weapon short name and `#DEFENSES` short name. A future
resolver must keep catalog identity and ownership explicit. This decoder
preserves the strings even if their referenced tables are absent. It does not
case-fold names or remove apostrophes (`'Copter` and `Copter` differ between
the inspected catalogs).

Both installed tables begin with Colony Pod, Formers and Scout Patrol. They
use Infantry with the Colony Pod, Formers and Gun weapon/package keys,
respectively, and the Scout defense key. Their plans are 8, 9 and 3. These
observations are read-only probe checks, not hardcoded rows or defaults. There
is no reactor column. Initial-reactor policy remains unresolved.

The original comment enumerates AI plans -1 through 12: auto calculation,
offensive, combat, defensive, reconnaissance, air superiority, planet buster,
naval superiority, naval transport, colonization, terraforming, supply convoy,
information warfare and alien artifact. This parser checks that set before
conversion to `UnitPlan`. It implements no AI behavior. Cost, carry and icon
are signed decimal int32 metadata, including negative values and endpoints.
The original cost/carry comments designate zero as automatic calculation;
zero remains zero here. Prerequisite tokens such as `None` and `Disable` are
preserved without technology lookup or eligibility checks.

Ability strings must contain one or more `0`/`1` characters. Width and leading
zeros are retained exactly; they are not converted to an integer or mapped to
ability IDs. Every inspected SMAC record has 24 characters and every SMAX
record has 26, while their `#ABILITIES` tables have 24 and 29 entries. Therefore
neither a universal width nor equality with the ability-table length is
assumed. No bit meaning is inferred from unit names or visual correlation.

The first noncomment section row must be a complete signed decimal int32
count, nonnegative and equal to the number of following records. Zero with no
records is valid. The count is checked against the available rows before any
record allocation based on it; a huge declared count cannot trigger a catalog
reservation. The count line may have an inline semicolon comment but is not a
comma-separated record. Unit records accept exactly ten fields and one optional
trailing comma. Required text must be nonempty and NUL-free. Section, whitespace,
comment and EOF handling follows the existing [component decoders](unit-defense-reactor-tables.md).
Missing or duplicate target sections, count mismatches, malformed numbers or
fields, unsupported plans and nonbinary ability strings throw
`std::invalid_argument`. Diagnostic data-row numbers include the count at row
1, so the first unit is row 2. No partial table escapes. These are explicit
GLSMAC ingestion policies, not claims about the original loader's handling of
malformed or modified inputs.

Native regressions cover every field, explicit identities, repeated display
names, unresolved exact references, all documented plans, signed endpoints,
variable-width ability strings, zero/mismatched/invalid counts, comments and
each malformed column. A read-only Release probe checks both installed tables,
the initial three records, ability widths and all component references. No
original tables or assets are redistributed. Catalog resolution, reactor
selection, calculated cost/carry, ability interpretation and live unit
activation remain separate work.
