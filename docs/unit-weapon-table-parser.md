# Unit weapon and package table parser

`loader::txt::UnitRulesTXTLoader::ParseWeapons(source, ruleset_id)` decodes one
`#WEAPONS` table into a value-owned catalog. Callers must explicitly supply
`smac` or `smax`. The result retains that identity and source row order; an
index is local to that catalog. This API does not select or authenticate a
file, cache content, load assets, change session settings, or create a live
unit definition.

This is the first component-loading prerequisite for conventional units. The
installed `#UNITS` definitions reference a chassis primary name, weapon/package
short name, and defense short name. Scout Patrol, Colony Pod and Formers all
use Infantry and Scout defense; their weapon/package references are Gun,
Colony Pod and Formers, with modes 0, 8 and 9 respectively. Package mode is
explicit metadata, independent of display names and row indices. These facts
do not establish their production cost, founding behavior, terraforming rules,
combat behavior, or rendering.

## Observed source layout

The read-only audit used the user's GOG installation. No installed files or
asset bytes are included in the repository.

| File | Bytes | SHA-256 |
| --- | ---: | --- |
| `alpha.txt` | 53073 | `38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4` |
| `alphax.txt` | 62778 | `bb01e3ac23793da0a46713f4851d65e63b599c24aef8300ffe45e7e57b332c42` |

Both files use seven weapon columns: full name, short name, offense, mode,
cost factor, icon, and prerequisite token. Some rows have a trailing comma;
the Colony Module row has a semicolon annotation. The original table comment
does not enumerate the icon column, so the actual records were checked too.

| Component table | SMAC rows | SMAX rows |
| --- | ---: | ---: |
| CHASSIS | 9 | 9 |
| REACTORS | 4 | 4 |
| WEAPONS | 21 | 26 |
| DEFENSES | 10 | 14 |
| ABILITIES | 24 | 29 |
| UNITS | 14 | 23 |

The observed Colony Module and Terraforming Unit indices are 14/15 in SMAC
and 17/18 in SMAX (zero-based); Hand Weapons is index 0 in both. The parser
does not hardcode these indices or the observed row counts.

## Ingestion contract

The parser requires one nonempty `#WEAPONS` section. The existing pure
`TXTLoader::ReadUniqueSection` helper handles LF/CRLF, surrounding whitespace,
blank lines and full-line semicolon/`##` comments. A subsequent section header
or EOF ends the table. Duplicate unrelated sections are ignored. The legacy
faction loader and cache are unchanged.

Each row must contain exactly seven fields, with at most one optional empty
trailing field from a final comma. Text fields must be nonempty and contain no
NUL. Full names, short names and prerequisite tokens are otherwise preserved
after whitespace trimming. A semicolon begins an annotation; quoting, embedded
commas and semicolons inside names are unsupported. Exact duplicate short names
are rejected; case folding, translated identifiers and technology alias
resolution are not implemented. Duplicate full display names are permitted.

Offense, mode, cost factor and icon must be complete signed decimal int32
values, with optional plus/minus signs. Representable endpoints and negative
metadata are retained without narrowing or clamping. Empty fields, `*`,
fractions, hexadecimal syntax, partial numbers and overflow are rejected.
Supported modes are 0/1/2 (projectile/energy/missile), 7/8/9/10/11/12
(transport/colonist/terraformer/convoy/information warfare/artifact), and
13/14 (tectonic/fungal payloads) only for SMAX. Other values are rejected before
conversion to the enum.

Malformed input throws `std::invalid_argument`; field errors identify the
one-based data row and column. Construction is local and no partially parsed
table is returned. This is a strict GLSMAC ingestion policy, not a claim that
the original loader rejects the same malformed or modified files. A nonempty
subset is accepted for semantic fixtures and future catalog assembly; complete
catalog validation and cross-table references are outside this API.

## Validation and next boundary

Native tests use synthetic names and values to exercise the actual production
parser. They cover the three initial roles, all supported modes, shifted
ruleset-local indices, signed endpoints, required text, malformed numbers in
every numeric column, duplicate identities and section isolation. They need
no original assets. A separate read-only local probe checks the installed
21/26-row catalogs and the three component references above.

The next slices can decode CHASSIS, DEFENSES and REACTORS, then resolve a
`#UNITS` design against an explicitly selected catalog. `#UNITS` has no reactor
column, so choosing its initial reactor requires separate original-loader
evidence. Ability bitfields, technology eligibility and calculated costs also
need their own validated contracts. Current `StaticDef` and sprite-only render
definitions are unchanged; conventional CVR models and original composition
remain separate work. This parser does not claim playable conventional units
or SMAC/SMAX gameplay fidelity.
